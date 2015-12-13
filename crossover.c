#include "header/structs.h"
#include "header/functions.h"

int single_point_crossover(SCHEDULE *offspring, SCHEDULE *population, int n) {
    int i = 0, j, k, g, s, total = 0;

    while(i < n) {
        /* 
            Determine splitpoint for crossover.
            1. Atleast 1/4 of the number of entries and maximum 3/4 of the number of entries. (To ensure deviance)
        */
        s = rand_num(((NUM_ENTRIES*0.75)+1), (NUM_ENTRIES*0.25));

        /*
            Determine if one or two children should be produced in the mating process.
            1. Atleast one and maximum two.
        */
        k = rand_num(3, 1);

        copy_schedule(&offspring[i], &population[i]);

        if(k != 2) { // Produce one child.
            for(j = 0; j < s; j++) {
                offspring[i].entry[j] = population[n].entry[j];
            }

            total += 1;
        } else { // Produce two children.
            copy_schedule(&offspring[n], &population[n]);

            for(j = 0; j < s; j++) {
                offspring[i].entry[j] = population[n].entry[j];
                offspring[n].entry[j] = population[i].entry[j];
            }

            total += 2;
        }

        i++;
        n--;
    }

    // Return total number of crossover products.
    return total;
} 