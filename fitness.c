#include <stdio.h>
#include <stdlib.h>
#include "header/structs.h"
#include "header/functions.h"

int MAX_FITNESS, MAX_LINE_FITNESS;

void calc_fitness(SCHEDULE **schedule){
	int fitness = 0, i, j, n, k, l;
	int ok1, ok2, ok3, ok4;
	MAX_LINE_FITNESS = 4;
	for(n = 0; n < POP; n++) {
		qsort(schedule[n]->entry, NUM_ENTRIES, sizeof(ENTRY), entry_cmp_by_day);
		MAX_FITNESS = 0;
	
		for(i = 0; i < NUM_ENTRIES; i++) {
			ok1 = 1, ok2 = 1, ok3 = 1, ok4 = 1;
	
			for(j = (i+1); j < NUM_ENTRIES; j++) {
				if(schedule[n]->entry[i].day == schedule[n]->entry[j].day && schedule[n]->entry[i].hour == schedule[n]->entry[j].hour){
						if(schedule[n]->entry[i].course.team.id == schedule[n]->entry[j].course.team.id){
							ok1 = 0;
						}		
						if(schedule[n]->entry[i].room.id == schedule[n]->entry[j].room.id){	
							ok2 = 0;
						}
						if (schedule[n]->entry[i].course.professor.id == schedule[n]->entry[j].course.professor.id){
							ok3 = 0;
						}
						

						for(k = 0; k < schedule[n]->entry[i].course.team.num_students; k++){
                            for(l = 0; l < schedule[n]->entry[j].course.team.num_students; l++){
                                if(schedule[n]->entry[i].course.team.student[k] == schedule[n]->entry[j].course.team.student[l]){
                                    ok4 = 0;
                                }
                            }
                        }


				} else {
					break;
				}
			}
			MAX_FITNESS += MAX_LINE_FITNESS;
	
			schedule[n]->entry[i].pts = ok1+ok2+ok3+ok4;
			fitness += ok1+ok2+ok3+ok4;
		}
	
		schedule[n]->fitness = fitness;
		fitness = 0;
	}
}