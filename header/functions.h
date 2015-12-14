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

// Import / parsing.
void validate_data();
int buffer_array(FILE *inp);
PROFESSOR * professors();
SUBJECT * subjects();
ROOM * rooms();
TEAM * teams();
COURSE * courses();

// Initializers.
void malloc_population(SCHEDULE *dest, int n);
void malloc_schedule(SCHEDULE *dest);
void dealloc_population(SCHEDULE *src, int n);
void dealloc_schedule(SCHEDULE *dest);
void fill_positions(POS * pos);
void init_population(SCHEDULE *dest, int n, POS *pos);

// Generators.
int rand_num(int max, int min);

// Calculators.
void calc_fitness(SCHEDULE *schedule, int pop);

// Selectors.
int roulette_selection(SCHEDULE *parent, SCHEDULE *population, int pop_size);
void insert_children(SCHEDULE *population, int pop, SCHEDULE *child, int int_pop);

// Assemblers.
void copy_schedule(SCHEDULE *dest, SCHEDULE *src);

// Sorters.
int schedule_cmp_by_fitness(const void *a, const void *b);
int entry_cmp_by_day(const void *a, const void *b);

// Crossover.
int single_point_crossover(SCHEDULE *offspring, SCHEDULE *population, int pop);

// Mutation.
void mutate(SCHEDULE * schedule, int pop, int mutrate, POS * pos);

// Tester.
int test(SCHEDULE *population, int pop, SCHEDULE *optimum);

// Export.
void export(SCHEDULE *src);

#endif