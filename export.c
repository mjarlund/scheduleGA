#include <stdio.h>
#include <stdlib.h>
#include "header/structs.h"
#include "header/functions.h"

void export(SCHEDULE *schedule) {
    FILE *out;

    out = fopen("result/schedule.csv", "w");

    int i = 0;

    while(i < NUM_ENTRIES) {
        fprintf(out, "%d;%d;%s;%s;%s;%d;%d\n",
            schedule->entry[i].day, schedule->entry[i].hour, schedule->entry[i].course.professor.name,
            schedule->entry[i].course.subject.name, schedule->entry[i].course.team.name,
            schedule->entry[i].room.id, schedule->entry[i].pts);
        i++;
    }

    fclose(out);
}