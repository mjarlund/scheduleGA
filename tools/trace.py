#!/usr/bin/env python3
"""Per-generation best/worst/mean fitness trace.

The GA itself only prints when the best fitness improves, so this builds an
INSTRUMENTED COPY of the current working tree in a scratch directory (the
program source is not modified in the repo) with one stats block inserted
after calc_fitness() in main.c, then runs it with a pinned seed.

    python3 tools/trace.py --label baseline --pop 50 --seed 3

Writes tools/trace-<label>.csv with gen;best;worst;mean and prints milestones.
"""
import argparse
import os
import statistics
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SO = os.path.join(HERE, "fixed_seed.so")

BLOCK = """
        {   /* TRACE: measurement instrumentation, not part of the program */
            int _i, _mx = population[0].fitness, _mn = population[0].fitness;
            long _sum = 0;
            for(_i = 0; _i < max_pop; _i++) {
                _sum += population[_i].fitness;
                if(population[_i].fitness > _mx) _mx = population[_i].fitness;
                if(population[_i].fitness < _mn) _mn = population[_i].fitness;
            }
            fprintf(stderr, "STAT;%d;%d;%d;%.2f\\n", generation, _mx, _mn,
                    (double)_sum/max_pop);
        }
"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--label", required=True)
    ap.add_argument("--pop", type=int, default=50)
    ap.add_argument("--seed", type=int, default=3)
    ap.add_argument("--timeout", type=int, default=900)
    a = ap.parse_args()

    src = open(os.path.join(ROOT, "main.c")).read()
    anchor = "        calc_fitness(population, max_pop);\n"
    if anchor not in src:
        sys.exit("anchor for instrumentation not found in main.c")
    src = src.replace(anchor, anchor + BLOCK, 1)

    tmp = tempfile.mkdtemp(prefix="sga-trace-")
    for f in os.listdir(ROOT):
        if f.endswith(".c"):
            open(os.path.join(tmp, f), "w").write(open(os.path.join(ROOT, f)).read())
    open(os.path.join(tmp, "main.c"), "w").write(src)
    os.makedirs(os.path.join(tmp, "header"))
    for f in os.listdir(os.path.join(ROOT, "header")):
        open(os.path.join(tmp, "header", f), "w").write(open(os.path.join(ROOT, "header", f)).read())
    os.symlink(os.path.join(ROOT, "data"), os.path.join(tmp, "data"))
    os.makedirs(os.path.join(tmp, "result"))

    cc = ["cc", "-O2", "-std=c99", "-o", "sga-trace", "main.c", "fitness.c", "parsing.c",
          "initialize.c", "functions.c", "crossover.c", "mutation.c", "selection.c", "export.c"]
    subprocess.run(cc, cwd=tmp, check=True)
    env = dict(os.environ, SGA_SEED=str(a.seed), LD_PRELOAD=SO)
    p = subprocess.run(["./sga-trace"], cwd=tmp, input=f"{a.pop}\n", capture_output=True,
                       text=True, env=env, timeout=a.timeout)

    stats = []
    for line in p.stderr.splitlines():
        if line.startswith("STAT;"):
            _, g, mx, mn, mean = line.split(";")
            stats.append((int(g), int(mx), int(mn), float(mean)))
    if not stats:
        sys.exit("no STAT lines; program output:\n" + p.stdout[-2000:] + p.stderr[-2000:])

    out = os.path.join(HERE, f"trace-{a.label}.csv")
    with open(out, "w") as f:
        f.write("gen;best;worst;mean\n")
        for r in stats:
            f.write(f"{r[0]};{r[1]};{r[2]};{r[3]:.2f}\n")

    best_final = stats[-1][1]
    def milestone(frac):
        target = frac * best_final
        for g, mx, mn, mean in stats:
            if mx >= target:
                return g
        return None

    print(f"{a.label}: pop={a.pop} seed={a.seed} generations={stats[-1][0]} "
          f"final best={best_final} worst={stats[-1][2]} mean={stats[-1][3]:.2f}")
    print(f"  gen1  best={stats[0][1]} worst={stats[0][2]} mean={stats[0][3]:.2f}")
    print(f"  gen reaching 50%/90%/100% of final best: "
          f"{milestone(.5)}/{milestone(.9)}/{milestone(1.0)}")
    step = max(1, len(stats) // 8)
    print("  sampled: " + ", ".join(f"g{s[0]}:{s[1]}/{s[2]}/{s[3]:.1f}" for s in stats[::step]))
    print(f"  -> {out}")


if __name__ == "__main__":
    main()
