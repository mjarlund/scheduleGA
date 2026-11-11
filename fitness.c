#include "header/structs.h"
#include "header/functions.h"
#include "header/objective.h"
#include <limits.h>

int MAX_FITNESS;

/* ====================================================================== *
 *  The objective.
 *
 *  Up to the previous pass, fitness was "how many entries are conflict-free"
 *  (MAX_FITNESS = NUM_ENTRIES) and the GA stopped at the first 267/267, so
 *  nothing in the model cared *where* a lesson was placed as long as no room,
 *  class or professor was double-booked.  The result was legal and unusable:
 *  the measured 267/267 schedules used 261-265 distinct days for 267 lessons
 *  (i.e. almost every lesson of every class fell on a different day).
 *
 *  The objective is now a two-tier integer:
 *
 *      fitness = (NUM_ENTRIES - clashes) * HARD_SCALE
 *              - W_ROOM_TYPE * type_bad
 *              - W_TEAM_DAY  * team_days
 *              - W_TEAM_GAP  * gap_hours
 *              - W_PROF_DAY  * prof_days
 *
 *  where `clashes` is the number of entries that share a (day,hour) with a
 *  room / team / professor / student clash, exactly as the old model computed
 *  it, and `violations = clashes + type_bad` is the number of entries a school
 *  would refuse (the old fitness was NUM_ENTRIES - violations, so
 *  "267/267" means exactly what it used to mean).
 *
 *  HARD_SCALE is the price of one clashing entry.  The soft terms are bounded
 *  by SOFT_MAX below, and `objective_check()` refuses to run unless
 *  SOFT_MAX < HARD_SCALE, so *by construction* one clash outweighs every soft
 *  gain the rest of the objective can offer.  The same check keeps
 *  W_ROOM_TYPE >= HARD_SCALE by default, so a lesson in a room of the wrong
 *  type costs as much as a clash (that is what the old model did implicitly:
 *  a mismatched entry simply never scored).
 *
 *  All weights are tunable with -D on the compiler command line, e.g.
 *
 *      make CFLAGS="-O2 -std=c99 -DW_TEAM_DAY=16 -DW_TEAM_GAP=1"
 *
 *  and `tools/objective_sensitivity.py` builds a few of those combinations.
 * ====================================================================== */

#ifndef HARD_SCALE
#define HARD_SCALE      (1 << 20)   /* 1,048,576: price of one clashing entry */
#endif

/* Room type.  A subject type that has no room of its own type cannot be
 * taught anywhere, so the default price is a whole hard violation: this is
 * the *same* rule the old fitness applied (entry never scores), only now it
 * is explicit, counted, and reported instead of being folded into "0". */
#ifndef W_ROOM_TYPE
#define W_ROOM_TYPE     HARD_SCALE
#endif

/* Compactness, in units of one "soft point".  Justification for the ratios
 * (see the report, section "Third pass"):
 *   W_TEAM_DAY   8  one extra school-day for a class (26 students travelling
 *                   in) is worth 4 idle lesson-hours for that class, i.e.
 *                   8 * W_TEAM_GAP.
 *   W_TEAM_GAP   2  the unit: one idle hour in a class's day.
 *   W_PROF_DAY   4  one extra teaching day for a professor costs half of what
 *                   it costs a class (travel only, no 26 students), i.e.
 *                   2 * W_TEAM_GAP.
 */
#ifndef W_TEAM_DAY
#define W_TEAM_DAY      8
#endif
#ifndef W_TEAM_GAP
#define W_TEAM_GAP      2
#endif
#ifndef W_PROF_DAY
#define W_PROF_DAY      4
#endif

/* Escape hatch for the experiment "what would a *soft* room-type term do?".
 * It is off by default because a soft type term can rank a schedule with a
 * biology class in the gym above one without, which is the exact failure this
 * pass exists to remove.  With ALLOW_SOFT_ROOM_TYPE=1 a low W_ROOM_TYPE is
 * accepted (the objective then no longer guarantees the hard gate for types;
 * clashes are still hard). */
#ifndef ALLOW_SOFT_ROOM_TYPE
#define ALLOW_SOFT_ROOM_TYPE 0
#endif

