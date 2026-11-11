#include "header/structs.h"
#include "header/functions.h"

int rand_num(int max, int min) {
    return (rand() % (max - min)) + min;
}

void copy_schedule(SCHEDULE *dest, SCHEDULE *src) {
    memcpy(dest->entry, src->entry, sizeof(ENTRY)*NUM_ENTRIES);
    dest->fitness = src->fitness;
    dest->violations = src->violations;
}

/* Pick a room whose type matches `type`, uniformly among the rooms that match.
 * Returns the room's index into ROOM_TABLE, consuming exactly one rand() call
 * in both branches so the RNG stream keeps its shape.  validate_data() rejects
 * a dataset where a subject type has no room, so the fallback is for callers
 * that run before validation only; it still consumes one rand(). */
int random_room_of_type(int type) {
    int i, count = 0, pick;

    for(i = 0; i < NUM_ROOMS; i++) {
        if(ROOM_TABLE[i].type == type) {
            count++;
        }
    }
    pick = rand_num(count > 0 ? count : NUM_ROOMS, 0);
    for(i = 0; i < NUM_ROOMS; i++) {
        if(ROOM_TABLE[i].type == type) {
            if(pick == 0) {
                return i;
            }
            pick--;
        }
    }
    return 0;
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