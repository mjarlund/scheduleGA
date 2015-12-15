#include "header/structs.h"
#include "header/functions.h"

int rand_num(int max, int min) {
    return (rand() % (max - min)) + min;
}

void copy_schedule(SCHEDULE *dest, SCHEDULE *src) {
    memcpy(dest->entry, src->entry, sizeof(ENTRY)*NUM_ENTRIES);
    dest->fitness = src->fitness;
    dest->id = src->id;
}

int entry_cmp_by_day(const void *a, const void *b){ 
    ENTRY *ia = (ENTRY *)a;
    ENTRY *ib = (ENTRY *)b;
    if ((ia->day - ib->day) != 0){
        return (ia->day - ib->day);
    } else {
        return (ia->hour - ib->hour);
    }
}

int schedule_cmp_by_fitness(const void *a, const void *b) {
    SCHEDULE *ia = (SCHEDULE *)a;
    SCHEDULE *ib = (SCHEDULE *)b;
    return -(ia->fitness - ib->fitness);
}

/*
int schedule_contains(ENTRY * arr, int n, ENTRY target) {
    int i, j;

    for(i = 0; i < n; i++) {
        if(arr[i].pos.id == target.pos.id) {
            return 1;
        }
    }

    return 0;
}
*/

/*
void fill_positions(POS * pos) {
    int i, j, k, e = 0;

    ROOM *room;
    room = rooms();

    for(i = 0; i < NUM_DAYS; i++) {
        for(j = 0; j < NUM_HRS_DAY; j++) {
            for(k = 0; k < NUM_ROOMS; k++) {
                pos[e].id = e;
                pos[e].day = rand_num(NUM_DAYS+1, 1);
                pos[e].hour = rand_num(NUM_HRS_DAY+1, 1);
                pos[e].room = room[rand_num(NUM_ROOMS, 0)];

                e++;
            }
        }
    }
}
*/