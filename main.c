#include "header/structs.h"
#include "header/functions.h"


int NUM_DAYS;
int NUM_HRS_DAY;
int NUM_POS;

int main(void) {
    int i, input, mutrate, generation = 0;
    int pop, int_pop;
    NUM_DAYS = 195;
    NUM_HRS_DAY = 4;

    // Validate the data provided before executing the program.
    validate_data();

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

    // Calculate number of positions.
    NUM_POS = NUM_DAYS * NUM_HRS_DAY * NUM_ROOMS;

    // Fill in the possible posistions.
    POS pos[NUM_POS];
    fill_positions(pos);

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

    init_population(population, pop, pos);

    input = 0;

    do{
        int_pop = roulette_selection(parent, population, pop);

        if(optimum.fitness < population[0].fitness) {
            printf("\nGeneration: %d\n", generation);
            printf("Best score so far: %d\n", population[0].fitness);
            
            printf("Generation results:\n");
            for(i=0; i < pop; i++) {
                printf("%d: %d\n", i, population[i].fitness);
            }
            // scanf("%d", &input);
            
        }

        if(!test(population, pop, &optimum)) {
            break;
        }

        int_pop = single_point_crossover(child, parent, int_pop);

        mutate(child, int_pop, mutrate, pos);

        insert_children(population, pop, child, int_pop);

        generation++;

    } while(input != 1);

    export(&optimum);

    dealloc_population(population, pop);
    dealloc_population(parent, pop);
    dealloc_population(child, pop);
    dealloc_schedule(&optimum);

    return 0;
}