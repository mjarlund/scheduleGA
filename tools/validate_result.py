#!/usr/bin/env python3
"""Independent checker for result/result.csv.

Re-implements the fitness model of fitness.c (conflict test in the same
(j,k) j<k order) from the CSV dataset, so the exported schedule can be
verified without trusting the C code:

  fitness = number of entries that have NO conflict with any LATER entry,
  where an entry pair in the same (day,hour) conflicts if it shares the room,
  the team, the professor, or any student; an entry whose subject.type does
  not match its room.type always scores 0.

Exit status 0 iff fitness == NUM_ENTRIES (a conflict-free timetable).
"""
import os
import sys
from collections import defaultdict

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
D = os.path.join(ROOT, "data")


def rows(name):
    with open(os.path.join(D, name)) as f:
        return [l.rstrip("\n").split(";") for l in f if l.strip()]


def load():
    profs = {int(r[0]): r[1] for r in rows("professors.csv")}
    subj = {int(r[0]): (int(r[1]), int(r[2]), r[3]) for r in rows("subjects.csv")}
    rooms = {int(r[0]): int(r[1]) for r in rows("rooms.csv")}
    teams = {int(r[0]): (r[1], [int(x) for x in r[2].split(",")]) for r in rows("teams.csv")}
    courses = [(int(r[0]), int(r[1]), int(r[2]), int(r[3])) for r in rows("courses.csv")]
    return profs, subj, rooms, teams, courses


def main():
    profs, subj, rooms, teams, courses = load()
    entries = sum(subj[c[2]][1] for c in courses)

    path = os.path.join(ROOT, "result", "result.csv")
    with open(path) as f:
        out = [l.rstrip("\n").split(";") for l in f if l.strip()]

    # course key lookup by (prof name, subject name, team name)
    by_name = {}
    for cid, p, s, t in courses:
        by_name[(profs[p], subj[s][2], teams[t][0])] = (cid, p, s, t)

    parsed, bad = [], 0
    for r in out:
        day, hour, pn, sn, tn, rid = int(r[0]), int(r[1]), r[2], r[3], r[4], int(r[5])
        key = (pn, sn, tn)
        if key not in by_name or rid not in rooms:
            bad += 1
            continue
        cid, p, s, t = by_name[key]
        parsed.append(dict(day=day, hour=hour, room=rid, roomtype=rooms[rid],
                           prof=p, team=t, subj=s, subjtype=subj[s][0],
                           students=teams[t][1], cid=cid))

    # --- replicate calc_fitness -----------------------------------------
    fit = 0
    kinds = defaultdict(int)
    n = len(parsed)
    for j in range(n):
        a = parsed[j]
        if a["subjtype"] != a["roomtype"]:
            kinds["subject/room type mismatch"] += 1
            continue
        ok = True
        for k in range(j + 1, n):
            b = parsed[k]
            if a["day"] != b["day"] or a["hour"] != b["hour"]:
                continue
            if a["room"] == b["room"]:
                kinds["room"] += 1; ok = False; break
            if a["team"] == b["team"]:
                kinds["team"] += 1; ok = False; break
            if a["prof"] == b["prof"]:
                kinds["professor"] += 1; ok = False; break
            if set(a["students"]) & set(b["students"]):
                kinds["student"] += 1; ok = False; break
        fit += ok

    # --- independent housekeeping checks --------------------------------
    counts = defaultdict(int)
    for e in parsed:
        counts[e["cid"]] += 1
    missing = [(c[0], counts[c[0]], subj[c[2]][1]) for c in courses if counts[c[0]] != subj[c[2]][1]]

    print(f"rows={len(out)} parsed={n} unparsable={bad} expected_entries={entries}")
    print(f"fitness={fit} max={entries} valid={fit == entries}")
    print(f"conflicts_by_kind={dict(kinds)}")
    print(f"courses_with_wrong_hour_count={len(missing)}")
    if missing:
        print("  e.g.", missing[:5])
    print(f"distinct_(day,hour)_slots={len({(e['day'], e['hour']) for e in parsed})}")
    return 0 if fit == entries and not bad and not missing else 1


if __name__ == "__main__":
    sys.exit(main())
