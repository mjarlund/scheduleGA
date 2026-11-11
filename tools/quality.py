#!/usr/bin/env python3
"""Independent quality report for result/result.csv.

Reads the exported timetable and the CSV dataset and prints every quantity the
objective is supposed to care about, computed from scratch in Python -- it
shares no code with fitness.c, so it double-checks the C side instead of
echoing it.

Hard (a schedule that violates any of these is not a timetable):
  * clash pairs  -- same (day,hour) and same room / team / professor / student
  * type_bad     -- entry whose course.subject.type has no match with room.type
  * short courses-- a course present a number of times != subject.hours

Soft (what a school actually wants):
  team_days      -- sum over teams of the number of distinct days used
  team_gaps      -- idle hours between a team's first and last lesson per day
  prof_days      -- same as team_days, per professor
  slots          -- distinct (day,hour) slots occupied (lower = denser)

Usage:
  python3 tools/quality.py                       # human readable
  python3 tools/quality.py --json                # one JSON object
  python3 tools/quality.py --excerpt             # small timetable excerpt
  python3 tools/quality.py --excerpt --days 1,2 --teams 1a,1b
"""
import argparse
import json
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


def analyse(path):
    profs, subj, rooms, teams, courses = load()
    with open(path) as f:
        raw = [l.rstrip("\n").split(";") for l in f if l.strip()]

    by_name = {}
    for cid, p, s, t in courses:
        by_name[(profs[p], subj[s][2], teams[t][0])] = (cid, p, s, t)

    parsed, unparsable = [], 0
    for r in raw:
        day, hour, pn, sn, tn, rid = int(r[0]), int(r[1]), r[2], r[3], r[4], int(r[5])
        if (pn, sn, tn) not in by_name or rid not in rooms:
            unparsable += 1
            continue
        cid, p, s, t = by_name[(pn, sn, tn)]
        parsed.append(dict(day=day, hour=hour, room=rid, roomtype=rooms[rid],
                           prof=p, profname=pn, team=t, teamname=tn, subj=s,
                           subjtype=subj[s][0], subjname=sn,
                           hours=subj[s][1], students=teams[t][1], cid=cid))

    # ---- hard -----------------------------------------------------------
    clash_pairs = defaultdict(int)          # kind -> number of conflicting pairs
    type_bad = [e for e in parsed if e["subjtype"] != e["roomtype"]]
    by_slot = defaultdict(list)
    for e in parsed:
        by_slot[(e["day"], e["hour"])].append(e)
    for cell in by_slot.values():
        for j in range(len(cell)):
            for k in range(j + 1, len(cell)):
                a, b = cell[j], cell[k]
                if a["room"] == b["room"]:
                    clash_pairs["room"] += 1
                elif a["team"] == b["team"]:
                    clash_pairs["team"] += 1
                elif a["prof"] == b["prof"]:
                    clash_pairs["professor"] += 1
                elif set(a["students"]) & set(b["students"]):
                    clash_pairs["student"] += 1

    counts = defaultdict(int)
    for e in parsed:
        counts[e["cid"]] += 1
    short = [(c[0], counts[c[0]], subj[c[2]][1]) for c in courses
             if counts[c[0]] != subj[c[2]][1]]

    # ---- soft -----------------------------------------------------------
    team_days, team_gaps = defaultdict(set), defaultdict(int)
    prof_days = defaultdict(set)
    for e in parsed:
        team_days[e["team"]].add(e["day"])
        prof_days[e["prof"]].add(e["day"])
    per_day = defaultdict(list)
    for e in parsed:
        per_day[(e["team"], e["day"])].append(e["hour"])
    for (t, d), hours in per_day.items():
        team_gaps[t] += (max(hours) - min(hours) + 1) - len(hours)

    team_day_sum = sum(len(v) for v in team_days.values())
    prof_day_sum = sum(len(v) for v in prof_days.values())
    gap_sum = sum(team_gaps.values())
    days_used = sorted({e["day"] for e in parsed})
    hours_used = sorted({e["hour"] for e in parsed})

    out = dict(
        file=os.path.relpath(path, ROOT),
        rows=len(raw), parsed=len(parsed), unparsable=unparsable,
        expected_entries=sum(subj[c[2]][1] for c in courses),
        # hard
        type_mismatches=len(type_bad), clash_pairs=dict(clash_pairs),
        clash_pairs_total=sum(clash_pairs.values()),
        courses_wrong_hours=len(short), short_courses=short[:5],
        feasible=not type_bad and not clash_pairs and not short and not unparsable,
        # soft
        team_days=team_day_sum,
        team_days_mean=round(team_day_sum / max(1, len(team_days)), 2),
        team_days_max=max((len(v) for v in team_days.values()), default=0),
        team_days_per_team={teams[t][0]: len(v) for t, v in sorted(team_days.items())},
        team_gaps=gap_sum,
        team_gaps_max=max(team_gaps.values(), default=0),
        team_gaps_per_team={teams[t][0]: team_gaps[t] for t in sorted(team_gaps)},
        prof_days=prof_day_sum,
        prof_days_mean=round(prof_day_sum / max(1, len(prof_days)), 2),
        prof_days_max=max((len(v) for v in prof_days.values()), default=0),
        distinct_slots=len(by_slot),
        days_used=len(days_used),
        day_range=[days_used[0], days_used[-1]] if days_used else [],
        hours_used=hours_used,
        entries_per_team=round(len(parsed) / max(1, len(team_days)), 2),
    )
    return out, parsed


