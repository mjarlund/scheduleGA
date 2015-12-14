#include "header/structs.h"
#include "header/functions.h"

int MAX_FITNESS, MAX_LINE_FITNESS;

void calc_fitness(SCHEDULE *schedule, int pop){
	int fitness, i, j, n, k, l;
	int ok1, ok2, ok3, ok4, ok5;
	MAX_LINE_FITNESS = 5;
	
	MAX_FITNESS = NUM_ENTRIES * MAX_LINE_FITNESS;

	for(i = 0; i < pop; i++) {
		fitness = 0;

		for(j = 0; j < NUM_ENTRIES; j++) {
			ok1 = 1, ok2 = 1, ok3 = 1, ok4 = 1, ok5 = 1;
			for(k = (j+1); k < NUM_ENTRIES; k++) {
				if(schedule[i].entry[j].genome.day == schedule[i].entry[k].genome.day && schedule[i].entry[j].genome.hour == schedule[i].entry[k].genome.hour){
					if(schedule[i].entry[j].course.team.id == schedule[i].entry[k].course.team.id){
						ok1 = 0;
					}		
					if(schedule[i].entry[j].genome.room.id == schedule[i].entry[k].genome.room.id){	
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


					if(schedule[i].entry[j].course.subject.type != schedule[i].entry[schedule[i].entry[k].genome.room.id].genome.room.type) {
						ok5 = 0;
					}
				}
			}

			schedule[i].entry[j].pts = ok1+ok2+ok3+ok4+ok5;
			fitness += schedule[i].entry[j].pts;
		}

		schedule[i].fitness = fitness;

	}
}