#include "header/structs.h"
#include "header/functions.h"

int single_point_crossover(SCHEDULE *offspring, SCHEDULE *population, int pop) {
    int i = 0, j = 0, k, g, s, total = 0;

    ENTRY c1[NUM_ENTRIES], c2[NUM_ENTRIES];

    /* 
        Determine how many individuals in the population should reproduce.
        1. Atleast 20% and maximum 80% of the population.
        3. Should be divisible by two.
    */
    while(j%2 == 0) {
        j = rand_num(pop*0.8, pop*0.2);
    }

    while(i < j) {
        /* 
            Determine splitpoint for crossover.
            1. Atleast 1/4 of the number of entries and maximum 3/4 of the number of entries. (To ensure deviance)
        */
        s = rand_num(((NUM_ENTRIES*0.75)+1), (NUM_ENTRIES*0.25));

        for(k = 0; k < NUM_ENTRIES; k++) {
            if(k < s) {
                c1[k] = population[i].entry[k];
                c2[k] = population[j].entry[k];

                /*
                offspring[i].entry[k].day = population[j].entry[k].day;
                offspring[i].entry[k].hour = population[j].entry[k].hour;
                offspring[i].entry[k].room = population[j].entry[k].room;

                offspring[j].entry[k].day = population[i].entry[k].day;
                offspring[j].entry[k].hour = population[i].entry[k].hour;
                offspring[j].entry[k].room = population[i].entry[k].room;
                */
            } else {
                c1[k] = population[j].entry[k];
                c2[k] = population[i].entry[k];

                /*
                offspring[i].entry[k].day = population[i].entry[k].day;
                offspring[i].entry[k].hour = population[i].entry[k].hour;
                offspring[i].entry[k].room = population[i].entry[k].room;

                offspring[j].entry[k].day = population[j].entry[k].day;
                offspring[j].entry[k].hour = population[j].entry[k].hour;
                offspring[j].entry[k].room = population[j].entry[k].room;
                */
            }

            offspring[i].entry[k] = c1[k];
            offspring[j].entry[k] = c2[k];
        }
       
        

        total += 2;
        i++;
        j--;
    }

    // Return total number of crossover products.
    return total;
} 