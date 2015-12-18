#include "header/structs.h"
#include "header/functions.h"

void uniform_crossover(SCHEDULE *child1, SCHEDULE *child2, SCHEDULE parent1, SCHEDULE parent2) {
    int i;
    double g, crossprob;

    // Make a copy of the parents, and assign it to the children.
    copy_schedule(child1, &parent1);
    copy_schedule(child2, &parent2);

    // Probability of mutation for each element, each element has an equal chance of being mutated.
    crossprob = (1.0/NUM_ENTRIES);

    for(i = 0; i < NUM_ENTRIES; i++) {
        // Random number determining wether to cross or not.
        g = randZeroToOne();

        if(crossprob >= g) {
            child1->entry[i].day =  parent2.entry[i].day;
            child1->entry[i].hour = parent2.entry[i].hour;
            child1->entry[i].room = parent2.entry[i].room;

            child2->entry[i].day =  parent1.entry[i].day;
            child2->entry[i].hour = parent1.entry[i].hour;
            child2->entry[i].room = parent1.entry[i].room;
        }     
    }
}