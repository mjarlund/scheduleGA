#include "header/structs.h"
#include "header/functions.h"

int rand_num(int max, int min) {
    return (rand() % (max - min)) + min;
}

void copy_schedule(SCHEDULE *dest, SCHEDULE *src) {
    memcpy(dest->entry, src->entry, sizeof(ENTRY)*NUM_ENTRIES);
    dest->fitness = src->fitness;
}

int entry_cmp_by_day(const void *a, const void *b){ 
    ENTRY *ia = (ENTRY *)a;
    ENTRY *ib = (ENTRY *)b;
    if ((ia->day - ib->day) != 0){
        return (ia->day - ib->day);
    } else {
        return (ia->hour - ib->hour);
    }
}

double randZeroToOne() {
    return rand() / (RAND_MAX + 1.);
}