#include "header/structs.h"
#include "header/functions.h"


int NUM_DAYS;

int main(void) {
    int generation = 0, input, i, j, x, y, z, mutrate;
    int pop, int_pop, elite_pop;
    NUM_DAYS = 195;

    // Validate the data provided before executing the program.
    validate_data();
    /*
    do {
        printf("Specify number of initial schedules to itterate over (25 recomended): ");
        scanf("%d", &pop);

        if(pop < 3) {
            printf("Minimum is 3\n");
        } 
    } while (pop < 3);
    */
    

    printf("Specify number of initial schedules to itterate over (25 recomended): ");
    scanf("%d", &pop);
    
    do {
        printf("Specify the mutation rate: ");
        scanf("%d", &input);

        if(input > 100) {
            printf("Number must be between 0-100\n");
        }
    } while( input > 100);
    mutrate = (NUM_ENTRIES/100)*input;

    // Set the max size of the intermediate population.
    // int_pop = pop * 2;

    // Initialize populations.
    SCHEDULE population[pop];
    SCHEDULE parent[pop];
    SCHEDULE child[pop];
    SCHEDULE optimum;

    malloc_population(population, pop);
    malloc_population(parent, pop);
    malloc_population(child, pop);
    malloc_schedule(&optimum);

    // Seed randomizer.
    srand(time(NULL));

    init_population(population, pop);


    /*
    SCHEDULE current[pop];
    SCHEDULE parent[pop];
    SCHEDULE child[pop];
    SCHEDULE intermediate[int_pop];
    */

    // Placeholder for the best scoring schedule in all of the generations.
    // SCHEDULE optimum;

    // Allocate memory to hold the specified number of schedules.
    /*
    malloc_population(population, pop);
    malloc_population(intermediate, int_pop);
    malloc_population(elite, elite_pop);
    */
    /*
    malloc_population(current, pop);
    malloc_population(parent, pop);
    malloc_population(child, pop);
    malloc_population(intermediate, int_pop);
    malloc_schedule(&optimum);
    */

    // init_population(current, pop);

    // Reset the input.
    input = 0;

    do{
        int_pop = roulette_selection(parent, population, pop);

        if(!test(population, pop, &optimum)) {
            break;
        }

        if(generation % 10 == 0) {
            printf("\nGeneration: %d\n", generation);
            printf("Best score so far: %d\n", optimum.fitness);
            
            printf("Generation results:\n");
            for(i=0; i < pop; i++) {
                printf("%d: %d\n", i, population[i].fitness);
            }
            
        }

        int_pop = single_point_crossover(child, parent, int_pop);

        mutate(child, int_pop, mutrate);

        insert_children(population, pop, child, int_pop);

        

        /*

        // A select few individuals are selected for reproduction. 
        // int_pop = roulette_selection(intermediate, population, pop);

        // int_pop = roulette_selection(intermediate, current, pop);

        elite_selection(elite, elite_pop, population, pop);

        

        single_point_crossover(intermediate, population, int_pop);

        mutate(intermediate, int_pop, mutrate);

        for(i=0; i < int_pop; i++) {
            copy_schedule(&population[(pop-1)-i], &intermediate[i]);
        }
        */
        /*
        if(generation % 1 == 0) {
            printf("\nGeneration: %d\n", generation);
            printf("Best score so far: %d\n", optimum.fitness);
            printf("Generation results:\n");
            for(i=0; i < pop; i++) {
                printf("%d: %d\n", i, current[i].fitness);
            }
        }
        */


        /*
        // If a new highscore was introduced, copy it to optimum, and check if optimum is a solution.
        if(!test(current, pop, &optimum)) {
            break;
        }
        */
        /*
        // If a new highscore was introduced, copy it to optimum, and check if optimum is a solution.
        if(!test(intermediate, int_pop, &optimum)) {
            break;
        }

        // The selected parents produce offspring.
        int_pop = single_point_crossover(child, parent, int_pop);

        // Perform mutation on the offspring.
        mutate(child, int_pop, mutrate);

        // Replace the worst scoring individuals in the population with the offspring.
        insert_offspring(current, pop, child, int_pop);
        */

        generation++;

    } while(input != 1);

    // export(&optimum);


    export(&optimum);

    // dealloc_population(current, POP);
    // dealloc_population(intermediate, POP);
    // dealloc_schedule(&optimum);

    return 0;
}