/* ---------------------------------------------------------------------- *
 *  Per-schedule evaluation.
 * ---------------------------------------------------------------------- */

typedef struct {
    int clashes;    /* entries that conflict with a later entry in the same slot */
    int type_bad;   /* entries whose room type does not fit the subject type    */
    int team_days;  /* sum over teams of the number of distinct days used        */
    int gap_hours;  /* idle hours between a team's first and last lesson, per day */
    int prof_days;  /* sum over professors of the number of distinct days used   */
} PARTS;

/* Conflict buckets: head[s] is the lowest entry index in slot s, next[j] the
 * next higher index in the same slot.  Walking next[] from j visits exactly
 * { k : k > j, slot(k) == slot(j) }, the same comparison set as the 2015
 * all-pairs loop, so the clash definition is untouched.
 *
 * Everything is allocated once (lazily) instead of per call: the arrays are
 * indexed by entity (team/professor) * day and are stamped rather than
 * cleared, so a schedule only ever touches the cells its 267 entries need. */
/* One (entity, day) cell.  The four fields are kept together on purpose: the
 * access pattern is "one random cell per entry", so an array of structs costs
 * one cache line per touched cell where four parallel int arrays cost four.
 * Measured: 1.94 ms -> 1.81 ms per generation for the whole fitness pass
 * (-7%); the rest of the soft cost is the second loop over touched cells. */
typedef struct {
    int stamp, first, last, count;
} DAYCELL;

static int *bucket_next, *bucket_head, *bucket_stamp;
/* course_team / course_prof: dense 0..n-1 indices for the class and professor of
 * every course.  team_overlap: NUM_TEAMS x NUM_TEAMS, 1 iff the two classes
 * share at least one student.  The 2015 conflict test compared the two student
 * lists element by element (26 x 26 = 676 comparisons per candidate pair),
 * which was free on a 195-day grid where almost no two entries share a slot but
 * dominates on a tight grid (~10x the per-generation cost at 12 days).  The
 * matrix answers the same question with one load, so the clash definition and
 * the fitness value are unchanged. */
static int course_team[500], course_prof[500];
static unsigned char *team_overlap;
#if SOFT_OBJECTIVE
static DAYCELL *team_cell, *prof_cell;
static int *team_touched, *prof_touched;
static int team_days_stride, prof_days_stride;
#endif
static int stamp_clock, workspace_ready;

static void *xcalloc(size_t n, size_t size) {
    void *p = calloc(n, size);
    if(p == NULL) {
        perror("calc_fitness");
        exit(EXIT_FAILURE);
    }
    return p;
}

/* Dense 0..n-1 indices for the team and the professor of every course, so the
 * (entity, day) stamps can be a flat array.  Team and professor *ids* are not
 * guaranteed to be contiguous, so deriving an index as `id-1` would be wrong.
 * Also builds the class-overlap matrix used by the clash test, which is why it
 * runs for both objectives. */
static void build_entity_indices(void) {
    int i, j, l, n, teams = 0, professors = 0;

    for(i = 0; i < NUM_COURSES; i++) {
        for(j = 0; j < i; j++) {
            if(COURSE_TABLE[j].team.id == COURSE_TABLE[i].team.id) break;
        }
        course_team[i] = (j < i) ? course_team[j] : teams++;
    }
    for(i = 0; i < NUM_COURSES; i++) {
        for(j = 0; j < i; j++) {
            if(COURSE_TABLE[j].professor.id == COURSE_TABLE[i].professor.id) break;
        }
        course_prof[i] = (j < i) ? course_prof[j] : professors++;
    }

    team_overlap = xcalloc((size_t)NUM_TEAMS * (size_t)NUM_TEAMS, 1);
    for(i = 0; i < NUM_COURSES; i++) {
        for(j = i + 1; j < NUM_COURSES; j++) {
            int ti = course_team[i], tj = course_team[j];

            if(ti == tj || team_overlap[ti * NUM_TEAMS + tj]) {
                continue;   /* same class, or already known to overlap */
            }
            for(l = 0; l < COURSE_TABLE[i].team.num_students; l++) {
                for(n = 0; n < COURSE_TABLE[j].team.num_students; n++) {
                    if(COURSE_TABLE[i].team.student[l] == COURSE_TABLE[j].team.student[n]) {
                        team_overlap[ti * NUM_TEAMS + tj] = 1;
                        team_overlap[tj * NUM_TEAMS + ti] = 1;
                        break;
                    }
                }
            }
        }
    }
#if SOFT_OBJECTIVE
    team_days_stride = NUM_DAYS + 1;
    prof_days_stride = NUM_DAYS + 1;
#endif
    (void)teams;
    (void)professors;
}

