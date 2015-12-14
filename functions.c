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
    if ((ia->pos.day - ib->pos.day) != 0){
        return (ia->pos.day - ib->pos.day);
    } else {
        return (ia->pos.hour - ib->pos.hour);
    }
}

int schedule_cmp_by_fitness(const void *a, const void *b) {
    SCHEDULE *ia = (SCHEDULE *)a;
    SCHEDULE *ib = (SCHEDULE *)b;
    return -(ia->fitness - ib->fitness);
}

int test(SCHEDULE *population, int pop, SCHEDULE *optimum, COURSE *course, POS *pos) {
    int i;

    SCHEDULE newguy;
    malloc_schedule(&newguy);

    if(optimum->fitness != population[0].fitness) {
        // A new optimum has been found.
        if(population[0].fitness > optimum->fitness) {
            // Save a copy of the best schedule so far.
            copy_schedule(optimum, &population[0]);

            // Generate a new random solution, and add it to the population, overwriting the least fit candidate.
            add_schedule(&newguy, course, pos);

            copy_schedule(&population[pop-1], &newguy);

            // Test wether or not opimum is a solution to the problem at hand.
            if(optimum->fitness >= MAX_FITNESS) {
                return 0;
            }
        } else {
            copy_schedule(&population[pop-1], optimum);
        }
    }

    return 1;
}

int schedule_contains(ENTRY * arr, int n, ENTRY target) {
    int i, j;

    for(i = 0; i < n; i++) {
        if(arr[i].pos.id == target.pos.id) {
            return 1;
        }
    }

    return 0;
}


void fill_positions(POS * pos) {
    int i, j, k, e;

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