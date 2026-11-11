#!/bin/sh
# Equivalence of the index-based ENTRY (this branch) against the parent branch's
# embedded-COURSE/ROOM ENTRY.
#
# The two layouts cannot be compiled into one binary, so both trees are built
# from their own sources and driven by the same harness (tools/dump_states.c)
# over identical seeded populations: uniform slots, everything in one slot,
# three heavy-collision slots, one slot + one room, and one slot + one room +
# one course.  Fitness and a digest of every resolved timetable row must match.
#
#   ./tools/run_equivalence.sh                 # parent = improvement/bucket-fitness
#   BASE=<branch> ./tools/run_equivalence.sh   # any other reference branch
set -e
cd "$(dirname "$0")/.."
BASE=${BASE:-improvement/bucket-fitness}
SRC="fitness.c parsing.c initialize.c functions.c crossover.c mutation.c selection.c export.c"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/old"

# Parent tree straight out of git (sources + data/), harness copied in.
git archive "$BASE" | tar -x -C "$TMP/old"
cp tools/dump_states.c "$TMP/old/tools/"

# Old: embedded COURSE/ROOM.  New: -DENTRY_INDICES.
(cd "$TMP/old" && cc -O2 -std=c99 -I. -o dump-old tools/dump_states.c $SRC)
cc -O2 -std=c99 -I. -DENTRY_INDICES -o "$TMP/dump-new" tools/dump_states.c $SRC

(cd "$TMP/old" && ./dump-old > "$TMP/old.txt")
"$TMP/dump-new" > "$TMP/new.txt"

echo "--- parent ($BASE) ---"
head -1 "$TMP/old.txt"
echo "--- index-based (working tree) ---"
head -1 "$TMP/new.txt"

grep '^S;' "$TMP/old.txt" > "$TMP/old.s"
grep '^S;' "$TMP/new.txt" > "$TMP/new.s"

if cmp -s "$TMP/old.s" "$TMP/new.s"; then
    echo "schedules compared: $(wc -l < "$TMP/new.s")"
    echo "md5 $(md5sum < "$TMP/old.s" | cut -d' ' -f1)  parent"
    echo "md5 $(md5sum < "$TMP/new.s" | cut -d' ' -f1)  index-based"
    echo "PASS"
    exit 0
fi

echo "schedules compared: $(wc -l < "$TMP/new.s")"
diff "$TMP/old.s" "$TMP/new.s" | head -20
echo "FAIL"
exit 1
