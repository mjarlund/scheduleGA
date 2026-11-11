#!/usr/bin/env python3
"""Benchmark ./sga end-to-end with a pinned RNG seed (tools/fixed_seed.so).

    python3 tools/bench.py --label baseline --pop 50 --seeds 1 2 3

Writes one JSON record per run to tools/bench-<label>.json and prints a summary.
Wall clock covers process start, CSV parsing and the whole GA loop.

The 2015 loop only stops at fitness == NUM_ENTRIES, so `generations` is taken
from the last "Generation:" line.  Builds that also print a
`Result: gens=.. best=.. worst=.. mean=.. converged=..` summary (the
fix/generation-loop branch) are read from that line instead, which also yields
the final worst/mean and whether the run converged or hit a bound.
"""
import argparse
import json
import os
import statistics
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
        out[k] = float(v) if "." in v else int(v)
    return out


def run_once(pop, seed, timeout):
    env = dict(os.environ, SGA_SEED=str(seed), LD_PRELOAD=SO)
    t0 = time.perf_counter()
    p = subprocess.run([os.path.join(ROOT, "sga")], cwd=ROOT, input=f"{pop}\n",
                       capture_output=True, text=True, env=env, timeout=timeout)
    dt = time.perf_counter() - t0
    gens, best, traj = 0, 0, []
    summary = {}
    stopped = ""
    for line in p.stdout.splitlines():
        if line.startswith("Generation:"):
            gens = int(line.split(":")[1])
        elif line.startswith("Best:"):
            best = int(line.split(":")[1])
            traj.append(best)
        elif line.startswith("Result:"):
            summary = parse_result_line(line)
        elif line.startswith("Stopped:"):
            stopped = line.split(":", 1)[1].strip()
    rec = dict(seed=seed, pop=pop, wall_s=round(dt, 3), generations=gens, best=best,
               first_best=traj[0] if traj else None, improvements=len(traj),
               exit=p.returncode, timed_out=False)
    if summary:
        rec.update(generations=summary["gens"], best=summary["best"],
                   final_worst=summary["worst"], final_mean=summary["mean"],
                   converged=bool(summary["converged"]))
    rec["stopped"] = stopped
    return rec


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--label", required=True)
    ap.add_argument("--pop", type=int, default=50)
    ap.add_argument("--seeds", type=int, nargs="+", default=[1, 2, 3])
    ap.add_argument("--timeout", type=int, default=600)
    a = ap.parse_args()

    if not os.path.exists(SO):
        sys.exit("build tools/fixed_seed.so first (cc -shared -fPIC -O2 ...)")
    out = os.path.join(HERE, f"bench-{a.label}.json")
    recs = []
    for s in a.seeds:
        r = run_once(a.pop, s, a.timeout)
        recs.append(r)
        extra = ""
        if "final_worst" in r:
            extra = (f" worst={r['final_worst']} mean={r['final_mean']:.2f}"
                     f" converged={int(r['converged'])}")
        print(f"  seed={s:>3} wall={r['wall_s']:>7.3f}s gens={r['generations']:>6} "
              f"best={r['best']} first_best={r['first_best']}{extra}", flush=True)
    with open(out, "w") as f:
        json.dump(recs, f, indent=1)
    w = [r["wall_s"] for r in recs]
    g = [r["generations"] for r in recs]
    conv = [r["converged"] for r in recs if "converged" in r]
    print(f"{a.label}: n={len(recs)} wall median={statistics.median(w):.3f}s range={min(w)}-{max(w)}s "
          f"gens median={statistics.median(g)} gens/sec median="
          f"{statistics.median([g[i]/w[i] for i in range(len(w))]):.0f}"
          + (f" converged={sum(conv)}/{len(conv)}" if conv else "") + f" -> {out}")


if __name__ == "__main__":
    main()
