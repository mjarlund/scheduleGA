#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "header/structs.h"
#include "header/functions.h"


int NUM_DAYS, POP;

int main(void) {
    int generation = 1, input, i, j, x;
    POP = 6;
    NUM_DAYS = 195;

    // Seed randomizer.
    srand(time(NULL));

    SCHEDULE *current[POP];
    SCHEDULE *intermediate[POP];
    SCHEDULE optimum;

    // Start (generate a random population).
    for(i = 0; i < POP; i++) {
        current[i] = schedule_init();
    }

    do{
        // Se den her video så giver det mening: https://www.youtube.com/watch?v=fQbmSJWL1FI&list=PLea0WJq13cnARQILcbHUPINYLy1lOSmjH&index=3&spfreload=10
        // Programmet er ikke færdigt, men det er ved at tage form.


        // Finder fitness værdien for populationen ved en ny generation.
        calc_fitness(current);
        
        printf("Før: %d\n", optimum.fitness);
        // Tager den bedste fitness værdi, og tjekker om den er bedre end den tidligere bedste værdi.
        // Hvis den ikke er bedre, indsættes den bedste værdi istedet for den dårligste i populationen, således den bedste bibeholdes i løsningsrummet.
        get_optimum(current, &optimum);

        printf("Efter: %d\n", optimum.fitness);
        for(i=0; i < POP; i++) {
            printf("%d: %d\n", i, current[i]->fitness);
        }
		
        // Udformningen af en ny population. Her udvælges en række indbyggere i den tidligere population på baggrund af roulette metoden. (se selection_roulette).
        printf("\n");
        for(i = 0; i < POP; i++) {
            x = selection_roulette(current);
            if(x == -1) {
                printf("Error: No selections were made.\n");
                exit(EXIT_FAILURE);
            } else {
                intermediate[i] = current[x];
            }
        }

        // Dernæst udføres krydsning af værdierne i den nye population, således en række nye mulige gode indbyggere opstår.
        printf("\n");
        for(i = 1; i < POP; i += 2) {
            single_point_crossover(intermediate[i], intermediate[i-1]);
        }

        calc_fitness(intermediate);

        for(i=0; i < POP; i++) {
            printf("%d: %d\n", i, intermediate[i]->fitness);
        }

        // Her mangler mutation, men jeg ville lige være færdig med det andet først.*
        for(i = 0; i < POP; i++){
			mutate(intermediate[i]);
		} 
		// Idéen er at hvert barn skal muteres efter "fødslen" i ovenstående krydsning.
        // Lige pt. vil den stoppe ret hurtigt, da mutationen mangler. Så den skal indføres hurtigst muligt, men igen, se vidoen og følg den generelle opsætning.


        // Her begynder populationsskifte. Hvert af de nye børn sættes istedet for de gamle fra den tidligere population.
        // Den bedste værdi er stadigvæk bibeholdt, og indsættes ved starten af en ny generation, hvis den nye generations bedste ikke er bedre.
        for(i = 0; i < POP; i++) {
            current[i] = intermediate[i];
        }

        // Her tjekkes om den bedste borger opfylder kravet for at der ikke skal laves flere. Hvis ikke hopper den videre.
        if(optimum.fitness == MAX_FITNESS) {
            break;
        }

        // Generationsskiftet gennemføres.
		printf("g = %d\n", generation);
        generation++;
		

    } while(input != 1);

    export(current[0]);

    return 0;
}