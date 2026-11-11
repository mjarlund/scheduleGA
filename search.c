#include "header/structs.h"
#include "header/functions.h"
#include "header/objective.h"

/* ====================================================================== *
 *  Fourth pass: a constraint-directed constructor and a repair operator.
 *
 *  Both work on the same conflict model as fitness.c -- two entries in one
 *  (day,hour) slot may not share a room, a class, a professor, or a student --
 *  but they count *clashing pairs*, which is strictly finer than the
 *  objective's "entries that have a later clash".  A schedule with zero
 *  clashing pairs is therefore also zero-violation for the hard gate, so the
 *  repair cannot buy feasibility by weakening what "267/267" means.
 *
 *  Both only ever place an entry in a room whose type matches its subject, so
 *  the room-type rule stays hard exactly as it is in the typed operators.
 *
 *  INIT_MODE=1 replaces the random initialiser with least-conflict placement;
 *  REPAIR=1 runs min-conflicts local search on the elite each generation.  The
 *  GA itself (selection, crossover, mutation, generational replacement, the
 *  hard gate) is untouched.
 * ====================================================================== */

/* Candidate placements examined per move.  512 is enough to cover a small
 * week completely (6 days x 4 hours x 7 classrooms = 168) and to sample a
 * 195-day grid thoroughly without making one repair step cost thousands of
 * conflict tests. */
#ifndef SEARCH_CAND_CAP
#define SEARCH_CAND_CAP 512
#endif

static int *slot_head, *slot_next, *slot_prev, *entry_slot;
static int nslots;
static int *team_of_course, *prof_of_course, *course_of_type_room_count;
static int *room_type_offset, *room_type_list;
static unsigned char *overlap;      /* NUM_TEAMS x NUM_TEAMS, symmetric        */
static int max_room_type, search_ready;

typedef struct {
    int day, hour, room;
} CAND;

/* ---- one-time tables ------------------------------------------------- */

static void search_init(void) {
    int i, j, t, n;

    nslots = (NUM_DAYS + 1) * (NUM_HRS_DAY + 1);
    slot_head = calloc((size_t)nslots, sizeof(int));
    slot_next = calloc((size_t)NUM_ENTRIES, sizeof(int));
    slot_prev = calloc((size_t)NUM_ENTRIES, sizeof(int));
    entry_slot = calloc((size_t)NUM_ENTRIES, sizeof(int));
    team_of_course = calloc((size_t)NUM_COURSES, sizeof(int));
    prof_of_course = calloc((size_t)NUM_COURSES, sizeof(int));
    if(!slot_head || !slot_next || !slot_prev || !entry_slot || !team_of_course
       || !prof_of_course) {
        perror("search_init");
        exit(EXIT_FAILURE);
    }

    /* Dense class / professor indices, by first appearance (as fitness.c). */
    {
        int teams = 0, profs = 0;

        for(i = 0; i < NUM_COURSES; i++) {
            for(j = 0; j < i; j++) {
                if(COURSE_TABLE[j].team.id == COURSE_TABLE[i].team.id) break;
            }
            team_of_course[i] = (j < i) ? team_of_course[j] : teams++;
        }
        for(i = 0; i < NUM_COURSES; i++) {
            for(j = 0; j < i; j++) {
                if(COURSE_TABLE[j].professor.id == COURSE_TABLE[i].professor.id) break;
            }
            prof_of_course[i] = (j < i) ? prof_of_course[j] : profs++;
        }
    }

    overlap = calloc((size_t)NUM_TEAMS * (size_t)NUM_TEAMS, 1);
    if(!overlap) {
        perror("search_init");
        exit(EXIT_FAILURE);
    }
    for(i = 0; i < NUM_COURSES; i++) {
        for(j = i + 1; j < NUM_COURSES; j++) {
            int ti = team_of_course[i], tj = team_of_course[j], l, m, hit = 0;

            if(ti == tj || overlap[ti * NUM_TEAMS + tj]) {
                continue;
            }
            for(l = 0; l < COURSE_TABLE[i].team.num_students && !hit; l++) {
                for(m = 0; m < COURSE_TABLE[j].team.num_students; m++) {
                    if(COURSE_TABLE[i].team.student[l] == COURSE_TABLE[j].team.student[m]) {
                        hit = 1;
                        break;
                    }
                }
            }
            if(hit) {
                overlap[ti * NUM_TEAMS + tj] = 1;
                overlap[tj * NUM_TEAMS + ti] = 1;
            }
        }
    }

    /* Rooms grouped by type (CSR), so a candidate list never contains a room
       of the wrong type. */
    max_room_type = 0;
    for(i = 0; i < NUM_ROOMS; i++) {
        if(ROOM_TABLE[i].type > max_room_type) {
            max_room_type = ROOM_TABLE[i].type;
        }
    }
    room_type_offset = calloc((size_t)max_room_type + 2, sizeof(int));
    course_of_type_room_count = calloc((size_t)max_room_type + 2, sizeof(int));
    if(!room_type_offset || !course_of_type_room_count) {
        perror("search_init");
        exit(EXIT_FAILURE);
    }
    for(i = 0; i < NUM_ROOMS; i++) {
        course_of_type_room_count[ROOM_TABLE[i].type]++;
    }
    for(t = 1; t <= max_room_type; t++) {
        room_type_offset[t + 1] = room_type_offset[t] + course_of_type_room_count[t];
        course_of_type_room_count[t] = room_type_offset[t];
    }
    room_type_list = calloc((size_t)NUM_ROOMS, sizeof(int));
    if(!room_type_list) {
        perror("search_init");
        exit(EXIT_FAILURE);
    }
    for(i = 0; i < NUM_ROOMS; i++) {
        int ty = ROOM_TABLE[i].type;

        room_type_list[course_of_type_room_count[ty]++] = i;
    }
    /* offsets are now consumed; rebuild them for read-only use */
    for(t = 0; t <= max_room_type; t++) {
        course_of_type_room_count[t] = 0;
    }
    for(i = 0; i < NUM_ROOMS; i++) {
        course_of_type_room_count[ROOM_TABLE[i].type]++;
    }
    room_type_offset[0] = 0;
    for(t = 1; t <= max_room_type + 1; t++) {
        room_type_offset[t] = room_type_offset[t - 1] + course_of_type_room_count[t - 1];
    }
    (void)n;
    search_ready = 1;
}

