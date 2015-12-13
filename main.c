#include "header/structs.h"
#include "header/functions.h"


int NUM_DAYS;

int main(void) {
    int generation = 0, input, i, j, x, y, z;
    int pop, int_pop;
    NUM_DAYS = 195;

    // Validate the data provided before executing the program.
    validate_data();

    printf("Specify number of initial schedules to itterate over: ");
    scanf("%d", &pop);

    // Maximum number of schedules to be stored in the intermediate population.
    int_pop = pop * 2;

    // Initialize populations.
    SCHEDULE current[pop];
    SCHEDULE intermediate[int_pop];

    // Placeholder for best found solution over the course of all generations performed.
    SCHEDULE optimum;

    // Allocate memory to hold the specified number of schedules.
    malloc_population(current, pop);
    malloc_population(intermediate, int_pop);
    malloc_schedule(&optimum);

    // Seed randomizer.
    srand(time(NULL));

    // Generate initial set of schedules.
    init_population(current, pop);

    do{
        if(generation % 10 == 0) {
            printf("\nGeneration: %d\n", generation);
            printf("Best score so far: %d\n", optimum.fitness);
            printf("Generation results:\n");
            for(i=0; i < pop; i++) {
                printf("%d: %d\n", i, current[i].fitness);
            }
            // scanf("%d", &input);
        }

        // Perform mutation on a random set of schedules in the current population.
        mutate(current, pop);

        int_pop = single_point_crossover(intermediate, current, pop);

        int_pop = append_population(intermediate, int_pop, current, pop);

        if(!test(intermediate, int_pop, &optimum)) {
            printf("A schedule that meets the criteria of a solution has been found, do you wish to export it? (Y/N)");
            // Måske bare lidt ala. det ovenover her? Donno :-P
            break;
        }

        selection_tournament(current, pop, intermediate, int_pop);

        generation++;

    } while(input != 1);

    export(&optimum);

    // dealloc_population(current, POP);
    // dealloc_population(intermediate, POP);
    // dealloc_schedule(&optimum);

    return 0;
}