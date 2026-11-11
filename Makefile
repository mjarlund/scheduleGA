# Minimal build for scheduleGA (2015 coursework, C99).
#   make              build ./sga with the soft objective (third pass)
#   make legacy       build ./sga-legacy: SOFT_OBJECTIVE=0 TYPED_INIT=0, i.e.
#                     the exact pre-third-pass program, for honest A/B
#   make both         both binaries
#   make run          build and run with the default population size
#   make bench        pinned-seed quality benchmark of both objectives
#   make bench-time   interleaved wall-clock A/B of both binaries
#   make equivalence  compare this tree's fitness with the parent branch
CC      ?= cc
CFLAGS  ?= -O2 -std=c99 -Wall -D_POSIX_C_SOURCE=200809L
SRC      = main.c fitness.c parsing.c initialize.c functions.c crossover.c mutation.c selection.c export.c search.c
HDR      = header/structs.h header/functions.h header/objective.h
BENCH_SEEDS = 1 2 3 4 5
LEGACY_FLAGS = -DSOFT_OBJECTIVE=0 -DTYPED_INIT=0

all: sga

sga: $(SRC) $(HDR) | result
	$(CC) $(CFLAGS) -o $@ $(SRC)

# The old objective and the old operators in one build, so before/after is a
# genuine A/B instead of a reconstruction.
sga-legacy: $(SRC) $(HDR) | result
	$(CC) $(CFLAGS) $(LEGACY_FLAGS) -o $@ $(SRC)

# Fourth pass: the same GA with constraint-directed initialisation and a
# min-conflicts repair operator on the elite.  Used for the tight-grid
# experiments; `make` is unaffected.
sga-tight: $(SRC) $(HDR) | result
	$(CC) $(CFLAGS) -DINIT_MODE=1 -DREPAIR=1 -o $@ $(SRC)

# The two halves, for the ablation in the fourth pass.
sga-greedy: $(SRC) $(HDR) | result
	$(CC) $(CFLAGS) -DINIT_MODE=1 -DREPAIR=0 -o $@ $(SRC)

sga-repair: $(SRC) $(HDR) | result
	$(CC) $(CFLAGS) -DINIT_MODE=0 -DREPAIR=1 -o $@ $(SRC)

both: sga sga-legacy

# The program writes result/result.csv; make sure the directory exists.
result:
	mkdir -p result

run: sga
	printf '50\n' | ./sga

# Reproducible quality benchmark: pinned seeds via the time() shim, both
# objectives, medians and ranges.
bench: sga sga-legacy tools/fixed_seed.so
	python3 tools/bench_quality.py --label soft   --seeds $(BENCH_SEEDS) --binary ./sga
	python3 tools/bench_quality.py --label legacy --seeds $(BENCH_SEEDS) --binary ./sga-legacy

# Interleaved wall-clock A/B (medians of alternating rounds).
bench-time: sga sga-legacy tools/fixed_seed.so
	python3 tools/bench_ab.py --a ./sga-legacy --b ./sga --pop 50 --seeds $(BENCH_SEEDS) --rounds 5

tools/fixed_seed.so: tools/fixed_seed.c
	$(CC) -shared -fPIC -O2 -o $@ $<

# Cross-build check of the index-based ENTRY against the parent branch.
equivalence: sga
	./tools/run_equivalence.sh

# SOFT_OBJECTIVE=0 + TYPED_INIT=0 against the parent branch: same generations,
# same result.csv, byte for byte.
equivalence-legacy: sga-legacy
	./tools/check_legacy_equivalence.sh

# Property test: the packed fitness must order schedules exactly like
# (violations, soft), and one hard violation must outrank every soft gain.
test-hard-gate: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o tools/test-hard-gate tools/test_hard_gate.c fitness.c parsing.c \
	    initialize.c functions.c
	./tools/test-hard-gate

clean:
	rm -f sga sga-legacy sga-tight sga-greedy sga-repair tools/fixed_seed.so tools/test-hard-gate
	rm -f result/result.csv

.PHONY: all run bench bench-time equivalence equivalence-legacy test-hard-gate clean both tight
