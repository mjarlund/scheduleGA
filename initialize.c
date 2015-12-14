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
        dest[i].id = 0;
    }
}

void dealloc_population(SCHEDULE *src, int n) {
    int i; 

    for(i = 0; i < n; i++) {
        free(src[i].entry);
        src[i].fitness = 0;
    }
}

void init_population(SCHEDULE *dest, int n, POS *pos) {
    int i, j, k, e = 0;

    COURSE *course;
    course = courses();

    for(k = 0; k < n; k++) {
        e = 0;
        // Itterate over all of the courses.
        for(i = 0; i < NUM_COURSES; i++) {
            // Count number of hours for the corresponding course.
            for(j = 0; j < course[i].subject.hours; j++) {
                dest[k].entry[e].id = e;
                dest[k].entry[e].course = course[i];
                dest[k].entry[e].pos = pos[rand_num(NUM_POS, 0)];
                e++;
            }
        } 
    }
}