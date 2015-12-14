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

int append_population(SCHEDULE *dest, int n1, SCHEDULE *src, int n2) {
    int i, j, n3;

    n3 = n1+n2;

    for(i = 0; i < n2; i++) {
        copy_schedule(&dest[n1], &src[i]);
        n1++;
    }

    // Return total ammount of elements in the population.
    return n3;
}

int entry_cmp_by_day(const void *a, const void *b){ 
    ENTRY *ia = (ENTRY *)a;
    ENTRY *ib = (ENTRY *)b;
    if ((ia->genome.day - ib->genome.day) != 0){
        return (ia->genome.day - ib->genome.day);
    } else {
        return (ia->genome.hour - ib->genome.hour);
    }
}

int schedule_cmp_by_fitness(const void *a, const void *b) {
    SCHEDULE *ia = (SCHEDULE *)a;
    SCHEDULE *ib = (SCHEDULE *)b;
    return -(ia->fitness - ib->fitness);
}

int test(SCHEDULE *population, int pop, SCHEDULE *optimum) {
    int i;

    if(optimum->fitness != population[0].fitness) {
        // A new optimum has been found.
        if(population[0].fitness > optimum->fitness) {
            // Save a copy of the best schedule so far.
            copy_schedule(optimum, &population[0]);

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