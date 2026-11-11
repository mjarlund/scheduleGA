#include "header/structs.h"
#include "header/functions.h"
#include "header/objective.h"


int NUM_DAYS;
int NUM_HRS_DAY;

/* Bounds that make the GA terminate even when MAX_FITNESS is unreachable (an
 * infeasible dataset, or a population that simply stops improving).  Both can be
 * overridden from the environment for experiments; the defaults are far beyond
 * what this dataset needs (400-1200 generations), so a converged run is
 * unaffected. */
#define DEFAULT_MAX_GENERATIONS 20000
#define DEFAULT_STAGNATION_LIMIT 3000

/* Generations to keep searching the *soft* terms after the first fully
 * acceptable (267/267) schedule has been found.  The hard score cannot improve
 * at that point, so every further improvement is compactness.  The default is
 * measured, not guessed: see the report's "third pass" section and
 * tools/refine_sweep.py. */
#define DEFAULT_REFINE 1000

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

#if SOFT_OBJECTIVE
/* Like env_int but 0 is a legal value (used to switch the refinement off). */
static int env_int0(const char *name, int dflt) {
    const char *s = getenv(name);
    char *end;
    long v;

    if(s == NULL || *s == '\0') {
        return dflt;
    }
    v = strtol(s, &end, 10);
    if(*end != '\0' || v < 0 || v > 100000000L) {
        fprintf(stderr, "%s: expected a non-negative integer, got \"%s\"\n", name, s);
        exit(EXIT_FAILURE);
    }
    return (int)v;
}

#endif /* SOFT_OBJECTIVE */

/* Entries a school would accept: NUM_ENTRIES minus the entries that clash or
 * sit in a room of the wrong type.  This is exactly the legacy fitness, so
 * "267/267" keeps its old meaning under either objective. */
static int acceptable(SCHEDULE *s) {
    return NUM_ENTRIES - s->violations;
}

int main(void) {
    int i, best, stagnant = 0, generation = 1;
    int pop, max_pop, max_generations, stagnation_limit;
    int refine, refined = 0, first_feasible = 0;
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
#if SOFT_OBJECTIVE
    refine = env_int0("SGA_REFINE", DEFAULT_REFINE);
#else
    /* The legacy objective has nothing left to optimise once every entry is
       acceptable, so its run must stop exactly where the parent branch does. */
    refine = 0;
#endif

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

    // Refuse to start if the weights cannot guarantee that one hard violation
    // outweighs every soft gain the objective can offer (fitness.c).
    objective_check();

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
#if SOFT_OBJECTIVE
            printf("Accepted: %d\n", acceptable(&optimum));
#endif

            stagnant = 0;
        } else {
            stagnant++;
        }

        if(optimum.violations == 0 && first_feasible == 0) {
            first_feasible = generation;
        }

        // Stop on success (the only stop condition of the original), or on the
        // generation / stagnation bounds below, which keep an unreachable
        // MAX_FITNESS from turning the run into an infinite loop.
        //
        // Success is "no unacceptable entry left", i.e. no clash and no room of
        // the wrong type -- the same event as the old `fitness == NUM_ENTRIES`.
        // With the soft objective the fitness can still rise after it (fewer
        // idle hours, fewer teaching days), so SGA_REFINE keeps the search
        // going for a bounded number of generations; 0 stops immediately.
        if(optimum.violations == 0) {
            if(refined >= refine) {
                stopped = "reached MAX_FITNESS";
                break;
            }
            refined++;
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
    // worst/mean come from the same evaluation of the population.  All three
    // are in acceptable entries, so they stay comparable across objectives.
    best_fit = acceptable(&population[0]);
    worst_fit = best_fit;
    sum = 0;
    for(i = 0; i < max_pop; i++) {
        sum += acceptable(&population[i]);
        if(acceptable(&population[i]) > best_fit) best_fit = acceptable(&population[i]);
        if(acceptable(&population[i]) < worst_fit) worst_fit = acceptable(&population[i]);
    }

    printf("Result: gens=%d best=%d worst=%d mean=%.2f converged=%d max=%d",
           generation, best_fit, worst_fit, (double)sum/max_pop,
           optimum.violations == 0, MAX_FITNESS);
#if SOFT_OBJECTIVE
    printf(" first_feasible=%d fitness=%d refine=%d", first_feasible,
           optimum.fitness, refined);
#endif
    printf("\n");
    printf("Stopped: %s after %d generations, best %d/%d\n",
           stopped, generation, acceptable(&optimum), MAX_FITNESS);

    fitness_report(&optimum);

    export(&optimum);

    return 0;
}
