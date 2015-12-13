#include "header/structs.h"
#include "header/functions.h"

void malloc_schedule(SCHEDULE *dest) {
    dest->entry = malloc(NUM_ENTRIES * sizeof(ENTRY));
    dest->fitness = 0;
}

void dealloc_schedule(SCHEDULE *src) {
    free(src->entry);
}

void malloc_population(SCHEDULE *dest, int n) {
    int i;

    for(i = 0; i < n; i++) {
        dest[i].entry = malloc(NUM_ENTRIES * sizeof(ENTRY));
        dest[i].fitness = 0;
    }
}

void dealloc_population(SCHEDULE *src, int n) {
    int i; 

    for(i = 0; i < n; i++) {
        free(src[i].entry);
        src[i].fitness = 0;
    }
}

void init_population(SCHEDULE *dest, int n) {
    int i, j, k, e = 0;

    COURSE *course;
    ROOM *room;
    room = rooms();
    course = courses();

    for(k = 0; k < n; k++) {
        e = 0;
        // Itterate over all of the courses.
        for(i = 0; i < NUM_COURSES; i++) {
            // Count number of hours for the corresponding course.
            for(j = 0; j < course[i].subject.hours; j++) {
                dest[k].entry[e].course = course[i];
                dest[k].entry[e].day = rand_num(NUM_DAYS + 1, 1); 
                dest[k].entry[e].hour = rand_num(5, 1);
                dest[k].entry[e].room = room[rand_num(NUM_ROOMS, 0)];
                e++;
            }
        } 
    }
}