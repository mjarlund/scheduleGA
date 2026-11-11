#ifndef OBJECTIVE_HEADER_INCLUDED
#define OBJECTIVE_HEADER_INCLUDED

/* Switches for the third pass (soft objective + type-aware operators).
 *
 * Both are compile-time and every weight in fitness.c can be overridden with
 * -D, so any comparison in the report can be rebuilt from the same tree:
 *
 *   make                                          soft objective, typed operators
 *   make legacy                                   SOFT_OBJECTIVE=0: the exact
 *                                                 pre-third-pass behaviour
 *   make CFLAGS="-O2 -std=c99 -DSOFT_OBJECTIVE=0 -DTYPED_INIT=0"
 *
 * SOFT_OBJECTIVE=0 + TYPED_INIT=0 reproduces the parent branch
 * (fix/generation-loop) byte for byte: same RNG stream, same operators, same
 * fitness.  Verified with md5 in tools/check_legacy_equivalence.sh.
 */

/* 1: weighted objective (hard tier + compactness terms), 0: legacy
 * "count the conflict-free entries" objective. */
#ifndef SOFT_OBJECTIVE
#define SOFT_OBJECTIVE 1
#endif

/* 1: initialisation and mutation draw the room from the rooms whose type
 *    matches the subject, when one exists.  0: uniform over all rooms, the
 *    2015 behaviour that made ~60 % of a fresh genome room-infeasible. */
#ifndef TYPED_INIT
#define TYPED_INIT 1
#endif

/* How far a mutation may move the *day* of a lesson.
 *
 * 0 redraws the day from all 195 days (the 2015 operator), so every mutated
 * gene teleports the lesson to a random point of the grid.  Measured: a
 * +/-5 / +/-20 / +/-50 window is *worse* (214 / 198 / 184 team-days at
 * SGA_REFINE=300, versus 185 for the uniform draw), because compacting a
 * timetable needs the jump "to a day my class already uses", and the uniform
 * draw proposes that jump far more often than a local one.  Default 0 = the
 * original operator; the constant exists so the ablation can be rebuilt. */
#ifndef MUT_DAY_WINDOW
#define MUT_DAY_WINDOW 0
#endif

#endif
