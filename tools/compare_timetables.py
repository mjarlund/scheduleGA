#!/usr/bin/env python3
"""Side-by-side human-readable excerpt of two timetables.

Day numbers are meaningless across two different runs (a 195-day grid, two
schedules, two sets of days), so the excerpt is indexed by *attendance day*:
the 1st, 2nd, 3rd ... day on which a given class actually has lessons.  That
is the quantity a school cares about ("this class comes in 9 days a week" vs
"3 days"), and it makes the two runs directly comparable.

    python3 tools/compare_timetables.py --old tools/quality-legacy-seed1.csv \
                                       --new tools/quality-s1000-seed1.csv \
                                       --teams 1a,1b,2c
"""
import argparse
import os
import sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import quality  # noqa: E402

HOURS = 4


def load(path):
    q, parsed = quality.analyse(path)
    cell = {(e["teamname"], e["day"], e["hour"]): e for e in parsed}
    days = defaultdict(set)
    for e in parsed:
        days[e["teamname"]].add(e["day"])
    return q, parsed, cell, {t: sorted(d) for t, d in days.items()}


def block(name, q, cell, days, team, width=21):
    order = days.get(team, [])
    if not order:
        return [f"  {team}: no lessons"]
    out = []
    lessons = 0
    gaps = 0
    for k, d in enumerate(order, 1):
        row = []
        for h in range(1, HOURS + 1):
            e = cell.get((team, d, h))
            if e is None:
                row.append("-" * 7)
            else:
                lessons += 1
                row.append("%s/%s" % (e["subjname"][:9], e["profname"][:9]))
        used = [h for h in range(1, HOURS + 1) if (team, d, h) in cell]
        if used:
            gaps += (max(used) - min(used) + 1) - len(used)
        out.append(f"  day {k:>2} (cal {d:>3}) | " + " | ".join("%-*s" % (width, r) for r in row))
    out.append(f"  -> {len(order)} attendance days, {lessons} lessons, "
               f"{gaps} idle hours in those days")
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--old", required=True)
    ap.add_argument("--new", required=True)
    ap.add_argument("--teams", default="1a,1b,2c")
    a = ap.parse_args()

    qo, _, co, do = load(a.old)
    qn, _, cn, dn = load(a.new)

    print("columns: hour 1 | hour 2 | hour 3 | hour 4        "
          "(no room type printed: every entry in both runs is in a matching room)")
    for team in [t for t in a.teams.split(",") if t]:
        print(f"\n=== class {team} ===")
        print(f"-- OLD objective ({qo['file']}): {qo['team_days']} team-days over "
              f"{qo['days_used']} calendar days, {qo['team_gaps']} idle hours total")
        for line in block("old", qo, co, do, team):
            print(line)
        print(f"-- NEW objective ({qn['file']}): {qn['team_days']} team-days over "
              f"{qn['days_used']} calendar days, {qn['team_gaps']} idle hours total")
        for line in block("new", qn, cn, dn, team):
            print(line)

    print("\nwhole timetable")
    print("  %-28s %10s %10s" % ("metric", "old", "new"))
    for k, label in (("team_days", "team-days (sum)"),
                     ("team_days_mean", "days per class"),
                     ("team_days_max", "max days, one class"),
                     ("team_gaps", "idle hours (sum)"),
                     ("prof_days", "professor-days (sum)"),
                     ("days_used", "calendar days used"),
                     ("distinct_slots", "distinct (day,hour) slots"),
                     ("type_mismatches", "room-type mismatches"),
                     ("clash_pairs_total", "clashing pairs")):
        print("  %-28s %10s %10s" % (label, qo.get(k), qn.get(k)))


if __name__ == "__main__":
    main()
