#include "header/structs.h"
#include "header/functions.h"
int MAX_FITNESS;

/* Per-slot bucketing of the conflict check.
 *
 * The original scanned every ordered pair (j,k), j<k, for every entry j, i.e.
 * NUM_ENTRIES*(NUM_ENTRIES-1)/2 iterations per schedule, although only entries
 * placed in the same (day,hour) slot can ever conflict.  The timetable has
 * NUM_DAYS*NUM_HRS_DAY = 780 slots, so ~99% of those comparisons were decided
 * by a single failed day/hour test.
 *
 * Here each schedule is indexed by slot once: head[s] holds the lowest entry
 * index in slot s and next[j] the next higher index in the same slot (-1
 * terminates).  Walking next[] from j therefore visits exactly
 * { k : k > j and slot(k) == slot(j) }, in increasing order -- the same
 * comparison set as before, so the fitness definition is untouched.
 */
void calc_fitness(SCHEDULE *schedule, int pop){
    int fitness, i, j, k, l, n, score, slot, cur = 0;
    int *next, *head, *stamp;
    int nslots = (NUM_DAYS + 1) * (NUM_HRS_DAY + 1);

    // Max fitness is equal to the number of permutations.
    MAX_FITNESS = NUM_ENTRIES;

    next = malloc((NUM_ENTRIES + 1) * sizeof(int));
    head = malloc(nslots * sizeof(int));
    stamp = calloc(nslots, sizeof(int));
    if(next == NULL || head == NULL || stamp == NULL) {
        perror("calc_fitness");
        exit(-1);
    }

    for(i = 0; i < pop; i++) {
        ENTRY *entry = schedule[i].entry;

        /* Slots are stamped instead of cleared: only the slots actually used
           by this schedule are touched (NUM_ENTRIES writes, not NUM_DAYS *
           NUM_HRS_DAY), and the timetable normally uses far fewer slots than
           the grid has. */
        cur++;
        for(j = NUM_ENTRIES - 1; j >= 0; j--) {
            slot = entry[j].day * (NUM_HRS_DAY + 1) + entry[j].hour;
            /* Data outside the expected ranges means the schedule itself is
               garbage (the CSV parsers truncate long lines, which can leave
               entries uninitialised); such an entry is simply not bucketed
               instead of indexing head[] out of bounds. */
            if(slot < 0 || slot >= nslots) {
                next[j] = -1;
            } else {
                if(stamp[slot] != cur) {
                    stamp[slot] = cur;
                    head[slot] = -1;
                }
                next[j] = head[slot];
                head[slot] = j;
            }
        }

        fitness = 0;
        for(j = 0; j < NUM_ENTRIES; j++) {
            ENTRY *a = &entry[j];

            score = 1;
            if(a->course.subject.type == a->room.type) {
                // Only entries in the same (day,hour) slot can conflict.
                for(k = next[j]; k != -1; k = next[k]) {
                    ENTRY *b = &entry[k];

                    if(a->room.id == b->room.id) {
                        score = 0;
                        break;
                    } else if(a->course.team.id == b->course.team.id) {
                        score = 0;
                        break;
                    } else if(a->course.professor.id == b->course.professor.id) {
                        score = 0;
                        break;
                    } else {
                        // Check if two or more students are set to be at the same place at the same time.
                        for(l = 0; l < a->course.team.num_students; l++) {
                            for(n = 0; n < b->course.team.num_students; n++) {
                                if(a->course.team.student[l] == b->course.team.student[n]) {
                                    score = 0;
                                    break;
                                }
                            }
                            if(!score) {
                                break;
                            }
                        }
                        if(!score) {
                            break;
                        }
                    }
                }
            } else {
                score = 0;
            }
            // Indent the fitness score.
            fitness += score;
        }
        // Set schedule fitness, and start over for next entry.
        schedule[i].fitness = fitness;
    }

    free(next);
    free(head);
    free(stamp);
}
