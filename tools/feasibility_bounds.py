#!/usr/bin/env python3
"""Analytic capacity bounds for a scheduleGA dataset (a lower bound on NUM_DAYS).

Every lesson occupies one hour of one *room of its subject's type*, one hour of
its *class* (team) and one hour of its *professor*.  Classes that share students
also cannot meet at the same time.  Each of those is a pigeonhole argument, and
each gives a minimum number of days below which **no** timetable exists,
independently of how good the search is:

    type t        sum of hours of subjects of type t <= (#rooms of type t) * H * D
    professor p   its hours                            <= H * D
    class c       its hours                            <= H * D
    student-pair  hours(c1) + hours(c2)                <= H * D   if they share a student

    python3 tools/feasibility_bounds.py [--data data] [--hours 4]
"""
import argparse
import collections
import csv
import math
import os


def read(path):
    with open(path, newline="") as f:
        return [r for r in csv.reader(f, delimiter=";") if r]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", default="data")
    ap.add_argument("--hours", type=int, default=4)
    a = ap.parse_args()
    H = a.hours

    rooms = {int(i): int(t) for i, t in read(os.path.join(a.data, "rooms.csv"))}
    subjects = {int(r[0]): (int(r[1]), int(r[2]), r[3])
                for r in read(os.path.join(a.data, "subjects.csv"))}
    courses = [(int(r[0]), int(r[1]), int(r[2]), int(r[3]))
               for r in read(os.path.join(a.data, "courses.csv"))]
    teams = {int(r[0]): set(int(x) for x in r[2].split(","))
             for r in read(os.path.join(a.data, "teams.csv"))}

    hours_by_course = [(c, subjects[s][1], p, t) for c, p, s, t in courses]
    total = sum(h for _, h, _, _ in hours_by_course)

    demand_type = collections.Counter()
    cap_type = collections.Counter(rooms.values())
    demand_prof = collections.Counter()
    demand_team = collections.Counter()
    for (cid, p, sid, tid), (_, h, _, _) in zip(courses, hours_by_course):
        demand_type[subjects[sid][0]] += h
        demand_prof[p] += h
        demand_team[tid] += h

    bounds = []      # (resource, demand, capacity per day, days needed)
    for t, d in sorted(demand_type.items()):
        bounds.append((f"room type {t}", d, cap_type[t] * H, math.ceil(d / (cap_type[t] * H))))
    for p, d in sorted(demand_prof.items()):
        bounds.append((f"professor {p}", d, H, math.ceil(d / H)))
    for t, d in sorted(demand_team.items()):
        bounds.append((f"class {t}", d, H, math.ceil(d / H)))
    names = {t.name for t in []}
    tid_name = {}
    for r in read(os.path.join(a.data, "teams.csv")):
        tid_name[int(r[0])] = r[1]
    pairs = []
    tl = sorted(teams)
    for i, x in enumerate(tl):
        for y in tl[i + 1:]:
            if teams[x] & teams[y]:
                pairs.append((x, y))
    for x, y in pairs:
        d = demand_team[x] + demand_team[y]
        bounds.append((f"classes {tid_name.get(x, x)}+{tid_name.get(y, y)} (shared students)",
                       d, H, math.ceil(d / H)))

    floors = [(r, d, cap, need) for r, d, cap, need in bounds if d > cap]
    worst = max(bounds, key=lambda b: b[3])

    print(f"data: {a.data}  hours/day: {H}  entries(lesson-hours): {total}")
    print(f"{'resource':52s} {'demand':>7s} {'cap/day':>8s} {'min days':>9s}")
    for r, d, cap, need in sorted(bounds, key=lambda b: -b[3])[:12]:
        print(f"{r:52s} {d:7d} {cap:8d} {need:9d}")
    print()
    print(f"pigeonhole lower bound on NUM_DAYS: {worst[3]}  (binding: {worst[0]})")
    if floors:
        print("constraints violated at D=1 day (i.e. binding for small D):")
        for r, d, cap, need in floors:
            print(f"  {r:50s} needs >= {need} days")
    for D in (195, 30, 20, 12, 11, 8, 7, 6, 5):
        slots = total / (len(rooms) * H * D) * 100.0
        room_cap_left = {t: cap_type[t] * H * D - demand_type[t] for t in demand_type}
        ok = all(v >= 0 for v in room_cap_left.values()) and \
            all(cap * D >= d for _, d, cap, _ in bounds)
        print(f"  D={D:3d}  room-slot utilisation {slots:6.1f}%   "
              f"classroom slack {room_cap_left.get(1, 0):+4d}   pigeonhole-feasible {ok}")


if __name__ == "__main__":
    main()
