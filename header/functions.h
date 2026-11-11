#ifndef FUNCTIONS_HEADER_INCLUDED
#define FUNCTIONS_HEADER_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

extern int NUM_DAYS;
extern int NUM_HRS_DAY;
extern int NUM_POS;

extern int NUM_PROFESSORS;
extern int NUM_SUBJECTS;
extern int NUM_ROOMS;
extern int NUM_TEAMS;
extern int NUM_COURSES;
extern int NUM_ENTRIES;

extern int MAX_FITNESS;

/* Global lookup tables for the index-based ENTRY (see header/structs.h).
 * courses() and rooms() point these at their static arrays, so every consumer
 * of an entry (fitness, export, mutation) resolves the same data the embedded
 * struct used to carry.  Only the entry's *identity* is an index; the fitness
 * function still compares the resolved ids and student lists. */
extern COURSE *COURSE_TABLE;
extern ROOM *ROOM_TABLE;

// Import / parsing.
void validate_data();
PROFESSOR * professors();
SUBJECT * subjects();
ROOM * rooms();
TEAM * teams();
COURSE * courses();

// Initializers.
void malloc_population(SCHEDULE *dest, int n);
void malloc_schedule(SCHEDULE *dest);
void init_population(SCHEDULE *dest, int n);

// Generators.
int rand_num(int max, int min);
double randZeroToOne();

// Calculators.
void calc_fitness(SCHEDULE *schedule, int pop);

// Selectors.
SCHEDULE tournament_selection(SCHEDULE *p, int popsize);

// Assemblers.
void copy_schedule(SCHEDULE *dest, SCHEDULE *src);

// Sorters.
int entry_cmp_by_day(const void *a, const void *b);

// Crossover.
void uniform_crossover(SCHEDULE *child1, SCHEDULE *child2, SCHEDULE parent1, SCHEDULE parent2);

// Mutation.
void mutation(SCHEDULE *child);

// Export.
void export(SCHEDULE *src);

#endif