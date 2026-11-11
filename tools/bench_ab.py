#!/usr/bin/env python3
"""Interleaved A/B benchmark of two sga binaries (same seeds, same dataset).

Builds are compared round-robin inside one session so that CPU frequency drift
and noisy-neighbour effects hit both variants equally; the order inside a round
is flipped every round.  Also checks that both binaries export a byte-identical
timetable for each seed, i.e. that the RNG stream and the GA trajectory match.

    python3 tools/bench_ab.py --a /tmp/sga-old --b ./sga --seeds 1 2 3 4 5 --rounds 5
"""
import argparse
import hashlib
import os
import statistics
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SO = os.path.join(HERE, "fixed_seed.so")


def run(binary, pop, seed, timeout):
    env = dict(os.environ, SGA_SEED=str(seed), LD_PRELOAD=SO)
    t0 = time.perf_counter()
    p = subprocess.run([binary], cwd=ROOT, input=f"{pop}\n", capture_output=True,
                       text=True, env=env, timeout=timeout)
    dt = time.perf_counter() - t0
    gens = 0
    for line in p.stdout.splitlines():
        if line.startswith("Generation:"):
            gens = int(line.split(":")[1])
    with open(os.path.join(ROOT, "result", "result.csv"), "rb") as f:
        digest = hashlib.md5(f.read()).hexdigest()
    return dt, gens, digest, p.returncode


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--a", required=True)
    ap.add_argument("--b", required=True)
    ap.add_argument("--pop", type=int, default=50)
    ap.add_argument("--seeds", type=int, nargs="+", default=[1, 2, 3])
    ap.add_argument("--rounds", type=int, default=5)
    ap.add_argument("--timeout", type=int, default=600)
    a = ap.parse_args()

    bins = {"A": os.path.abspath(a.a), "B": os.path.abspath(a.b)}
    res = {"A": [], "B": []}
    gens = {"A": {}, "B": {}}
    dig = {"A": {}, "B": {}}

    for r in range(a.rounds):
        order = ["A", "B"] if r % 2 == 0 else ["B", "A"]
        for seed in a.seeds:
            for k in order:
                dt, g, d, rc = run(bins[k], a.pop, seed, a.timeout)
                res[k].append(dt)
                gens[k][seed] = g
                dig[k][seed] = d
                if rc != 0:
                    sys.exit(f"{k} failed on seed {seed} (exit {rc})")

    same_traj = all(gens["A"][s] == gens["B"][s] for s in a.seeds)
    same_out = all(dig["A"][s] == dig["B"][s] for s in a.seeds)

    print(f"pop={a.pop} seeds={a.seeds} rounds={a.rounds} "
          f"({len(res['A'])} runs per variant)")
    for k in ("A", "B"):
        v = res[k]
        print(f"  {k}: median={statistics.median(v):.3f}s min={min(v):.3f}s "
              f"max={max(v):.3f}s mean={statistics.mean(v):.3f}s")
    med_a, med_b = statistics.median(res["A"]), statistics.median(res["B"])
    print(f"  median speedup A->B: {med_a / med_b:.2f}x  "
          f"(min-based {min(res['A']) / min(res['B']):.2f}x)")
    print(f"  identical generation count per seed: {same_traj}")
    print(f"  identical exported result.csv per seed: {same_out}")
    print(f"  generations per seed: {gens['A']}")
    return 0 if same_traj and same_out else 1


if __name__ == "__main__":
    sys.exit(main())
