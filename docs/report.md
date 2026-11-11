# scheduleGA (2015) — build, data reconstruction, baseline and one measured improvement

Repo: `github.com/mjarlund/scheduleGA`, commit `1063c15` (2015-12-18), branch `master`.
Nothing was pushed; all work is local in `/home/hermes/work/experiments/scheduleGA`.

Branches in the local clone:

| branch | contents |
|---|---|
| `master` | untouched original (`1063c15`) |
| `build-and-data` | original sources + `Makefile` + `data/` + `tools/` (the branch to check out) |
| `improvement/bucket-fitness` | `build-and-data` + the improved `fitness.c` |

---

## 1. What the program does

A generational genetic algorithm that assigns a set of teaching *entries* to
`(day, hour, room)` triples.

* `main.c` hard-codes `NUM_DAYS = 195`, `NUM_HRS_DAY = 4` (so 780 slots), reads a
  population size from stdin, and loops generations with **no termination bound**
  other than `while (optimum.fitness < MAX_FITNESS)` (`main.c:83`).
* One *entry* is one hour of one course. `NUM_ENTRIES` is **not** the number of
  courses: `courses()` sums `subject.hours` over all courses (`parsing.c:204`),
  and `init_population()` emits that many entries per individual (`initialize.c:27-33`).
* Genome: for every entry, `day ∈ [1,195]`, `hour ∈ [1,4]`, `room` (chosen uniformly
  from the room table, **ignoring subject type** — `initialize.c:31`).
* Operators: uniform crossover with per-gene probability `1/NUM_ENTRIES`
  (`crossover.c:13`), mutation of `(day,hour,room)` with the same probability
  (`mutation.c:9`), tournament selection over a random window
  (`selection.c:9-20`), generational replacement of the whole population
  (`main.c:69-79`).

### Fitness model (`fitness.c`)

`MAX_FITNESS = NUM_ENTRIES`, so fitness is "how many entries are conflict-free".

For each entry `j` (`fitness.c:10`):

1. if `course.subject.type != room.type`, score `0` immediately (`fitness.c:38-40`);
2. otherwise score `1`, unless some **later** entry `k > j` sits in the same
   `(day,hour)` and shares the room id, the team id, the professor id, or **any
   student** (`fitness.c:15-35`).

Consequences worth knowing:

* A pair conflict is charged to the *earlier* entry only, so
  `fitness == NUM_ENTRIES` ⇔ **no** conflicting pair exists. Valid schedules are
  therefore recognisable, but fitness is not a count of violations: two schedules
  with the same number of clashes can score differently depending on entry order.
* `subject.type` must match `room.type` or the entry can *never* score; the model
  silently requires every subject type to own at least one room. Nothing validates
  that, and if it is violated the program never terminates.
* Only hard conflicts are modelled. There is no preference for compactness, room
  utilisation, gap-free days or professor travel, so a "perfect" 267/267 schedule
  can still be a bad timetable (mine uses 224 different slots out of 780).

---

## 2. Data schemas (reverse-engineered)

The parsers use `char tmp[sizeof(STRUCT)]` as the `fgets()` buffer, so **a line must
fit in `sizeof(struct)-1` bytes including the newline**, otherwise it is silently
split into two records. Struct sizes on x86-64 (verified with a `sizeof` probe):

| file | line format | struct | max line | limits implied by the code |
|---|---|---|---|---|
| `data/professors.csv` | `id;name` | `PROFESSOR`=16 | 14 B | name ≤ 9 chars (`char name[10]`, `%s` has no width) |
| `data/subjects.csv` | `id;type;hours;name` | `SUBJECT`=24 | 22 B | name ≤ 9; `type` is the room-type key; `hours` is the number of weekly entries |
| `data/rooms.csv` | `id;type` | `ROOM`=8 | 6 B | both ids tiny (≤ 3 digits in practice) |
| `data/teams.csv` | `id;name;sid,sid,...` | `TEAM`=220 | 218 B | name ≤ 9 (`strcpy`, no check); ≤ 50 students (`student[50]`) |
| `data/courses.csv` | `id;professor_id;subject_id;team_id` | parsed with **`sizeof(PROFESSOR)`=16** | **14 B** | 4 small integers only; ids must stay ≤ 3 digits |

Other hard limits: static destination arrays of 255 professors, 100 subjects,
150 rooms, 255 teams, 500 courses (`parsing.c:54,78,102,130,178`). Exceeding any of
them silently overflows.

`courses.csv` is joined to the other tables by id at parse time; unknown ids leave
the corresponding struct zero-filled (id 0), with no error.

### Synthetic dataset shipped in `data/`

Generable deterministically with `python3 tools/generate_data.py`
(seed `20151218`), a Danish gymnasium (STX) timetable:

* 20 professors (Hansen … Toft), each qualified in 2–4 subjects, ids 1–20
* 18 subjects, 4 room types (classroom / lab / gym hall / music-art), 3–5 hours each
* 12 rooms: 6 classrooms, 3 labs, 1 gym hall, 2 music rooms
* 30 classes (`1a`…`3j`), 26 students each, 780 student ids; 6 pairs of classes
  share 3 students ("valgfag" overlap) so the duplicate-student branch is exercised
* 78 courses (18 classes × 3 + 12 classes × 2) → **`NUM_ENTRIES` = 267**
* every subject type has at least one matching room type (required, see above)

---

## 3. Build

```bash
sudo apt-get install -y gcc make      # gcc 14.2.0, GNU Make 4.4.1 on Debian 13
cd /home/hermes/work/experiments/scheduleGA
make                                  # -> ./sga
```

**No source change and no flag workaround was needed.** The only build artefact
added is a small `Makefile` (`-O2 -std=c99 -Wall`); C99 is required because of the
`//` comments, the declarations after statements and the VLA
`SCHEDULE population[max_pop]` (`main.c:23`). Two pre-existing warnings remain
(`parsing.c:12` unused `errnum`, `parsing.c:120` unused `d`). `make` also creates
`result/`, which the program does not create itself.

Run: `printf '50\n' | ./sga` (population size on stdin) → `result/result.csv`
(`day;hour;professor;subject;team;room`).

`master` is left at the original `1063c15`; `build-and-data` is the branch that
adds only the Makefile, the dataset and the harness. A fresh `git clone` of this
local repo checks out whichever branch the source worktree is on, so use
`git clone -b build-and-data` (or `-b improvement/bucket-fitness`) to be explicit.

---

## 4. Baseline (pop = 50, 5 seeds, 2015 code unmodified)

Seeds are pinned with `tools/fixed_seed.so` (`LD_PRELOAD` shim that overrides
`time()`, so `srand(time(NULL))` is reproducible; **the program itself is not
modified for measurement**).

| seed | wall clock | generations | best fitness ever | best at gen 1 |
|---|---|---|---|---|
| 1 | 1.41 s | 526 | 267 | 98 |
| 2 | 1.09 s | 414 | 267 | 88 |
| 3 | 1.86 s | 671 | 267 | 95 |
| 4 | 1.68 s | 587 | 267 | 105 |
| 5 | 3.29 s | 1157 | 267 | 113 |

median 1.63–1.68 s / 587 generations (~360 generations/s). All runs reach
`fitness == NUM_ENTRIES`.

Progression (seed 3, `tools/trace-baseline-pop50.csv`, best/worst/mean over the
100 individuals of the population, measured with an instrumented *copy* of the tree):

| gen | best | worst | mean |
|---|---|---|---|
| 1 | 113 | 77 | 94.8 |
| 10 | 130 | 126 | 128.4 |
| 50 | 180 | 177 | 179.0 |
| 100 | 216 | 212 | 214.8 |
| 200 | 250 | 245 | 249.0 |
| 300 | 260 | 255 | 258.1 |
| 400 | 264 | 258 | 263.4 |
| 500 | 266 | 260 | 264.9 |
| 600 | 266 | 261 | 265.4 |
| 671 | 267 | 263 | 266.2 |

Shape: 50 % of the final fitness in 12 generations, 90 % in 158, then a ~500
generation plateau chasing the last 1–3 entries. Generation 1 already sits at
~40 % fitness *by construction*: ~60 % of the initially chosen rooms have the wrong
type for their subject, so the first few generations are spent only fixing rooms.

Validity: the exported schedule was re-checked by an independent Python
re-implementation of `fitness.c` (`tools/validate_result.py`) —
`fitness = 267/267`, 0 room/team/professor/student clashes, 267 rows, every course
present exactly `subject.hours` times, 224 distinct `(day,hour)` slots.

---

## 5. The improvement: bucket the conflict check per `(day, hour)`

Profiling (gprof, `-O2 -std=c99 -pg`, seed 3) put **98 % of the time in
`calc_fitness`**. It compared every pair `(j,k), j<k` — 35,511 comparisons per
schedule (267·266/2) — although only entries in the same `(day,hour)` slot
can conflict, and 99 % of those comparisons fail on the first `day` test.

`fitness.c` (branch `improvement/bucket-fitness`) now indexes each schedule by slot
once: `head[slot]` is the lowest entry index in that slot and `next[j]` the next
higher index in the same slot. Walking `next[]` from `j` visits exactly
`{k : k > j, slot(k) = slot(j)}` in increasing order — the same comparison set, so
**the fitness definition is untouched**. Slots are stamped (`stamp[]` + a per-schedule
counter) instead of clearing all 980 slots, and entries whose `day/hour` fall outside
the grid are left unbucketed (memory-safety guard for corrupt input; the original
code could not go out of bounds there, so this was required to avoid introducing one).

### Equivalence evidence

`./tools/run_equivalence.sh` compiles the original `calc_fitness` from `master` and
the new one into one binary and runs both over identical populations — uniform,
all-entries-in-one-slot, and three-slot worst cases:

```
mode 0: 40 schedules, entries=267  mode 1: 20 schedules  mode 2: 40 schedules
100 schedules compared, 0 mismatches -> PASS
```

End-to-end: for every seed, old and new builds produce **identical generation counts
and byte-identical `result/result.csv`** (`tools/bench_ab.py`), and the full
per-generation best/worst/mean traces are byte-identical — the GA walks exactly the
same search path, only cheaper.

### Numbers (same dataset, same seeds, interleaved A/B, 5 seeds × 5 rounds = 25 runs each)

| metric | before | after | ratio |
|---|---|---|---|
| `calc_fitness` per call (gprof, same flags/seed) | 2.55 ms | 1.09 ms | **2.34×** |
| wall clock, median of 25 interleaved runs | 1.633 s | 1.061 s | **1.54×** |
| wall clock, fastest run | 1.055 s | 0.630 s | 1.67× |
| generations / final fitness / output | 414–1157 / 267 | identical | — |

The end-to-end gain is smaller than the kernel gain because fitness was ~80 % of the
run and the untouched remainder is substantial: `copy_schedule()` memcpy's the whole
74,760-byte genome **100 times per generation** (7.5 MB per generation, ~5 GB per
671-generation run), plus ~20,000 `rand()` calls per generation.

Measurement notes: this VM (4 vCPU, shared) drifts ±15 % between sessions, so all
comparisons above are interleaved A/B in one session; an A=A control run gives 1.01×.
An earlier version of the patch cleared all 980 slots per schedule and its runtime
swung 0.86–1.63 s between compile layouts of identical source; the stamping version
is stable at 1.06–1.16 s across four different flag sets, which is why it is the
shipped form.

