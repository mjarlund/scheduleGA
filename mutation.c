#include "header/structs.h"
#include "header/functions.h"

void mutate(SCHEDULE * schedule, int pop, int mutrate, POS * pos) {
    int i, j, k, n, g;

    ROOM *room;
    room = rooms();

    for(i = 0; i < pop; i++) {

        for(j = 0; j < mutrate; j++) {
            // Choose what entry to mutate.
            k = rand_num(NUM_ENTRIES, 0);

            schedule[i].entry[k].pos = pos[rand_num(NUM_POS, 0)];
        }
    }
}