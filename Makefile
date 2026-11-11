# Minimal build for scheduleGA (2015 coursework, C99).
CC      ?= cc
CFLAGS  ?= -O2 -std=c99 -Wall
SRC      = main.c fitness.c parsing.c initialize.c functions.c crossover.c mutation.c selection.c export.c
HDR      = header/structs.h header/functions.h

sga: $(SRC) $(HDR)
	mkdir -p result
	$(CC) $(CFLAGS) -o $@ $(SRC)

run: sga
	printf '50\n' | ./sga

clean:
	rm -f sga

.PHONY: clean
