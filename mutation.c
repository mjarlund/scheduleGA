#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "header/structs.h"
#include "header/functions.h"

void mutate(SCHEDULE *schedule) {
    int i, j, n;

    ROOM *room;
    room = rooms();

    // Number of entries to mutate.
    n = NUM_ENTRIES;//*0.01;

    for(i = 0; i < n; i++) {
		if(schedule->entry[i].pts != MAX_LINE_FITNESS){
        schedule->entry[i].day = rand_num(NUM_DAYS);
        schedule->entry[i].hour = rand_num(4)+1;
        schedule->entry[i].room = room[rand_num(NUM_ROOMS)];
		}
    }

    free(room);
}