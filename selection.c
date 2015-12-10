#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "header/structs.h"
#include "header/functions.h"




void get_optimum(SCHEDULE **population, SCHEDULE *optimum) {
    // Sort by fitness score in descending order.
    qsort(population, POP, sizeof(SCHEDULE*), schedule_cmp_by_fitness);

    // Gets the highest scoring solution throughout the generations.
    if(population[0]->fitness > optimum->fitness) {
        *optimum = *population[0];
    }

}


int selection_roulette(SCHEDULE **population) {
    int total_sum = 0, partial_sum = 0, i, j, r;

    qsort(population, POP, sizeof(SCHEDULE*), schedule_cmp_by_fitness);

    // Get total sum of fitness values.
    for(i = 0; i < POP; i++) {
        total_sum += population[i]->fitness;
    }
    
    partial_sum = 0;

    // Generate random number between 0 and total_sum.
    r = rand_num(total_sum)+1;

    // Loop through the population once more, and calculate a partial sum.
    // When the partial sum is greater than or equal to r, return the index of the solution. (Selected solution).
    for(j = POP-1; j >= 0; j--) {
        partial_sum += population[j]->fitness;
        if(partial_sum >= r) {
            return j;
        }
    }

    // Something went wrong.
    return -1;

}