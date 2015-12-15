#include "header/structs.h"
#include "header/functions.h"

/*
void swap_mutation(SCHEDULE *schedule, int pop, int mutrate) {
    int i, j, k, n, g;

    // Placeholder for the swapping positions.
    POS buffer[2];

    for(i = 0; i < pop; i++) {
        for(j = 0; j < mutrate; j++) {
            // Select first position up for swapping.
            k = rand_num(NUM_ENTRIES, 0);

            buffer[0] = schedule[i].entry[k].pos;

            // Select second position up for swapping.
            g = rand_num(NUM_ENTRIES, 0);

            buffer[1] = schedule[i].entry[g].pos;

            // Swap the two positions.
            schedule[i].entry[g].pos = buffer[0];
            schedule[i].entry[k].pos = buffer[1];
        }
    }
}
*/

void mutation(SCHEDULE *schedule, int pop, int mutrate, ROOM *room) {
    int i, j, k, r, x, mutprob;

    
    for(i = 0; i < pop; i++) {
        k = rand_num(101, 0);
        
        // A mutation probability is set to 95%, meaning there is a 5% chance of not applying mutation.
        mutprob = 100 / pop;

        if(k <= mutprob) {
            
            for(j = 0; j < mutrate; j++) {

                // Select entry for mutation.
                r = rand_num(3, 0);

                if(r==2) {
                    schedule[i].entry[j].day = rand_num(NUM_DAYS+1, 1);
                } else if(r==1) {
                    schedule[i].entry[j].hour = rand_num(NUM_HRS_DAY+1, 1);
                } else {
                    schedule[i].entry[j].room = room[rand_num(NUM_ROOMS, 0)];
                }
                
                
                
            }
        }  
    }
}