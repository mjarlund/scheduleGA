#include "header/structs.h"
#include "header/functions.h"

int roulette_selection(SCHEDULE *parent, SCHEDULE *population, int n) {
    int sum = 0, i, n_parents = 1, r, j, partial_sum;

    // Calculate the fitness for the entire population.
    calc_fitness(population, n);

    // Sort the population by fitness in descending order. (highest to lowest)
    qsort(population, n, sizeof(SCHEDULE), schedule_cmp_by_fitness);

    // Sum the fitnesses of the entire population.
    for(i = 0; i < n; i++) {
        sum += population[i].fitness;
    }

    /* 
        Determine how many individuals in the population should be selected for reproduction.
        1. Atleast 80% and maximum 95% of the population.
        3. Should be divisible by two.
    */
    while(n_parents%2 == 0) {
        n_parents = rand_num(n*0.95, n*0.80);
    }

    // Loop through select parents for n parents wanted.
    for(i = 0; i < n_parents; i++) {
        partial_sum = 0;
        // Generate a randum number from interval (0,sum).
        r = rand_num(sum, 0);

        // Loop through the population according to fitness in ascending order (lowest to highest)
        for(j = n-1; j >= 0; j--) {
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
    return n_parents;
}

void reduce_population(SCHEDULE *survivor, int pop_size, SCHEDULE *population, int n) {
    int i, j, r, duplicate;

    // Give every member of the population a unique id.
    for(i = 0; i < n; i++) {
        population[i].id = i+1;
    }

    i = 0;

    while(i < pop_size) {
        duplicate = 0;

        // Generate a random number (0,n)
        r = rand_num(n, 0);

        // Check if population[r] has already been chosen once.
        for(j = 0; j < i; j++) {
            if(survivor[j].id == population[r].id) {
                duplicate = 1;
            }
        }  

        if(!duplicate) {
            // Let population[r] survive.
            copy_schedule(&survivor[i], &population[r]);
            survivor[i].id = population[r].id;
            i++; 
        }
    }
}

/*
void selection_tournament(SCHEDULE *winner, int n1, SCHEDULE *competetor, int n2) {
    int i, j, k = 0, r = 0, d, n;

    // Calculate the competetors fitness values.
    calc_fitness(competetor, n2);

    for(i = 0; i < n1; i++) {
        // Find two random competetors for the tournament.
        r = rand_num(n2, 0);
        k = rand_num(n2, 0);
        while(r == k) {
            r = rand_num(n2, 0);
        }

        // Choose winner and assign to array of winners.
        d = competetor[r].fitness >= competetor[k].fitness ? r : k;

        copy_schedule(&winner[i], &competetor[d]);
    }
}
*/


