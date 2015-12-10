#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "header/structs.h"
#include "header/functions.h"


void single_point_crossover(SCHEDULE *mom, SCHEDULE *dad) {
    int i, j;
    ENTRY c1[NUM_ENTRIES], c2[NUM_ENTRIES];

        j = rand_num(NUM_ENTRIES);

        for(i = 0; i < NUM_ENTRIES; i++) {
            if(i < j) {
                c1[i] = mom->entry[i];
                c2[i] = dad->entry[i];
            } else {
                c2[i] = mom->entry[i];
                c1[i] = dad->entry[i];
            }

            dad->entry[i].day = c2[i].day;
            dad->entry[i].hour = c2[i].hour;
            dad->entry[i].room = c2[i].room;

            mom->entry[i].day = c1[i].day;
            mom->entry[i].hour = c1[i].hour;
            mom->entry[i].room = c1[i].room;

            // Reset fitness values.
            dad->fitness = 0;
            mom->fitness = 0;

        }

} 