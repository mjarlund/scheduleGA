#include "header/structs.h"
#include "header/functions.h"

void export(SCHEDULE *src) {
    FILE *out;
    int i;

    qsort(src->entry, NUM_ENTRIES, sizeof(ENTRY), entry_cmp_by_day);

    out = fopen("result/result.csv", "w");

    for(i = 0; i < NUM_ENTRIES; i++) {
        fprintf(out, "%d;%d;%s;%s;%s;%d\n",
            src->entry[i].day, src->entry[i].hour, src->entry[i].course.professor.name,
            src->entry[i].course.subject.name, src->entry[i].course.team.name,
            src->entry[i].room.id);
    }

    fclose(out);
}