static int rooms_of_type(int type, int *start, int *count) {
    if(type < 0 || type > max_room_type) {
        *start = 0;
        *count = 0;
        return 0;
    }
    *start = room_type_offset[type];
    *count = course_of_type_room_count[type];
    return *count;
}

/* ---- slot lists ------------------------------------------------------ */

static void lists_clear(void) {
    int i;

    for(i = 0; i < nslots; i++) {
        slot_head[i] = -1;
    }
}

static void list_insert(ENTRY *e, int j) {
    int s = e[j].day * (NUM_HRS_DAY + 1) + e[j].hour;

    entry_slot[j] = s;
    slot_next[j] = slot_head[s];
    slot_prev[j] = -1;
    if(slot_head[s] >= 0) {
        slot_prev[slot_head[s]] = j;
    }
    slot_head[s] = j;
}

static void list_remove(int j) {
    int s = entry_slot[j];

    if(slot_prev[j] >= 0) {
        slot_next[slot_prev[j]] = slot_next[j];
    } else {
        slot_head[s] = slot_next[j];
    }
    if(slot_next[j] >= 0) {
        slot_prev[slot_next[j]] = slot_prev[j];
    }
}

/* Two entries in the same slot clash if they share a room, a class, a
 * professor or a student.  Same rule as fitness.c, pair-wise instead of
 * charged to the earlier entry. */
static int clash(ENTRY *e, int j, int k) {
    COURSE *cj = &COURSE_TABLE[e[j].course];
    COURSE *ck = &COURSE_TABLE[e[k].course];

    if(e[j].room == e[k].room) return 1;
    if(cj->team.id == ck->team.id) return 1;
    if(cj->professor.id == ck->professor.id) return 1;
    if(overlap[team_of_course[e[j].course] * NUM_TEAMS
               + team_of_course[e[k].course]]) return 1;
    return 0;
}

/* Conflicts that entry j would have if it sat in slot s, ignoring j itself. */
static int slot_cost(ENTRY *e, int j, int s) {
    int k, c = 0;

    for(k = slot_head[s]; k >= 0; k = slot_next[k]) {
        if(k != j && clash(e, j, k)) {
            c++;
        }
    }
    return c;
}

