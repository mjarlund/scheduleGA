#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "header/structs.h"
#include "header/functions.h"

int rand_num(int max) {
    return ((rand() % max));
}

int schedule_cmp_by_fitness(const void *a, const void *b) {
    SCHEDULE *ia = *(SCHEDULE **)a;
    SCHEDULE *ib = *(SCHEDULE **)b;
    return -(ia->fitness - ib->fitness);
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
