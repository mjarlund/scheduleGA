#!/bin/sh
# Compare the original calc_fitness (git master) with the current working tree's
# implementation on identical randomized populations.
set -e
cd "$(dirname "$0")/.."
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

git show master:fitness.c > "$TMP/old_fitness.c"
cc -O2 -std=c99 -I. -Dcalc_fitness=calc_fitness_old -DMAX_FITNESS=MAX_FITNESS_OLD \
   -c "$TMP/old_fitness.c" -o "$TMP/old_fitness.o"
cc -O2 -std=c99 -I. -o "$TMP/eq" tools/equivalence_test.c fitness.c parsing.c \
   initialize.c functions.c "$TMP/old_fitness.o"
"$TMP/eq"
