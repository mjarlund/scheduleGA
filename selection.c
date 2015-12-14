#include "header/structs.h"
#include "header/functions.h"

int roulette_selection(SCHEDULE *parent, SCHEDULE *population, int pop) {
    int sum = 0, i, n, r, j, partial_sum;

    // Calculate the fitness for the entire population.
    calc_fitness(population, pop);

    // Sort the population by fitness in descending order. (highest to lowest)
    qsort(population, pop, sizeof(SCHEDULE), schedule_cmp_by_fitness);

    // Sum the fitnesses of the entire population.
    for(i = 0; i < pop; i++) {
        sum += population[i].fitness;
    }

    /* 
        Determine how many individuals in the population should be selected for reproduction.
        1. Atleast 80% and maximum all of the population.
        3. Should be divisible by two.
    */
    do {
        n = rand_num(pop+1, pop*0.80);
    } while(n%2 != 0);

    // Loop through the selector for n parents wanted.
    for(i = 0; i < n; i++) {
        partial_sum = 0;
        // Generate a randum number from interval (0,sum).
        r = rand_num(sum, 0);

        // Loop through the population according to fitness in ascending order (lowest to highest)
        for(j = pop-1; j >= 0; j--) {
            // Sum their fitnesses from 0 to sum.
            partial_sum += population[j].fitness;

            // When the partial sum is greater then r, stop and return the parent.
            if(partial_sum >= r) {
                copy_schedule(&parent[i], &population[j]);
                break;
            }
        }
    }

    // Return the number of parents selected.
    return n;
}

void insert_children(SCHEDULE *population, int pop, SCHEDULE *child, int int_pop) {
    int i, r;

    for(i = 0; i < pop; i++) {
        population[i].id = 0;
    }

    i = 0;
    while(i < int_pop) {
        // Generate a randum number denoting the placement of a child in the population.
        r = rand_num(pop, 0);

        if(population[r].id != 1) {
            copy_schedule(&population[r], &child[i]);
            population[r].id = 1;
            i++;
        }
    }
}


