#!/bin/sh
# Prove that `SOFT_OBJECTIVE=0 TYPED_INIT=0` is the parent program, bit for bit.
#
# Builds ./sga-legacy from this tree and runs it beside a worktree of the parent
# branch (default: fix/generation-loop) with the same pinned seeds, then compares
# generation counts and the md5 of result/result.csv.  Anything that differs is
# a real behavioural change in the "legacy" configuration, which would make the
# before/after numbers in the report dishonest.
#
#   ./tools/check_legacy_equivalence.sh [parent-worktree] [seeds...]
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(dirname "$HERE")
PARENT=${1:-/home/hermes/work/experiments/wt/generation-loop}
if [ $# -gt 1 ]; then shift; SEEDS=$*; else SEEDS="1 2 3 4 5"; fi
SO="$HERE/fixed_seed.so"

cd "$ROOT"
make -s sga-legacy >/dev/null
cc -shared -fPIC -O2 -o "$SO" "$HERE/fixed_seed.c"

if [ ! -x "$PARENT/sga" ]; then
    echo "parent build missing: $PARENT/sga (make in that worktree first)" >&2
    exit 2
fi

fail=0
printf '%-5s %-10s %-10s %-34s %-34s %s\n' seed "gens-L" "gens-P" "md5 legacy" "md5 parent" verdict
for s in $SEEDS; do
    ( cd "$ROOT" && printf '50\n' | SGA_SEED=$s LD_PRELOAD="$SO" ./sga-legacy > /tmp/eq-legacy-$s.txt )
    ( cd "$PARENT" && printf '50\n' | SGA_SEED=$s LD_PRELOAD="$PARENT/tools/fixed_seed.so" ./sga > /tmp/eq-parent-$s.txt )
    gl=$(sed -n 's/.*gens=\([0-9]*\).*/\1/p' /tmp/eq-legacy-$s.txt | head -1)
    gp=$(sed -n 's/.*gens=\([0-9]*\).*/\1/p' /tmp/eq-parent-$s.txt | head -1)
    ml=$(md5sum "$ROOT/result/result.csv" | cut -d' ' -f1)
    mp=$(md5sum "$PARENT/result/result.csv" | cut -d' ' -f1)
    if [ "$ml" = "$mp" ] && [ "$gl" = "$gp" ]; then v=ok; else v=DIFF; fail=1; fi
    printf '%-5s %-10s %-10s %-34s %-34s %s\n' "$s" "$gl" "$gp" "$ml" "$mp" "$v"
done
[ $fail -eq 0 ] && echo "PASS: SOFT_OBJECTIVE=0 TYPED_INIT=0 reproduces the parent exactly" \
               || { echo "FAIL: legacy configuration diverges from the parent" >&2; exit 1; }
