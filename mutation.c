#include "header/structs.h"
#include "header/functions.h"

void mutate(SCHEDULE * schedule, int pop, int mutrate) {
    int i, j, k, n, g;

    ROOM *room;
    room = rooms();

    for(i = 0; i < pop; i++) {

        for(j = 0; j < mutrate; j++) {
            // Choose what entry to mutate.
            k = rand_num(NUM_ENTRIES, 0);

            schedule[i].entry[k].genome.day = rand_num(NUM_DAYS+1, 1);
            schedule[i].entry[k].genome.hour = rand_num(5, 1);
            schedule[i].entry[k].genome.room = room[rand_num(NUM_ROOMS, 0)];

        }
    }
}