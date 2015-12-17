#include "header/structs.h"
#include "header/functions.h"


int NUM_DAYS;
int NUM_HRS_DAY;

int main(void) {
    int i, new_opt, generation = 1;
    int pop, max_pop;
    NUM_DAYS = 195;
    NUM_HRS_DAY = 4;

    // Validate the data provided before executing the program.
    validate_data();

    printf("Specify number of initial schedules to itterate over: ");
    scanf("%d", &pop);

    max_pop = pop*2;

    // Initialize populations.
    SCHEDULE population[max_pop];
    SCHEDULE parent[pop];
    SCHEDULE optimum;

    malloc_population(population, max_pop);
    malloc_population(parent, pop);
    malloc_schedule(&optimum);

    // Seed randomizer.
    srand(time(NULL));

    COURSE *course;
    course = courses();

    ROOM *room;
    room = rooms();

    init_population(population, max_pop, course, room);

    do{ 

        // Calculate the fitness for the entire population.
        calc_fitness(population, max_pop);

        // Check if a new optimum (best solution) is present. If so, make a copy, and break the loop.
        new_opt = 0;
        for(i = 0; i < max_pop; i++) {
            if(population[i].fitness > optimum.fitness) {
                copy_schedule(&optimum, &population[i]);
                printf("Generation: %d\n", generation);
                printf("Best: %d\n", optimum.fitness);

                new_opt = 1;
                break;
            } else if (population[i].fitness == optimum.fitness) {
                new_opt = -1;
                break;
            }
        }

        // If above loops return negative, and no new optimum has been found, inject the optimum into the population.
        if(!new_opt) {
            copy_schedule(&population[rand_num(max_pop, 0)], &optimum);
        }

        // Loop until n candidates fit for reproduction has been found.
        for(i = 0; i < pop; i++) {
            parent[i] = tournament_selection(population, max_pop);
            // When 2 candidates have been chosen, perform crossover.
            if(i %  2) {
                uniform_crossover(&population[i], &population[i-1], parent[i], parent[i-1]);
                // Mutate the offspring.
                mutation(&population[i], room);
                mutation(&population[i-1], room);
            }
            copy_schedule(&population[(max_pop-1)-i], &parent[i]); 
        }

        generation++;

    } while(optimum.fitness < MAX_FITNESS);

    export(&optimum);

    return 0;
}