def excerpt(parsed, teams_wanted, days_wanted, max_teams=4, max_days=3):
    """A small human-readable slice: rows = hours, columns = (team, day)."""
    if not teams_wanted:
        # teams with the most lessons, to show something representative
        n = defaultdict(int)
        for e in parsed:
            n[e["teamname"]] += 1
        teams_wanted = [t for t, _ in sorted(n.items(), key=lambda kv: -kv[1])][:max_teams]
    if not days_wanted:
        n = defaultdict(int)
        for e in parsed:
            n[e["day"]] += 1
        days_wanted = sorted(d for d, _ in sorted(n.items(), key=lambda kv: -kv[1])[:max_days])

    cell = {}
    for e in parsed:
        if e["teamname"] in teams_wanted and e["day"] in days_wanted:
            cell[(e["hour"], e["teamname"], e["day"])] = e

    lines = []
    for t in teams_wanted:
        for d in days_wanted:
            row = []
            for h in (1, 2, 3, 4):
                e = cell.get((h, t, d))
                if e is None:
                    row.append("--")
                else:
                    row.append("%s/%s/r%d(t%d)" % (e["subjname"], e["profname"][:6],
                                                   e["room"], e["roomtype"]))
            lines.append("  %-3s day %-3d | " % (t, d) + " | ".join("%-26s" % c for c in row))
        lines.append("")
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--file", default=os.path.join(ROOT, "result", "result.csv"))
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--excerpt", action="store_true")
    ap.add_argument("--teams", default="")
    ap.add_argument("--days", default="")
    a = ap.parse_args()

    if not os.path.exists(a.file):
        sys.exit("%s does not exist" % a.file)
    q, parsed = analyse(a.file)

    if a.excerpt:
        teams = [s for s in a.teams.split(",") if s]
        days = [int(s) for s in a.days.split(",") if s]
        print(excerpt(parsed, teams, days))
        return 0
    if a.json:
        print(json.dumps(q, sort_keys=True))
        return 0

    print("file              %s" % q["file"])
    print("rows %d parsed %d unparsable %d expected %d"
          % (q["rows"], q["parsed"], q["unparsable"], q["expected_entries"]))
    print("HARD  feasible=%s type_mismatches=%d clash_pairs=%d %s courses_wrong_hours=%d"
          % (q["feasible"], q["type_mismatches"], q["clash_pairs_total"],
             q["clash_pairs"], q["courses_wrong_hours"]))
    print("SOFT  team_days=%d (mean %.2f, max %d)  team_gaps=%d (max %d)  prof_days=%d (mean %.2f, max %d)"
          % (q["team_days"], q["team_days_mean"], q["team_days_max"],
             q["team_gaps"], q["team_gaps_max"],
             q["prof_days"], q["prof_days_mean"], q["prof_days_max"]))
    print("      distinct_slots=%d days_used=%d day_range=%s"
          % (q["distinct_slots"], q["days_used"], q["day_range"]))
    return 0 if q["feasible"] else 1


if __name__ == "__main__":
    sys.exit(main())