static int type_ok(ENTRY *e, int j) {
    return ROOM_TABLE[e[j].room].type == COURSE_TABLE[e[j].course].subject.type;
}

/* ---- candidates ------------------------------------------------------ */

/* Enumerate (day, hour, room of the subject's type) placements for entry j.
 * When there are more than `cap` of them (a 195-day grid has thousands), a
 * systematic sample with a random offset is used, so the cost of the
 * constructor and of one repair step does not scale with the calendar. */
static int candidates(ENTRY *e, int j, CAND *out, int cap) {
    int start, count, r, total, stride, offset, i, idx, type;
    int rooms;

    type = COURSE_TABLE[e[j].course].subject.type;
    rooms = rooms_of_type(type, &start, &count);
    if(rooms <= 0) {
        return 0;
    }
    total = NUM_DAYS * NUM_HRS_DAY * count;
    if(total <= cap) {
        stride = 1;
        offset = 0;
        cap = total;
    } else {
        stride = total / cap;
        offset = rand_num(stride, 0);
    }
    for(i = 0; i < cap; i++) {
        idx = offset + i * stride;
        r = idx % count;
        out[i].hour = (idx / count) % NUM_HRS_DAY + 1;
        out[i].day = idx / (count * NUM_HRS_DAY) + 1;
        out[i].room = room_type_list[start + r];
    }
    return cap;
}

/* Pick uniformly among the candidates that attain the minimum cost. */
static int best_candidate(ENTRY *e, int j, CAND *cand, int n, int *out_cost) {
    int i, cost, best = -1, seen = 0;

    *out_cost = 0;
    if(n == 0) {
        return -1;
    }
    for(i = 0; i < n; i++) {
        int s = cand[i].day * (NUM_HRS_DAY + 1) + cand[i].hour;

        cost = slot_cost(e, j, s);
        if(best < 0 || cost < *out_cost) {
            *out_cost = cost;
            best = i;
            seen = 1;
        } else if(cost == *out_cost) {
            seen++;
            /* reservoir sampling: keep each tied candidate with 1/seen */
            if(rand_num(seen, 0) == 0) {
                best = i;
            }
        }
    }
    return best;
}

/* ---- constructor ----------------------------------------------------- */

/* Least-conflict placement: entries are placed one at a time (in a different
 * random order per individual), each into a slot/room that adds the fewest
 * conflicts with what is already placed.  Ties are broken randomly, which is
 * where the population's diversity comes from. */
void greedy_init(SCHEDULE *dest, int n) {
    int k, j, i, cap, n_cand, pick, cost;
    int *order;
    CAND *cand;

    if(!search_ready) {
        search_init();
    }
    order = malloc((size_t)NUM_ENTRIES * sizeof(int));
    cap = SEARCH_CAND_CAP;
    cand = malloc((size_t)cap * sizeof(CAND));
    if(!order || !cand) {
        perror("greedy_init");
        exit(EXIT_FAILURE);
    }

    for(k = 0; k < n; k++) {
        for(i = 0; i < NUM_ENTRIES; i++) {
            order[i] = i;
        }
        for(i = NUM_ENTRIES - 1; i > 0; i--) {
            j = rand_num(i + 1, 0);
            pick = order[i];
            order[i] = order[j];
            order[j] = pick;
        }
        /* entry -> course, exactly as init_population() assigns it */
        {
            int e = 0, c;

            for(c = 0; c < NUM_COURSES; c++) {
                for(j = 0; j < COURSE_TABLE[c].subject.hours; j++) {
                    dest[k].entry[e].course = c;
                    e++;
                }
            }
        }

        lists_clear();
        for(i = 0; i < NUM_ENTRIES; i++) {
            j = order[i];
            n_cand = candidates(dest[k].entry, j, cand, cap);
            pick = best_candidate(dest[k].entry, j, cand, n_cand, &cost);
            if(pick < 0) {   /* no room of the type: leave it where it was */
                dest[k].entry[j].day = 1;
                dest[k].entry[j].hour = 1;
                dest[k].entry[j].room = 0;
            } else {
                dest[k].entry[j].day = cand[pick].day;
                dest[k].entry[j].hour = cand[pick].hour;
                dest[k].entry[j].room = cand[pick].room;
            }
            list_insert(dest[k].entry, j);
        }
    }

    free(order);
    free(cand);
}

