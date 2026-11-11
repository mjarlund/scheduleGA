#!/usr/bin/env python3
"""Generate a realistic Danish upper-secondary (gymnasium) timetable dataset for scheduleGA.

The 2015 parsers use `char tmp[sizeof(STRUCT)]` as the fgets() buffer, so each CSV
line must fit inside sizeof(struct)-1 bytes INCLUDING the newline.  The limits below
are derived from the real struct sizes on x86-64 (verified with a sizeof probe):

    PROFESSOR=16 -> line content <= 14   ("id;name"           name <= 9 bytes)
    SUBJECT  =24 -> line content <= 22   ("id;type;hours;name" name <= 9 bytes)
    ROOM     = 8 -> line content <= 6    ("id;type")
    TEAM    =220 -> line content <= 218  ("id;name;1,2,3,...")
    COURSE     -> parsed with sizeof(PROFESSOR)=16, line content <= 14  ("id;prof;subject;team")

All strings are ASCII on purpose: `sscanf("%s")` has no field width and the struct
name buffers are 10 bytes, so a 10+ byte name would smash the struct.
"""
import os
import random

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data")

# --- subjects: id, type, hours/week, name -------------------------------
# type 1 = classroom (lokale), 2 = lab, 3 = gym hall, 4 = music/art room
SUBJECTS = [
    (1, 1, 5, "Matematik"),
    (2, 1, 4, "Dansk"),
    (3, 1, 4, "Engelsk"),
    (4, 2, 4, "Fysik"),
    (5, 2, 4, "Kemi"),
    (6, 2, 3, "Biologi"),
    (7, 1, 3, "Historie"),
    (8, 1, 4, "Samfund"),
    (9, 1, 3, "Tysk"),
    (10, 1, 3, "Fransk"),
    (11, 3, 3, "Idraet"),
    (12, 4, 3, "Musik"),
    (13, 4, 3, "Billedk"),
    (14, 2, 4, "Info"),
    (15, 1, 3, "Geografi"),
    (16, 1, 3, "Religion"),
    (17, 1, 3, "Psykologi"),
    (18, 1, 4, "Erhverv"),
]

# --- professors: 20, Danish surnames, <= 9 ASCII chars -------------------
PROF_NAMES = [
    "Hansen", "Jensen", "Nielsen", "Larsen", "Madsen",
    "Olsen", "Thomsen", "Pedersen", "Andersen", "Poulsen",
    "Knudsen", "Jakobsen", "Mikkelsen", "Frederik", "Holm",
    "Skov", "Dam", "Vester", "Brandt", "Toft",
]

# --- rooms: id, type ----------------------------------------------------
ROOMS = [
    (1, 1), (2, 1), (3, 1), (4, 1), (5, 1), (6, 1),   # classrooms
    (7, 2), (8, 2), (9, 2),                           # labs
    (10, 3),                                          # gym hall
    (11, 4), (12, 4),                                 # music / art
]

N_TEAMS = 30
STUDENTS_PER_TEAM = 26
COURSES = 78  # 18 teams x 3 + 12 teams x 2


def main():
    rng = random.Random(20151218)  # deterministic dataset
    os.makedirs(OUT, exist_ok=True)

    # ---- professors, each qualified in 2-4 subjects -------------------
    prof_subjects = {p: set() for p in range(1, len(PROF_NAMES) + 1)}
    subj_ids = [s[0] for s in SUBJECTS]
    for s in subj_ids:                      # every subject gets >= 1 teacher
        prof_subjects[rng.randint(1, len(PROF_NAMES))].add(s)
    for p in prof_subjects:                 # nobody teaches only one thing
        while len(prof_subjects[p]) < 2:
            prof_subjects[p].add(rng.choice(subj_ids))

    # ---- teams and students -------------------------------------------
    # Student ids are a shared pool: a few "valgfag" students sit in two
    # teams, which exercises the duplicate-student branch of calc_fitness.
    teams = []           # (id, name, [student ids])
    sid = 1
    for t in range(1, N_TEAMS + 1):
        year = (t - 1) // 10 + 1
        letter = "abcdefghij"[(t - 1) % 10]
        name = f"{year}{letter}"
        students = list(range(sid, sid + STUDENTS_PER_TEAM))
        sid += STUDENTS_PER_TEAM
        teams.append([t, name, students])
    # 6 pairs of teams share 3 students each (elective overlap)
    for i in range(0, 12, 2):
        shared = teams[i][2][:3]
        teams[i + 1][2][-3:] = shared

    # ---- courses: each team takes N_COURSES_PER_TEAM subjects ----------
    courses = []
    cid = 1
    plan = [3] * 18 + [2] * 12   # 18*3 + 12*2 = 78 courses
    rng.shuffle(plan)
    for t, n_subj in zip(teams, plan):
        picked = rng.sample(subj_ids, n_subj)
        for s in picked:
            owners = [p for p in prof_subjects if s in prof_subjects[p]]
            courses.append((cid, rng.choice(owners), s, t[0]))
            cid += 1
    assert len(courses) == COURSES, len(courses)

    # ---- write files ---------------------------------------------------
    def write(fname, rows, limit, label):
        for n, row in enumerate(rows):
            assert len(row) <= limit, f"{label} line {n} is {len(row)} bytes > {limit}: {row!r}"
        with open(os.path.join(OUT, fname), "w", newline="\n") as f:
            f.write("\n".join(rows) + "\n")

    write("professors.csv",
          [f"{p};{n}" for p, n in zip(range(1, len(PROF_NAMES) + 1), PROF_NAMES)],
          14, "professors")
    write("subjects.csv",
          [f"{i};{ty};{h};{n}" for i, ty, h, n in SUBJECTS], 22, "subjects")
    write("rooms.csv",
          [f"{i};{ty}" for i, ty in ROOMS], 6, "rooms")
    write("teams.csv",
          [f"{i};{n};{','.join(map(str, s))}" for i, n, s in teams], 218, "teams")
    write("courses.csv",
          [f"{c};{p};{s};{t}" for c, p, s, t in courses], 14, "courses")

    # ---- report --------------------------------------------------------
    hours = {s[0]: s[2] for s in SUBJECTS}
    entries = sum(hours[c[2]] for c in courses)
    room_types = {ty for _, ty in ROOMS}
    missing = {s[1] for s in SUBJECTS} - room_types
    print(f"professors={len(PROF_NAMES)} subjects={len(SUBJECTS)} rooms={len(ROOMS)} "
          f"teams={len(teams)} courses={len(courses)} entries={entries}")
    print(f"subject types without a matching room type: {missing or 'none'}")
    print(f"students in pool={sid - 1}, shared-student team pairs=6")


if __name__ == "__main__":
    main()
