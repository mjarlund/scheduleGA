#include "header/structs.h"
#include "header/functions.h"

void mutate(SCHEDULE * schedule, int pop) {
    int i, j, n, g;

    ROOM *room;
    room = rooms();

    for(i = 0; i < pop; i++) {
        // Generate a random number between 0-10.
        g = rand_num(10, 0);
        // Let the random number denote the percentage of mutation performed on the schedules.
        n = (NUM_ENTRIES/100)*g;

        for(j = 0; j < n; j++) {
            schedule[i].entry[rand_num(NUM_ENTRIES, 0)].day = rand_num(NUM_DAYS, 0);
            schedule[i].entry[rand_num(NUM_ENTRIES, 0)].hour = rand_num(5, 1);
            schedule[i].entry[rand_num(NUM_ENTRIES, 0)].room = room[rand_num(NUM_ROOMS, 0)];
        }
    }
}