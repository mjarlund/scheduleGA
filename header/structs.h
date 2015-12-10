#ifndef STRUCTS_HEADER_INCLUDED
#define STRUCTS_HEADER_INCLUDED

typedef struct {
    int         id;
    char        name[10];
} PROFESSOR;

typedef struct {
    int         id, type, hours;
    char        name[10];
} SUBJECT;

typedef struct {
    int         id, type;
} ROOM;

typedef struct {
    int         id;
    char        name[10];
} TEAM;

typedef struct {
    int         id;
    PROFESSOR   professor;
    SUBJECT     subject;
    TEAM        team;
} COURSE;

typedef struct {
    int         day, hour, pts;
    COURSE      course;
    ROOM        room;
} ENTRY;

typedef struct {
    int         fitness;
    ENTRY       entry[];
} SCHEDULE;

#endif