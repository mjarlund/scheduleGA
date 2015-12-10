#include <stdio.h>
#include <stdlib.h>
#include "header/structs.h"
#include "header/functions.h"

SCHEDULE * schedule_init() {
    int i, j, e = 0;

    COURSE *course;
    ROOM *room;
    room = rooms();
    course = courses();

    // Allocate memory corresponding the datatype + the the flexible array member.
    SCHEDULE *dest = malloc(sizeof(SCHEDULE) + NUM_ENTRIES * sizeof(ENTRY));

    // Check for allocation failure.
    if(dest == NULL) {
        printf("Out of memory\n");
        exit(EXIT_FAILURE);
    }

    // Itterate over all of the courses.
    for(i = 0; i < NUM_COURSES; i++) {
        // Count number of hours for the corresponding course.
        for(j = 0; j < course[i].subject.hours; j++) {
            dest->entry[e].course = course[i];
            dest->entry[e].day = rand_num(NUM_DAYS) + 1; // Days start at 1.
            dest->entry[e].hour = rand_num(4) + 1; // Hours start at 1.
            dest->entry[e].room = room[rand_num(NUM_ROOMS)];
            e++;
        }
    }

    // Initialize fitness value.
    dest->fitness = 0;

    free(course);
    free(room);

    return dest;
}
