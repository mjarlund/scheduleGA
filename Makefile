# Minimal build for scheduleGA (2015 coursework, C99).
#   make          build ./sga and create result/
#   make run      build and run with the default population size
#   make bench    build and run the pinned-seed benchmark (tools/bench.py)
#   make equivalence  compare this tree's fitness with the parent branch
CC      ?= cc
CFLAGS  ?= -O2 -std=c99 -Wall -D_POSIX_C_SOURCE=200809L
SRC      = main.c fitness.c parsing.c initialize.c functions.c crossover.c mutation.c selection.c export.c
HDR      = header/structs.h header/functions.h
BENCH_SEEDS = 1 2 3 4 5

all: sga

sga: $(SRC) $(HDR) | result
	$(CC) $(CFLAGS) -o $@ $(SRC)

# The program writes result/result.csv; make sure the directory exists.
result:
	mkdir -p result

run: sga
	printf '50\n' | ./sga

# Reproducible benchmark: pinned seeds via the time() shim, same population.
bench: sga tools/fixed_seed.so
	python3 tools/bench.py --label bench --pop 50 --seeds $(BENCH_SEEDS)

tools/fixed_seed.so: tools/fixed_seed.c
	$(CC) -shared -fPIC -O2 -o $@ $<

# Cross-build check of the index-based ENTRY against the parent branch.
equivalence: sga
	./tools/run_equivalence.sh

clean:
	rm -f sga tools/fixed_seed.so
	rm -f result/result.csv

.PHONY: all run bench equivalence clean
