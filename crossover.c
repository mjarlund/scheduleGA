#include "header/structs.h"
#include "header/functions.h"

void single_point_crossover(SCHEDULE *child, SCHEDULE *parent, int pop) {
    int i, j, k, r;

    for(i = 0; i < pop; i += 2) {

        // Choose a random splitpoint.
        r = rand_num(NUM_ENTRIES, 0);

        // Copy parent1 to child1
        copy_schedule(&child[i], &parent[i]);
        // Copy parent2 to child2
        copy_schedule(&child[i+1], &parent[i+1]);

        // Copy position elements that are not already in child from parent.
        for(j = 0; j < r; j++) {

            child[i].entry[j].day = parent[i+1].entry[j].day;  
            child[i].entry[j].hour = parent[i+1].entry[j].hour; 
            child[i].entry[j].room = parent[i+1].entry[j].room; 

            child[i+1].entry[j].day = parent[i].entry[j].day;   
            child[i+1].entry[j].hour = parent[i].entry[j].hour; 
            child[i+1].entry[j].room = parent[i].entry[j].room; 


            /*
            if(!schedule_contains(child[i].entry, NUM_ENTRIES, parent[i+1].entry[j])) {

                child[i].entry[j].pos = parent[i+1].entry[j].pos;

            } else if(!schedule_contains(child[i+1].entry, NUM_ENTRIES, parent[i].entry[j])) {

                child[i+1].entry[j].pos = parent[i].entry[j].pos;
            }
            */
        }
    }
    
} 