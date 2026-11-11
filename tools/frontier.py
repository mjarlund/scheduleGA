#!/usr/bin/env python3
"""Feasibility frontier: run the GA on a sequence of grid sizes, pinned seeds.

    python3 tools/frontier.py --label soft --days 195 30 20 12 8 6 5 \
            --seeds 1 2 3 4 5 --binary ./sga

Each run is one process with SGA_DAYS set; the seed is pinned through
tools/fixed_seed.so (LD_PRELOAD time() shim).  Uses the same run format as
tools/bench_quality.py, so the two are directly comparable.  Writes
tools/frontier-<label>.json and prints one row per (days, seed) plus a summary
per days value (median of the seeds).
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
SO = os.path.join(HERE, "fixed_seed.so")


def parse_result_line(line):
    out = {}
    for field in line.split(":", 1)[1].split():
        k, v = field.split("=", 1)
        try:
            out[k] = float(v) if "." in v else int(v)
        except ValueError:
            out[k] = v
    return out


def run(cfg, seed, binary, pop, timeout):
    env = dict(os.environ, SGA_SEED=str(seed), LD_PRELOAD=SO)
    env.update({k: str(v) for k, v in cfg.items() if v is not None})
    t0 = time.perf_counter()
    p = subprocess.run([binary], cwd=ROOT, input=f"{pop}\n", capture_output=True,
                       text=True, env=env, timeout=timeout)
    wall = time.perf_counter() - t0
    if "cannot be preloaded" in p.stderr:
        sys.exit(f"LD_PRELOAD={SO} did not load; the seed is not pinned")
    summary, stopped = {}, ""
    for line in p.stdout.splitlines():
        if line.startswith("Result:"):
            summary = parse_result_line(line)
        elif line.startswith("Stopped:"):
            stopped = line.split(":", 1)[1].strip()
    return wall, summary, stopped, p.returncode


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--label", required=True)
    ap.add_argument("--days", type=int, nargs="+", required=True)
    ap.add_argument("--hours", type=int, default=None,
                    help="SGA_HOURS (default: leave the binary default, 4)")
    ap.add_argument("--seeds", type=int, nargs="+", default=[1, 2, 3, 4, 5])
    ap.add_argument("--pop", type=int, default=50)
    ap.add_argument("--binary", default=os.path.join(ROOT, "sga"))
    ap.add_argument("--timeout", type=int, default=3600)
    ap.add_argument("--env", default="", help="extra env, k=v;k=v")
    a = ap.parse_args()

    extra = {}
    for kv in filter(None, a.env.split(";")):
        k, v = kv.split("=", 1)
        extra[k] = v

    recs = []
    for d in a.days:
        cfg = dict(extra, SGA_DAYS=d)
        if a.hours:
            cfg["SGA_HOURS"] = a.hours
        for s in a.seeds:
            wall, summary, stopped, rc = run(cfg, s, a.binary, a.pop, a.timeout)
            rec = dict(days=d, seed=s, wall_s=round(wall, 3), exit=rc,
                       best=summary.get("best"), converged=summary.get("converged"),
                       generations=summary.get("gens"),
                       first_feasible=summary.get("first_feasible"),
                       max_entries=summary.get("max"),
                       fitness=summary.get("fitness"), stopped=stopped)
            recs.append(rec)
            print("D=%-3d seed=%2d wall=%7.3fs gens=%5s best=%3s/%s conv=%s ff=%-5s %s"
                  % (d, s, rec["wall_s"], rec["generations"], rec["best"],
                     rec["max_entries"], rec["converged"], rec["first_feasible"],
                     stopped[:48]), flush=True)

    out = os.path.join(HERE, f"frontier-{a.label}.json")
    with open(out, "w") as f:
        json.dump(recs, f, indent=1)

    print("\n%-5s %-9s %8s %8s %8s %9s" %
          ("days", "conv", "best_med", "best_min", "ff_med", "wall_med"))
    for d in a.days:
        rs = [r for r in recs if r["days"] == d]
        conv = sum(bool(r["converged"]) for r in rs)
        bests = [r["best"] for r in rs if r["best"] is not None]
        ffs = [r["first_feasible"] for r in rs if r["first_feasible"]]
        print("%-5d %d/%-7d %8.1f %8d %8s %8.3f" %
              (d, conv, len(rs), st.median(bests), min(bests),
               st.median(ffs) if ffs else "-", st.median([r["wall_s"] for r in rs])))
    print(f"\n-> {out}")


if __name__ == "__main__":
    main()
