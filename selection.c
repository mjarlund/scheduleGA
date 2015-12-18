#include "header/structs.h"
#include "header/functions.h"

SCHEDULE tournament_selection(SCHEDULE *p, int popsize) {
    int i, s, e;
    SCHEDULE *best;

    // Randomly pick a random individual from P with replacement.
    best = &p[rand_num(popsize,0)];

    // Randomly pick the tournament size in which the individual is to compete against other individuals.
    s = rand_num(popsize/2, 0);
    e = rand_num(popsize, s+1);

    for(i = s; i < e; i++) {
        if(best->fitness < p[i].fitness) {
            // a new "best" individual has been chosen, so replace best.
            best = &p[i];
        }
    }

    // Return the winner of the tournament.
    return *best;
}