---

## 6. Top 3 remaining weaknesses

1. **The population is copied by value, not by reference.** Every selection step and
   every child memcpy's 74,760 bytes (`functions.c:8-11`, called from
   `main.c:70-78`, `crossover.c:9-10`). 7.5 MB per generation dwarfs the search work
   that is left. Store `ENTRY *` per individual and permute pointers/indices; this is
   the single biggest remaining win and needs no algorithm change.
2. **The CSV layer is unguarded and silently lossy** — no `fopen` check (crash), no
   line-length check (records split in half), no id validation, no bound checks
   against the static arrays. Any real school dataset with a 4-digit id or a 10-char
   name corrupts the model instead of failing. Fix: fixed 512-byte line buffer,
   `sscanf` with field widths, check every join, validate that
   `subject.type` has a matching room type.
3. **Search quality and termination.** The best individual is not reliably preserved
   (see bugs), mutation/crossover rates are `1/NUM_ENTRIES`, and there is no
   generation or time cap, so the program either reaches `MAX_FITNESS` or runs
   forever. With `NUM_DAYS = 195` the search space is also unrealistically sparse,
   which is why random initialisation starts at 40 % fitness. A weekday domain
   (5 days), elitism, and a cap/time budget would change the problem into a real
   timetabling search.

---

## 7. Outright bugs found (with file:line)

