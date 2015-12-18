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
            if(schedule[i].entry[j].course.subject.type == schedule[i].entry[j].room.type) {
                for(k = (j+1); k < NUM_ENTRIES; k++) {
                // The position ID is not equal, meaning we have to check each of the three position variables.
                if(schedule[i].entry[j].day == schedule[i].entry[k].day && schedule[i].entry[j].hour == schedule[i].entry[k].hour) {
                    if (schedule[i].entry[j].room.id == schedule[i].entry[k].room.id) {
                        score = 0;
                        break;
                    } else if(schedule[i].entry[j].course.team.id == schedule[i].entry[k].course.team.id) {
                        score = 0;
                        break;
                    } else if(schedule[i].entry[j].course.professor.id == schedule[i].entry[k].course.professor.id) {
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
                            }
                        }
                    } 
                }
            } else {
                score = 0;
            }
            // Indent the fitness score.
            fitness += score;
		}
        // Set schedule fitness, and start over for next entry.
		schedule[i].fitness = fitness;
	}
}

