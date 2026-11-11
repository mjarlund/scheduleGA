/* Kernel-level timing of the parts of one GA generation, without gprof
 * instrumentation or process start-up noise:
 *
 *   fitness   one calc_fitness() over a 100-individual population
 *   copy      the 100 copy_schedule() memcpys a generation performs
 *   rest      the whole reproduction loop except calc_fitness: 50 tournament
 *             selections, 25 crossovers, 50 mutations and the 100 copies
 *   rng       the ~20,000 rand() calls those operators make (reference cost)
 *
 * Compiled against either tree; the parent (embedded COURSE/ROOM) build omits
 * -DENTRY_INDICES.  tools/bench_fitness.sh does both builds and interleaves.
 */
#include "header/structs.h"
#include "header/functions.h"
#include <time.h>

static COURSE *CT;
static ROOM *RT;

int NUM_DAYS, NUM_HRS_DAY;

#ifdef ENTRY_INDICES
#  define ENTRY_SET(e, ci, ri) do { (e).course = (ci); (e).room = (ri); } while(0)
#  define MUTATE(s) mutation(s)
#else
#  define ENTRY_SET(e, ci, ri) do { (e).course = CT[ci]; (e).room = RT[ri]; } while(0)
#  define MUTATE(s) mutation(s, RT)
#endif

#define POP 100
#define HALF 50
#define REPS 200
#define RNG_PER_GEN 20025

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(void) {
    int i, j, r;
    SCHEDULE pop[POP], parent[HALF];
    double t0, t1, tf, tc, tr, tg;

    NUM_DAYS = 195;
    NUM_HRS_DAY = 4;

    if(!professors() || !subjects() || !rooms() || !teams() || !courses()) {
        printf("data load failed\n");
        return 2;
    }
    CT = courses();
    RT = rooms();

    srand(99);
    malloc_population(pop, POP);
    malloc_population(parent, HALF);
    for(i = 0; i < POP; i++) {
        int e = 0, ci, h;
        /* Exactly the order init_population() uses: course i repeated
           subject.hours[i] times, so the course table is walked sequentially
           and the entry array matches the real program's shape. */
        for(ci = 0; ci < NUM_COURSES; ci++) {
            for(h = 0; h < CT[ci].subject.hours; h++) {
                ENTRY_SET(pop[i].entry[e], ci, rand_num(NUM_ROOMS, 0));
                pop[i].entry[e].day = rand_num(NUM_DAYS + 1, 1);
                pop[i].entry[e].hour = rand_num(NUM_HRS_DAY + 1, 1);
                e++;
            }
        }
    }

    /* warm-up */
    calc_fitness(pop, POP);

    t0 = now_ms();
    for(r = 0; r < REPS; r++) {
        calc_fitness(pop, POP);
    }
    t1 = now_ms();
    tf = (t1 - t0) / REPS;

    t0 = now_ms();
    for(r = 0; r < REPS; r++) {
        for(i = 0; i < POP; i++) {
            copy_schedule(&pop[i], &pop[POP - 1 - i]);
        }
    }
    t1 = now_ms();
    tc = (t1 - t0) / REPS;

    /* Standalone rand() cost for one generation's worth of calls. */
    t0 = now_ms();
    for(r = 0; r < REPS; r++) {
        for(i = 0; i < RNG_PER_GEN; i++) {
            (void) randZeroToOne();
        }
        for(i = 0; i < 452; i++) {
            (void) rand_num(NUM_DAYS + 1, 1);
        }
    }
    t1 = now_ms();
    tr = (t1 - t0) / REPS;

    /* The reproduction loop of main.c, minus calc_fitness.  It consumes and
       overwrites `pop`, so it runs last. */
    t0 = now_ms();
    for(r = 0; r < REPS; r++) {
        for(i = 0; i < HALF; i++) {
            parent[i] = tournament_selection(pop, POP);
            if(i % 2) {
                uniform_crossover(&pop[i], &pop[i - 1], parent[i], parent[i - 1]);
                MUTATE(&pop[i]);
                MUTATE(&pop[i - 1]);
            }
            copy_schedule(&pop[(POP - 1) - i], &parent[i]);
        }
    }
    t1 = now_ms();
    tg = (t1 - t0) / REPS;

    printf("sizeof_ENTRY=%zu  fitness_ms_per_gen=%.3f  copy_ms_per_gen=%.3f  "
           "rest_ms_per_gen=%.3f  rng_ms_per_gen=%.3f  fitness_us_per_schedule=%.3f\n",
           sizeof(ENTRY), tf, tc, tg, tr, tf * 1000.0 / POP);
    return 0;
}