| # | location | bug | evidence |
|---|---|---|---|
| 1 | `parsing.c:51,75,99,127,175` + `:36` | `fopen` result is never checked; `buffer_array(NULL)` calls `fgets(NULL,…)` → SIGSEGV, so the intended clean error in `validate_data()` (`parsing.c:25-28`) is unreachable. Removing `data/professors.csv` and running gives **exit 139**. | reproduced |
| 2 | `parsing.c:47,71,95,121,189` | `fgets(tmp, sizeof(STRUCT), …)` uses the *destination struct* as the line buffer size, so long lines are split into extra records. `courses()` is worst: `sizeof(PROFESSOR)`=16 → course lines are capped at 14 bytes. `sscanf` then leaves unmatched fields at their **stale previous values** (`parsing.c:170,190`), so a split line yields a wrong course and the record count no longer matches `NUM_COURSES`. | a single 16-byte `courses.csv` line makes the program never terminate (see #4) |
| 3 | `parsing.c:221` vs `initialize.c:25-33` | `NUM_ENTRIES` is computed while parsing, but entries are filled per `NUM_COURSES` records; if they disagree (see #2), the tail of the `malloc`'d entry array stays **uninitialised** and is then evaluated and exported. | same reproduction as #2 |
| 4 | `main.c:83` | No generation or time cap: `do { … } while (optimum.fitness < MAX_FITNESS)`. Unreachable `MAX_FITNESS` (infeasible subject/room types, or #3) means an infinite loop. | 16-byte line in `courses.csv` → still running after 20 s, no `result.csv` |
| 5 | `main.c:57-60` | The optimum scan stops at the **first** individual with `fitness == optimum.fitness` and breaks out, so a strictly better individual later in the array is missed for that generation. | code inspection |
| 6 | `main.c:64-79` | Elitism is only applied when `new_opt == 0`; when a new best is found, the population is still fully rewritten by selection+crossover, so the best genome survives only if tournament selection happens to pick it. No elitism at all on the `new_opt == -1` path. | code inspection |
| 7 | `export.c:10-13` | `fopen("result/result.csv","w")` is unchecked; with no `result/` directory the program runs the whole GA and then segfaults in `fprintf` (**exit 139**, reproduced). | reproduced |
| 8 | `main.c:17-24` | Population size from `scanf` is unchecked (`scanf` return ignored): `pop = 0` or a non-number is undefined, and `population[2*pop]` is a VLA on the stack → overflow for large input. `selection.c:12` divides by `popsize/2` → modulo by zero for `pop = 1`. | code inspection |
| 9 | `fitness.c:38-40` | An entry whose `subject.type` has no room of that type scores 0 forever, so `MAX_FITNESS` is unreachable; nothing validates feasibility. | code inspection |
| 10 | `fitness.c:27-34` (original) | The student-overlap `break` only leaves the inner loop, so the outer loop keeps scanning — wasted work, not a wrong answer. | code inspection |
| 11 | `initialize.c:31` | Rooms are drawn uniformly, ignoring `subject.type`, so ~60 % of the initial genome is infeasible by construction and the first generations only repair room types. | gen-1 fitness ≈ 95/267 |

---

## 8. Reproduce every number

```bash
cd /home/hermes/work/experiments/scheduleGA
git checkout build-and-data            # baseline + data + tools
make                                   # sga, creates result/
printf '50\n' | ./sga                  # end-to-end run, writes result/result.csv

python3 tools/generate_data.py         # regenerate data/ (deterministic seed 20151218)
python3 tools/validate_result.py       # independent fitness re-implementation -> 267/267
cc -shared -fPIC -O2 -o tools/fixed_seed.so tools/fixed_seed.c

# baseline timing, 5 seeds (seeds pinned via LD_PRELOAD)
python3 tools/bench.py --label baseline-pop50 --pop 50 --seeds 1 2 3 4 5
# per-generation best/worst/mean (instrumented copy of the tree, repo untouched)
python3 tools/trace.py --label baseline-pop50 --pop 50 --seed 3

# ---- improvement ----
git worktree add /tmp/sga-base build-and-data && make -C /tmp/sga-base
git checkout improvement/bucket-fitness && make
./tools/run_equivalence.sh             # 100 schedules, 0 mismatches
python3 tools/bench_ab.py --a /tmp/sga-base/sga --b ./sga --pop 50 --seeds 1 2 3 4 5 --rounds 5

# kernel cost per call (gprof, both variants, same flags and seed)
git show master:fitness.c > /tmp/f_old.c
gcc -O2 -std=c99 -pg -I. -o /tmp/sga-old main.c /tmp/f_old.c parsing.c initialize.c \
    functions.c crossover.c mutation.c selection.c export.c
gcc -O2 -std=c99 -pg -o /tmp/sga-new main.c fitness.c parsing.c initialize.c \
    functions.c crossover.c mutation.c selection.c export.c
# in a directory containing data/ and result/:
SGA_SEED=3 LD_PRELOAD=$PWD/tools/fixed_seed.so sh -c 'printf "50\n" | /tmp/sga-old && gprof -b /tmp/sga-old gmon.out'

# bug 1 (missing data file -> SIGSEGV instead of the intended perror/exit)
mkdir -p /tmp/d1 && cp -r data result /tmp/d1/ && rm /tmp/d1/data/professors.csv
(cd /tmp/d1 && printf '20\n' | /home/hermes/work/experiments/scheduleGA/sga; echo "exit=$?")   # 139

# bug 7 (no result/ directory -> runs the whole GA, then SIGSEGV in export)
mkdir -p /tmp/d2 && cp -r data /tmp/d2/
(cd /tmp/d2 && printf '20\n' | /home/hermes/work/experiments/scheduleGA/sga; echo "exit=$?")   # 139

# bug 2/3/4 (one 16-byte courses.csv line -> silent mis-parse -> never terminates)
mkdir -p /tmp/d3/result && cp -r data /tmp/d3/
sed -i '1s/.*/1000;101;102;103/' /tmp/d3/data/courses.csv        # 16 bytes
(cd /tmp/d3 && printf '20\n' | timeout 20 /home/hermes/work/experiments/scheduleGA/sga; echo "exit=$?")  # 124
```

---

# Second optimisation pass

Branches added on top of `improvement/bucket-fitness`:

| branch | contents |
|---|---|
| `perf/entry-indices` | `ENTRY` stores course/room **indices** instead of embedded structs; new harnesses |
| `fix/generation-loop` | `perf/entry-indices` + the three `main.c` loop fixes + data-I/O hardening + `make bench` |

`master` is still `1063c15`; nothing was pushed.

## 9. Workstream A — entry indices (`perf/entry-indices`)

### What changed and why

`struct ENTRY` embedded a whole `COURSE` (280 bytes: `PROFESSOR` + `SUBJECT` +
`TEAM.student[50]`) and a `ROOM`, so one genome was 267 × 280 = **74,760 bytes**
and `copy_schedule()` memcpy'd it 100 times per generation (7.5 MB/gen, ~4 GB per
526-generation run).

An entry now holds four `int`s (day, hour, **room index**, **course index**),
**16 bytes**, one genome = **4,272 bytes** (17.5× less; 427 KB per generation
copied instead of 7.5 MB).

This is information-preserving, not an approximation:

* `init_population()` gives the entries of course `i` the index `i`; neither
  crossover nor mutation ever changes an entry's course (crossover copies only
  `day`, `hour`, `room`), so the index carries exactly what the struct did.
* rooms were *already* selected as table indices (`room[rand_num(NUM_ROOMS,0)]`),
  so `room` stores that same index.
* `courses()` and `rooms()` publish their static arrays as `COURSE_TABLE` /
  `ROOM_TABLE`; `fitness.c` and `export.c` resolve the two indices and then
  compare exactly the same ids, student lists and types as before. The conflict
  definition, the comparison set, the `j < k` order and the scoring rule are
  untouched (the `(day,hour)` bucket index from the previous pass is unchanged).
* the number and order of `rand()` calls is identical, so a pinned seed follows
  the same search path.

`ENTRY_SET`-style accessors exist only in the test harnesses (`#ifdef
ENTRY_INDICES`), not in the program.

### Equivalence evidence

**(a) Kernel level, cross-build** — the two layouts cannot live in one binary, so
`tools/run_equivalence.sh` builds each tree from its own sources and drives both
with `tools/dump_states.c` over identical seeded populations in five placement
modes (uniform; every entry in one slot; three heavy-collision days; one slot +
one room; one slot + one room + one course — i.e. the deliberately degenerate
cases). For every schedule it compares the fitness **and** an FNV digest of every
*resolved* timetable row (`day;hour;professor;subject;team;room` + names):

```
--- parent (improvement/bucket-fitness) ---
SIZE;sizeof_ENTRY=280;entries=267;courses=78;rooms=12
--- index-based (working tree) ---
SIZE;sizeof_ENTRY=16;entries=267;courses=78;rooms=12
schedules compared: 180
md5 5a68907f703180e984f8122ca65e5baf  parent
md5 5a68907f703180e984f8122ca65e5baf  index-based
PASS
```

**(b) End to end, 5 pinned seeds** — identical generation count, identical
`result/result.csv`:

| seed | generations | md5 parent | md5 index-based |
|---|---|---|---|
| 1 | 526 | `fed4d509d1c8d241f0d7ab01c219b855` | same |
| 2 | 414 | `2ea1dec40268fa136fe33a758ae3bdee` | same |
| 3 | 671 | `eb760eb883434e0e0025712377d173ee` | same |
| 4 | 587 | `5753c1463b1fccdda18497d37319ed9a` | same |
| 5 | 1157 | `0007d60ac494377176cc31f9d0c756ba` | same |

**(c) Per-generation best/worst/mean traces, 5 seeds** — `tools/trace.py` run on
both trees, then `cmp`: byte-identical, 527 / 415 / 672 / 588 / 1158 lines
(generation 1 … last). So the population distributions match at every generation,
not just at the end.

**(d) Deliberately degenerate end to end** — both trees patched in a scratch copy
(not in the repo) to `NUM_DAYS = 1` and a 300-generation stop, seed 1: 267 entries
crammed into four slots (60/76/73/58 entries per slot), so `MAX_FITNESS` is
unreachable and the run is pure conflict-saturated scoring. Both produce the same
20 improvements, the same final best (29/267) and

```
1a18bc28b6b4ff42c33b64a4e73c82f0  P/result/result.csv
1a18bc28b6b4ff42c33b64a4e73c82f0  A/result/result.csv
```

### Benchmark (interleaved A/B, shipped `-O2` binaries)

`tools/bench_ab.py`, pop 50, seeds 1–5, 5 alternating rounds (25 runs per variant,
order flipped each round, binaries swapped between rounds), cwd = repo root, seed
pinned with `tools/fixed_seed.so`:

| seed | A: bucket only (ms) | B: + entry indices (ms) | speedup | generations |
|---|---|---|---|---|
| 1 | 908 | 633 | **1.43×** | 526 |
| 2 | 635 | 452 | **1.41×** | 414 |
| 3 | 1155 | 842 | **1.37×** | 671 |
| 4 | 1019 | 741 | **1.38×** | 587 |
| 5 | 2063 | 1504 | **1.37×** | 1157 |
| median (25 runs each) | **1019** | **741** | **1.38×** | — |

Control: A vs A in the same harness gives 1.00× / 0.99× / 1.00× per seed.
A repeat of the whole experiment later in the session (3 rounds) reproduced
1.37–1.40× with the same medians within 4 % (876/631, 633/454, 1161/840,
1016/743, 2086/1510).

### Where the remaining time goes

Three independent measurements, taken in one session.

**In-situ split** (`tools/time_parts.py`: an instrumented copy of the tree that
times `calc_fitness` with `CLOCK_MONOTONIC`, integer-ns accumulator), median of 9
runs over seeds 1/3/5:

| per generation | parent (bucket) | index-based |
|---|---|---|
| wall | 1.558 ms | 1.287 ms |
| inside `calc_fitness` | 1.006 ms (64.6 %) | 0.969 ms (75.3 %) |
| everything else | 0.553 ms | 0.310 ms |

**Kernel bench** (`tools/bench_fitness.sh`, 10 alternating runs each, per
generation of a 100-individual population):

| part | parent | index | ratio |
|---|---|---|---|
| `calc_fitness` (whole population) | 0.542 ms | 0.530 ms | 1.02× |
| the 100 `copy_schedule` memcpys | 0.264 ms | 0.009 ms | **29×** |
| reproduction loop incl. those copies | 0.547 ms | 0.304 ms | 1.80× |
| ~20,000 `rand()` calls (standalone) | 0.297 ms | 0.298 ms | 1.00× |

**Scoring probe** (`calc_fitness` with the scoring pass replaced by one trivial
linear read of the entries, same tree, same harness): 0.068 ms/gen vs 0.533 ms/gen
full — i.e. **87 % of `calc_fitness` is the scoring pass**, not the bucket build
and not the linear scan.

Put together, for the shipped binary at seed 1 (633 ms / 526 generations =
1.20 ms/gen):

* ~0.97 ms (75 %) `calc_fitness`, of which ~0.87 × 0.97 ≈ **0.85 ms is scoring**;
* ~0.30 ms (23 %) the RNG: crossover draws 6,675 `rand()`s per generation and
  mutation 13,350, ~20,000 per generation in total;
* ~0.01 ms the 100 memcpys (was 0.26 ms);
* the rest is process start-up, CSV parsing, export and the loop overhead.

`gprof` (`-pg`, seed 1) attributes 95.8 % of samples to `calc_fitness` and 1.1 %
each to `randZeroToOne` and `mutation`, which agrees in shape; but a `-pg` build
of either tree runs 1.3–1.9× slower than its `-O2` build on this box, so `gprof`
is used here for attribution only, never for timing.

## 10. Workstream B — generation loop (`fix/generation-loop`)

### The three defects, and what the fix does

1. **The optimum scan stopped at the first individual that was `>=` the
   optimum.** Evidence that this is not cosmetic: for seed 1, generation 1, the
   population's real maximum is **120**, but population[0] is 98, so the elite the
   old code adopted (and printed as "Best") was 98. It is the same in all 20 seeds
   measured — the old code's first "Best:" is 78–113, the true maximum is 109–122.
   The scan now finds the true maximum of the population.
2. **Elitism ran only when `new_opt == 0`** (no individual had reached the
   optimum — rare once the population converges) **and it injected the optimum
   before reproduction**, which rewrites all 2·pop slots. So the elite was only
   *guaranteed* to survive if it was re-selected; it did still bias that
   generation's tournaments. It is now written back to slot 0 **after**
   reproduction, on every path.
3. **No termination bound.** `while (optimum.fitness < MAX_FITNESS)` runs forever
   when `MAX_FITNESS` is unreachable. The loop now stops on the unchanged success
   criterion (`fitness >= NUM_ENTRIES`) or after `SGA_MAX_GENERATIONS`
   (default 20000) / `SGA_STAGNATION` generations without improvement (default
   3000), prints `Stopped: <reason> …`, and always exports the best schedule
   found. A `Result: gens=… best=… worst=… mean=… converged=… max=…` line gives
   `tools/bench.py` the final population statistics.

Nothing else in the GA changed (same operators, same rates, same population
sizes, same success criterion).

### Quality and time, 20 pinned seeds (A = no fixes, B = fixes)

| seed | A gens | A wall s | B gens | B wall s | Δ gens | B final worst | B final mean |
|---|---|---|---|---|---|---|---|
| 1 | 526 | 0.63 | 728 | 1.09 | +202 | 259 | 265.19 |
| 2 | 414 | 0.45 | 412 | 0.46 | −2 | 262 | 265.49 |
| 3 | 671 | 0.84 | 654 | 0.89 | −17 | 263 | 265.48 |
| 4 | 587 | 0.74 | 586 | 0.75 | −1 | 261 | 265.31 |
| 5 | 1157 | 1.53 | 657 | 0.81 | −500 | 262 | 265.42 |
| 6 | 570 | 0.69 | 558 | 0.73 | −12 | 262 | 265.54 |
| 7 | 523 | 0.74 | 521 | 0.65 | −2 | 262 | 265.51 |
| 8 | 572 | 0.84 | 570 | 0.71 | −2 | 262 | 265.29 |
| 9 | 566 | 0.72 | 566 | 0.72 | 0 | 261 | 265.22 |
| 10 | 962 | 1.24 | 754 | 1.00 | −208 | 261 | 265.38 |
| 11 | 815 | 0.87 | 957 | 1.09 | +142 | 263 | 265.64 |
| 12 | 774 | 0.96 | 773 | 0.98 | −1 | 261 | 265.02 |
| 13 | 780 | 1.13 | 427 | 0.56 | −353 | 258 | 264.90 |
| 14 | 630 | 0.88 | 629 | 0.92 | −1 | 263 | 265.44 |
| 15 | 665 | 0.87 | 664 | 0.91 | −1 | 262 | 265.61 |
| 16 | 568 | 0.70 | 668 | 0.89 | +100 | 262 | 265.35 |
| 17 | 781 | 1.02 | 780 | 1.01 | −1 | 262 | 265.31 |
| 18 | 855 | 1.04 | 854 | 1.05 | −1 | 262 | 265.55 |
| 19 | 583 | 0.64 | 957 | 1.25 | +374 | 261 | 265.04 |
| 20 | 678 | 0.89 | 562 | 0.75 | −116 | 263 | 265.58 |
| **median** | **647.5** | 0.853 | **655.5** | 0.885 | **+8 (+1.2 %)** | | |
| **mean** | **683.9** | | **663.9** | | **−20 (−2.9 %)** | | |

* Both converge **20/20** (final best 267/267) — on this feasible dataset the old
  code always converged too, so the fixes do not change *whether* it converges.
* 15 seeds improved, 1 unchanged, 4 got worse; the median is 8 generations
  (1.2 %) worse, the mean 20 generations (2.9 %) better. The gain is entirely in
  the tails (1157→657, 780→427, 962→754, 678→562) and is partly paid for by new
  tails (526→728, 583→957, 568→668).
* Verdict: **the fixes are neutral on this instance, not an improvement in
  search quality.** That is the honest result. They are worth having because the
  elite is now actually preserved (the old "Best" line was not the population
  maximum), and because an unreachable `MAX_FITNESS` no longer means an infinite
  loop.
* Final population diversity is essentially unchanged and slightly lower
  (final worst 258–263, mean 264.90–265.64, versus 262–264 / 265.70–266.45 for A
  on seeds 1–5) — what guaranteed elitism buys is exactly a little less
  diversity.
* Cost per generation: 771 gens/s (B) vs 788 gens/s (A), −2 % — the full 2·pop
  scan plus the extra `copy_schedule` per generation. Wall time per seed follows
  the generation count (single, non-interleaved runs in the table above): faster
  on 7 seeds, equal on 1, slower on 12, median 0.885 s vs 0.853 s.
* The stagnation bound (3000) never fires on this dataset; it is exercised with
  `SGA_STAGNATION=50` and `SGA_MAX_GENERATIONS=100` (see §11).

## 11. Workstream C — housekeeping and data I/O

| # | problem (before) | fix | evidence |
|---|---|---|---|
| 1 | `export()` segfaulted (139) when `result/` was missing (binary run from another directory, or after a `clean`); the Makefile's inline `mkdir -p result` only covered `make` itself and ran on every build | `result` is an order-only prerequisite (created once, not on each build) **and** `export()` creates the directory itself and checks `fopen` | run from a directory without `result/` → creates it, writes `result.csv`, exit 0 (was exit 139) |
| 2 | missing data file → `fgets(NULL)` → SIGSEGV (139), the intended `perror` unreachable | every parser checks `fopen` and exits 1 with the file name | `data/professors.csv: No such file or directory`, exit 1 |
| 3 | unparseable line → `sscanf` left stale values, silent corruption | `sscanf` return value checked, `file:line` in the message, exit 1 | `data/professors.csv:3: expected "id;name"`, exit 1 |
| 4 | `char tmp[sizeof(STRUCT)]` split long lines into two records (`courses.csv` capped at **14 characters**), which made the program hang | one 512-byte line reader shared by all five parsers; an over-long line is a fatal error | a valid 19-byte line `0001;0009;0008;0001` → identical schedule (md5 `02720f64ac2b33950543bc09ea1f1905`, 586 generations, same as unpadded data); the parent binary on the same file **times out (exit 124)** |
| 5 | names copied with `%s`/`strcpy` into `char[10]` (overflow at 10 chars); >50 students overflowed `student[50]` | widths + explicit length checks, exit 1 with the line number | code + `fail()` paths |
| 6 | unknown id in `courses.csv` left the struct zero-filled and changed the fitness model silently | course→professor/subject/team joins are fatal when unresolved | `data/courses.csv:2: professor_id does not exist in professors.csv`, exit 1 |
| 7 | no capacity check against the static arrays (255/100/150/255/500) | `check_count()` before parsing | code |
| 8 | a subject type with no room ⇒ `MAX_FITNESS` unreachable ⇒ infinite loop | `validate_data()` rejects the dataset at start-up | message names the subject and type |
| 9 | `scanf("%d", &pop)` unchecked (`pop = 0/1` → UB, modulo by zero in `selection.c`) | validated, `2 … 100000` | `printf '0\n' \| ./sga` → clear error, exit 1 |
| 10 | `make bench` did not exist | runs `tools/bench.py --pop 50 --seeds 1 2 3 4 5` with the pinned-seed shim, which it builds first | `make bench` → `bench: n=5 … converged=5/5` |

The termination bounds in §10 are demonstrated with:

```
SGA_STAGNATION=50 …  →  Result: gens=385 best=264 … converged=0
                        Stopped: no improvement within the stagnation limit after 385 generations, best 264/267
SGA_MAX_GENERATIONS=100 …  →  Result: gens=100 best=215 … converged=0
                        Stopped: generation cap reached after 100 generations, best 215/267
```

Note: an unconverged run still writes `result/result.csv` (the best schedule
found), which can contain clashes. That is deliberate "best effort", but nothing
marks the file as invalid — see §12.

## 12. Top 2 remaining hotspots / weaknesses

**1. `calc_fitness` is still 75 % of the run, and 87 % of it is the scoring pass.**
0.97 ms per generation for 100 schedules = 5.3 µs per schedule, 20 ns per entry.
The scoring probe (§9: same code with the scoring pass replaced by one linear read
of the entries) drops it to 0.068 ms per generation, so the bucket build and the
entry scan are cheap and the cost is the scoring itself: for each of the 267
entries, one dependent load into `COURSE_TABLE` (78 × 280 B = 21.8 KB) while
427 KB of entry data streams through a 32 KB L1, then a walk of the ~0.2 same-slot
successors, each requiring two more table loads. The next step that needs no
algorithm change is a compact per-course key table (type, professor id, team id,
`num_students`, student pointer ≈ 16–24 B, 1.5 KB for 78 courses) filled in
`courses()` and used by the scoring loop, so the indirection stays in L1; a second
option is to keep the bucket arrays allocated once instead of `malloc`/`free` per
call (3 allocations per generation — measurable but small). **Not implemented**;
the probe above is the evidence that the scoring pass is where to aim.

**2. ~23 % of the run is `rand()` — ~20,000 calls per generation.**
Measured standalone at 0.297 ms per generation (0.30 of the 1.29 ms budget), and
the reproduction loop minus the copies is 0.30 ms, so essentially the whole
non-fitness remainder is the RNG. Crossover draws one number per entry (6,675 per
generation) and mutation one per entry per child (13,350), all through two
function calls (`randZeroToOne`, `rand_num`) with a modulo. Drawing 32 genes per
`rand()` word, or a faster generator, would remove most of it — but it changes
every trajectory and breaks reproducibility against the existing pinned-seed
results, so it is a search change, not a speed-up.

Also worth recording, from the same runs:

* **`result/result.csv` is not validated.** A run stopped by a bound exports a
  schedule with clashes with no marker (reproduced: best 264/267 exported).
* **`MAX_FITNESS` is a weak definition of quality.** The 267/267 schedules are
  sparse (one seed-5 run used 234 of the 780 `(day,hour)` slots), and a clash is
  charged to the earlier entry only, so fitness is not a violation count. Nothing
  in the model prefers compact or gap-free timetables.
* `NUM_DAYS = 195` / `NUM_HRS_DAY = 4` are hard-coded in `main.c`; the slot grid
  is not part of the data.

## 13. Reproduce every number in this section

```bash
cd /home/hermes/work/experiments/scheduleGA
W=/home/hermes/work/experiments/wt

# ---- workstream A ----
git checkout perf/entry-indices && make
./tools/run_equivalence.sh                  # 180 schedules, md5 5a68907f…, PASS

# parent (bucket only) and index builds side by side
git worktree add --detach $W/parent improvement/bucket-fitness
git worktree add --detach $W/idx     perf/entry-indices
(cd $W/parent && make && cc -shared -fPIC -O2 -o tools/fixed_seed.so tools/fixed_seed.c)
(cd $W/idx    && make && cc -shared -fPIC -O2 -o tools/fixed_seed.so tools/fixed_seed.c)

# interleaved A/B, 5 seeds x 5 alternating rounds  (1.38x, identical md5s)
python3 tools/bench_ab.py --a $W/parent/sga --b $W/idx/sga --pop 50 \
        --seeds 1 2 3 4 5 --rounds 5
# control (A vs A) -> 1.00x
python3 tools/bench_ab.py --a $W/parent/sga --b $W/parent/sga --pop 50 \
        --seeds 1 2 3 --rounds 3
# flag sensitivity: 1.37x with -falign-functions=32, 1.21x with -O3
S="main.c fitness.c parsing.c initialize.c functions.c crossover.c mutation.c selection.c export.c"
(cd $W/parent && cc -O2 -std=c99 -falign-functions=32 -o /tmp/P32 $S)
(cd $W/idx    && cc -O2 -std=c99 -falign-functions=32 -o /tmp/A32 $S)
python3 tools/bench_ab.py --a /tmp/P32 --b /tmp/A32 --pop 50 --seeds 1 3 --rounds 3
(cd $W/parent && cc -O3 -std=c99 -o /tmp/P3 $S)
(cd $W/idx    && cc -O3 -std=c99 -o /tmp/A3 $S)
python3 tools/bench_ab.py --a /tmp/P3 --b /tmp/A3 --pop 50 --seeds 1 3 --rounds 3

# md5 table, 5 seeds, both trees
for d in $W/parent $W/idx; do (cd $d && for s in 1 2 3 4 5; do \
  SGA_SEED=$s LD_PRELOAD=./tools/fixed_seed.so ./sga <<< "50" >/dev/null; \
  echo "$s $(md5sum result/result.csv | cut -d' ' -f1)"; done); done

# per-generation traces, 5 seeds, identical
for s in 1 2 3 4 5; do
  (cd $W/idx && python3 tools/trace.py --label a$s --pop 50 --seed $s >/dev/null)
  (cd $W/parent && python3 tools/trace.py --label p$s --pop 50 --seed $s >/dev/null)
  cmp $W/idx/tools/trace-a$s.csv $W/parent/tools/trace-p$s.csv && echo "seed $s identical"
done

# kernel parts and in-situ split
./tools/bench_fitness.sh 5                   # fitness 0.542/0.530, copy 0.264/0.009
python3 tools/time_parts.py --pop 50 --seeds 1 3 5 --runs 3
cp tools/time_parts.py $W/parent/tools/      # the parent tree needs the tool too
(cd $W/parent && python3 tools/time_parts.py --pop 50 --seeds 1 3 5 --runs 3)

# degenerate end-to-end: 267 entries in 4 slots, 300 generations, seed 1
T=$(mktemp -d); cp -r $W/parent $T/P; cp -r $W/idx $T/A
for d in P A; do (cd $T/$d && sed -i 's/NUM_DAYS = 195;/NUM_DAYS = 1;/' main.c &&
  sed -i 's|} while(optimum.fitness < MAX_FITNESS);|} while(optimum.fitness < MAX_FITNESS \&\& generation <= 300);|' main.c &&
  make clean >/dev/null && make >/dev/null && \
  SGA_SEED=1 LD_PRELOAD=$PWD/tools/fixed_seed.so ./sga <<< "50" > out.txt); done
md5sum $T/P/result/result.csv $T/A/result/result.csv   # 1a18bc28… both

# ---- workstream B ----
git checkout fix/generation-loop && make
make bench                                   # one-command pinned-seed benchmark
python3 tools/bench.py --label fix-B --pop 50 --seeds 1 2 3 4 5 6 7 8 9 10
python3 tools/bench.py --label fix-B2 --pop 50 --seeds 11 12 13 14 15 16 17 18 19 20
(cd $W/idx && python3 tools/bench.py --label idx-A  --pop 50 --seeds 1 2 3 4 5 6 7 8 9 10)
(cd $W/idx && python3 tools/bench.py --label idx-A2 --pop 50 --seeds 11 12 13 14 15 16 17 18 19 20)
# 98 vs 120 at generation 1: the trace's gen 1 is the true maximum
(cd $W/parent && python3 tools/trace.py --label p1 --pop 50 --seed 1 | head -2)
python3 tools/trace.py --label b1 --pop 50 --seed 1 | head -2
# bounds
SGA_STAGNATION=50  SGA_SEED=1 LD_PRELOAD=./tools/fixed_seed.so sh -c "printf '50\n' | ./sga" | tail -2
SGA_MAX_GENERATIONS=100 SGA_SEED=1 LD_PRELOAD=./tools/fixed_seed.so sh -c "printf '50\n' | ./sga" | tail -2

# ---- workstream C ----
T=$(mktemp -d); cp -r data $T/; cd $T
rm data/professors.csv; printf '20\n' | ~/work/experiments/scheduleGA/sga; echo "exit=$?"   # 1
cp ~/work/experiments/scheduleGA/data/professors.csv data/
printf '20\n' | ~/work/experiments/scheduleGA/sga | tail -1                                   # creates result/
cp data/courses.csv /tmp/c.bak; sed -i '1s/.*/0001;0009;0008;0001/' data/courses.csv          # 19-byte line
SGA_SEED=4 LD_PRELOAD=~/work/experiments/scheduleGA/tools/fixed_seed.so \
  ~/work/experiments/scheduleGA/sga <<< "50" | tail -2; md5sum result/result.csv              # 586 gens, 02720f64…
```

## 14. Caveats and what was not verified

* **Binary layout is worth 10–15 % on this box.** Two builds of the *same*
  source, differing only by a local variable in `main.c`, ran 792 ms and 908 ms
  for seed 1; `-O3` gives a 1.21× gain for the index change where `-O2` gives
  1.38×. The headline 1.38× is the A/B of the two *shipped* `-O2` binaries (the
  ones `make` produces); the part of it directly attributable to the change is
  the measured 0.25 ms/gen of memcpy out of a 1.2–1.7 ms/gen budget, i.e. an
  expected 1.15–1.25×, and the rest is the parent binary landing unluckily. Both
  numbers are reported above; treat 1.2–1.4× as the range, not 1.38× as a
  constant.
* The VM is shared (4 vCPU) and drifts ±15 % between sessions, so every A/B here
  is interleaved in one session with a control run (A vs A = 1.00×). Absolute ms
  are not comparable across sessions; ratios are.
* `gprof` numbers are not used for timing: the `-pg` build is 1.3–1.9× slower
  than `-O2`, so only its sample *shares* are quoted.
* The in-situ split (§9) uses the same instrumentation for both trees but its
  parent build happened to run 13 % faster than the parent's shipped binary, so
  its ratio (1.21×) is not the shipped ratio (1.38×); only its *shares* (and the
  rest-vs-RNG split, which agrees with the standalone RNG measurement) are used.
  On `fix/generation-loop` the same instrumented copy runs 1.75× slower than the
  shipped binary (1906 ms vs 1094 ms for seed 1), which is why no in-situ split is
  quoted for workstream B: inserting statements into this program perturbs its
  binary layout by that much. A double accumulator instead of the integer-ns one
  made it worse still (625 ms → 1093 ms on a build that was otherwise identical).
* Workstream B changes the search trajectories (that is the point), so "A vs B
  generations" compares two different walks from the same initial population and
  the same RNG stream up to the first divergence. The only strictly comparable
  quantity is the success criterion (267/267) and the cost per generation.
* "How often it converges" is only measured on this one dataset, where both
  versions converge 20/20. For an *infeasible* dataset the comparison is no
  longer converges-vs-hangs: `validate_data()` now rejects it at start-up, and a
  merely unreachable optimum now ends at the stagnation bound.
* The 20 seeds are 20 draws of `time(NULL)`-derived seeds through the shim; they
  are a sample, not a distribution. The median difference of 8 generations
  (1.2 %) is well inside the seed-to-seed spread (412–1157).
* Not done: no attempt to fix the `NUM_DAYS`/`NUM_HRS_DAY` hard-coding, the
  missing result-file validation marker, or the fitness model itself (no
  compactness/preference terms, clash charged to the earlier entry only).


# Third pass — a real objective

Branch `feat/soft-objective`, branched from `fix/generation-loop` (`f90f067`).
`master` is still `1063c15`. Nothing was pushed.

The previous passes made the program fast, correct and terminating. They left the
*model* untouched: fitness was "how many of the 267 entries are conflict-free" and
the GA stopped at the first 267/267, so once a timetable was legal the search had
nothing left to say about it. Measured on five pinned seeds, the "perfect"
timetables use **261–265 distinct days for 267 lessons** — 8.8 school days per
class for 9–10 lessons. They are legal and unusable.

## 15.1 First, a correction to the brief

The brief says the old code "ignores type entirely" and that a biology class in
the gym is what the old objective accepts. That is true of the **operators** and
false of the **objective**, and the distinction changes what the fix can buy:

* `initialize.c:36` drew the room uniformly from the 12 rooms, so **63 % of a
  fresh genome (168 of 267 entries) sat in a room whose type does not fit the
  subject** (subject types 3 and 4 have 1 and 2 rooms out of 12). Generation 1 is
  therefore a room-repair problem, not a timetabling problem: the best of 100
  random individuals scores **120/267**.
* `fitness.c:69` (parent branch) then refuses to score such an entry at all —
  `if(ca->subject.type == ra->type) { … } else score = 0;`. A type mismatch was
  already *fatal* to an entry, exactly like a clash.

Consequence: **0 of the 267 exported entries had a wrong-type room in all five
legacy runs** (`tools/quality.py`, independent of the C code). "Biology in the
gym" was already impossible at 267/267. Demoting type to a soft term — as the
brief literally asks — would *weaken* the existing constraint and could accept
exactly the timetable the brief is worried about. So type stays hard, and the
fix is in the place where the defect really is: the operators. What that buys is
**cost, not final quality** — 654 generations → 3–7 (§15.5) — and it is measured
separately from the objective in §15.5 so the two effects are not confused.

The genuinely unaddressed part of the brief is the other three terms; those are
new, and they move the numbers.

## 15.2 The objective

`fitness.c`, top of the file, all tunable with `-D`:

```
fitness = (NUM_ENTRIES - clashes) * HARD_SCALE
        - W_ROOM_TYPE * type_bad
        - W_TEAM_DAY  * team_days      # sum over teams of distinct days used
        - W_TEAM_GAP  * gap_hours      # idle hours inside a team's day
        - W_PROF_DAY  * prof_days      # same as team_days, per professor
```

* `clashes` is exactly the old computation: an entry that shares its `(day,hour)`
  with a later entry in the same room, the same class, the same professor, or
  with a shared student. Same bucket walk, same comparison set, same
  charge-to-the-earlier-entry rule — so the hard gate is *unchanged code*.
* `violations = clashes + type_bad` is the number of entries a school would
  reject, i.e. `NUM_ENTRIES - legacy_fitness`. **"267/267" therefore means
  precisely what it meant before** — no clash *and* no wrong room — and
  `converged` is `violations == 0` under either objective.
* `soft = W_ROOM_TYPE·type_bad + W_TEAM_DAY·team_days + W_TEAM_GAP·gap_hours +
  W_PROF_DAY·prof_days` only orders schedules that already have the same number
  of hard violations. That is the whole trick: a single integer with a
  lexicographic meaning, because the hard part is scaled past everything the
  soft part can reach.

`SOFT_OBJECTIVE=0` restores the legacy number exactly and `TYPED_INIT=0` restores
the legacy operators, so `make legacy` builds the old program beside the new one
and every comparison below is a real A/B, not a reconstruction (§15.5).

## 15.3 The weights, and why those numbers

| define | value | what it prices |
|---|---|---|
| `HARD_SCALE` | `1<<20` = 1,048,576 | one clashing entry |
| `W_ROOM_TYPE` | `HARD_SCALE` | one entry in a room of the wrong type |
| `W_TEAM_DAY` | 8 | one extra school **day** for a class (26 students travel in) |
| `W_TEAM_GAP` | 2 | one **idle hour** inside a class's day (the unit) |
| `W_PROF_DAY` | 4 | one extra teaching **day** for a professor |

Ratios: one extra attendance day for a class is priced at four idle hours for
that class (`W_TEAM_DAY = 4 × W_TEAM_GAP`) — a class coming in for a single
lesson costs 26 students a trip; four dead hours in a day costs one class four
hours. A professor's extra teaching day is priced at half a class's
(`W_PROF_DAY = 2 × W_TEAM_GAP`): 20 professors versus 30 classes, and the cost
is travel, not 26 students sitting idle. Only the *ratios* have meaning; any
common scaling of the three compactness weights leaves the search order
identical.

Magnitudes on this dataset (`objective_check()` prints this line at start-up):

```
Objective: soft=1 hard_scale=1048576 w_room_type=1048576 w_team_day=8 w_team_gap=2 w_prof_day=4
           soft_max=41336 (3.9% of hard_scale)
```

`soft_max` is a conservative worst case computed from the dataset
(`team_days ≤ NUM_ENTRIES`, `gap_hours ≤ teams·days·2`, `prof_days ≤
professors·days`); the worst the constructed extremes actually reach is **3,188
(0.3 %)**. For scale, the soft score of a *real* timetable is:

| timetable | team-days | gaps | prof-days | soft score |
|---|---|---|---|---|
| anything the old objective produces | 263 | 2 | 258 | **3,140** |
| the new objective (seed 1, default budget) | 141 | 22 | 167 | **1,840** |
| what the metrics call perfect | 90 | 0 | 80 | ~1,040 |

**Does the type price matter at all?** No, not on this dataset — and that is the
point. Rebuilding with `-DW_ROOM_TYPE=1024 -DALLOW_SOFT_ROOM_TYPE=1` (a type
mismatch priced 1000× cheaper than a clash) produces **byte-identical
timetables** for seeds 1–3 (md5 `1bec8111…`, `6926fbe7…`, `6cceaab2…`, the same
files as the default build). With typed initialisation and typed room mutation a
mismatch is never *created*, so its price never comes into play; the price only
matters to a build whose operators can create one (`TYPED_INIT=0`). That is why
the fix belongs in `initialize.c`/`mutation.c` and why demoting type to a soft
term would have bought nothing while weakening the gate.

So one clash is worth **334×** the entire soft score of an old-style timetable
and **570×** the new one: the two tiers are separated by 2.5 orders of magnitude,
and the widest soft swing the metrics can produce (3,188) is still 329× smaller
than one clash.

## 15.4 The hard gate cannot be bought off

Three layers, all in the tree:

1. **By construction.** `objective_check()` computes the soft bound above and
   `exit(1)`s if `soft_max >= HARD_SCALE`. It also refuses `W_ROOM_TYPE <
   HARD_SCALE` unless the build explicitly says `-DALLOW_SOFT_ROOM_TYPE=1`
   (the escape hatch exists only to run the "what if type were soft?" experiment,
   and it prints a loud warning). A rebuild with weights that break the invariant
   does not start.
2. **By test.** `make test-hard-gate` (`tools/test_hard_gate.c`, links the real
   `fitness.c`) builds 300 schedules from the real dataset — random, crammed into
   3 days, hand-built worst-compactness — and checks 44,850 pairs:

   ```
   -- (1) fitness is exactly the packed (violations, soft) scalar --
     300 schedules: fitness == (NUM_ENTRIES-clashes)*HARD_SCALE - soft
   -- (2) packed ordering == lexicographic (violations, soft) --
     44850 pairs compared, 0 mismatches
   -- (3) one hard violation outweighs every soft gain --
     worst soft cost seen in this run: 3188  (HARD_SCALE = 1048576, 0.3%)
     lowest score with 0 violations : 279966604
     highest score with >=1 violation: 276820968
     margin                         : 3145636
     spread timetable      : violations=0 soft=3188 fitness=279966604
     same + one clash      : violations=1 soft=3176 fitness=278918040
   PASS (0 failures)
   ```

   The last pair is the exact scenario in the brief: the *spread* timetable
   (worst compactness the grid allows, zero violations) against the same
   timetable with one lesson moved onto another's slot. The clash costs
   1,048,564 = `HARD_SCALE` + the 12 soft points that move gained.
3. **By construction of the search.** `converged` is `violations == 0`, so a run
   that ends on a timetable with a wrong-type room cannot be reported as
   converged, whatever its soft score.

## 15.5 Feasibility: still 267/267, five seeds out of five

`make bench` (pinned seeds via `tools/fixed_seed.so`, pop 50, 5 seeds). A = the
old program, B = the new one at the default budget, C/D = the two halves of the
change in isolation (`-DSOFT_OBJECTIVE=0 -DTYPED_INIT=1` and the reverse).

| configuration | gens to first 267/267 | total gens | wall, median | converged |
|---|---|---|---|---|
| A: legacy (old objective, old operators) | 654 \* | 654 (412–728) | 0.81 s | 5/5 |
| C: typed init, old objective | 5 (4–7) | 5 | 0.01 s | 5/5 |
| D: old init, soft objective (+refine) | 683 (413–1221) | 1683 (1413–2221) | 2.85 s | 5/5 |
| B: new (typed init, soft objective) | **3 (3–7)** | 1003 | 2.09 s | **5/5** |

\* A has no separate "first feasible": it stops the moment it gets there.

Reading:

* **Feasibility is not harder.** 5/5 in every configuration; the new build never
  ended on a schedule with a clash or a wrong-type room, and `tools/quality.py`
  re-derives 0 clash pairs and 0 type mismatches from the exported CSV of all
  five seeds every time.
* **The 200× reduction in generations-to-feasible comes from typed
  initialisation, not from the objective** (A 654 → C 5). It is the single
  biggest effect in this pass and it is a *cost* effect: the old program spent
  654 generations undoing the ~168 wrong rooms it had created itself, and 5
  generations later it had the same timetable quality it always had (team-days
  263 → 262 — nothing).
* **The objective, on its own, makes feasibility slightly slower** (D: 683
  generations versus A's 654, +4 %). That is not noise in the wrong direction:
  compactness and clash-freedom pull against each other (putting a class's
  lessons on the same days makes those days crowded), so the soft terms fight
  the hard tier a little. It costs 4 % of the feasibility phase and repays it
  in quality (§15.6).
* **The two halves together are better than either**: with typed initialisation
  there is nothing left to repair, so the run reaches 267/267 in 3–7 generations
  and can spend the remaining budget on quality.

### The refinement budget

Because 267/267 now arrives at generation 3, stopping there would deliver the
old quality (team-days 261, §15.5 row `refine=0`). `SGA_REFINE` (default 1000)
keeps the GA running after the first acceptable schedule; the elite is preserved
every generation, so feasibility can only be gained, never lost, and every
subsequent improvement is compactness. Median of 5 seeds:

| `SGA_REFINE` | gens | team-days | idle hours | prof-days | calendar days | wall s |
|---|---|---|---|---|---|---|
| 0 (stop at first 267) | 3 | 261 | 5 | 254 | 145 | 0.008 |
| 100 | 103 | 218 | 24 | 213 | 126 | 0.19 |
| 300 | 303 | 181 | 31 | 183 | 101 | 0.58 |
| **1000 (default)** | 1003 | **141** | 30 | 158 | 81 | 2.09 |
| 3000 | 3003 | 113 | 24 | 137 | 69 | 6.58 |

Diminishing but not dead: +900 generations buys 40 team-days (25 per second),
the next +2000 buys 28 (6 per second). 1000 is the knee — 4.7 attendance days
per class, which is a plausible school week, at a wall time 2.5× the original.
`SGA_REFINE=3000 make bench` is the "best quality, 7 seconds" setting.

## 15.6 Quality, old vs new

Median [min–max] over 5 pinned seeds, computed from the exported CSV by
`tools/quality.py` (independent re-implementation, not the C code):

| metric | old objective | new objective | change |
|---|---|---|---|
| team-days (Σ distinct days per class) | 263 [261–265] | **141 [139–146]** | **−46 %** |
| attendance days per class | 8.8 | **4.7** | −46 % |
| worst class (max days, one class) | 11–12 days | **6–7 days** | |
| worst professor (max days, one professor) | 41–43 days | **19–28 days** | |
| professor-days (Σ per professor) | 258 [250–259] | **158 [145–167]** | **−39 %** |
| idle hours inside classes' days | 2 [0–5] | 30 [22–40] | **+28 (15× worse)** |
| calendar days used (of 195) | 149 [142–153] | 81 [77–86] | −46 % |
| distinct `(day,hour)` slots | 233 | 216 | −7 % |
| room-type mismatches | 0 | 0 | — |
| clashing pairs | 0 | 0 | — |

The idle-hours row is the honest cost of compaction and it is not a bug: the old
timetable has 2 idle hours *because* no class ever has two lessons on one day, so
there is no room for a gap. As soon as lessons are gathered into a few days,
gaps appear unless the packing is perfect. The weighting says that is still a
good trade (one team-day = four idle hours; the new schedule is 122 team-days
cheaper and 28 idle hours dearer), and the refine curve shows gaps *falling*
again at 3000 (30 → 24) as the packing tightens. A reader who cares about gaps
more than attendance days should raise `W_TEAM_GAP`; that is one `-D`.

## 15.7 Cost

| measure | old | new | ratio |
|---|---|---|---|
| wall clock per run (interleaved A/B, 25 runs each) | 0.818 s | 2.085 s | **2.55×** |
| wall clock per generation | 1.24 ms | 2.07 ms | 1.67× |
| `calc_fitness`, per generation (instrumented copies, same session) | 1.13 ms | 1.81 ms | **1.60×** |
| **share of the run inside `calc_fitness`** | **78.3 %** | **85.4 %** | +7.1 pts |
| generations | 654 | 1003 | 1.53× |

The fitness pass grew by 0.68 ms per generation (11.3 µs → 18.1 µs per schedule,
42 → 68 ns per entry) for three extra whole-timetable metrics. Two thirds of
that is two random accesses per entry into the `(team, day)` and
`(professor, day)` stamp tables; grouping a cell's four fields into one 16-byte
struct (instead of four parallel `int` arrays) recovered 7 % of it. Everything is
allocated once and stamped rather than cleared, so a schedule touches only the
~260 cells its 267 entries need.

Note the shape of the cost: **the search itself got ~130× cheaper** (654
generations → 3–7 to reach feasibility) and the extra wall time is the refinement
budget, deliberately spent on quality. `SGA_REFINE=0` runs in **8 ms**, 100×
faster than the original, with the old timetable quality. The 2.55× headline is
the price of the timetable, not of the objective.

## 15.8 The result, side by side

`tools/compare_timetables.py`, seed 1, same population, same pinned seed. Rows
are indexed by *attendance day* — the 1st, 2nd, 3rd … day the class actually
comes in — because calendar day numbers mean nothing across two runs on a
195-day grid. Hours 1–4 left to right; `-------` is a free hour; no room type is
printed because every entry in both runs is in a matching room.

Class **1a** — OLD: 9 attendance days for 10 lessons (one day holds two).

| attendance day (calendar) | h1 | h2 | h3 | h4 |
|---|---|---|---|---|
| 1 (1) | – | – | – | Idraet/Jakobsen |
| 2 (12) | – | Samfund/Andersen | – | – |
| 3 (39) | Idraet/Jakobsen | – | – | – |
| 4 (58) | – | – | – | Samfund/Andersen |
| 5 (89) | – | – | Psykologi/Toft | Psykologi/Toft |
| 6 (124) | – | Samfund/Andersen | – | – |
| 7 (138) | – | – | Samfund/Andersen | – |
| 8 (156) | – | – | Idraet/Jakobsen | – |
| 9 (169) | – | – | Psykologi/Toft | – |

Class **1a** — NEW: 5 attendance days for the same 10 lessons.

| attendance day (calendar) | h1 | h2 | h3 | h4 |
|---|---|---|---|---|
| 1 (19) | Samfund/Andersen | Samfund/Andersen | – | – |
| 2 (34) | Samfund/Andersen | Idraet/Jakobsen | Samfund/Andersen | Idraet/Jakobsen |
| 3 (64) | Psykologi/Toft | Psykologi/Toft | – | – |
| 4 (124) | – | Idraet/Jakobsen | – | – |
| 5 (178) | – | – | – | Psykologi/Toft |

Class **2c** — OLD: 7 days, 7 lessons, all at a different hour of a different day.

| attendance day (calendar) | h1 | h2 | h3 | h4 |
|---|---|---|---|---|
| 1 (18) | Erhverv/Mikkelsen | – | – | – |
| 2 (97) | – | – | Erhverv/Mikkelsen | – |
| 3 (105) | – | – | – | Erhverv/Mikkelsen |
| 4 (144) | – | – | – | Historie/Vester |
| 5 (161) | – | Historie/Vester | – | – |
| 6 (169) | – | – | – | Historie/Vester |
| 7 (186) | – | – | – | Erhverv/Mikkelsen |

Class **2c** — NEW: 4 days.

| attendance day (calendar) | h1 | h2 | h3 | h4 |
|---|---|---|---|---|
| 1 (64) | – | – | Erhverv/Mikkelsen | Historie/Vester |
| 2 (85) | – | – | – | Erhverv/Mikkelsen |
| 3 (136) | – | Erhverv/Mikkelsen | – | – |
| 4 (162) | Historie/Vester | Historie/Vester | Erhverv/Mikkelsen | – |

## 15.9 Failure modes, regressions, and what this does not fix

* **Idle hours get worse** (2 → 30, §15.6). Unavoidable consequence of
  compaction with an imperfect packer; the objective prices it correctly, a
  reader who disagrees can re-weight it.
* **Feasibility is 4 % slower with the soft terms alone** (D: 683 vs A: 654
  generations). Small, but it is a real tension rather than noise, and it is the
  reason typed initialisation matters: without it the soft terms spend their
  budget fighting the room-type repair.
* **The soft objective does not make a tight instance soluble.** With a
  realistic grid (`NUM_DAYS = 6` instead of 195 — 12 rooms × 4 hours × 6 days =
  288 slots for 267 lessons, so a near-perfect packing is *required*), neither
  objective converges: the old one stalls at 219–223/267 (44–48 violations) and
  the new one at 228–229/267 (38–39, all clashes, no type errors) after
  3000–5000 generations. The soft terms help (6–9 fewer violations) but the GA
  cannot solve a 93 %-occupancy packing problem by single-gene mutation. This is
  the honest answer to "would a school accept it": the *shape* is now right, the
  *grid* is still a 195-day toy, and fixing that is a modelling change
  (`NUM_DAYS` is hard-coded in `main.c`), not a weight.
* **The gap metric is not independent of the spread metric** — as §15.6 shows,
  they trade off directly. That is why both are in the sum with explicit prices
  instead of being optimised one at a time.
* **`NUM_DAYS = 195` still makes the calendar meaningless**: 81 distinct days
  for a timetable that would run over 6 in a real school. The class-level
  compaction (4.7 days each) is what the objective can see; the calendar is not
  a term.
* **The packer is still a generational GA with 1/267 mutation and uniform
  crossover.** I tried one search change and it did **not** help: moving the day
  by a ±5 / ±20 / ±50 window instead of redrawing it from all 195 days gives
  214 / 198 / 184 team-days versus **185** for the original operator at
  `SGA_REFINE=300`. Compacting needs the jump "to a day my class already uses",
  and the uniform draw proposes it far more often than a local move. The
  constant (`MUT_DAY_WINDOW`) stays at 0 — the 2015 operator — and the
  experiment is rebuildable with `-DMUT_DAY_WINDOW=n`.
* **A clash is still charged to the earlier entry only**, so `violations` is not
  a count of conflicting pairs; it is the old fitness inverted. I left that
  alone on purpose: changing it would change what "267/267" means, and the whole
  point of the legacy switch is that the two runs are comparable.
* **The weights are tuned for this dataset's shape** (30 classes, 20 professors,
  195 days). The ratios are defensible, the bound is enforced at start-up, and
  on a dataset an order of magnitude larger the bound check would fail and demand
  re-tuning rather than silently letting soft outrank hard.

## 15.10 Reproduce

```bash
cd /home/hermes/work/experiments/scheduleGA
git checkout feat/soft-objective

make both                     # ./sga (new) and ./sga-legacy (old, for A/B)
make bench                    # 5 pinned seeds, both objectives, quality tables
make test-hard-gate           # the ordering proof (§15.4)
make equivalence-legacy       # SOFT_OBJECTIVE=0/TYPED_INIT=0 == parent, 5/5 md5s
./tools/run_third_pass.sh     # every table in §15.5-15.7, ~2 minutes

# the side-by-side excerpt
python3 tools/compare_timetables.py --old tools/quality-legacy-seed1.csv \
                                   --new tools/quality-soft-seed1.csv --teams 1a,2c

# one run by hand
SGA_SEED=1 LD_PRELOAD=./tools/fixed_seed.so ./sga <<< "50" | tail -3
python3 tools/quality.py                 # independent check of result/result.csv
SGA_REFINE=3000 make bench               # the 7-second, 113-team-day setting
SGA_REFINE=0   make bench                # stop at the first 267/267 (8 ms)

# the ablations, one command each
S="main.c fitness.c parsing.c initialize.c functions.c crossover.c mutation.c selection.c export.c"
cc -O2 -std=c99 -DSOFT_OBJECTIVE=0 -DTYPED_INIT=1 -o /tmp/typed-only $S   # C
cc -O2 -std=c99 -DSOFT_OBJECTIVE=1 -DTYPED_INIT=0 -o /tmp/soft-only  $S   # D

# weights are compile-time; this is the "what if type were soft?" experiment
make clean && make CFLAGS="-O2 -std=c99 -Wall -D_POSIX_C_SOURCE=200809L \
                            -DW_ROOM_TYPE=1024 -DALLOW_SOFT_ROOM_TYPE=1"

# a tight, realistic grid (neither objective solves it -- §15.9)
T=$(mktemp -d); cp -r . $T/nd6; cd $T/nd6
sed -i 's/    NUM_DAYS = 195;/    NUM_DAYS = 6;/' main.c && make -s sga sga-legacy
SGA_SEED=1 SGA_MAX_GENERATIONS=3000 \
  LD_PRELOAD=/home/hermes/work/experiments/scheduleGA/tools/fixed_seed.so \
  ./sga-legacy <<< "50" | tail -2
```

Files added by this pass: `header/objective.h` (switches), `tools/quality.py`
(independent metric check), `tools/bench_quality.py` (seeded quality harness),
`tools/compare_timetables.py` (side-by-side excerpt), `tools/test_hard_gate.c`
(the ordering proof), `tools/run_third_pass.sh` (every table above),
`tools/check_legacy_equivalence.sh` (the byte-identity check).

---

# Fourth pass — a realistic instance

Branch `feat/realistic-grid`, branched from `feat/soft-objective` (`dac34f6`).
`master` is still `1063c15`; nothing was pushed.

The third pass ended with an honest failure: on a 6-day grid neither objective
converged (old 219–223/267, new 228–229/267). This pass asked where the boundary
actually is. The short answer is that **the 6-day instance was never a search
problem** — it is an unsatisfiable instance — and that with one repair operator
the GA solves every feasible grid this dataset can express, including the 
tightest one possible at 4 hours/day.

| question | answer |
|---|---|
| utilisation where the plain GA stops | **85.6 % → 89 %** (succeeds at 85.6 %, median 8196 generations; fails at 89 %, best 262–265/267 after 20 000) |
| did the grid become a parameter? | yes: `SGA_DAYS` / `SGA_HOURS` / `-DSGA_DEFAULT_DAYS`, defaults 195 × 4, byte-identical output |
| did better initialisation move the boundary? | **no.** Least-conflict construction solved *nothing* the random init did not and was worse at 89 % |
| did repair/local search move it? | **yes, to the instance's ceiling.** 89 % solved in 8–11 generations, 0.31 s, 5/5 seeds |
| what did CP-SAT do? | solved all three feasible instances (3.6–3.9 s) and **proved the committed 6-day instance infeasible in 4.8 s** |
| the previous "6 days = 92.7 %" failure | unsatisfiable: one professor has 45 lessons, one room type has 154 lessons for 144 slots |

## 16.1 The grid is now a parameter

`main.c` held `NUM_DAYS = 195; NUM_HRS_DAY = 4;` in `main()`. It is now

```c
#ifndef SGA_DEFAULT_DAYS
#define SGA_DEFAULT_DAYS 195
#endif
...
NUM_DAYS     = env_int("SGA_DAYS",  SGA_DEFAULT_DAYS);
NUM_HRS_DAY  = env_int("SGA_HOURS", SGA_DEFAULT_HOURS);
```

read before `validate_data()`, so every consumer (the bucket arrays, the
soft-objective stamp tables, mutation, export, the objective's soft bound)
derives its bounds from the same two globals — nothing else needed changing.
`SGA_DATA_DIR` (default `data`) points the parsers at a generated dataset
variant, so the committed CSVs are untouched and every earlier number still
reproduces:

```bash
SGA_DAYS=6 SGA_DATA_DIR=data-d6 ./sga <<< 50
make CFLAGS="-O2 -std=c99 -DSGA_DEFAULT_DAYS=6"        # compile-time default
```

Verification of "default behaviour is unchanged": seeds 1–3 still produce
`result.csv` md5 `1bec8111…`, `6926fbe7…`, `6cceaab2…` (§15.3), and
`make equivalence-legacy` still passes byte-for-byte against the parent branch.

## 16.2 First, the dataset is not a 6-day school

`tools/feasibility_bounds.py` computes the pigeonhole bounds: every resource a
lesson consumes (a room of its subject's type, its class, its professor) gives
`demand ≤ capacity × hours × days`, and classes that share a student add up.
No search can beat those bounds, so they separate "the GA failed" from "no
timetable exists", which the previous pass could not.

Committed dataset (`data/`, 267 lesson-hours, 12 rooms, 20 professors):

| resource | demand | capacity/day | **min days** |
|---|---|---|---|
| professor 8 | 45 h | 4 | **12** |
| room type 1 (classroom, 6 rooms) | 154 | 24 | 7 |
| professor 14 | 32 h | 4 | 8 |
| room type 3 (gym, 1 room) | 21 | 4 | 6 |
| professor 9 | 28 h | 4 | 7 |
| room type 2 (lab, 3 rooms) | 56 | 12 | 5 |
| class pair 1a+1b (share 3 students) | 20 | 4 | 5 |

**Lower bound: 12 days.** At the "realistic" 6-day grid, one professor needs 45
of the 24 slots and the classrooms need 154 of 144 — two independent
impossibilities. The previous pass measured a GA stalling at 228/267 on that
grid; the correct reading is that 228/267 is as close as anything can get.

## 16.3 The feasibility frontier, committed dataset

Soft objective, hard gate, typed operators (`./sga`), 5 pinned seeds, budget
20 000 generations / 3000 stagnant, `SGA_REFINE=0` so the run stops at the first
legal timetable. *Feasible* is the analytic verdict of §16.2, not the GA's.

| days | room-slot utilisation | pigeonhole-feasible | converged | gens to 267/267 (median) | wall (median) | best when it fails |
|---|---|---|---|---|---|---|
| 195 | 2.9 % | yes | 5/5 | 3 | 0.005 s | — |
| 30 | 18.5 % | yes | 5/5 | 51 | 0.033 s | — |
| 20 | 27.8 % | yes | 5/5 | 89 | 0.057 s | — |
| 12 | 46.4 % | yes | 5/5 | 456 | 0.29 s | — |
| 11 | 50.6 % | **no** (professor 8) | — | — | — | — |
| 8 | 69.5 % | **no** (professor 8) | 0/5 | — | 9.9 s | 253–254 |
| 6 | 92.7 % | **no** (professor 8, classrooms) | 0/5 | — | 12.7 s | 229–231 |
| 5 | 111.2 % | **no** (classrooms, gym) | 0/5 | — | 13.4 s | 203–205 |

So on this dataset the GA's failure boundary coincides *exactly* with the
feasibility boundary: it solves the only feasible tight grid (12 days, 46.4 %)
5/5, and fails at every grid where no timetable exists. That is a property of
the dataset, not a measurement of the search. To find a search boundary the
instance has to be made feasible at a high occupancy first.

## 16.4 Making a genuine tight instance (and what changed)

`tools/make_school_data.py` derives a *staffed* variant: professors above a
weekly cap are split into `ceil(hours/cap)` teachers (a school hires a second
teacher), and classrooms/gyms are appended as needed. Lessons, classes,
subjects, students and `NUM_ENTRIES=267` are byte-identical — the same
timetabling instance, with enough staff and rooms to be possible:

```bash
python3 tools/make_school_data.py --out data-d7 --days 7 --max-load 20
python3 tools/make_school_data.py --out data-d6 --days 6 --add-classrooms 1 --max-load 20
python3 tools/make_school_data.py --out data-d5 --days 5 --add-classrooms 2 --add-gyms 1 --max-load 20
```

| dataset | days | rooms | professors | room-slot util. | classroom | gym | lab | music | pigeonhole |
|---|---|---|---|---|---|---|---|---|---|
| `data` (committed) | 6 | 12 | 20 | 92.7 % | **106.9 %** | 87.5 % | 77.8 % | 75.0 % | **no** |
| `data-d7` | 7 | 12 | 25 | 79.5 % | 91.7 % | 75.0 % | 66.7 % | 64.3 % | yes |
| `data-d6` | 6 | 13 | 25 | 85.6 % | 91.7 % | 87.5 % | 77.8 % | 75.0 % | yes |
| `data-d5` | 5 | 15 | 25 | 89.0 % | 96.2 % | 52.5 % | 93.3 % | 90.0 % | yes |

5 days is the smallest grid these 267 lessons can live in at 4 hours/day
(binding: the shared-student class pairs need 20 slots, the classrooms need
154 ≤ 8 rooms), and 15 rooms is the smallest room set that fits — so **89 % is
the highest utilisation this instance can reach at all**. It is a ceiling of the
dataset, not of the GA.

## 16.5 Better search: what helped and what did not

Two changes, both compile-time and off by default, in `search.c`:

* `INIT_MODE=1` — **least-conflict construction**. Each entry is placed in the
  `(day, hour, room-of-type)` that adds the fewest conflicts to what is already
  placed; ties are random, which is the population's diversity.
* `REPAIR=n` — **min-conflicts repair on the elite**. While the elite has a
  violation, pick a random offending entry, move it to the placement with the
  fewest conflicts (random tie-break), with a 1-in-8 random kick and a budget of
  `SGA_REPAIR_STEPS` moves (default 2000); keep the result only if it scores
  better. `calc_fitness` runs on the repaired schedule, so the hard gate still
  decides what "feasible" means. The GA loop, selection, crossover, mutation and
  generational replacement are untouched — this is a memetic GA+repair, not a
  replacement.

Binaries: `./sga` (plain), `./sga-greedy` (init only), `./sga-repair` (repair
only), `./sga-tight` (both).

5 pinned seeds, budget 20 000 generations / 3000 stagnant, `SGA_REFINE=0`:

| instance | plain GA | + least-conflict init | + repair only | + both |
|---|---|---|---|---|
| `data-d7`, 79.5 % | 5/5, 2774 gens, 2.00 s | 5/5, 2064 gens, 1.58 s | — | 5/5, **1 gen**, 0.10 s |
| `data-d6`, 85.6 % | 5/5, 8196 gens, 6.17 s | 5/5, 5852 gens, 4.47 s | 5/5, **1 gen**, 0.018 s | 5/5, 1 gen, 0.12 s |
| `data-d5`, 89.0 % | **0/5**, best 260–265 | **0/5**, best 261–266 | 5/5, **11 gens**, 0.32 s | 5/5, 8 gens, 0.31 s |

(medians; per-seed ranges: `data-d5` repair 9–13 generations, both 3–28.)

Reading, honestly:

* **The repair operator is the whole result.** It turns an instance the GA
  cannot solve in 20 000 generations (best 262–266 of 267) into one it solves in
  a median of 11 generations and 0.3 s. It also makes the 85.6 % instance
  340× cheaper (6.17 s → 0.018 s).
* **Constraint-directed initialisation did not move the boundary.** It helps a
  little at 79.5 % and 85.6 % (2774 → 2064, 8196 → 5852 generations) and *not at
  all* at 89 % (0/5 either way). A greedy constructor places early entries in
  locally good slots and paints itself into a corner when the grid is 96 %
  occupied per room type; min-conflicts can undo those choices, a constructor
  cannot. On the easy 195-day instance the constructor is pure overhead
  (§16.7).
* **Repair alone ≥ repair + greedy init**: 0.018 s vs 0.12 s at 85.6 %, 11 vs 8
  generations at 89 % (a wash). `./sga-tight` exists for the ablation, not
  because the combination is better.
* Nothing regressed on the original instance (5 seeds, 195 days): `./sga-tight`
  is feasible 5/5 with team-days 147 [146–150] vs 141 [139–146] and 1.53× the
  wall time (0.80 s vs 0.52 s, 9 interleaved runs each) — a 4 % quality loss and
  the cost of the constructor, not of the repair (repair does not fire once the
  elite is legal).

## 16.6 CP-SAT, the same instance

OR-Tools is installed in a venv (`/tmp/ortsat`) because the system `python3`
has no `pip`; `uv pip install ortools` worked, `pip install --user ortools` was
not possible on this box. `tools/cpsat_solve.py` builds the identical model:
one `(slot, room-of-subject-type)` decision per lesson, at-most-one per
(room, slot), (class, slot), (professor, slot), and per (shared-student pair,
slot); symmetry broken by forcing one course's lessons into increasing slots.
`--objective team-days` adds the GA's compaction term (Σ distinct days per
class) as a minimisation.

| instance | objective | status | build | solve | first solution | objective |
|---|---|---|---|---|---|---|
| `data` D=6 (92.7 %) | none | **INFEASIBLE (proved)** | 0.16 s | 4.80 s | — | — |
| `data-d7` D=7 (79.5 %) | none | OPTIMAL | 0.17 s | 3.85 s | — | 0 |
| `data-d6` D=6 (85.6 %) | none | OPTIMAL | 0.17 s | 3.94 s | — | 0 |
| `data-d5` D=5 (89.0 %) | none | OPTIMAL | 0.17 s | 3.64 s | 3.59 s | 0 |
| `data-d6` D=6 | team-days | FEASIBLE (not proved) | 0.17 s | 120 s (limit) | — | **78** (bound 46) |

Every solution was re-checked by `tools/quality.py` (which shares no code with
either solver): 0 clash pairs, 0 type mismatches, 267 rows, every course with
exactly its hours.

Head-to-head, same dataset and grid:

* **Feasibility.** On the 89 % instance the GA+repair is *faster* (0.31 s vs
  3.6 s) and on the 85.6 % instance much faster (0.018 s vs 3.9 s). CP-SAT has
  one decisive win the GA cannot match at any budget: it **proves** the
  committed 6-day instance unsatisfiable in 4.8 s, where the GA can only be
  stopped by a generation bound after reporting 228–231/267.
* **Quality** (`data-d6`, team-days, lower is better; 30 classes, 6 days, so
  30 would be perfect): GA+repair 129 (0.92 s, default `SGA_REFINE=1000`);
  CP-SAT hard-only 149 (3.9 s); CP-SAT with the team-days objective **78**
  (120 s, not proven optimal, bound 46). So the GA is ~130× cheaper for a
  good-but-not-best timetable, and CP-SAT is 40 % better given two minutes.
* These are different problems: the GA minimises a weighted sum (team-days 8,
  gaps 2, professor-days 4) and CP-SAT here minimises team-days alone. The
  comparison is fair on feasibility and indicative, not exact, on quality.

## 16.7 One speed-up that was needed to measure any of this

At high occupancy the 2015 clash test walked the two 26-student lists for every
candidate pair (676 comparisons). On a 195-day grid almost no two entries share
a slot so it never mattered; at 12 days it was 70–90 % of the run. `fitness.c`
now precomputes a 30 × 30 class-overlap matrix once (`team_overlap`) and tests
it with one load — the same predicate, so the clash definition, the fitness
value and every earlier output are unchanged:

* 195-day default workload: **2100 ms → 522 ms (4.0×)**, interleaved A/B
  against the parent commit's own binary, 3 seeds × 3 rounds, byte-identical
  `result.csv` and `make equivalence-legacy` PASS;
* tight grids: 400 generations at 12 days **7.75 s → 0.26 s (30×)**, at 8 days
  **10.9 s → 0.28 s (39×)**.

This is why `make bench` now reports 0.52 s for the soft objective where §15.7
reported 2.09 s: the earlier number was dominated by the student scan, not by
the objective.

## 16.8 Verdict

At 4 lessons/day this dataset stops being a *solvable* timetabling instance at
**89 % room-slot utilisation** (5 days, the minimum grid, 15 rooms, 96 % of the
classrooms booked), and the plain GA stops earlier, between **85.6 % and 89 %**.
The repair operator removes that gap entirely: it solves the 89 % instance in
8–11 generations and 0.3 s, which is the ceiling of the instance — you cannot
build a tighter feasible grid from these 267 lessons at 4 hours/day, so this is
where the measurement ends, not where the method provably ends. The 92.7 %
figure the previous pass reported as a GA failure is not a search boundary at
all: no timetable exists there, and CP-SAT proves it in 4.8 s.

What would move the boundary further is a *different instance*, not a better
search: more teaching hours per day (a 6- or 8-hour school day raises the
denominator), more classes (more lessons per room), or a second gym. The binding
resource at 89 % is the classroom (96.2 %), and the binding constraint on the
calendar is the shared-student class pairs, which need 20 of the 20 weekly slots.
The honest summary of the search work: a 200-line min-conflicts repair, called
on the elite once per generation, is worth more here than any amount of
population size or initialisation cleverness.

## 16.9 Reproduce

```bash
cd /home/hermes/work/experiments/scheduleGA
git checkout feat/realistic-grid
make both sga-tight sga-greedy sga-repair
./tools/run_fourth_pass.sh              # every table in this section, ~6 minutes

# the analytic bounds (no search involved)
python3 tools/feasibility_bounds.py                       # committed data: min 12 days
python3 tools/feasibility_bounds.py --data data-d5        # min 5 days

# the staffed variants (already committed; regenerate deterministically)
python3 tools/make_school_data.py --out data-d7 --days 7 --max-load 20
python3 tools/make_school_data.py --out data-d6 --days 6 --add-classrooms 1 --max-load 20
python3 tools/make_school_data.py --out data-d5 --days 5 --add-classrooms 2 --add-gyms 1 --max-load 20

# the frontier on the committed data (§16.3)
python3 tools/frontier.py --label random --days 195 30 20 12 8 6 5 --seeds 1 2 3 4 5 \
        --env "SGA_REFINE=0"

# the tight instances, four builds, five seeds each (§16.5)
for b in sga sga-greedy sga-repair sga-tight; do
  python3 tools/frontier.py --label d5-$b --days 5 --seeds 1 2 3 4 5 --binary ./$b \
    --env "SGA_DATA_DIR=data-d5;SGA_MAX_GENERATIONS=20000;SGA_STAGNATION=3000;SGA_REFINE=0"
done

# one run by hand on the 89 % instance
SGA_DATA_DIR=data-d5 SGA_DAYS=5 SGA_REFINE=0 SGA_SEED=1 LD_PRELOAD=./tools/fixed_seed.so \
  ./sga-tight <<< "50" | tail -3
SGA_DATA_DIR=data-d5 python3 tools/quality.py            # independent: 267 rows, 0 clashes

# CP-SAT (§16.6).  The system python3 has no pip; this is how ortools got installed.
uv venv /tmp/ortsat && VIRTUAL_ENV=/tmp/ortsat uv pip install ortools
/tmp/ortsat/bin/python tools/cpsat_solve.py --data data    --days 6 --time-limit 60   # INFEASIBLE
/tmp/ortsat/bin/python tools/cpsat_solve.py --data data-d5 --days 5 --time-limit 60   # 3.6 s
/tmp/ortsat/bin/python tools/cpsat_solve.py --data data-d6 --days 6 --time-limit 120 \
        --objective team-days --out result/cpsat-d6-teamdays.csv

# the 4.0x kernel fix, A/B against the parent commit
git worktree add --detach /tmp/sga-parent dac34f6 && make -C /tmp/sga-parent sga
for r in 1 2 3; do for s in 1 2 3; do
  for b in /tmp/sga-parent/sga $PWD/sga; do
    t0=$(date +%s%3N); (cd $(dirname $b) && SGA_SEED=$s LD_PRELOAD=$PWD/tools/fixed_seed.so \
      ./$(basename $b) <<< "50" >/dev/null); echo "$(basename $b) $(( $(date +%s%3N)-t0 )) ms"
  done; done; done
```

Files added by this pass: `search.c` (`greedy_init`, `repair_schedule`),
`tools/frontier.py` (multi-grid seeded harness), `tools/feasibility_bounds.py`
(pigeonhole bounds), `tools/make_school_data.py` (staffed dataset variants),
`tools/cpsat_solve.py` (OR-Tools model), `tools/run_fourth_pass.sh`, and the
generated datasets `data-d7/`, `data-d6/`, `data-d5/`.

## 16.10 Caveats

* **The tight instances are derived, not given.** `data-d5/d6/d7` change the
  staffing and the room list (and nothing else) because the committed dataset
  cannot be timetabled below 12 days. Every "utilisation" figure above is
  *rooms × hours × days* for 267 lessons; it is a coarse measure — the binding
  resource is the classroom (96.2 % at the 89 % point), and a dataset with more
  lab slack can show a lower total utilisation while being harder.
* **89 % is the ceiling of this lesson set at 4 hours/day**, established by the
  per-type bounds, so "the repair solves everything feasible" is a statement
  about this instance family only. The room arithmetic alone would allow a
  3-day grid at 96.7 % (23 rooms × 12 slots), but it is infeasible: the six
  shared-student class pairs each need 20 slots out of 12. A larger dataset, a
  6-hour day or a second gym would each move the ceiling; none was tested.
* **The repair does the work, and it is a local search.** The GA's population is
  still there (and a repaired elite participates in selection and crossover),
  but on the tight instances the population is closer to a diversification
  device around a min-conflicts solver. Calling this "the GA solved it" would
  overstate the genetic part.
* **The repair counts clashing pairs, the objective charges a clash to the
  earlier entry.** Zero pairs implies zero violations, so the hard gate is not
  weakened, but the two numbers are not the same statistic and are not compared
  as such.
* **CP-SAT timings are one formulation on one box.** 3.6–4.8 s is not a lower
  bound on what CP-SAT could do; an integer/decomposition model would be
  faster, and the team-days run hit the 120 s limit without proving optimality
  (bound 46, found 78).
* **Quality comparison is not like-for-like**: the GA optimises a weighted sum
  of four terms, CP-SAT's objective run minimises team-days only.
* Seeds are 5 pinned draws, the box drifts ±15 % between sessions and everything
  in §16.5 was measured in one session; the wall times are medians of 5 and are
  not comparable with earlier sections' absolute numbers.
* The repair operator's 2000-move budget is a constant, not a measured optimum;
  at 89 % it succeeds with a median of 8–11 GA generations and a few thousand
  moves total, but no sweep of `SGA_REPAIR_STEPS` was run.
* Not verified: datasets much larger than 267 entries (the static arrays still
  cap at 500 courses / 255 teams), and grids with a different `SGA_HOURS`. The
  parameter works (a smoke test at 30 days × 5 hours returns 267/267 in 29
  generations), but every measurement in this section is 4 hours/day.