static void init_workspace(void) {
    int nslots = (NUM_DAYS + 1) * (NUM_HRS_DAY + 1);

    bucket_next = xcalloc(NUM_ENTRIES + 1, sizeof(int));
    bucket_head = xcalloc(nslots, sizeof(int));
    bucket_stamp = xcalloc(nslots, sizeof(int));
    build_entity_indices();
#if SOFT_OBJECTIVE
    team_cell = xcalloc((size_t)NUM_TEAMS * team_days_stride, sizeof(DAYCELL));
    prof_cell = xcalloc((size_t)NUM_PROFESSORS * prof_days_stride, sizeof(DAYCELL));
    team_touched = xcalloc(NUM_ENTRIES + 1, sizeof(int));
    prof_touched = xcalloc(NUM_ENTRIES + 1, sizeof(int));
#endif
    stamp_clock = 0;
    workspace_ready = 1;
}

/* The stamps compare against a monotonically increasing clock, so they have to
 * be cleared once in a (very long) while instead of per schedule. */
static void bump_clock(void) {
    if(stamp_clock >= INT_MAX - 2) {
        int nslots = (NUM_DAYS + 1) * (NUM_HRS_DAY + 1);
        memset(bucket_stamp, 0, (size_t)nslots * sizeof(int));
#if SOFT_OBJECTIVE
        memset(team_cell, 0, (size_t)NUM_TEAMS * team_days_stride * sizeof(DAYCELL));
        memset(prof_cell, 0, (size_t)NUM_PROFESSORS * prof_days_stride * sizeof(DAYCELL));
#endif
        stamp_clock = 0;
    }
    stamp_clock++;
}

static void evaluate(ENTRY *entry, PARTS *p) {
    int j, k, slot, cur;
#if SOFT_OBJECTIVE
    int day, hour, n;
#endif

    memset(p, 0, sizeof(*p));
    bump_clock();
    cur = stamp_clock;

    /* --- build the (day,hour) buckets ---------------------------------- */
    for(j = NUM_ENTRIES - 1; j >= 0; j--) {
        slot = entry[j].day * (NUM_HRS_DAY + 1) + entry[j].hour;
        /* Out-of-range data means the schedule is garbage (truncated CSV
           input); such an entry is not bucketed instead of indexing head[]
           out of bounds. */
        if(slot < 0 || slot >= (NUM_DAYS + 1) * (NUM_HRS_DAY + 1)) {
            bucket_next[j] = -1;
        } else {
            if(bucket_stamp[slot] != cur) {
                bucket_stamp[slot] = cur;
                bucket_head[slot] = -1;
            }
            bucket_next[j] = bucket_head[slot];
            bucket_head[slot] = j;
        }
    }

    /* --- scoring + soft terms ------------------------------------------ */
    for(j = 0; j < NUM_ENTRIES; j++) {
        ENTRY *a = &entry[j];
        COURSE *ca = &COURSE_TABLE[a->course];
        ROOM *ra = &ROOM_TABLE[a->room];

        /* A room of the wrong type cannot host the lesson at all: this is the
           same rule the 2015 code applied (score 0), counted explicitly. */
        if(ca->subject.type != ra->type) {
            p->type_bad++;
        } else {
            for(k = bucket_next[j]; k != -1; k = bucket_next[k]) {
                ENTRY *b = &entry[k];
                COURSE *cb = &COURSE_TABLE[b->course];
                ROOM *rb = &ROOM_TABLE[b->room];

                if(ra->id == rb->id) {
                    p->clashes++;
                    break;
                } else if(ca->team.id == cb->team.id) {
                    p->clashes++;
                    break;
                } else if(ca->professor.id == cb->professor.id) {
                    p->clashes++;
                    break;
                } else if(team_overlap[course_team[a->course] * NUM_TEAMS
                                      + course_team[b->course]]) {
                    /* Two or more students in the same place at the same time. */
                    p->clashes++;
                    break;
                }
            }
        }

#if SOFT_OBJECTIVE
        /* Team: distinct days (counted as the number of touched (team,day)
           cells) and the idle hours inside those days.  Both are derived from
           one pass; the second loop only walks the touched cells. */
        day = a->day;
        hour = a->hour;
        if(day >= 1 && day <= NUM_DAYS && hour >= 1 && hour <= NUM_HRS_DAY) {
            DAYCELL *c = &team_cell[course_team[a->course] * team_days_stride + day];
            if(c->stamp != cur) {
                c->stamp = cur;
                c->first = hour;
                c->last = hour;
                c->count = 0;
                team_touched[p->team_days++] = (int)(c - team_cell);
            }
            if(hour < c->first) c->first = hour;
            if(hour > c->last) c->last = hour;
            c->count++;

            c = &prof_cell[course_prof[a->course] * prof_days_stride + day];
            if(c->stamp != cur) {
                c->stamp = cur;
                c->first = hour;
                c->last = hour;
                c->count = 0;
                prof_touched[p->prof_days++] = (int)(c - prof_cell);
            }
            if(hour < c->first) c->first = hour;
            if(hour > c->last) c->last = hour;
            c->count++;
        }
#endif
    }

#if SOFT_OBJECTIVE
    for(k = 0; k < p->team_days; k++) {
        DAYCELL *c = &team_cell[team_touched[k]];
        n = (c->last - c->first + 1) - c->count;
        if(n > 0) p->gap_hours += n;   /* a clash can make this negative */
    }
#endif

    /* p->team_days / p->prof_days are already the distinct-day sums: one
       (entity, day) cell per distinct day, exactly as counted above. */
}

