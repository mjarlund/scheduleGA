#include "header/structs.h"
#include "header/functions.h"
#include "header/objective.h"

void mutation(SCHEDULE *child) {
    int i;
    double mutprob, g;

    // Probability of mutation for each element, each element has an equal chance of being mutated.
    mutprob = (1.0/NUM_ENTRIES);

    for(i = 0; i < NUM_ENTRIES; i++) {
        // Random number determining wether to mutate or not.
        g = randZeroToOne();

        if(mutprob >= g) {

            /* Day.  MUT_DAY_WINDOW = 0 reproduces the 2015 operator (a day
               drawn from all NUM_DAYS); a window makes it a creep mutation so
               the time-of-week terms can be hill-climbed instead of being
               fought by the operator.  One rand() call either way. */
#if MUT_DAY_WINDOW > 0
            {
                int d = child->entry[i].day
                      + rand_num(2 * MUT_DAY_WINDOW + 1, 0) - MUT_DAY_WINDOW;
                if(d < 1) d = 1;
                if(d > NUM_DAYS) d = NUM_DAYS;
                child->entry[i].day = d;
            }
#else
            child->entry[i].day = rand_num(NUM_DAYS+1, 1);
#endif
            child->entry[i].hour = rand_num(NUM_HRS_DAY+1, 1);
            /* TYPED_INIT=1: a mutated room is drawn from the rooms of the
               subject's own type, so mutation can rearrange a lesson but never
               turn it into a biology-in-the-gym entry.  The old code re-drew
               from all rooms, which broke the type match on ~3 of every 4 room
               mutations and made the search spend its time undoing its own
               mutations.  One rand() call either way. */
#if TYPED_INIT
            child->entry[i].room =
                random_room_of_type(COURSE_TABLE[child->entry[i].course].subject.type);
#else
            child->entry[i].room = rand_num(NUM_ROOMS, 0);
#endif
        }     
    }
}
