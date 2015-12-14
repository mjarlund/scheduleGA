#include "header/structs.h"
#include "header/functions.h"

void swap_mutation(SCHEDULE * schedule, int pop, int mutrate) {
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