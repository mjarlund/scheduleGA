#include "header/structs.h"
#include "header/functions.h"

int rand_num(int max, int min) {
    return (rand() % (max - min)) + min;
}

int append_population(SCHEDULE *dest, int n1, SCHEDULE *src, int n2) {
    int i, j, n3;

    n3 = n1+n2;
    ENTRY buffer[NUM_ENTRIES];

    for(i = 0; i < n2; i++) {
        for(j=0; j < NUM_ENTRIES; j++) {
            buffer[j] = src[i].entry[j]; 
        }
        for(j=0; j < NUM_ENTRIES; j++) {
            dest[n1].entry[j] = buffer[j];   
        }
        n1++;
    }

    // Return total ammount of elements in the population.
    return n3;
}

/* 
double rand_double(double max) {
    return fmod(rand(), max);
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

*/

int schedule_cmp_by_fitness(const void *a, const void *b) {
    SCHEDULE *ia = (SCHEDULE *)a;
    SCHEDULE *ib = (SCHEDULE *)b;
    return -(ia->fitness - ib->fitness);
}

int test(SCHEDULE *population, int pop, SCHEDULE *optimum) {
    int i;
    ENTRY buffer[NUM_ENTRIES];

    // Calculate the fitness values of the schedules.
    calc_fitness(population, pop);

    // Sort by fitness score in descending order.
    qsort(population, pop, sizeof(SCHEDULE), schedule_cmp_by_fitness);

    // Save a copy of the best schedule so far.
    if(population[0].fitness > optimum->fitness) {
        for(i = 0; i < NUM_ENTRIES; i++) {
            buffer[i] = population[0].entry[i];
        }

        for(i = 0; i < NUM_ENTRIES; i++) {
             optimum->entry[i] = buffer[i];
        }

        optimum->fitness = population[0].fitness;
    } else {
        for(i = 0; i < NUM_ENTRIES; i++) {
            buffer[i] = optimum->entry[i];
        }

        for(i = 0; i < NUM_ENTRIES; i++) {
             population[pop-5].entry[i] = buffer[i];
        }
    }


    // Test wether or not opimum is a solution to the problem at hand.
    if(optimum->fitness >= MAX_FITNESS) {
        return 0;
    }

    return 1;
} 