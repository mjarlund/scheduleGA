#include "header/structs.h"
#include "header/functions.h"
#include <errno.h>
#include <sys/stat.h>

/* Write the timetable.
 *
 * The original opened result/result.csv unchecked, so a binary run without the
 * directory (or from another working directory) segfaulted inside fprintf after
 * the whole GA had run.  The directory is created here if it is missing, and a
 * remaining failure is reported instead of dereferencing a NULL FILE*.
 */
void export(SCHEDULE *src) {
    FILE *out;
    int i;

    qsort(src->entry, NUM_ENTRIES, sizeof(ENTRY), entry_cmp_by_day);

    out = fopen("result/result.csv", "w");
    if(out == NULL) {
        if(mkdir("result", 0755) != 0 && errno != EEXIST) {
            perror("mkdir result");
            exit(EXIT_FAILURE);
        }
        out = fopen("result/result.csv", "w");
    }
    if(out == NULL) {
        perror("result/result.csv");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < NUM_ENTRIES; i++) {
        COURSE *course = &COURSE_TABLE[src->entry[i].course];
        ROOM *room = &ROOM_TABLE[src->entry[i].room];

        fprintf(out, "%d;%d;%s;%s;%s;%d\n",
            src->entry[i].day, src->entry[i].hour, course->professor.name,
            course->subject.name, course->team.name, room->id);
    }

    if(fclose(out) != 0) {
        perror("result/result.csv");
        exit(EXIT_FAILURE);
    }
}
