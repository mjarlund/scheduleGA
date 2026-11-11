/* Test: no soft gain can ever outrank a hard violation.
 *
 * This is the property the third pass promises, and it must hold for *every*
 * pair of schedules, not just the ones the GA happens to visit:
 *
 *   (1) fitness(A) > fitness(B)  <=>  (violations(A), soft(A)) is
 *       lexicographically better than (violations(B), soft(B))
 *   (2) min fitness over zero-violation schedules > max fitness over schedules
 *       with at least one violation
 *   (3) the worst timetable the metrics can describe still costs less than one
 *       hard violation (HARD_SCALE), so one clash buys more than every
 *       compactness gain put together
 *
 * The test builds schedules of both kinds from the real dataset (no GA), asks
 * the real calc_fitness() for the numbers, and checks the packed integer
 * against an independent lexicographic comparison done here.
 *
 *   make test-hard-gate
 */
#include "../header/structs.h"
#include "../header/functions.h"
#include "../header/objective.h"

int NUM_DAYS = 195;
int NUM_HRS_DAY = 4;

static int failures;
static long HARD_SCALE_V, W_ROOM_TYPE_V;
static int W_TEAM_DAY_V, W_TEAM_GAP_V, W_PROF_DAY_V;

static void check(int cond, const char *what) {
    if(!cond) {
        printf("  FAIL  %s\n", what);
        failures++;
    }
}

/* Independent recomputation of the packed score from the components. */
static long expected_score(int clashes, int type_bad, int team_days, int gaps, int prof_days) {
#if SOFT_OBJECTIVE
    return (long)(NUM_ENTRIES - clashes) * HARD_SCALE_V
         - W_ROOM_TYPE_V * type_bad
         - (long)W_TEAM_DAY_V * team_days
         - (long)W_TEAM_GAP_V * gaps
         - (long)W_PROF_DAY_V * prof_days;
#else
    return NUM_ENTRIES - clashes - type_bad;
#endif
}

static long soft_of(int team_days, int gaps, int prof_days) {
#if SOFT_OBJECTIVE
    return (long)W_TEAM_DAY_V * team_days + (long)W_TEAM_GAP_V * gaps
         + (long)W_PROF_DAY_V * prof_days;
#else
    (void)team_days; (void)gaps; (void)prof_days;
    return 0;
#endif
}

/* Every entry in its own (day,hour) slot: no clash is possible, and the rooms
 * keep the type they were initialised with, so the schedule has no violation
 * of any kind.  Days are spread as far as the grid allows (each team's lessons
 * land on different days), which is close to the worst compactness the grid
 * permits -- the point of the test. */
static void build_spread(SCHEDULE *s) {
    int j, day, hour;
    int per_day = NUM_ENTRIES <= NUM_DAYS ? 1 : NUM_HRS_DAY;

    for(j = 0; j < NUM_ENTRIES; j++) {
        day = 1 + (j % NUM_DAYS);
        hour = 1 + (per_day > 1 ? (j / NUM_DAYS) % NUM_HRS_DAY : 0);
        s->entry[j].day = day;
        s->entry[j].hour = hour;
    }
}

/* Same schedule, with one entry moved onto another entry's exact slot and room:
 * exactly one conflict (charged once to the earlier of the pair), nothing else
 * changed.  Everything the soft terms see stays (almost) identical. */
static void build_one_violation(SCHEDULE *s, int victim, int host) {
    s->entry[victim].day = s->entry[host].day;
    s->entry[victim].hour = s->entry[host].hour;
    s->entry[victim].room = s->entry[host].room;
}

/* A timetable whose only purpose is to make the soft terms as expensive as the
 * data allows: no two lessons of a team share a day (267 team-days), same for
 * professors, and the grid is filled with plausible but useless placements. */
static void build_worst_soft(SCHEDULE *s, int seed_shift) {
    int j, course, team, lesson, day;
    int lesson_of_course[500];

    memset(lesson_of_course, 0, sizeof(lesson_of_course));
    for(j = 0; j < NUM_ENTRIES; j++) {
        course = s->entry[j].course;
        team = 0;
        {
            int i;
            for(i = 0; i < NUM_COURSES; i++) {
                if(COURSE_TABLE[i].team.id == COURSE_TABLE[course].team.id) {
                    team = i;
                    break;
                }
            }
        }
        lesson = lesson_of_course[course]++;
        /* one distinct day per lesson of the team, as far apart as possible */
        day = 1 + ((team * 13 + lesson * 7 + seed_shift) % NUM_DAYS);
        s->entry[j].day = day;
        s->entry[j].hour = 1 + ((lesson + seed_shift) % NUM_HRS_DAY);
    }
}

