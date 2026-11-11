#include "header/structs.h"
#include "header/functions.h"

/* Data loading.
 *
 * The 2015 parsers used `char tmp[sizeof(STRUCT)]` as the fgets() buffer, so a
 * line one byte too long was silently read as two records (courses.csv was the
 * worst case: sizeof(PROFESSOR) = 16, i.e. 14 usable characters) and sscanf left
 * unmatched fields at their previous values.  The model was corrupted without a
 * single error message, which typically showed up much later as a run that never
 * terminated.
 *
 * Now:
 *   - every parser shares one 512-byte line reader that rejects over-long lines;
 *   - every field has an explicit width and is length-checked (the structs hold
 *     10-byte names, so a longer name is an error, not a buffer overflow);
 *   - a missing file, an unparseable line, an unknown id in courses.csv, too many
 *     records or a subject type without a matching room all exit(1) with the
 *     file, line and reason instead of segfaulting or looping forever.
 */

#define LINE_MAX 512
#define NAME_MAX_LEN 9      /* char name[10]; 9 characters + NUL */

int NUM_PROFESSORS;
int NUM_SUBJECTS;
int NUM_ROOMS;
int NUM_TEAMS;
int NUM_COURSES;
int NUM_ENTRIES;

/* Pointers to the tables that ENTRY indices refer to (see header/structs.h).
 * Both are filled in by their parser below, which owns the static storage. */
COURSE *COURSE_TABLE;
ROOM *ROOM_TABLE;

/* Data directory.  Defaults to "data", so the committed dataset and every
 * earlier result are unchanged; SGA_DATA_DIR points an experiment at a
 * generated variant (a tighter grid needs more teachers, see the fourth pass)
 * without copying the tree. */
static const char *data_path(const char *name, char *buf, size_t size) {
    const char *dir = getenv("SGA_DATA_DIR");

    if(dir == NULL || *dir == '\0') {
        dir = "data";
    }
    snprintf(buf, size, "%s/%s", dir, name);
    return buf;
}

static void fail(const char *path, int lineno, const char *msg) {
    if(lineno > 0) {
        fprintf(stderr, "%s:%d: %s\n", path, lineno, msg);
    } else {
        fprintf(stderr, "%s: %s\n", path, msg);
    }
    exit(EXIT_FAILURE);
}

/* Read one line.  Returns 0 at end of file; an over-long line is fatal. */
static int read_line(FILE *inp, char *buf, const char *path, int lineno) {
    size_t len;

    if(fgets(buf, LINE_MAX, inp) == NULL) {
        return 0;
    }
    len = strlen(buf);
    if(len == (size_t)(LINE_MAX - 1) && buf[len - 1] != '\n') {
        fail(path, lineno, "line longer than 511 bytes (does not match the schema)");
    }
    return 1;
}

static int buffer_array(FILE *inp, const char *path) {
    int count = 0;
    char temp[LINE_MAX];

    while(read_line(inp, temp, path, count + 1)) {
        count++;
    }

    rewind(inp);

    return count;
}

static void check_count(const char *path, int n, int max, const char *what) {
    char msg[128];

    if(n < 1) {
        fail(path, 0, "no records");
    }
    if(n > max) {
        snprintf(msg, sizeof(msg), "%d records, but the static %s table holds %d",
                 n, what, max);
        fail(path, 0, msg);
    }
}

static void copy_name(char *dst, const char *src, const char *path, int lineno) {
    if(strlen(src) > NAME_MAX_LEN) {
        fail(path, lineno, "name longer than 9 characters (char name[10])");
    }
    strcpy(dst, src);
}

void validate_data() {
    PROFESSOR *p;
    SUBJECT *s;
    ROOM *r;
    TEAM *t;
    COURSE *c;
    int i, j, ok;

    p = professors();
    s = subjects();
    r = rooms();
    t = teams();
    c = courses();

    if(!p || !s || !r || !t || !c) {
        // The parsers exit on failure; this stays as a last resort.
        fprintf(stderr, "data: could not load the tables\n");
        exit(EXIT_FAILURE);
    }

    /* A subject whose type has no room can never score (fitness.c: 38-40), so
       MAX_FITNESS is unreachable and the GA would search forever.  Reject the
       dataset up front instead. */
    for(i = 0; i < NUM_SUBJECTS; i++) {
        ok = 0;
        for(j = 0; j < NUM_ROOMS; j++) {
            if(s[i].type == r[j].type) {
                ok = 1;
                break;
            }
        }
        if(!ok) {
            fprintf(stderr, "data: subject %d (%s) has type %d but no room has "
                            "that type, so fitness can never reach %d\n",
                    s[i].id, s[i].name, s[i].type, NUM_ENTRIES);
            exit(EXIT_FAILURE);
        }
    }
}

PROFESSOR * professors() {
    char pathbuf[512];
    const char *path = data_path("professors.csv", pathbuf, sizeof pathbuf);
    int i = 0, lineno = 0;
    char tmp[LINE_MAX];
    char name[64];
    FILE *inp;
    static PROFESSOR dest[255];

    inp = fopen(path, "r");
    if(inp == NULL) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    NUM_PROFESSORS = buffer_array(inp, path);
    check_count(path, NUM_PROFESSORS, 255, "professor");

    while(read_line(inp, tmp, path, lineno + 1)) {
        lineno++;
        if(sscanf(tmp, "%d;%63s", &dest[i].id, name) != 2) {
            fail(path, lineno, "expected \"id;name\"");
        }
        copy_name(dest[i].name, name, path, lineno);
        i++;
    }

    fclose(inp);

    return dest;
}

