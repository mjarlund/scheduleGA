#!/usr/bin/env python3
"""End-to-end quality benchmark: generations, wall time and timetable quality.

Runs ./sga once per pinned seed, keeps each exported timetable, and scores it
with tools/quality.py (an independent re-implementation, not the C code).

    python3 tools/bench_quality.py --label old --seeds 1 2 3 4 5
    python3 tools/bench_quality.py --label new --seeds 1 2 3 4 5 --binary ./sga-new

Records go to tools/quality-<label>.json, timetables to
tools/quality-<label>-seed<n>.csv.  Prints medians and ranges so A/B stays
honest across the +/-15 % drift of this box.
"""
import argparse
import json
import os
import statistics as st
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import quality  # noqa: E402

SO = os.path.join(HERE, "fixed_seed.so")

FIELDS = ["wall_s", "generations", "gen_first_feasible", "team_days", "team_gaps",
          "prof_days", "distinct_slots", "days_used", "type_mismatches",
          "clash_pairs_total", "courses_wrong_hours"]


def parse_result_line(line):
    out = {}
    for field in line.split(":", 1)[1].split():
        k, v = field.split("=", 1)
        try:
            out[k] = float(v) if "." in v else int(v)
        except ValueError:
            out[k] = v
    return out


def run(seed, pop, binary, timeout, env_extra):
    env = dict(os.environ, SGA_SEED=str(seed), LD_PRELOAD=SO)
    env.update(env_extra)
    t0 = time.perf_counter()
    p = subprocess.run([binary], cwd=ROOT, input=f"{pop}\n", capture_output=True,
                       text=True, env=env, timeout=timeout)
    wall = time.perf_counter() - t0
    # A missing shim only produces a loader warning and an unseeded run, which
    # would silently turn every "pinned seed" number into noise.  Fail loudly.
    if "cannot be preloaded" in p.stderr:
        sys.exit(f"LD_PRELOAD={SO} did not load; the seed is not pinned. "
                 f"Rebuild it: cc -shared -fPIC -O2 -o {SO} {HERE}/fixed_seed.c")
    summary, stopped, gens = {}, "", 0
    for line in p.stdout.splitlines():
        if line.startswith("Result:"):
            summary = parse_result_line(line)
        elif line.startswith("Stopped:"):
            stopped = line.split(":", 1)[1].strip()
        elif line.startswith("Generation:"):
            gens = int(line.split(":")[1])
    return wall, summary, stopped, p.returncode, gens


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--label", required=True)
    ap.add_argument("--seeds", type=int, nargs="+", default=[1, 2, 3, 4, 5])
    ap.add_argument("--pop", type=int, default=50)
    ap.add_argument("--binary", default=os.path.join(ROOT, "sga"))
    ap.add_argument("--timeout", type=int, default=1800)
    ap.add_argument("--env", default="", help="extra env assignments, k=v;k=v")
    a = ap.parse_args()

    env_extra = {}
    for kv in filter(None, a.env.split(";")):
        k, v = kv.split("=", 1)
        env_extra[k] = v

    recs = []
    for s in a.seeds:
        wall, summary, stopped, rc, last_gen = run(s, a.pop, a.binary, a.timeout, env_extra)
        csv = os.path.join(HERE, f"quality-{a.label}-seed{s}.csv")
        if os.path.exists(os.path.join(ROOT, "result", "result.csv")):
            with open(os.path.join(ROOT, "result", "result.csv")) as f:
                data = f.read()
            with open(csv, "w") as f:
                f.write(data)
        q, _ = quality.analyse(os.path.join(ROOT, "result", "result.csv"))
        rec = dict(seed=s, wall_s=round(wall, 3), exit=rc, stopped=stopped,
                   generations=summary.get("gens", last_gen),
                   gen_first_feasible=summary.get("first_feasible", None),
                   best=summary.get("best"), converged=summary.get("converged"),
                   fitness=summary.get("fitness"), soft=summary.get("soft"),
                   **{k: q[k] for k in ("team_days", "team_gaps", "prof_days",
                                        "distinct_slots", "days_used",
                                        "type_mismatches", "clash_pairs_total",
                                        "courses_wrong_hours")},
                   feasible=q["feasible"], csv=os.path.relpath(csv, ROOT))
        recs.append(rec)
        print("  seed=%2d wall=%6.3fs gens=%5d ff=%-5s best=%s conv=%s | type=%d clashes=%d "
              "team_days=%3d gaps=%3d prof_days=%3d slots=%3d"
              % (s, rec["wall_s"], rec["generations"], rec["gen_first_feasible"], rec["best"],
                 rec["converged"], rec["type_mismatches"], rec["clash_pairs_total"],
                 rec["team_days"], rec["team_gaps"], rec["prof_days"], rec["distinct_slots"]),
              flush=True)

    out = os.path.join(HERE, f"quality-{a.label}.json")
    with open(out, "w") as f:
        json.dump(recs, f, indent=1)

    print("\n%-20s %10s %10s %10s" % ("field", "median", "min", "max"))
    for k in FIELDS:
        vals = [r[k] for r in recs if r.get(k) is not None]
        if not vals:
            continue
        print("%-20s %10.3f %10.3f %10.3f"
              % (k, st.median(vals), min(vals), max(vals)))
    print("feasible %d/%d  converged %d/%d -> %s"
          % (sum(r["feasible"] for r in recs), len(recs),
             sum(bool(r["converged"]) for r in recs), len(recs), out))


if __name__ == "__main__":
    main()
