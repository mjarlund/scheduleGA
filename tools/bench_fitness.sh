#!/bin/sh
# Kernel-level A/B of calc_fitness and copy_schedule, alternating rounds.
#   ./tools/bench_fitness.sh [rounds]
# Builds tools/bench_fitness.c against the parent branch (embedded ENTRY) and
# against this working tree (index ENTRY), then prints median ms per generation
# for each part.  Both binaries stay resident in page cache, so the numbers are
# far more stable than whole-process timings.
set -e
cd "$(dirname "$0")/.."
BASE=${BASE:-improvement/bucket-fitness}
ROUNDS=${1:-5}
SRC="fitness.c parsing.c initialize.c functions.c crossover.c mutation.c selection.c export.c"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/old"

git archive "$BASE" | tar -x -C "$TMP/old"
cp tools/bench_fitness.c "$TMP/old/tools/"

(cd "$TMP/old" && cc -O2 -std=c99 -D_POSIX_C_SOURCE=199309L -I. -o bench-fitness tools/bench_fitness.c $SRC)
cc -O2 -std=c99 -D_POSIX_C_SOURCE=199309L -I. -DENTRY_INDICES -o "$TMP/bench-fitness-new" tools/bench_fitness.c $SRC

: > "$TMP/old.log"
: > "$TMP/new.log"
r=0
while [ "$r" -lt "$ROUNDS" ]; do
    (cd "$TMP/old" && ./bench-fitness) >> "$TMP/old.log"
    "$TMP/bench-fitness-new" >> "$TMP/new.log"
    "$TMP/bench-fitness-new" >> "$TMP/new.log"
    (cd "$TMP/old" && ./bench-fitness) >> "$TMP/old.log"
    r=$((r + 1))
done

python3 - "$TMP/old.log" "$TMP/new.log" <<'EOF'
import statistics, sys
def load(p):
    rows = []
    for line in open(p):
        d = dict(kv.split("=") for kv in line.split())
        rows.append({k: float(v) for k, v in d.items()})
    return rows
a, b = load(sys.argv[1]), load(sys.argv[2])
print(f"{len(a)} runs each, median")
for key in ("fitness_ms_per_gen", "copy_ms_per_gen", "rest_ms_per_gen",
            "rng_ms_per_gen", "fitness_us_per_schedule"):
    ma = statistics.median(x[key] for x in a)
    mb = statistics.median(x[key] for x in b)
    print(f"  {key:26} parent={ma:8.3f}  index={mb:8.3f}  ratio={ma/mb:5.2f}x")
EOF
