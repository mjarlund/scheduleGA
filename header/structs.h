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

struct ENTRY {
    int day, hour;
    ROOM room;
    COURSE course;
};

struct SCHEDULE {
    int fitness, id;
    ENTRY *entry;
};

#endif