#!/usr/bin/env python3
"""Derive a *staffed* variant of the committed dataset.

The committed dataset is not a school: one professor teaches 45 lessons a week
(professor 8), which cannot fit in a 6-day x 4-hour week (24 slots) whatever the
search does, and the 154 classroom lessons do not fit in 6 classrooms x 24 slots
= 144.  This tool makes the same timetable instance feasible on a real week by
changing only the staffing and the room list -- never the lessons, classes,
subjects or hours:

  * every professor with more hours than one week can hold is split into
    ceil(hours / (hours_per_day * days)) teachers, balancing the hours
    (a school hires a second teacher; it does not ask one to teach 45 lessons);
  * N extra classrooms are appended (a school rents a room).

Nothing else changes: NUM_ENTRIES, the 30 classes, the 78 courses and the
student lists stay byte-identical, so the instance is the same instance.

    python3 tools/make_school_data.py --out data-d6 --days 6 --add-classrooms 1
"""
import argparse
import csv
import math
import os


def read(path):
    with open(path, newline="") as f:
        return [r for r in csv.reader(f, delimiter=";") if r]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", default="data")
    ap.add_argument("--out", required=True)
    ap.add_argument("--days", type=int, default=6)
    ap.add_argument("--hours", type=int, default=4)
    ap.add_argument("--add-classrooms", type=int, default=1)
    ap.add_argument("--add-gyms", type=int, default=0)
    ap.add_argument("--add-rooms", default="",
                    help="generic form, e.g. 1:4,2:2,3:1,4:1 (type:count)")
    ap.add_argument("--hours-per-day", type=int, default=None,
                    help="default: the --hours value")
    ap.add_argument("--max-load", type=int, default=None,
                    help="max lessons per teacher per week (default: every slot, "
                         "i.e. hours*max(hours-per-day,days) cap => hours_per_day*days)")
    a = ap.parse_args()
    hpd = a.hours_per_day or a.hours
    cap = a.max_load if a.max_load else hpd * a.days

    professors = read(os.path.join(a.src, "professors.csv"))
    subjects = {int(r[0]): (int(r[1]), int(r[2])) for r in read(os.path.join(a.src, "subjects.csv"))}
    courses = read(os.path.join(a.src, "courses.csv"))
    rooms = read(os.path.join(a.src, "rooms.csv"))

    hours = {}
    load = {}
    for cid, pid, sid, tid in courses:
        h = subjects[int(sid)][1]
        hours[cid] = h
        load[int(pid)] = load.get(int(pid), 0) + h

    names = {int(r[0]): r[1] for r in professors}
    next_id = max(names) + 1
    suffixes = "BCDEFGHIJKLMNOPQRSTUVWXYZ"
    new_professors = []          # (id, name)
    course_professor = {}        # course id -> new professor id

    by_prof = {}
    for cid, pid, sid, tid in courses:
        by_prof.setdefault(int(pid), []).append(cid)

    for pid in sorted(load):
        k = max(1, math.ceil(load[pid] / cap))
        groups = [[] for _ in range(k)]
        g_load = [0] * k
        # longest-processing-time first, into the least loaded group
        for cid in sorted(by_prof[pid], key=lambda c: (-hours[c], c)):
            g = min(range(k), key=lambda i: (g_load[i], i))
            groups[g].append(cid)
            g_load[g] += hours[cid]
        ids = []
        for gi, group in enumerate(groups):
            if gi == 0:
                npid = pid
            else:
                npid = next_id
                next_id += 1
                base = names[pid][:7]
                new_professors.append((npid, base + suffixes[(gi - 1) % 26]))
            ids.append(npid)
            for cid in group:
                course_professor[cid] = npid
        print(f"professor {pid:2d} ({names[pid]:9s}) {load[pid]:3d} h -> "
              f"{g_load} h over {ids}")

    os.makedirs(a.out, exist_ok=True)

    with open(os.path.join(a.out, "professors.csv"), "w", newline="") as f:
        w = csv.writer(f, delimiter=";", lineterminator="\n")
        for r in professors:
            w.writerow(r)
        for pid, name in new_professors:
            w.writerow([pid, name])

    with open(os.path.join(a.out, "courses.csv"), "w", newline="") as f:
        w = csv.writer(f, delimiter=";", lineterminator="\n")
        for cid, pid, sid, tid in courses:
            w.writerow([cid, course_professor[cid], sid, tid])

    with open(os.path.join(a.out, "rooms.csv"), "w", newline="") as f:
        w = csv.writer(f, delimiter=";", lineterminator="\n")
        for r in rooms:
            w.writerow(r)
        nid = max(int(r[0]) for r in rooms) + 1
        for i in range(a.add_classrooms):
            w.writerow([nid + i, 1])
        for i in range(a.add_gyms):
            w.writerow([nid + a.add_classrooms + i, 3])
        extra = nid + a.add_classrooms + a.add_gyms
        if a.add_rooms:
            for spec in a.add_rooms.split(","):
                typ, cnt = spec.split(":")
                for i in range(int(cnt)):
                    w.writerow([extra + i, int(typ)])
                extra += int(cnt)

    for name in ("subjects.csv", "teams.csv"):
        with open(os.path.join(a.src, name), newline="") as f:
            data = f.read()
        with open(os.path.join(a.out, name), "w") as f:
            f.write(data)

    print(f"\n-> {a.out}: {len(professors) + len(new_professors)} professors, "
          f"{len(rooms) + a.add_classrooms + a.add_gyms + sum(int(x.split(':')[1]) for x in a.add_rooms.split(',') if x)} rooms, courses unchanged ("
          f"{len(courses)}), entries unchanged")


if __name__ == "__main__":
    main()
