# scheduleGA

A genetic algorithm that builds a school timetable: courses are assigned to
(day, hour, room) slots so that no room, class or professor is double-booked.

Written in C in 2015 as university coursework, left untouched since. `master` still
holds that original code; this branch is a 2026 revival — the algorithm now runs on a
realistic grid, the missing input data has been reconstructed, and the whole thing is
measured rather than assumed.

## Read this first: what the original actually did

The 2015 code hardcoded **195 days × 4 hours = 780 slots**. With 267 lessons and 12
rooms that is ~3 % occupancy, so a fitness function that counts conflict-free lessons
reaches "267/267" easily and stops. It looked like a working timetabler.

On a **6-day × 4-hour** grid the same code cannot produce a valid timetable — and on the
original dataset it *cannot be done at all*: one professor teaches 45 lessons into 24
slots, and the required room type needs 154 lessons in 144 slots. The pigeonhole lower
bound is 12 days, and a CP-SAT model proves infeasibility in 4.8 s. The GA was never
solving timetabling; it was solving a toy.

The honest summary of the revival: **the genetic operators are not what makes this
work.** A repair operator (memetic local search) is. Constraint-aware initialisation,
which sounds obviously right, measured as pure overhead at high occupancy.

## Quick start

```bash
make both                 # ./sga (soft objective) and ./sga-legacy (2015 objective)
printf '50\n' | ./sga     # population size on stdin; writes result/result.csv

make bench                # pinned-seed benchmark, both objectives (5 seeds)
make test-hard-gate       # proves a clash always outweighs any soft-score gain
make equivalence-legacy   # legacy objective reproduces the original output bit-for-bit
```

Grid is configurable, defaulting to the original 195×4:

```bash
SGA_DAYS=12 printf '50\n' | ./sga          # 12 days × 4 hours
SGA_DAYS=6 SGA_REFINE=0 printf '50\n' | ./sga
SGA_DATA_DIR=data-d5 ./sga-repair          # tight 5-day instance, with repair
```

## What changed, with numbers

| pass | change | measured effect |
| --- | --- | --- |
| 1 | per-`(day,hour)` bucket index in `calc_fitness` instead of all 35,511 pairs | fitness 2.55 → 1.09 ms/call (2.34×), byte-identical output |
| 2 | `struct ENTRY` stores indices instead of embedding `COURSE`/`ROOM` (280 → 16 bytes) | genome 74,760 → 4,272 B, 1.38× end-to-end, identical output |
| 2 | full optimum scan, real elitism, generation/stagnation bounds | convergence neutral; fixed the fact that the old "elite" was never the best individual |
| 3 | hard feasibility gate + soft objective (room type, class day-spread, gaps, professor spread) | classes attend 4.7 days instead of 8.8 (−46 %), cost 2.55× wall |
| 4 | precomputed class-overlap matrix for the clash test | 4.0× on the default workload, 30–39× on tight grids, identical output |
| 4 | repair operator on the elite | 89 % occupancy: unsolvable → 267/267 in 13 generations (0.38 s) |

Feasibility frontier on the committed dataset (5 seeds): 195/30/20/12 days solve, 8/6/5
do not — and that boundary is exactly where the instance becomes mathematically
infeasible, so search quality cannot be measured on it. The `data-d5/d6/d7` directories
reuse the same 267 lessons with enough teachers and rooms to be satisfiable; they are
where the search is actually measurable.

## The input data was lost

The program reads `data/{professors,subjects,rooms,teams,courses}.csv`, none of which
were ever committed. The schemas were recovered from the parsers — including the trap
that `fgets` uses `char tmp[sizeof(STRUCT)]`, so each line had to fit inside the struct,
and `courses()` sized its buffer with `sizeof(PROFESSOR)`, capping course lines at 14
bytes. `data/` now holds a deterministic Danish STX dataset (20 professors, 18 subjects,
12 rooms, 30 classes, 78 courses → 267 lesson-hours); the schema of every file is
documented in [docs/report.md](docs/report.md).

## Documentation

`docs/report.md` is the full engineering log: fitness model, the recovered data schemas,
every benchmark with its reproduction command, the eleven bugs found in the 2015 code
(with `file:line`), the feasibility analysis, and — where relevant — what did not work.
It is long on purpose; the tables are meant to be checkable, not believed.

## Status

Experimental branch, not merged. `master` remains the original 2015 code. Nothing here is
production software: it is a well-measured answer to "could this old thing be made to
work, and what would it take?".
