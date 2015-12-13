#ifndef FUNCTIONS_HEADER_INCLUDED
#define FUNCTIONS_HEADER_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

extern int MIN_POP;
extern int MAX_POP;
extern int NUM_PROFESSORS;
extern int NUM_SUBJECTS;
extern int NUM_ROOMS;
extern int NUM_TEAMS;
extern int NUM_COURSES;
extern int NUM_ENTRIES;
extern int NUM_DAYS;
extern int MAX_FITNESS;
extern int MAX_LINE_FITNESS;

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
void init_population(SCHEDULE *dest, int n);

// Generators.
int rand_num(int max, int min);
double rand_double(double max);

// Calculators.
void calc_fitness(SCHEDULE *schedule, int pop);

// Selectors.
int roulette_selection(SCHEDULE *parent, SCHEDULE *population, int pop_size);
void reduce_population(SCHEDULE *survivor, int pop_size, SCHEDULE *population, int n);

void selection_rank(SCHEDULE *selected, int n1, SCHEDULE *selection, int n2);
void selection_tournament(SCHEDULE *winner, int n1, SCHEDULE *competetor, int n2);

// Assemblers.
int append_population(SCHEDULE *dest, int n1, SCHEDULE *src, int n2);
void copy_schedule(SCHEDULE *dest, SCHEDULE *src);

// Sorters.
int schedule_cmp_by_fitness(const void *a, const void *b);
int entry_cmp_by_day(const void *a, const void *b);

// Crossover.
int single_point_crossover(SCHEDULE *offspring, SCHEDULE *population, int pop);

// Mutation.
void mutate(SCHEDULE * schedule, int pop);

// Tester.
int test(SCHEDULE *population, int pop, SCHEDULE *optimum);

// Export.
void export(SCHEDULE *src);

#endif