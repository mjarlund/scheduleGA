#!/bin/sh
# Reproduce every number in the report's "Third pass" section.
#
#   ./tools/run_third_pass.sh            # 5 seeds, ~2 minutes
#   SEEDS="1 2 3" ./tools/run_third_pass.sh
#
# Writes tools/quality-<label>.json for each configuration and prints the tables.
# Every configuration is a build of THIS tree; nothing is reconstructed from
# memory, and the legacy configuration is checked against the parent branch
# separately by ./tools/check_legacy_equivalence.sh.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(dirname "$HERE")
SEEDS=${SEEDS:-1 2 3 4 5}
cd "$ROOT"

CFLAGS="-O2 -std=c99 -Wall -D_POSIX_C_SOURCE=200809L"
SRC="main.c fitness.c parsing.c initialize.c functions.c crossover.c mutation.c selection.c export.c"

echo "== build =="
make -s sga sga-legacy
cc -shared -fPIC -O2 -o tools/fixed_seed.so tools/fixed_seed.c
# ablation builds: typed initialisation only, and the soft objective only
cc $CFLAGS -DSOFT_OBJECTIVE=0 -DTYPED_INIT=1 -o /tmp/sga-typedonly $SRC
cc $CFLAGS -DSOFT_OBJECTIVE=1 -DTYPED_INIT=0 -o /tmp/sga-softonly  $SRC

echo
echo "== A. the old program, and the new one with the default budget =="
python3 tools/bench_quality.py --label legacy --seeds $SEEDS --binary ./sga-legacy
python3 tools/bench_quality.py --label soft   --seeds $SEEDS --binary ./sga

echo
echo "== B. ablation: which change buys what =="
python3 tools/bench_quality.py --label typedonly --seeds $SEEDS --binary /tmp/sga-typedonly
python3 tools/bench_quality.py --label softonly  --seeds $SEEDS --binary /tmp/sga-softonly

echo
echo "== C. refine budget sweep (soft objective) =="
for r in 0 300 1000 3000; do
    python3 tools/bench_quality.py --label "ref$r" --seeds $SEEDS --binary ./sga --env "SGA_REFINE=$r"
done

echo
echo "== D. interleaved wall clock, 5 alternating rounds =="
python3 tools/bench_ab.py --a ./sga-legacy --b ./sga --pop 50 --seeds $SEEDS --rounds 5

echo
echo "== E. where the time goes (per generation, instrumented copies) =="
python3 tools/time_parts.py --pop 50 --seeds 1 3 --runs 3
python3 tools/time_parts.py --pop 50 --seeds 1 3 --runs 3 --cflags "-DSOFT_OBJECTIVE=0 -DTYPED_INIT=0"

echo
echo "== F. the hard-gate proof =="
make -s test-hard-gate