/* ---------------------------------------------------------------------- *
 *  Weights: public check, called once at start-up (main.c).
 * ---------------------------------------------------------------------- */

void objective_check(void) {
    long soft_max, total;
#if SOFT_OBJECTIVE
    int days = NUM_DAYS + 1;
    int max_gap = NUM_HRS_DAY > 2 ? NUM_HRS_DAY - 2 : 0;
#endif

#if SOFT_OBJECTIVE
    /* Worst case reachable by the compactness terms on THIS dataset:
       - team_days <= NUM_ENTRIES        (one day per lesson is the maximum)
       - gap_hours <= teams * days * 2   (a team-day can hold at most two
                                          idle hours between first and last)
       - prof_days <= professors * days  */
    soft_max = (long)W_TEAM_DAY * NUM_ENTRIES
             + (long)W_TEAM_GAP * NUM_TEAMS * days * max_gap
             + (long)W_PROF_DAY * NUM_PROFESSORS * days;
#else
    soft_max = 0;
#endif
    total = soft_max + (long)W_ROOM_TYPE * NUM_ENTRIES;

    printf("Objective: soft=%d hard_scale=%ld w_room_type=%ld w_team_day=%d "
           "w_team_gap=%d w_prof_day=%d soft_max=%ld (%.1f%% of hard_scale)\n",
           SOFT_OBJECTIVE, (long)HARD_SCALE, (long)W_ROOM_TYPE, W_TEAM_DAY,
           W_TEAM_GAP, W_PROF_DAY, soft_max,
           HARD_SCALE ? 100.0 * (double)soft_max / (double)HARD_SCALE : 0.0);

    if((long)NUM_ENTRIES * HARD_SCALE >= (long)INT_MAX) {
        fprintf(stderr, "objective: NUM_ENTRIES * hard_scale = %ld does not fit in an int; "
                        "lower HARD_SCALE\n", (long)NUM_ENTRIES * HARD_SCALE);
        exit(EXIT_FAILURE);
    }

#if SOFT_OBJECTIVE
    if(soft_max >= HARD_SCALE) {
        fprintf(stderr, "objective: the soft terms can reach %ld, which is not below "
                        "hard_scale %ld: one clash would no longer outweigh them. "
                        "Lower the compactness weights.\n", soft_max, (long)HARD_SCALE);
        exit(EXIT_FAILURE);
    }
#endif

    if(W_ROOM_TYPE < HARD_SCALE) {
#if ALLOW_SOFT_ROOM_TYPE
        printf("Objective: WARNING w_room_type < hard_scale -- a room of the wrong type is "
               "cheaper than a clash, so the run may stop on a timetable with a biology class "
               "in the gym (only total soft budget %ld of %ld is guaranteed)\n",
               total, (long)HARD_SCALE);
#else
        fprintf(stderr, "objective: w_room_type=%ld is below hard_scale=%ld, so a lesson in a "
                        "room of the wrong type would be cheaper than a clash and the search "
                        "could settle on an unusable timetable. Raise W_ROOM_TYPE, or build "
                        "with -DALLOW_SOFT_ROOM_TYPE=1 to accept that on purpose.\n",
                (long)W_ROOM_TYPE, (long)HARD_SCALE);
        exit(EXIT_FAILURE);
#endif
    }

    if(total >= (long)INT_MAX) {
        fprintf(stderr, "objective: weights overflow an int (total %ld)\n", total);
        exit(EXIT_FAILURE);
    }
}

