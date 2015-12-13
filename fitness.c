#include "header/structs.h"
#include "header/functions.h"

int MAX_FITNESS, MAX_LINE_FITNESS;

void calc_fitness(SCHEDULE *schedule, int pop){
	int fitness, i, j, n, k, l;
	int ok1, ok2, ok3, ok4;
	MAX_LINE_FITNESS = 4;
	
	MAX_FITNESS = NUM_ENTRIES * MAX_LINE_FITNESS;

	for(i = 0; i < pop; i++) {
		fitness = 0;

		for(j = 0; j < NUM_ENTRIES; j++) {
			ok1 = 1, ok2 = 1, ok3 = 1, ok4 = 1;
			for(k = (j+1); k < NUM_ENTRIES; k++) {
				if(schedule[i].entry[j].day == schedule[i].entry[k].day && schedule[i].entry[j].hour == schedule[i].entry[k].hour){
					if(schedule[i].entry[j].course.team.id == schedule[i].entry[k].course.team.id){
						ok1 = 0;
					}		
					if(schedule[i].entry[j].room.id == schedule[i].entry[k].room.id){	
						ok2 = 0;
					}
					if (schedule[i].entry[j].course.professor.id == schedule[i].entry[k].course.professor.id){
						ok3 = 0;
					}

					for(l = 0; l < schedule[i].entry[j].course.team.num_students; l++){
                        for(n = 0; n < schedule[i].entry[k].course.team.num_students; n++){
                            if(schedule[i].entry[j].course.team.student[l] == schedule[i].entry[k].course.team.student[n]){
                                ok4 = 0;
                            }
                        }
                    }

				}
			}

			schedule[i].entry[j].pts = ok1+ok2+ok3+ok4;
			fitness += schedule[i].entry[j].pts;
		}

		schedule[i].fitness = fitness;

	}
}
/*

// Calculate the individual probability of survival in the population.
void calc_pos(SCHEDULE *population) {
	int sf = 0, i;

	for(i = 0; i < POP; i++) {
		sf += population[i].fitness;
	}

	// Probability of survival in the population is fitness / sum of fitnesses.
	for(i = 0; i < POP; i++) {
		population[i].pos = (double) population[i].fitness/sf;
	}
}
*/