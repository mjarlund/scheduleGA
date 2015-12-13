#include "header/structs.h"
#include "header/functions.h"


/*
void selection_roulette(SCHEDULE *selected, SCHEDULE *selection) {
    int sf = 0, i, j, g, ps, rank_score = 1;
    int rank[MAX_POP];

    calc_fitness(selection, MAX_POP); // Calculate fitness values in the selection pool.

    // Sort elements in the selection pool by fitness values in descending order.
    qsort(selection, MAX_POP, sizeof(SCHEDULE), schedule_cmp_by_fitness); 

    for(i = pop-1; i >= 0; i--) {
        rank[i] = rank_score++;
        sf += rank[i];
    }

    for(i = 0; i < POP; i++) {
        g = rand_num(sf)+1;

        ps = 0;
        for(j = (MAX_POP)-1; j >= 0; j--) {
            if(j+1 != MAX_POP && rank[j] != rank[j+1]) {
                ps += rank[j];
            }

            if(ps >= g) {
                selected[i] = selection[j];
                break;
            }
        }
    }
}
*/

void selection_tournament(SCHEDULE *winner, int n1, SCHEDULE *competetor, int n2) {
    int i, j, k = 0, r = 0, d, n;
    ENTRY buffer[NUM_ENTRIES];
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

        for(n = 0; n < NUM_ENTRIES; n++) {
            buffer[n] = competetor[d].entry[n];
            winner[i].entry[n] = buffer[n];
        }
        
        winner[i].fitness = competetor[d].fitness;
    }
}