/* ---- repair / local search ------------------------------------------ */

/* Count of entries that conflict with at least one other entry, i.e. the
 * number a school would reject under the pair-wise reading. */
static int offenders(ENTRY *e, int *list) {
    int j, n = 0;

    for(j = 0; j < NUM_ENTRIES; j++) {
        if(!type_ok(e, j) || slot_cost(e, j, entry_slot[j]) > 0) {
            if(list) list[n] = j;
            n++;
        }
    }
    return n;
}

/* Min-conflicts local search on one schedule: repeatedly take an offending
 * entry, move it to the placement that minimises its conflicts (ties random),
 * with an occasional random kick so it cannot cycle.  Returns the number of
 * clashing entries left (0 = a legal timetable). */
int repair_schedule(SCHEDULE *s, int max_steps) {
    int step, j, n_off, n_cand, pick, cost, cur, idle = 0;
    int *offs, best_bad;
    ENTRY *best_copy;
    CAND *cand;
    int cap;

    if(!search_ready) {
        search_init();
    }
    offs = malloc((size_t)NUM_ENTRIES * sizeof(int));
    best_copy = malloc((size_t)NUM_ENTRIES * sizeof(ENTRY));
    cap = SEARCH_CAND_CAP;
    cand = malloc((size_t)cap * sizeof(CAND));
    if(!offs || !best_copy || !cand) {
        perror("repair_schedule");
        exit(EXIT_FAILURE);
    }

    lists_clear();
    for(j = 0; j < NUM_ENTRIES; j++) {
        if(!type_ok(s->entry, j)) {
            /* snap a wrong-type room to a room of the right type first; the
               min-conflicts loop then treats it like any other offender */
            int start, count;

            if(rooms_of_type(COURSE_TABLE[s->entry[j].course].subject.type,
                             &start, &count)) {
                s->entry[j].room = room_type_list[start + rand_num(count, 0)];
            }
        }
        list_insert(s->entry, j);
    }

    n_off = offenders(s->entry, offs);
    best_bad = n_off;
    memcpy(best_copy, s->entry, (size_t)NUM_ENTRIES * sizeof(ENTRY));

    for(step = 0; step < max_steps && best_bad > 0; step++) {
        if(n_off == 0) {
            best_bad = 0;
            memcpy(best_copy, s->entry, (size_t)NUM_ENTRIES * sizeof(ENTRY));
            break;
        }
        j = offs[rand_num(n_off, 0)];
        cur = type_ok(s->entry, j) ? slot_cost(s->entry, j, entry_slot[j])
                                   : slot_cost(s->entry, j, entry_slot[j]) + 1;
        n_cand = candidates(s->entry, j, cand, cap);
        pick = best_candidate(s->entry, j, cand, n_cand, &cost);
        if(pick < 0) {
            break;
        }

        if(cost < cur) {
            list_remove(j);
            s->entry[j].day = cand[pick].day;
            s->entry[j].hour = cand[pick].hour;
            s->entry[j].room = cand[pick].room;
            list_insert(s->entry, j);
            idle = 0;
        } else if(rand_num(8, 0) == 0) {
            /* random kick: standard min-conflicts random walk */
            pick = rand_num(n_cand, 0);
            list_remove(j);
            s->entry[j].day = cand[pick].day;
            s->entry[j].hour = cand[pick].hour;
            s->entry[j].room = cand[pick].room;
            list_insert(s->entry, j);
            idle = 0;
        } else if(++idle > 4 * NUM_ENTRIES) {
            break;      /* the walk has stopped making progress */
        }

        n_off = offenders(s->entry, offs);
        if(n_off < best_bad) {
            best_bad = n_off;
            memcpy(best_copy, s->entry, (size_t)NUM_ENTRIES * sizeof(ENTRY));
        }
    }

    memcpy(s->entry, best_copy, (size_t)NUM_ENTRIES * sizeof(ENTRY));
    free(offs);
    free(best_copy);
    free(cand);
    return best_bad;
}
