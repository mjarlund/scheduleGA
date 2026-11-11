/* Equivalence test: original calc_fitness (from master) vs the bucketed one.
 *
 * Build (see tools/run_equivalence.sh):
 *   git show master:fitness.c > /tmp/old_fitness.c
 *   cc -O2 -std=c99 -Dcalc_fitness=calc_fitness_old -DMAX_FITNESS=MAX_FITNESS_OLD \
 *      -c /tmp/old_fitness.c -o /tmp/old_fitness.o
 *   cc -O2 -std=c99 -o /tmp/eq tools/equivalence_test.c fitness.c parsing.c \
 *      functions.c /tmp/old_fitness.o
 *
 * Both implementations are run over identical randomized populations (including
 * adversarial ones where every entry is crammed into a single slot, or into a
 * handful of slots, so the conflict branches are exercised heavily) and the
 * per-schedule fitness arrays must match exactly.
 */
#include "header/structs.h"
#include "header/functions.h"

int NUM_DAYS, NUM_HRS_DAY;
void calc_fitness_old(SCHEDULE *schedule, int pop);

#define CASES 300

static void fill(SCHEDULE *p, int n, COURSE *c, ROOM *r, int mode) {
    int i, j;
    for(i = 0; i < n; i++) {
        for(j = 0; j < NUM_ENTRIES; j++) {
            p[i].entry[j].course = c[rand_num(NUM_COURSES, 0)];
            switch(mode) {
                case 0:  /* uniform, like init_population */
                    p[i].entry[j].day = rand_num(NUM_DAYS + 1, 1);
                    p[i].entry[j].hour = rand_num(NUM_HRS_DAY + 1, 1);
                    break;
                case 1:  /* everything in one slot: worst case for the old scan */
                    p[i].entry[j].day = 1;
                    p[i].entry[j].hour = 1;
                    break;
                default: /* three slots, heavy collisions */
                    p[i].entry[j].day = rand_num(3, 0);
                    p[i].entry[j].hour = rand_num(NUM_HRS_DAY + 1, 1);
                    break;
            }
            p[i].entry[j].room = r[rand_num(NUM_ROOMS, 0)];
        }
        p[i].fitness = 0;
    }
}

int main(void) {
    int mode, i, mismatches = 0, cases = 0;
    COURSE *c;
    ROOM *r;

    NUM_DAYS = 195;
    NUM_HRS_DAY = 4;

    if(!professors() || !subjects() || !rooms() || !teams() || !courses()) {
        printf("data load failed\n");
        return 2;
    }
    c = courses();
    r = rooms();

    srand(12345);

    for(mode = 0; mode < 3; mode++) {
        int pop = (mode == 1) ? 20 : 40;   /* mode 1 is O(n^2) heavy, keep it smaller */
        SCHEDULE a[pop], b[pop];

        malloc_population(a, pop);
        malloc_population(b, pop);
        fill(a, pop, c, r, mode);
        for(i = 0; i < pop; i++) {
            memcpy(b[i].entry, a[i].entry, NUM_ENTRIES * sizeof(ENTRY));
            b[i].fitness = 0;
        }

        calc_fitness_old(a, pop);
        calc_fitness(b, pop);

        for(i = 0; i < pop; i++) {
            cases++;
            if(a[i].fitness != b[i].fitness) {
                mismatches++;
                if(mismatches <= 10)
                    printf("MISMATCH mode=%d schedule=%d old=%d new=%d\n",
                           mode, i, a[i].fitness, b[i].fitness);
            }
        }
        printf("mode %d: %d schedules, entries=%d, MAX_FITNESS old/new=%d/%d\n",
               mode, pop, NUM_ENTRIES, NUM_ENTRIES, MAX_FITNESS);
    }

    printf("%d schedules compared, %d mismatches -> %s\n", cases, mismatches,
           mismatches ? "FAIL" : "PASS");
    return mismatches ? 1 : 0;
}