SUBJECT * subjects() {
    char pathbuf[512];
    const char *path = data_path("subjects.csv", pathbuf, sizeof pathbuf);
    int i = 0, lineno = 0;
    char tmp[LINE_MAX];
    char name[64];
    FILE *inp;
    static SUBJECT dest[100];

    inp = fopen(path, "r");
    if(inp == NULL) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    NUM_SUBJECTS = buffer_array(inp, path);
    check_count(path, NUM_SUBJECTS, 100, "subject");

    while(read_line(inp, tmp, path, lineno + 1)) {
        lineno++;
        if(sscanf(tmp, "%d;%d;%d;%63s", &dest[i].id, &dest[i].type,
                  &dest[i].hours, name) != 4) {
            fail(path, lineno, "expected \"id;type;hours;name\"");
        }
        if(dest[i].type < 1) {
            fail(path, lineno, "type must be a positive room type");
        }
        if(dest[i].hours < 1) {
            fail(path, lineno, "hours must be at least 1");
        }
        copy_name(dest[i].name, name, path, lineno);
        i++;
    }

    fclose(inp);

    return dest;
}

ROOM * rooms() {
    char pathbuf[512];
    const char *path = data_path("rooms.csv", pathbuf, sizeof pathbuf);
    int i = 0, lineno = 0;
    char tmp[LINE_MAX];
    FILE *inp;
    static ROOM dest[150];

    inp = fopen(path, "r");
    if(inp == NULL) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    NUM_ROOMS = buffer_array(inp, path);
    check_count(path, NUM_ROOMS, 150, "room");

    while(read_line(inp, tmp, path, lineno + 1)) {
        lineno++;
        if(sscanf(tmp, "%d;%d", &dest[i].id, &dest[i].type) != 2) {
            fail(path, lineno, "expected \"id;type\"");
        }
        i++;
    }

    fclose(inp);

    ROOM_TABLE = dest;

    return dest;
}

TEAM * teams() {
    char pathbuf[512];
    const char *path = data_path("teams.csv", pathbuf, sizeof pathbuf);
    int i = 0, lineno = 0;
    char tmp[LINE_MAX];
    char name[64];
    char *token;
    FILE *inp;
    static TEAM dest[255];

    inp = fopen(path, "r");
    if(inp == NULL) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    NUM_TEAMS = buffer_array(inp, path);
    check_count(path, NUM_TEAMS, 255, "team");

    while(read_line(inp, tmp, path, lineno + 1)) {
        int j = 0;

        lineno++;
        if(sscanf(tmp, "%d;%63[^;]", &dest[i].id, name) != 2) {
            fail(path, lineno, "expected \"id;name;student,student,...\"");
        }
        copy_name(dest[i].name, name, path, lineno);

        /* The student list is the rest of the line.  The struct holds 50. */
        token = strchr(tmp, ';');
        token = token ? strchr(token + 1, ';') : NULL;
        if(token == NULL) {
            fail(path, lineno, "missing student list");
        }

        for(token = strtok(token + 1, ",\n"); token != NULL;
            token = strtok(NULL, ",\n")) {
            if(j >= 50) {
                fail(path, lineno, "more than 50 students (int student[50])");
            }
            dest[i].student[j] = atoi(token);
            j++;
        }

        dest[i].num_students = j;
        i++;
    }

    fclose(inp);

    return dest;
}

COURSE * courses() {
    char pathbuf[512];
    const char *path = data_path("courses.csv", pathbuf, sizeof pathbuf);
    int i = 0, j, lineno = 0, p, s, t, count = 0;
    char tmp[LINE_MAX];
    FILE *inp;
    static COURSE dest[500];

    PROFESSOR *professor;
    SUBJECT *subject;
    TEAM *team;

    professor = professors();
    subject = subjects();
    team = teams();

    inp = fopen(path, "r");
    if(inp == NULL) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    NUM_COURSES = buffer_array(inp, path);
    check_count(path, NUM_COURSES, 500, "course");

    while(read_line(inp, tmp, path, lineno + 1)) {
        int found;

        lineno++;
        if(sscanf(tmp, "%d;%d;%d;%d", &dest[i].id, &p, &s, &t) != 4) {
            fail(path, lineno, "expected \"id;professor_id;subject_id;team_id\"");
        }

        /* Get corresponding professor.  An unknown id used to leave the struct
           zero-filled, which silently changed the fitness model. */
        found = 0;
        for(j = 0; j < NUM_PROFESSORS; j++) {
            if(p == professor[j].id) {
                dest[i].professor = professor[j];
                found = 1;
                break;
            }
        }
        if(!found) {
            fail(path, lineno, "professor_id does not exist in professors.csv");
        }

        // Get corresponding subject.
        found = 0;
        for(j = 0; j < NUM_SUBJECTS; j++) {
            if(s == subject[j].id) {
                dest[i].subject = subject[j];
                count += subject[j].hours;
                found = 1;
                break;
            }
        }
        if(!found) {
            fail(path, lineno, "subject_id does not exist in subjects.csv");
        }

        // Get corresponding team.
        found = 0;
        for(j = 0; j < NUM_TEAMS; j++) {
            if(t == team[j].id) {
                dest[i].team = team[j];
                found = 1;
                break;
            }
        }
        if(!found) {
            fail(path, lineno, "team_id does not exist in teams.csv");
        }

        i++;
    }

    fclose(inp);

    NUM_ENTRIES = count;
    if(NUM_ENTRIES < 1) {
        fail(path, 0, "the timetable has no entries");
    }

    COURSE_TABLE = dest;

    return dest;
}