int main(void) {
    int i, j, pop = 300;
    int clashes, type_bad, team_days, gaps, prof_days;
    long soft, worst_soft = 0, worst_zero = 0, best_nonzero = 0;
    int have_zero = 0, have_nonzero = 0, seen_zero = 0, seen_nonzero = 0;
    SCHEDULE *s;
    long hard_scale;

    validate_data();
    courses();
    rooms();
    objective_check();
    objective_weights(&HARD_SCALE_V, &W_ROOM_TYPE_V, &W_TEAM_DAY_V,
                      &W_TEAM_GAP_V, &W_PROF_DAY_V);
    hard_scale = HARD_SCALE_V;

    s = malloc(pop * sizeof(SCHEDULE));
    if(s == NULL) {
        perror("malloc");
        return 1;
    }
    malloc_population(s, pop);
    init_population(s, pop);              /* random, typed rooms */

    for(i = pop / 2; i < pop; i++) {      /* half of them crammed */
        for(j = 0; j < NUM_ENTRIES; j++) {
            s[i].entry[j].day = 1 + (j % 3);
            s[i].entry[j].hour = 1 + (j % NUM_HRS_DAY);
        }
    }

    for(i = 0; i < 8; i++) {              /* and a few hand-built extremes */
        build_worst_soft(&s[pop - 1 - i], i);
    }
    build_spread(&s[0]);
    build_one_violation(&s[1], 5, 6);

    calc_fitness(s, pop);

    printf("\n-- (1) fitness is exactly the packed (violations, soft) scalar --\n");
    for(i = 0; i < pop; i++) {
        long want;

        fitness_parts(&s[i], &clashes, &type_bad, &team_days, &gaps, &prof_days);
        want = expected_score(clashes, type_bad, team_days, gaps, prof_days);
        if((long)s[i].fitness != want) {
            printf("  FAIL  schedule %d: fitness %d, recomputed %ld "
                   "(clashes=%d type_bad=%d team_days=%d gaps=%d prof_days=%d)\n",
                   i, s[i].fitness, want, clashes, type_bad, team_days, gaps, prof_days);
            failures++;
        }
        if((long)s[i].violations != (long)clashes + type_bad) {
            printf("  FAIL  schedule %d: violations %d != clashes+type_bad %d\n",
                   i, s[i].violations, clashes + type_bad);
            failures++;
        }

        soft = soft_of(team_days, gaps, prof_days);
        if(soft > worst_soft) worst_soft = soft;
        if(s[i].violations == 0) {
            if(!seen_zero || s[i].fitness < worst_zero) {
                worst_zero = s[i].fitness;
                seen_zero = 1;
            }
        } else {
            if(!seen_nonzero || s[i].fitness > best_nonzero) {
                best_nonzero = s[i].fitness;
                seen_nonzero = 1;
            }
        }
    }
    have_zero = seen_zero;
    have_nonzero = seen_nonzero;
    printf("  %d schedules: fitness == (NUM_ENTRIES-clashes)*HARD_SCALE - soft\n", pop);

    printf("\n-- (2) packed ordering == lexicographic (violations, soft) --\n");
    {
        long pairs = 0, bad = 0;
        for(i = 0; i < pop; i++) {
            int ai, at, atd, ag, ap;
            long asoft;
            fitness_parts(&s[i], &ai, &at, &atd, &ag, &ap);
            asoft = soft_of(atd, ag, ap);
            for(j = i + 1; j < pop; j++) {
                int bi, bt, btd, bg, bp;
                long bsoft, cmp_int;
                fitness_parts(&s[j], &bi, &bt, &btd, &bg, &bp);
                bsoft = soft_of(btd, bg, bp);
                pairs++;
                /* independent lexicographic comparison */
                cmp_int = (long)s[i].fitness - (long)s[j].fitness;
                {
                    int av = ai + at, bv = bi + bt;
                    int lex = (av != bv) ? ((av < bv) ? 1 : -1)
                                         : ((asoft != bsoft) ? ((asoft < bsoft) ? 1 : -1) : 0);
                    if((cmp_int > 0) != (lex > 0) || (cmp_int < 0) != (lex < 0)) {
                        if(bad < 5) {
                            printf("  FAIL  order mismatch i=%d j=%d fit %d/%d "
                                   "viol %d/%d soft %ld/%ld\n", i, j, s[i].fitness,
                                   s[j].fitness, av, bv, asoft, bsoft);
                        }
                        bad++;
                    }
                }
            }
        }
        printf("  %ld pairs compared, %ld mismatches\n", pairs, bad);
        failures += (int)bad;
    }

    printf("\n-- (3) one hard violation outweighs every soft gain --\n");
    printf("  worst soft cost seen in this run: %ld  (HARD_SCALE = %ld, %.1f%%)\n",
           worst_soft, hard_scale, 100.0 * (double)worst_soft / (double)hard_scale);
    check(worst_soft < hard_scale, "worst soft cost stays below one hard violation");
    check(have_zero, "test population contains a zero-violation schedule");
    check(have_nonzero, "test population contains a violating schedule");
    if(have_zero && have_nonzero) {
        printf("  lowest score with 0 violations : %ld\n", worst_zero);
        printf("  highest score with >=1 violation: %ld\n", best_nonzero);
        printf("  margin                         : %ld\n", worst_zero - best_nonzero);
        check(worst_zero > best_nonzero,
              "every zero-violation schedule outranks every violating one");
    }

    /* The extreme case the report claims: the *spread* timetable (worst
     * compactness this grid allows, no violation) against the same timetable
     * with a single clash and otherwise identical placement. */
    {
        long with_none, with_one;
        int c0, t0, td0, g0, p0, c1, t1, td1, g1, p1;

        build_spread(&s[0]);
        calc_fitness(s, 1);
        fitness_parts(&s[0], &c0, &t0, &td0, &g0, &p0);
        with_none = s[0].fitness;

        build_one_violation(&s[0], 5, 6);
        calc_fitness(s, 1);
        fitness_parts(&s[0], &c1, &t1, &td1, &g1, &p1);
        with_one = s[0].fitness;

        printf("  spread timetable      : violations=%d soft=%ld fitness=%ld\n",
               c0 + t0, soft_of(td0, g0, p0), with_none);
        printf("  same + one clash      : violations=%d soft=%ld fitness=%ld\n",
               c1 + t1, soft_of(td1, g1, p1), with_one);
        check(c0 + t0 == 0, "the spread timetable has no violation");
        check(c1 + t1 >= 1, "moving one lesson creates a violation");
        check(with_none > with_one,
              "one clash outranks the best possible compactness of the same timetable");
    }

    printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "PASS",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
