#include "header/structs.h"
#include "header/functions.h"

void mutation(SCHEDULE *child, ROOM *room) {
    int i;
    double mutprob, g;

    // Probability of mutation for each element, each element has an equal chance of being mutated.
    mutprob = (1.0/NUM_ENTRIES);

    for(i = 0; i < NUM_ENTRIES; i++) {
        // Random number determining wether to mutate or not.
        g = randZeroToOne();

        if(mutprob >= g) {

            child->entry[i].day = rand_num(NUM_DAYS+1, 1);
            child->entry[i].hour = rand_num(NUM_HRS_DAY+1, 1);
            child->entry[i].room = room[rand_num(NUM_ROOMS, 0)];
        }     
    }
}