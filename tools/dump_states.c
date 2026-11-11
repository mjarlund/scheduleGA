/* Cross-build equivalence harness for the index-based ENTRY.
 *
 * The parent branch (`improvement/bucket-fitness`) embeds a whole COURSE and a
 * ROOM in every ENTRY; this branch stores table indices instead.  The two
 * layouts cannot coexist in one binary, so the harness is compiled twice - once
 * against each tree - and the two programs are fed identical, seeded
 * populations.  For every schedule it prints the fitness plus an FNV-1a digest
 * of the *resolved* rows (day;hour;professor;subject;team;room), which is what
 * the exported CSV is made of.
 *
 * Build (see tools/run_equivalence.sh); the parent build omits -DENTRY_INDICES:
 *   cc -O2 -std=c99 -I. -o dump-old tools/dump_states.c fitness.c parsing.c \
 *      initialize.c functions.c crossover.c mutation.c selection.c export.c
 *   cc -O2 -std=c99 -I. -DENTRY_INDICES -o dump-new tools/dump_states.c ...
 */
#include "header/structs.h"
#include "header/functions.h"

static COURSE *CT;
static ROOM *RT;

/* main.c, which normally owns these, is not part of the harness. */
int NUM_DAYS, NUM_HRS_DAY;

/* Single place that knows how an ENTRY stores its course and room. */
#ifdef ENTRY_INDICES
#  define ENTRY_SET(e, ci, ri) do { (e).course = (ci); (e).room = (ri); } while(0)
#  define ENTRY_COURSE(e) (&CT[(e).course])
#  define ENTRY_ROOM(e)   (&RT[(e).room])
#else
#  define ENTRY_SET(e, ci, ri) do { (e).course = CT[ci]; (e).room = RT[ri]; } while(0)
#  define ENTRY_COURSE(e) (&(e).course)
#  define ENTRY_ROOM(e)   (&(e).room)
#endif

#define MODES 5

static unsigned long fnv(const char *s) {
    unsigned long h = 2166136261UL;
    while(*s) {
        h ^= (unsigned char)*s++;
        h = (h * 16777619UL) & 0xffffffffUL;
    }
    return h;
}

static void fill(SCHEDULE *p, int n, int mode) {
    int i, j;

    for(i = 0; i < n; i++) {
        for(j = 0; j < NUM_ENTRIES; j++) {
            int ci = rand_num(NUM_COURSES, 0);
            int ri = rand_num(NUM_ROOMS, 0);

            ENTRY_SET(p[i].entry[j], ci, ri);
            switch(mode) {
                case 0:  /* uniform, exactly like init_population */
                    p[i].entry[j].day = rand_num(NUM_DAYS + 1, 1);
                    p[i].entry[j].hour = rand_num(NUM_HRS_DAY + 1, 1);
                    break;
                case 1:  /* every entry in one slot: worst case for the scan */
                    p[i].entry[j].day = 1;
                    p[i].entry[j].hour = 1;
                    break;
                case 2:  /* three days, heavy collisions */
                    p[i].entry[j].day = rand_num(3, 0);
                    p[i].entry[j].hour = rand_num(NUM_HRS_DAY + 1, 1);
                    break;
                case 3:  /* one slot AND one room: every pair clashes */
                    p[i].entry[j].day = 42;
                    p[i].entry[j].hour = 3;
                    ri = 0;
                    ENTRY_SET(p[i].entry[j], ci, ri);
                    break;
                default: /* one slot, one room, one course: identical entries */
                    p[i].entry[j].day = 7;
                    p[i].entry[j].hour = 2;
                    ENTRY_SET(p[i].entry[j], 0, 0);
                    break;
            }
        }
        p[i].fitness = 0;
    }
}

int main(void) {
    int mode, i, j;

    NUM_DAYS = 195;
    NUM_HRS_DAY = 4;

    if(!professors() || !subjects() || !rooms() || !teams() || !courses()) {
        printf("data load failed\n");
        return 2;
    }
    CT = courses();
    RT = rooms();

    printf("SIZE;sizeof_ENTRY=%zu;entries=%d;courses=%d;rooms=%d\n",
           sizeof(ENTRY), NUM_ENTRIES, NUM_COURSES, NUM_ROOMS);

    srand(424242);

    for(mode = 0; mode < MODES; mode++) {
        int pop = (mode == 1) ? 20 : 40;   /* mode 1 is quadratic for the parent */
        SCHEDULE s[pop];
        int fp;

        malloc_population(s, pop);
        fill(s, pop, mode);
        fp = s[0].fitness;
        (void)fp;

        calc_fitness(s, pop);

        for(i = 0; i < pop; i++) {
            char row[160];
            unsigned long h = 2166136261UL;

            for(j = 0; j < NUM_ENTRIES; j++) {
                COURSE *c = ENTRY_COURSE(s[i].entry[j]);
                ROOM *r = ENTRY_ROOM(s[i].entry[j]);

                snprintf(row, sizeof(row), "%d;%d;%d;%d;%d;%d;%s;%s;%s",
                         s[i].entry[j].day, s[i].entry[j].hour,
                         c->professor.id, c->subject.id, c->team.id, r->id,
                         c->professor.name, c->subject.name, c->team.name);
                h ^= fnv(row);
                h = (h * 16777619UL) & 0xffffffffUL;
            }
            printf("S;%d;%d;%d;%08lx\n", mode, i, s[i].fitness, h);
        }
        for(i = 0; i < pop; i++) {
            free(s[i].entry);
        }
    }

    return 0;
}
