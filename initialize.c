#include "header/structs.h"
#include "header/functions.h"
#include "header/objective.h"

void malloc_schedule(SCHEDULE *dest) {
    dest->entry = malloc(NUM_ENTRIES * sizeof(ENTRY));
    dest->fitness = 0;
    dest->violations = NUM_ENTRIES;   /* "nothing scheduled yet", so the first
                                         evaluated individual always wins */
}

void malloc_population(SCHEDULE *dest, int n) {
    int i;

    for(i = 0; i < n; i++) {
        dest[i].entry = malloc(NUM_ENTRIES * sizeof(ENTRY));
        dest[i].fitness = 0;
        dest[i].violations = NUM_ENTRIES;
    }
}

/* The genome is a list of (day, hour, room index, course index).  Course c owns
 * exactly course[c].subject.hours entries, and its *index* is stored instead of
 * a copy of the 280-byte course.  The three rand() calls per entry are unchanged
 * and in the same order, so the random stream is bit-identical to the embedded
 * version.
 *
 * TYPED_INIT=1: the room is drawn from the rooms whose type matches the
 * subject's type instead of from all twelve rooms.  The old code drew
 * uniformly, so ~60 % of a fresh genome sat in a room that could never host
 * its subject and the first generations were pure room repair (generation 1
 * scored ~95/267).  Still one rand() call per entry, so the stream keeps its
 * shape. */
void init_population(SCHEDULE *dest, int n) {
    int i, j, k, e;

    for(k = 0; k < n; k++) {
        e = 0;

        // Itterate over all of the courses.
        for(i = 0; i < NUM_COURSES; i++) {
            // Count number of hours for the corresponding course.
            for(j = 0; j < COURSE_TABLE[i].subject.hours; j++) {
                dest[k].entry[e].course = i;
                dest[k].entry[e].day = rand_num(NUM_DAYS+1, 1);
                dest[k].entry[e].hour = rand_num(NUM_HRS_DAY+1, 1);
#if TYPED_INIT
                dest[k].entry[e].room = random_room_of_type(COURSE_TABLE[i].subject.type);
#else
                dest[k].entry[e].room = rand_num(NUM_ROOMS, 0);
#endif
                e++;
            }
        } 
    }
}