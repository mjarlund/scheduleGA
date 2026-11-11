#!/usr/bin/env python3
"""The same timetable instance as a CP-SAT model (OR-Tools), hard constraints only.

    python3 tools/cpsat_solve.py --data data-d6 --days 6 --hours 4 --time-limit 60
    python3 tools/cpsat_solve.py --data data --days 6 --time-limit 60      # proves UNSAT

Model (exactly the GA's conflict definition, no objective unless asked):
  * one decision per lesson: (slot, room), room restricted to the subject's type;
  * at most one lesson per room and slot;
  * at most one lesson per class and slot;
  * at most one lesson per professor and slot;
  * classes that share a student cannot meet in the same slot;
  * symmetry break: the lessons of one course occupy increasing slots.

--objective team-days adds the same compaction term the GA's soft objective
uses (sum over classes of distinct days used) and minimises it.

Writes result/cpsat-result.csv in the program's own export format, so
tools/quality.py can check it independently.
"""
import argparse
import csv
import os
import sys
import time

from ortools.sat.python import cp_model


def read(path):
    with open(path, newline="") as f:
        return [r for r in csv.reader(f, delimiter=";") if r]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", default="data")
    ap.add_argument("--days", type=int, default=6)
    ap.add_argument("--hours", type=int, default=4)
    ap.add_argument("--time-limit", type=float, default=60.0)
    ap.add_argument("--workers", type=int, default=4)
    ap.add_argument("--objective", choices=["none", "team-days"], default="none")
    ap.add_argument("--out", default="result/cpsat-result.csv")
    a = ap.parse_args()

    rooms = [(int(r[0]), int(r[1])) for r in read(os.path.join(a.data, "rooms.csv"))]
    subjects = {int(r[0]): (int(r[1]), int(r[2]), r[3])
                for r in read(os.path.join(a.data, "subjects.csv"))}
    professors = {int(r[0]): r[1] for r in read(os.path.join(a.data, "professors.csv"))}
    teams = {}
    for r in read(os.path.join(a.data, "teams.csv")):
        teams[int(r[0])] = (r[1], set(int(x) for x in r[2].split(",")))
    courses = [(int(r[0]), int(r[1]), int(r[2]), int(r[3]))
               for r in read(os.path.join(a.data, "courses.csv"))]

    # lesson-level entries
    entries = []      # (course id, professor id, subject type, team id, subject name)
    for cid, pid, sid, tid in courses:
        for _ in range(subjects[sid][1]):
            entries.append((cid, pid, subjects[sid][0], tid, subjects[sid][2]))
    n = len(entries)
    D, H = a.days, a.hours
    nslots = D * H

    rooms_of_type = {}
    for rid, typ in rooms:
        rooms_of_type.setdefault(typ, []).append(rid)
    for typ, sid_hours in ((t, h) for t, h, _ in subjects.values()):
        if not rooms_of_type.get(typ):
            sys.exit(f"subject type {typ} has no room")

    t_build = time.perf_counter()
    m = cp_model.CpModel()

    # x[i][slot] = 1 if lesson i is in this slot, room chosen separately
    x = {}
    for i, (cid, pid, typ, tid, _name) in enumerate(entries):
        for s in range(nslots):
            for rid in rooms_of_type[typ]:
                x[i, s, rid] = m.NewBoolVar("")

    for i in range(n):
        m.AddExactlyOne(x[i, s, rid] for s in range(nslots)
                        for rid in rooms_of_type[entries[i][2]])

    # room capacity
    for rid, typ in rooms:
        for s in range(nslots):
            idx = [x[i, s, rid] for i in range(n) if entries[i][2] == typ]
            if idx:
                m.AddAtMostOne(idx)

    def slot_of(i, s):
        return [x[i, s, rid] for rid in rooms_of_type[entries[i][2]]]

    # class and professor capacity
    by_team = {}
    by_prof = {}
    for i, (cid, pid, typ, tid, _name) in enumerate(entries):
        by_team.setdefault(tid, []).append(i)
        by_prof.setdefault(pid, []).append(i)
    for tid, idx in by_team.items():
        for s in range(nslots):
            terms = [v for i in idx for v in slot_of(i, s)]
            if terms:
                m.AddAtMostOne(terms)
    for pid, idx in by_prof.items():
        for s in range(nslots):
            terms = [v for i in idx for v in slot_of(i, s)]
            if terms:
                m.AddAtMostOne(terms)

    # classes sharing a student cannot meet at the same time
    tids = sorted(teams)
    shared = [(p, q) for ai, p in enumerate(tids) for q in tids[ai + 1:]
              if teams[p][1] & teams[q][1]]
    for p, q in shared:
        for s in range(nslots):
            terms = [v for i in by_team.get(p, []) for v in slot_of(i, s)]
            terms += [v for i in by_team.get(q, []) for v in slot_of(i, s)]
            if terms:
                m.AddAtMostOne(terms)

    # symmetry: the lessons of one course occupy strictly increasing slots
    by_course = {}
    for i, (cid, pid, typ, tid, _name) in enumerate(entries):
        by_course.setdefault(cid, []).append(i)
    for cid, idx in by_course.items():
        for a_i, b_i in zip(idx, idx[1:]):
            sa = sum(s * v for s in range(nslots) for v in slot_of(a_i, s))
            sb = sum(s * v for s in range(nslots) for v in slot_of(b_i, s))
            m.Add(sa + 1 <= sb)

    if a.objective == "team-days":
        day = {}
        obj = []
        for tid, idx in by_team.items():
            for d in range(D):
                b = m.NewBoolVar("")
                day[tid, d] = b
                terms = [x[i, s, rid] for i in idx for s in range(d * H, (d + 1) * H)
                         for rid in rooms_of_type[entries[i][2]]]
                m.AddMaxEquality(b, terms) if len(terms) > 1 else m.Add(b == terms[0])
                obj.append(b)
        m.Minimize(sum(obj))

    build_s = time.perf_counter() - t_build

    class FirstSolution(cp_model.CpSolverSolutionCallback):
        def __init__(self):
            super().__init__()
            self.t = None

        def on_solution_callback(self):
            if self.t is None:
                self.t = time.perf_counter() - t0

    cb = FirstSolution()
    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = a.time_limit
    solver.parameters.num_search_workers = a.workers
    t0 = time.perf_counter()
    status = solver.Solve(m, cb)
    wall = time.perf_counter() - t0
    name = solver.StatusName(status)
    print(f"data={a.data} days={D} hours={H} entries={n} objective={a.objective}")
    print(f"status={name} build={build_s:.2f}s solve={wall:.3f}s "
          f"first_solution={('%.3fs' % cb.t) if cb.t is not None else '-'} "
          f"conflicts={solver.NumConflicts()} branches={solver.NumBranches()} "
          f"best_bound={solver.BestObjectiveBound()}"
          + (f" objective={solver.ObjectiveValue()}" if status in
             (cp_model.OPTIMAL, cp_model.FEASIBLE) else ""))

    if status not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        sys.exit(0 if status == cp_model.INFEASIBLE else 2)

    rows = []
    for i, (cid, pid, typ, tid, _name) in enumerate(entries):
        for s in range(nslots):
            for rid in rooms_of_type[typ]:
                if solver.Value(x[i, s, rid]):
                    rows.append((s // H + 1, s % H + 1, professors[pid],
                                 entries[i][4], teams[tid][0], rid))
    os.makedirs(os.path.dirname(a.out), exist_ok=True)
    with open(a.out, "w", newline="") as f:
        w = csv.writer(f, delimiter=";", lineterminator="\n")
        for r in sorted(rows):
            w.writerow(r)
    print(f"wrote {a.out} ({len(rows)} rows)")


if __name__ == "__main__":
    main()
