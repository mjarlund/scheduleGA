#include "header/structs.h"
#include "header/functions.h"

void export(SCHEDULE *src) {
    FILE *out;
    int i;

    qsort(src->entry, NUM_ENTRIES, sizeof(ENTRY), entry_cmp_by_day);

    out = fopen("result/result.csv", "w");
    if(out == NULL) {
        perror("result/result.csv");
        exit(-1);
    }

    for(i = 0; i < NUM_ENTRIES; i++) {
        COURSE *course = &COURSE_TABLE[src->entry[i].course];
        ROOM *room = &ROOM_TABLE[src->entry[i].room];

        fprintf(out, "%d;%d;%s;%s;%s;%d\n",
            src->entry[i].day, src->entry[i].hour, course->professor.name,
            course->subject.name, course->team.name, room->id);
    }

    fclose(out);
}