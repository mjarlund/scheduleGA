#ifndef FUNCTIONS_HEADER_INCLUDED
#define FUNCTIONS_HEADER_INCLUDED

extern int POP;
extern int NUM_PROFESSORS;
extern int NUM_SUBJECTS;
extern int NUM_ROOMS;
extern int NUM_TEAMS;
extern int NUM_COURSES;
extern int NUM_ENTRIES;
extern int NUM_DAYS;
extern int MAX_FITNESS;
extern int MAX_LINE_FITNESS;

int buffer_array(FILE *inp);
PROFESSOR * professors();
SUBJECT * subjects();
ROOM * rooms();
TEAM * team();
COURSE * courses();

SCHEDULE * schedule_init();

int rand_num(int max);

void calc_fitness(SCHEDULE **schedule);
int entry_cmp_by_day(const void *a, const void *b);

// selection.c
int selection_roulette(SCHEDULE **population);
void get_optimum(SCHEDULE **schedule, SCHEDULE *optimum);

int schedule_cmp_by_fitness(const void *a, const void *b);

// crossover.c
void single_point_crossover(SCHEDULE *mom, SCHEDULE *dad);

void mutate(SCHEDULE *schedule);

void export(SCHEDULE *schedule);

#endif