#include "header/structs.h"
#include "header/functions.h"

int MAX_FITNESS;

void calc_fitness(SCHEDULE *schedule, int pop){
	int fitness, i, j, n, k, l;

	// Max fitness is equal to the number of permutations.
	MAX_FITNESS = NUM_ENTRIES;

	for(i = 0; i < pop; i++) {
		fitness = 0;

		for(j = 0; j < NUM_ENTRIES; j++) {


			for(k = (j+1); k < NUM_ENTRIES; k++) {

				
				if(schedule[i].entry[j].pos.id == schedule[i].entry[k].pos.id) {
					// If position of one entry is equal to another...
					break;

				} else if(schedule[i].entry[j].pos.day == schedule[i].entry[k].pos.day && schedule[i].entry[j].pos.hour == schedule[i].entry[k].pos.hour) {
					// If one individual ()...
					if(schedule[i].entry[j].course.team.id == schedule[i].entry[k].course.team.id) {
						
						break;

					} else if(schedule[i].entry[j].course.professor.id == schedule[i].entry[k].course.professor.id) {
						
						break;

					} else if(schedule[i].entry[j].course.subject.type != schedule[i].entry[schedule[i].entry[k].pos.room.id].pos.room.type) {
						
						break;

					} else {

						// Check if two or more students are set to be at the same place at the same time.
						for(l = 0; l < schedule[i].entry[j].course.team.num_students; l++){
                        	for(n = 0; n < schedule[i].entry[k].course.team.num_students; n++){
                            	if(schedule[i].entry[j].course.team.student[l] == schedule[i].entry[k].course.team.student[n]){
                                	
                            	}
                        	}

                        	break;

                    	}
                    	// Every condition was met, so indent the fitness score.
                    	fitness++;

					}
				}
			}
		}

		schedule[i].fitness = fitness;

	}

}

