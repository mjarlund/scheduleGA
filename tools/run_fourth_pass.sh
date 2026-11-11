#!/bin/sh
# Fourth pass: every table in scheduleGA-report.md §16, in one go (~6 minutes).
#
#   ./tools/run_fourth_pass.sh [--quick]
#
# Needs gcc/make and python3.  The CP-SAT table needs ortools in a venv; see
# §16.9.  Nothing here touches git, network or result/ beyond the program's own
# output.
set -e
cd "$(dirname "$0")/.."

QUICK=""
[ "$1" = "--quick" ] && QUICK=1

echo "== build"
make -s both sga-tight sga-greedy sga-repair tools/fixed_seed.so

echo
echo "== §16.2/16.4 capacity bounds (no search)"
python3 tools/feasibility_bounds.py
for d in data-d7 data-d6 data-d5; do
    echo
    python3 tools/feasibility_bounds.py --data $d | sed -n '1p;/lower bound/p'
done

echo
echo "== §16.3 frontier on the committed dataset (5 seeds)"
python3 tools/frontier.py --label random --days 195 30 20 12 8 6 5 --seeds 1 2 3 4 5 \
    --env "SGA_REFINE=0"

if [ -z "$QUICK" ]; then
    echo
    echo "== §16.5 the tight instances, four builds x 5 seeds"
    for spec in d7:7:data-d7 d6:6:data-d6 d5:5:data-d5; do
        label=$(echo $spec | cut -d: -f1)
        days=$(echo $spec | cut -d: -f2)
        data=$(echo $spec | cut -d: -f3)
        for b in sga sga-greedy sga-repair sga-tight; do
            python3 tools/frontier.py --label $label-$b --days $days --seeds 1 2 3 4 5 \
                --binary ./$b \
                --env "SGA_DATA_DIR=$data;SGA_MAX_GENERATIONS=20000;SGA_STAGNATION=3000;SGA_REFINE=0" \
                | tail -3
        done
    done
fi

echo
echo "== §16.6 CP-SAT (skipped if ortools is not importable)"
PY=""
for cand in /tmp/ortsat/bin/python python3; do
    if $cand -c "from ortools.sat.python import cp_model" 2>/dev/null; then
        PY=$cand
        break
    fi
done
if [ -z "$PY" ]; then
    echo "ortools not importable: uv venv /tmp/ortsat && VIRTUAL_ENV=/tmp/ortsat uv pip install ortools"
else
    $PY tools/cpsat_solve.py --data data    --days 6 --time-limit 60 --out result/cpsat-committed-d6.csv
    $PY tools/cpsat_solve.py --data data-d7 --days 7 --time-limit 60 --out result/cpsat-d7.csv
    $PY tools/cpsat_solve.py --data data-d6 --days 6 --time-limit 60 --out result/cpsat-d6.csv
    $PY tools/cpsat_solve.py --data data-d5 --days 5 --time-limit 60 --out result/cpsat-d5.csv
    if [ -z "$QUICK" ]; then
        $PY tools/cpsat_solve.py --data data-d6 --days 6 --time-limit 120 \
            --objective team-days --out result/cpsat-d6-teamdays.csv
    fi
    echo
    echo "== independent check of the CP-SAT schedules"
    SGA_DATA_DIR=data-d5 python3 tools/quality.py --file result/cpsat-d5.csv | head -3
    if [ -z "$QUICK" ]; then
        SGA_DATA_DIR=data-d6 python3 tools/quality.py --file result/cpsat-d6-teamdays.csv | head -3
    fi
fi

echo
echo "== §16.7 default behaviour is unchanged (md5 1bec8111..., 6926fbe7..., 6cceaab2...)"
for s in 1 2 3; do
    printf '50\n' | SGA_SEED=$s LD_PRELOAD=./tools/fixed_seed.so ./sga > /dev/null
    md5sum result/result.csv
done
echo "done."
