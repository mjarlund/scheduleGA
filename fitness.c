#include "header/structs.h"
#include "header/functions.h"

int MAX_FITNESS;

void calc_fitness(SCHEDULE *schedule, int pop){
	int fitness, i, j, n, k, l, score;

	// Max fitness is equal to the number of permutations.
	MAX_FITNESS = NUM_ENTRIES;

	for(i = 0; i < pop; i++) {
		fitness = 0;

		for(j = 0; j < NUM_ENTRIES; j++) {

            score = 1;

			for(k = (j+1); k < NUM_ENTRIES; k++) {

				
				if(schedule[i].entry[j].pos.id == schedule[i].entry[k].pos.id) {
					// If position of one entry is equal to another...
                    score = 0;
                    break;

				} else {

                    // The position ID is not equal, meaning we have to check each of the three position variables.
                    if(schedule[i].entry[j].pos.day == schedule[i].entry[k].pos.day && schedule[i].entry[j].pos.hour == schedule[i].entry[k].pos.hour) {
                        // If one individual ()...
                        if(schedule[i].entry[j].course.team.id == schedule[i].entry[k].course.team.id) {
                            score = 0;
                            break;

                        } else if(schedule[i].entry[j].course.professor.id == schedule[i].entry[k].course.professor.id) {
                            score = 0;
                            break;

                        } else if(schedule[i].entry[j].course.subject.type != schedule[i].entry[schedule[i].entry[k].pos.room.id].pos.room.type) {
                            score = 0;
                            break;

                        } else { 

                            // Check if two or more students are set to be at the same place at the same time.
                            for(l = 0; l < schedule[i].entry[j].course.team.num_students; l++){
                                for(n = 0; n < schedule[i].entry[k].course.team.num_students; n++){
                                    if(schedule[i].entry[j].course.team.student[l] == schedule[i].entry[k].course.team.student[n]){
                                        score = 0;
                                        break;
                                    }
                                }

                                if(score == 0) {
                                    break;
                                }

                            }

                        }

                    } 
						
				}

            }

            // Indent the fitness score.
            fitness += score;

		}

        // Set schedule fitness, and start over for next entry.
		schedule[i].fitness = fitness;

	}

}

