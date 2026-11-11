#ifndef STRUCTS_HEADER_INCLUDED
#define STRUCTS_HEADER_INCLUDED

typedef struct PROFESSOR PROFESSOR;
typedef struct SUBJECT SUBJECT;
typedef struct ROOM ROOM;
typedef struct TEAM TEAM;
typedef struct COURSE COURSE;
typedef struct ENTRY ENTRY;
typedef struct SCHEDULE SCHEDULE;

struct PROFESSOR {
    int id;
    char name[10];
};

struct SUBJECT {
    int id, type, hours;
    char name[10];
};

struct ROOM {
    int id, type;
};

struct TEAM {
    int id, student[50], num_students;
    char name[10];
};

struct COURSE {
    int id;
    PROFESSOR professor;
    SUBJECT subject;
    TEAM team;
};

/* An entry no longer embeds its course (280 bytes of professor, subject and
 * student list) and its room; it stores the index of each into the global
 * COURSE_TABLE / ROOM_TABLE that courses() / rooms() fill in.  A whole genome
 * is therefore NUM_ENTRIES * 16 bytes instead of NUM_ENTRIES * 280, which is
 * what copy_schedule() memcpy's around a hundred times per generation.
 *
 * The GA treats an entry's course as immutable: init_population() assigns
 * course i to the entries of course i, and neither crossover nor mutation ever
 * changes it, so the index carries exactly the information the struct did.
 * Rooms were already chosen by table index in the original code
 * (room[rand_num(NUM_ROOMS, 0)]), so that index is carried over unchanged. */
struct ENTRY {
    int day, hour;
    int room;   /* index into ROOM_TABLE, 0 .. NUM_ROOMS-1 */
    int course; /* index into COURSE_TABLE, 0 .. NUM_COURSES-1 */
};

struct SCHEDULE {
    int fitness;    /* the objective (see fitness.c); drives selection          */
    int violations; /* entries a school would reject: clashes + wrong room type */
    ENTRY *entry;
};

#endif