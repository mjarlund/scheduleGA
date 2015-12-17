#include "header/structs.h"
#include "header/functions.h"

int NUM_PROFESSORS;
int NUM_SUBJECTS;
int NUM_ROOMS;
int NUM_TEAMS;
int NUM_COURSES;
int NUM_ENTRIES;

void validate_data() {
    int errnum;
    PROFESSOR *p;
    SUBJECT *s;
    ROOM *r;
    TEAM *t;
    COURSE *c;

    p = professors();
    s = subjects();
    r = rooms();
    t = teams();
    c = courses();
    
    if(!p || !s || !r || !t || !c) {
        perror("Error printed by perror");
        exit(-1);
    }

}

int buffer_array(FILE *inp) {
    int count = 0;
    char temp[BUFSIZ];

    while((fgets(temp, BUFSIZ, inp)) != NULL) {
        count++;
    }

    rewind(inp);

    return count;
}

PROFESSOR * professors() {
    int i = 0;
    char tmp[sizeof(PROFESSOR)];
    FILE *inp;

    // Open filestream.
    inp = fopen("data/professors.csv", "r");

    NUM_PROFESSORS = buffer_array(inp);
    static PROFESSOR dest[255];

    // Get values in filestream.
    while(fgets(tmp, sizeof(PROFESSOR), inp) != NULL) {

        sscanf(tmp, "%d;%s", &dest[i].id, dest[i].name);
        i++;
    }

    // Close the filestream.
    fclose(inp);

    return dest;
}

SUBJECT * subjects() {
    int i = 0;
    char tmp[sizeof(SUBJECT)];
    FILE *inp;

    // Open filestream.
    inp = fopen("data/subjects.csv", "r");

    NUM_SUBJECTS = buffer_array(inp);
    static SUBJECT dest[100];

    // Get values in filestream.
    while(fgets(tmp, sizeof(SUBJECT), inp) != NULL) {
        sscanf(tmp, "%d;%d;%d;%s", &dest[i].id, &dest[i].type, &dest[i].hours, dest[i].name);
        i++;
    }

    // Close the filestream.
    fclose(inp);

    // Return the array containing informations from the filestream.
    return dest;
}

ROOM * rooms() {
    int i = 0;
    char tmp[sizeof(ROOM)];
    FILE *inp;

    // Open filestream.
    inp = fopen("data/rooms.csv", "r");

    NUM_ROOMS = buffer_array(inp);
    static ROOM dest[150];

    // Get values in filestream.
    while(fgets(tmp, sizeof(ROOM), inp) != NULL) {

        sscanf(tmp, "%d;%d", &dest[i].id, &dest[i].type);

        i++;
    }

    // Close the filestream.
    fclose(inp);

    // Return the array containing informations from the filestream.
    return dest;
}

TEAM * teams() {
    int i = 0, j, d;
    char tmp[sizeof(TEAM)];
    char *token;
    char *buffer;
    FILE *inp;

    // Open filestream.
    inp = fopen("data/teams.csv", "r");

    NUM_TEAMS = buffer_array(inp);
    static TEAM dest[255];

    // Get values in filestream.
    while(fgets(tmp, sizeof(TEAM), inp) != NULL) {
        token = strtok(tmp, ";");

        dest[i].id = atoi(token);

        token = strtok(NULL, ";");

        strcpy(dest[i].name, token);

        token = strtok(NULL, ";");

        buffer = token;

        token = strtok(buffer, ",");

        j = 0;
        
        while( token != NULL ) 
        {
            dest[i].student[j] = atoi(token);
            token = strtok(NULL, ",");
            j++;
        }

        dest[i].num_students = j;

        i++;
    }

    // Close the filestream.
    fclose(inp);

    // Return the array containing informations from the filestream.
    return dest;
}

COURSE * courses() {
    int i = 0, j, p, s, t, count = 0;
    char tmp[sizeof(COURSE)];
    FILE *inp;

    // Open filestream.
    inp = fopen("data/courses.csv", "r");

    NUM_COURSES = buffer_array(inp);
    static COURSE dest[500];

    PROFESSOR *professor;
    SUBJECT *subject;
    TEAM *team;

    professor = professors();
    subject = subjects();
    team = teams();

    // Get values in filestream.
    while(fgets(tmp, sizeof(PROFESSOR), inp) != NULL) {
        sscanf(tmp, "%d;%d;%d;%d", &dest[i].id, &p, &s, &t);

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

    // Return the array containing informations from the filestream.
    return dest;
}