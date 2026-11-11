#!/usr/bin/env python3
"""In-situ split of one generation into calc_fitness vs everything else.

Builds an INSTRUMENTED COPY of the working tree (like tools/trace.py, the repo
source is not touched) that measures, with CLOCK_MONOTONIC, how much of the run
is spent inside calc_fitness().  The rest of the wall clock is process start-up,
CSV parsing, the 100 copy_schedule() memcpys, the RNG-driven operators and the
export; the same run's wall clock is measured by tools/bench.py.

    python3 tools/time_parts.py --pop 50 --seeds 1 3 --runs 3
"""
import argparse
import os
import statistics
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SO = os.path.join(HERE, "fixed_seed.so")

DECL_ANCHORS = (
    "    int i, new_opt, generation = 1;\n",            # parent / perf branch
    "    int i, best, stagnant = 0, generation = 1;\n", # fix/generation-loop
    "int main(void) {\n",                              # fallback
)
# Integer nanoseconds on purpose: a double accumulator in the generation loop
# measurably perturbs this binary (625 ms -> 1093 ms for one seeded run on the
# dev box), which would make the split report its own overhead.
DECL_SUFFIX = "    struct timespec _t0, _t1; long long _tf = 0;\n"
CALL = "        calc_fitness(population, max_pop);\n"
CALL_NEW = ("        clock_gettime(CLOCK_MONOTONIC, &_t0);\n" + CALL +
            "        clock_gettime(CLOCK_MONOTONIC, &_t1);\n"
            "        _tf += (_t1.tv_sec - _t0.tv_sec) * 1000000000LL + "
            "(_t1.tv_nsec - _t0.tv_nsec);\n")
EXPORT = "    export(&optimum);"
EXPORT_NEW = ('    fprintf(stderr, "TIME;%lld;%d\\n", _tf, generation - 1);\n'
              + EXPORT)


def build(src_dir, cflags=()):
    tmp = tempfile.mkdtemp(prefix="sga-time-")
    for f in os.listdir(src_dir):
        if f.endswith(".c"):
            open(os.path.join(tmp, f), "w").write(open(os.path.join(src_dir, f)).read())
    os.makedirs(os.path.join(tmp, "header"))
    for f in os.listdir(os.path.join(src_dir, "header")):
        open(os.path.join(tmp, "header", f), "w").write(
            open(os.path.join(src_dir, "header", f)).read())

    src = open(os.path.join(tmp, "main.c")).read()
    decl = next((d for d in DECL_ANCHORS if d in src), None)
    if decl is None:
        sys.exit("instrumentation anchor not found: " + " | ".join(DECL_ANCHORS))
    if CALL not in src or EXPORT not in src:
        sys.exit("instrumentation anchor not found (calc_fitness/export)")
    src = src.replace(decl, decl + DECL_SUFFIX, 1)
    src = src.replace(CALL, CALL_NEW, 1)
    src = src.replace(EXPORT, EXPORT_NEW, 1)
    open(os.path.join(tmp, "main.c"), "w").write(src)

    os.symlink(os.path.join(ROOT, "data"), os.path.join(tmp, "data"))
    os.makedirs(os.path.join(tmp, "result"))
    cc = ["cc", "-O2", "-std=c99", "-D_POSIX_C_SOURCE=199309L"] + list(cflags) + \
         ["-o", "sga-time",
          "main.c", "fitness.c", "parsing.c", "initialize.c", "functions.c",
          "crossover.c", "mutation.c", "selection.c", "export.c"]
    subprocess.run(cc, cwd=tmp, check=True)
    return tmp


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pop", type=int, default=50)
    ap.add_argument("--seeds", type=int, nargs="+", default=[1, 3])
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--src", default=ROOT, help="tree to instrument (default: this one)")
    ap.add_argument("--cflags", default="", help="extra compiler flags, e.g. -DSOFT_OBJECTIVE=0")
    ap.add_argument("--env", default="", help="extra env assignments, k=v;k=v")
    a = ap.parse_args()

    env_extra = {}
    for kv in filter(None, a.env.split(";")):
        k, v = kv.split("=", 1)
        env_extra[k] = v

    tmp = build(a.src, a.cflags.split())
    rows = []
    for r in range(a.runs):
        for s in a.seeds:
            env = dict(os.environ, SGA_SEED=str(s), LD_PRELOAD=SO)
            env.update(env_extra)
            t0 = time.perf_counter()
            p = subprocess.run(["./sga-time"], cwd=tmp, input=f"{a.pop}\n",
                               capture_output=True, text=True, env=env, timeout=900)
            wall = (time.perf_counter() - t0) * 1000.0
            tf = gens = None
            for line in p.stderr.splitlines():
                if line.startswith("TIME;"):
                    _, tf, gens = line.split(";")
                    tf, gens = float(tf), int(gens)
            if tf is None:
                sys.exit("no TIME line:\n" + p.stdout[-1000:] + p.stderr[-1000:])
            tf = tf / 1e6
            rows.append((s, wall, tf, gens))
            print(f"  seed={s} wall={wall:7.0f}ms fitness_total={tf:7.0f}ms "
                  f"gens={gens} rest={wall - tf:6.0f}ms "
                  f"({100 * tf / wall:4.1f}% in calc_fitness)", flush=True)

    print("  median per generation: "
          f"wall={statistics.median(r[1] / r[3] for r in rows):.3f}ms "
          f"fitness={statistics.median(r[2] / r[3] for r in rows):.3f}ms "
          f"rest={statistics.median((r[1] - r[2]) / r[3] for r in rows):.3f}ms "
          f"fitness_share={100 * statistics.median(r[2] / r[1] for r in rows):.1f}%")


if __name__ == "__main__":
    main()
