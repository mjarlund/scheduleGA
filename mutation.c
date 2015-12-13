#include "header/structs.h"
#include "header/functions.h"

void mutate(SCHEDULE * schedule, int pop) {
    int i, j, k, n, g;

    ROOM *room;
    room = rooms();

    for(i = 0; i < pop; i++) {
        // Mutation rate 1%.
        n = (NUM_ENTRIES/100);

        for(j = 0; j < n; j++) {
            // Choose what entry to mutate.
            k = rand_num(NUM_ENTRIES, 0);

            schedule[i].entry[k].day = rand_num(NUM_DAYS, 0);
            schedule[i].entry[k].hour = rand_num(5, 1);
            schedule[i].entry[k].room = room[rand_num(NUM_ROOMS, 0)];

        }
    }
}