/* The weights, for callers that need the numbers rather than the printed line
 * (tools/test_hard_gate.c recomputes the packed score from them). */
void objective_weights(long *hard_scale, long *w_room_type, int *w_team_day,
                       int *w_team_gap, int *w_prof_day) {
    *hard_scale = (long)HARD_SCALE;
    *w_room_type = (long)W_ROOM_TYPE;
    *w_team_day = W_TEAM_DAY;
    *w_team_gap = W_TEAM_GAP;
    *w_prof_day = W_PROF_DAY;
}

/* ---------------------------------------------------------------------- *
 *  Fitness of a whole population.
 * ---------------------------------------------------------------------- */

void calc_fitness(SCHEDULE *schedule, int pop) {
    int i;

    // Max fitness is equal to the number of entries: one acceptable entry each.
    MAX_FITNESS = NUM_ENTRIES;

    if(!workspace_ready) {
        init_workspace();
    }

    for(i = 0; i < pop; i++) {
        PARTS p;

        evaluate(schedule[i].entry, &p);
        schedule[i].violations = p.clashes + p.type_bad;

#if SOFT_OBJECTIVE
        schedule[i].fitness = (NUM_ENTRIES - p.clashes) * HARD_SCALE
                            - W_ROOM_TYPE * p.type_bad
                            - W_TEAM_DAY * p.team_days
                            - W_TEAM_GAP * p.gap_hours
                            - W_PROF_DAY * p.prof_days;
#else
        /* Legacy objective: exactly the 2015/`fix/generation-loop` number, so
           SOFT_OBJECTIVE=0 is comparable bit for bit with that branch. */
        schedule[i].fitness = NUM_ENTRIES - p.clashes - p.type_bad;
#endif
    }
}

/* One line with every component of the objective for a single schedule, so a
 * run log states what the exported timetable scored and why. */
void fitness_report(SCHEDULE *s) {
    PARTS p;

    if(!workspace_ready) {
        init_workspace();
    }
    evaluate(s->entry, &p);
#if SOFT_OBJECTIVE
    printf("Objective: violations=%d clashes=%d type_mismatches=%d team_days=%d "
           "team_gaps=%d prof_days=%d fitness=%d\n",
           p.clashes + p.type_bad, p.clashes, p.type_bad, p.team_days,
           p.gap_hours, p.prof_days, s->fitness);
#else
    printf("Objective: violations=%d clashes=%d type_mismatches=%d fitness=%d\n",
           p.clashes + p.type_bad, p.clashes, p.type_bad, s->fitness);
#endif
}

/* The same components for callers that want numbers instead of a printed line
 * (tools/test_hard_gate.c uses this to check the ordering property). */
void fitness_parts(SCHEDULE *s, int *clashes, int *type_bad, int *team_days,
                   int *gap_hours, int *prof_days) {
    PARTS p;

    if(!workspace_ready) {
        init_workspace();
    }
    evaluate(s->entry, &p);
    *clashes = p.clashes;
    *type_bad = p.type_bad;
    *team_days = p.team_days;
    *gap_hours = p.gap_hours;
    *prof_days = p.prof_days;
}
