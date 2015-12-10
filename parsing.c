#include <stdio.h>
#include <stdlib.h>
#include "header/structs.h"
#include "header/functions.h"

int NUM_PROFESSORS;
int NUM_SUBJECTS;
int NUM_ROOMS;
int NUM_TEAMS;
int NUM_COURSES;
int NUM_ENTRIES;

int buffer_array(FILE *inp) {
    int count = 0;
    char str[BUFSIZ];

    while((fgets(str, BUFSIZ, inp)) != NULL) {
        count++;
    }

    return count;
}

PROFESSOR * professors() {
    int i = 0;
    FILE *inp;

    // Open filestream.
    inp = fopen("data/professors.csv", "r");

    // Count number of lines in the filestream.
    NUM_PROFESSORS = buffer_array(inp); 

    // Allocate memory corresponding the number of lines in the filestream.
    PROFESSOR *dest = malloc(NUM_PROFESSORS*sizeof(PROFESSOR)); 

    // Check for allocation failure.
    if(dest == NULL) {
        printf("Out of memory\n");
        exit(EXIT_FAILURE);
    }

    // Rewind the filestream back to first entry.
    rewind(inp); 

    // Get values in filestream.
    while(!feof(inp)) {
        fscanf(inp, "%d;%s", &dest[i].id, dest[i].name);
        i++;
    }

    // Close the filestream.
    fclose(inp);

    // Return the array containing informations from the filestream.
    return dest;
}

SUBJECT * subjects() {
    int i = 0;
    FILE *inp;

    // Open filestream.
    inp = fopen("data/subjects.csv", "r");

    // Count number of lines in the filestream.
    NUM_SUBJECTS = buffer_array(inp); 

    // Allocate memory corresponding the number of lines in the filestream.
    SUBJECT *dest = malloc(NUM_SUBJECTS*sizeof(SUBJECT)); 

    // Check for allocation failure.
    if(dest == NULL) {
        printf("Out of memory\n");
        exit(EXIT_FAILURE);
    }

    // Rewind the filestream back to first entry.
    rewind(inp); 

    // Get values in filestream.
    while(!feof(inp)) {
        fscanf(inp, "%d;%d;%d;%s", &dest[i].id, &dest[i].type, &dest[i].hours, dest[i].name);
        i++;
    }

    // Close the filestream.
    fclose(inp);

    // Return the array containing informations from the filestream.
    return dest;
}

ROOM * rooms() {
    int i = 0;
    FILE *inp;

    // Open filestream.
    inp = fopen("data/rooms.csv", "r");

    // Count number of lines in the filestream.
    NUM_ROOMS = buffer_array(inp); 

    // Allocate memory corresponding the number of lines in the filestream.
    ROOM *dest = malloc(NUM_ROOMS*sizeof(ROOM)); 

    // Check for allocation failure.
    if(dest == NULL) {
        printf("Out of memory\n");
        exit(EXIT_FAILURE);
    }

    // Rewind the filestream back to first entry.
    rewind(inp); 

    // Get values in filestream.
    while(!feof(inp)) {
        fscanf(inp, "%d;%d", &dest[i].id, &dest[i].type);
        i++;
    }

    // Close the filestream.
    fclose(inp);

    // Return the array containing informations from the filestream.
    return dest;
}

TEAM * teams() {
    int i = 0;
    FILE *inp;

    // Open filestream.
    inp = fopen("data/teams.csv", "r");

    // Count number of lines in the filestream.
    NUM_TEAMS = buffer_array(inp); 

    // Allocate memory corresponding the number of lines in the filestream.
    TEAM *dest = malloc(NUM_TEAMS*sizeof(TEAM)); 

    // Check for allocation failure.
    if(dest == NULL) {
        printf("Out of memory\n");
        exit(EXIT_FAILURE);
    }

    // Rewind the filestream back to first entry.
    rewind(inp); 

    // Get values in filestream.
    while(!feof(inp)) {
        fscanf(inp, "%d;%s", &dest[i].id, dest[i].name);
        i++;
    }

    // Close the filestream.
    fclose(inp);

    // Return the array containing informations from the filestream.
    return dest;
}

COURSE * courses() {
    int i = 0, j, p, s, t, count = 0;
    FILE *inp;

    // Open filestream.
    inp = fopen("data/courses.csv", "r");

    // Count number of lines in the filestream.
    NUM_COURSES = buffer_array(inp); 

    // Allocate memory corresponding the number of lines in the filestream.
    COURSE *dest = malloc(NUM_COURSES*sizeof(COURSE)); 

    // Check for allocation failure.
    if(dest == NULL) {
        printf("Out of memory\n");
        exit(EXIT_FAILURE);
    }

    PROFESSOR *professor;
    SUBJECT *subject;
    TEAM *team;

    professor = professors();
    subject = subjects();
    team = teams();

    // Rewind the filestream back to first entry.
    rewind(inp); 

    // Get values in filestream.
    while(!feof(inp)) {
        fscanf(inp, "%d;%d;%d;%d", &dest[i].id, &p, &s, &t);

        // Get corresponding professor.
        for(j = 0; j < NUM_PROFESSORS; j++) {
            if(p == professor[j].id) {
                dest[i].professor = professor[j];
                break;
            }
        }

        // Get corresponding subject.
        for(j = 0; j < NUM_SUBJECTS; j++) {
            if(s == subject[j].id) {
                dest[i].subject = subject[j];
                count += subject[j].hours;
                break;
            }
        }

        // Get corresponding team.
        for(j = 0; j < NUM_TEAMS; j++) {
            if(t == team[j].id) {
                dest[i].team = team[j];
                break;
            }
        }

        i++;
    }

    NUM_ENTRIES = count;

    // Close the filestream.
    fclose(inp);

    // Free the allocated arrays.
    free(professor);
    free(subject);
    free(team);

    // Return the array containing informations from the filestream.
    return dest;
}