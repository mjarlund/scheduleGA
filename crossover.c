#include "header/structs.h"
#include "header/functions.h"

int single_point_crossover(SCHEDULE *offspring, SCHEDULE *population, int int_pop) {
    int i, j, k, s, g, count = 0;

    for(i = 0; i < int_pop; i++) {
        j = i+1;

        /* 
            Determine splitpoint for crossover.
            1. Atleast 1/4 of the number of entries and maximum 3/4 of the number of entries. (To ensure deviance)
        */
        s = rand_num(((NUM_ENTRIES*0.75)+1), (NUM_ENTRIES*0.25));

        /* 
            Determine number of offspring produced by crossover.
            1. Atleast one and maximum two.
        */
        g = rand_num(3, 1);

        copy_schedule(&offspring[i], &population[i]);

        if(g != 2) {
            for(k = 0; k < s; k++) {
                offspring[i].entry[k].pos = population[j].entry[k].pos;  
            }
            count++;
            int_pop--;
        } else {
            copy_schedule(&offspring[j], &population[j]);

            for(k = 0; k < s; k++) {
                offspring[i].entry[k].pos = population[j].entry[k].pos;
                offspring[j].entry[k].pos = population[i].entry[k].pos;
            }
            count += 2;
            i++;
        }
    }

    return count;
} 