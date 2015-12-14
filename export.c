#include "header/structs.h"
#include "header/functions.h"

void export(SCHEDULE *src) {
    FILE *out;
    int i;

    qsort(src->entry, NUM_ENTRIES, sizeof(ENTRY), entry_cmp_by_day);

    out = fopen("result/result.csv", "w");

    for(i = 0; i < NUM_ENTRIES; i++) {
        fprintf(out, "%d;%d;%s;%s;%s;%d;%d\n",
            src->entry[i].genome.day, src->entry[i].genome.hour, src->entry[i].course.professor.name,
            src->entry[i].course.subject.name, src->entry[i].course.team.name,
            src->entry[i].genome.room.id, src->entry[i].pts);
    }

    fclose(out);
}