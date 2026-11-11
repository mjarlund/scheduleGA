#include "header/structs.h"
#include "header/functions.h"


int NUM_DAYS;
int NUM_HRS_DAY;

/* Bounds that make the GA terminate even when MAX_FITNESS is unreachable (an
 * infeasible dataset, or a population that simply stops improving).  Both can be
 * overridden from the environment for experiments; the defaults are far beyond
 * what this dataset needs (400-1200 generations), so a converged run is
 * unaffected. */
#define DEFAULT_MAX_GENERATIONS 20000
#define DEFAULT_STAGNATION_LIMIT 3000

static int env_int(const char *name, int dflt) {
    const char *s = getenv(name);
    char *end;
    long v;

    if(s == NULL || *s == '\0') {
        return dflt;
    }
    v = strtol(s, &end, 10);
    if(*end != '\0' || v <= 0 || v > 100000000L) {
        fprintf(stderr, "%s: expected a positive integer, got \"%s\"\n", name, s);
        exit(EXIT_FAILURE);
    }
    return (int)v;
}

int main(void) {
    int i, best, stagnant = 0, generation = 1;
    int pop, max_pop, max_generations, stagnation_limit;
    int best_fit, worst_fit;
    long sum;
    const char *stopped = "loop exited";
    NUM_DAYS = 195;
    NUM_HRS_DAY = 4;

    // Validate the data provided before executing the program.
    validate_data();

    printf("Specify number of initial schedules to itterate over: ");
    if(scanf("%d", &pop) != 1 || pop < 2 || pop > 100000) {
        fprintf(stderr, "expected a population size between 2 and 100000\n");
        exit(EXIT_FAILURE);
    }

    max_generations = env_int("SGA_MAX_GENERATIONS", DEFAULT_MAX_GENERATIONS);
    stagnation_limit = env_int("SGA_STAGNATION", DEFAULT_STAGNATION_LIMIT);

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

    // Build the course and room tables (courses() also sets NUM_ENTRIES) that
    // the entries' indices refer to.
    courses();
    rooms();

    init_population(population, max_pop);

    do{ 

        // Calculate the fitness for the entire population.
        calc_fitness(population, max_pop);

        // Find the best individual of this generation.  The scan must cover the
        // whole population: the original stopped at the first individual whose
        // fitness was >= the optimum, so a strictly better individual later in
        // the array was never adopted as the elite.
        best = 0;
        for(i = 1; i < max_pop; i++) {
            if(population[i].fitness > population[best].fitness) {
                best = i;
            }
        }

        // Check if a new optimum (best solution) is present.  If so, make a copy.
        if(population[best].fitness > optimum.fitness) {
            copy_schedule(&optimum, &population[best]);
            printf("Generation: %d\n", generation);
            printf("Best: %d\n", optimum.fitness);

            stagnant = 0;
        } else {
            stagnant++;
        }

        // Stop on success (the only stop condition of the original), or on the
        // generation / stagnation bounds below, which keep an unreachable
        // MAX_FITNESS from turning the run into an infinite loop.
        if(optimum.fitness >= MAX_FITNESS) {
            stopped = "reached MAX_FITNESS";
            break;
        }
        if(generation >= max_generations) {
            stopped = "generation cap reached";
            break;
        }
        if(stagnant >= stagnation_limit) {
            stopped = "no improvement within the stagnation limit";
            break;
        }

        // Loop until n candidates fit for reproduction has been found.
        for(i = 0; i < pop; i++) {
            parent[i] = tournament_selection(population, max_pop);
            // When 2 candidates have been chosen, perform crossover.
            if(i %  2) {
                uniform_crossover(&population[i], &population[i-1], parent[i], parent[i-1]);
                // Mutate the offspring.
                mutation(&population[i]);
                mutation(&population[i-1]);
            }
            copy_schedule(&population[(max_pop-1)-i], &parent[i]); 
        }

        // Elitism, on every path: reproduction above overwrites every slot, so
        // the optimum has to be written back afterwards to be guaranteed a place
        // in the next generation's evaluation.  The original injected it before
        // reproduction (and only when no individual had reached the optimum), so
        // it survived by luck alone.
        copy_schedule(&population[0], &optimum);

        generation++;

    } while(1);

    // Final population statistics: best is the elite that was just evaluated,
    // worst/mean come from the same evaluation of the population.
    best_fit = population[0].fitness;
    worst_fit = population[0].fitness;
    sum = 0;
    for(i = 0; i < max_pop; i++) {
        sum += population[i].fitness;
        if(population[i].fitness > best_fit) best_fit = population[i].fitness;
        if(population[i].fitness < worst_fit) worst_fit = population[i].fitness;
    }

    printf("Result: gens=%d best=%d worst=%d mean=%.2f converged=%d max=%d\n",
           generation, best_fit, worst_fit, (double)sum/max_pop,
           optimum.fitness >= MAX_FITNESS, MAX_FITNESS);
    printf("Stopped: %s after %d generations, best %d/%d\n",
           stopped, generation, optimum.fitness, MAX_FITNESS);

    export(&optimum);

    return 0;
}