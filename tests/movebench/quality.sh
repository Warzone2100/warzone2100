#!/bin/sh
# Compares the movement quality of two binaries over the bench suite, cell by cell.
#
# This is the acceptance test for a change that is allowed to alter the simulation. Once a change
# deliberately moves the sync stream, synccrc.sh can only fail, and the question becomes whether the
# game got worse. This script attempts to provide an indicator.
#
# The metrics are exact, not statistical. Every scenario is deterministic, so the same binary on the
# same arrangement produces byte-identical scorecards, and any difference reported here is real. Run
# --self first on a machine where that is in doubt: it runs every cell twice with one binary and must
# report nothing.
#
# Comparison is paired per cell, since arrangement variance dwarfs most real effects and unpaired
# medians hide changes a paired diff shows plainly. run.sh reports ranges across arrangements and is
# the tool for judging a mechanism in isolation. This one judges a diff.
#
# In the output, `cells` is how many cells changed the metric at all - the first thing to look at.
# `net` sums the per-cell deltas. `worst` names the cell that regressed most, which is where to look
# when net is small but cells is large. Journey rows are compared only where both sides kept the same
# population, see JOURNEY below. Every scorecard is written to $OUT/scorecards.json, so a row can be
# drilled into without running the suite again.
#
# The exit status fails only on units that never arrived, unitsNear falling or unitsNeverNear rising.
# Everything else is a judgement that belongs to a person.
#
# The scenario lists and arrangement counts match synccrc.sh, so the two gates cover the same cells.
#
# NOTE: data/ changes only take effect after a full `ninja -C build`.
set -e

SELF=0
if [ "$1" = "--self" ]; then
	SELF=1
	shift
	BASE=$1
	NEW=$1
else
	BASE=$1
	NEW=$2
fi

if [ -z "$BASE" ] || [ -z "$NEW" ]; then
	echo "usage: $0 <base-binary> <new-binary>" >&2
	echo "       $0 --self <binary>" >&2
	exit 2
fi
for bin in "$BASE" "$NEW"; do
	if [ ! -x "$bin" ]; then
		echo "warzone2100 binary not found at $bin" >&2
		exit 1
	fi
done

# Set either list to the empty string to skip it, ex. SCENARIOS= to run only the perf_* ones.
SCENARIOS=${SCENARIOS-"counterflow_tracked oneway_tracked counterflow_cyborg counterflow_w1 counterflow_w3 counterflow_w4 counterflow_w6 counterflow_w8 tworoute crossing separating corner corner_mixed blob parking openfield strafe enemyblock enemyblock_press counterflow_hostile mountain_chain mountain_chain_cross rush_turn rush_corner open_corner"}
PERF_SCENARIOS=${PERF_SCENARIOS-"perf_cyborg_blob perf_battle perf_walled_assault perf_artillery perf_vtol_rearm perf_scout_patrol perf_repair_search perf_repair_facility"}
ARRANGEMENTS=${ARRANGEMENTS:-9}
PERF_ARRANGEMENTS=${PERF_ARRANGEMENTS:-3}
CONCURRENT=${CONCURRENT:-1}
OUT=${OUT:-/tmp/movebenchquality}
mkdir -p "$OUT"

python3 - "$BASE" "$NEW" "$SELF" "$ARRANGEMENTS" "$PERF_ARRANGEMENTS" "$CONCURRENT" "$OUT" \
	"$SCENARIOS" "$PERF_SCENARIOS" <<'PYEOF'
import json, os, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

base, new = sys.argv[1], sys.argv[2]
self_mode = sys.argv[3] == "1"
arrangements, perf_arrangements = int(sys.argv[4]), int(sys.argv[5])
workers = max(1, int(sys.argv[6]))
out = sys.argv[7]
cells = [(s, i) for s in sys.argv[8].split() for i in range(arrangements)]
cells += [(s, i) for s in sys.argv[9].split() for i in range(perf_arrangements)]

# Every metric worth judging, with the direction that counts as better.
METRICS = [
    ("unitsNear",          +1),
    ("unitsNeverNear",     -1),
    ("unitsArrived",       +1),
    ("unitsRouteStarved",  -1),
    ("arrival_p50",        -1),
    ("arrival_p95",        -1),
    ("worstArrival_s",     -1),
    ("secPerTile_p50",     -1),
    ("secPerTile_p95",     -1),
    ("settle_p50_s",       -1),
    ("settle_p95_s",       -1),
    ("stallSharePct_p50",  -1),
    ("stallSharePct_p95",  -1),
    ("hardStops",          -1),
    ("hardStopDroids",     -1),
    ("giveUps",            -1),
    ("repaths",            -1),
    ("bumps",              -1),
]
# A drop in these means a unit did not get where it was sent.
HARD = ["unitsNear", "unitsNeverNear"]

# Percentiles and worst cases over the units of a run, comparable only when both sides kept the same
# population: a stranded unit does not merely shift such a figure, it replaces it, and three of these
# rows reverse sign depending on the rule. The counts total over every ordered unit, so the rule
# never leaves them out.
JOURNEY = {"arrival_p50", "arrival_p95", "worstArrival_s", "secPerTile_p50", "secPerTile_p95",
           "settle_p50_s", "settle_p95_s", "stallSharePct_p50", "stallSharePct_p95"}

# Each worker gets its own config dir. Sharing one lets two concurrent runs read
# and write the same settings file, which shows up as a single cell disagreeing
# for no reason and does not reproduce when that cell is re-run alone.
def config_dir(tag):
    d = os.path.join(out, "cfg-%s-%d" % (tag, id_of_thread()))
    os.makedirs(d, exist_ok=True)
    return d

import threading
_ids, _lock = {}, threading.Lock()
def id_of_thread():
    key = threading.current_thread().name
    with _lock:
        if key not in _ids:
            _ids[key] = len(_ids)
        return _ids[key]

def run(binary, tag, scenario, index):
    # A run that prints no scorecard is a process-level failure, not a result.
    # The scenarios are deterministic, so a real difference reproduces while a
    # startup hiccup does not.
    cmd = [binary, "--movementbench=" + scenario, "--movementarrangement=%d" % index,
           "--configdir=" + config_dir(tag)]
    for _ in range(4):
        raw = subprocess.run(cmd, capture_output=True, text=True).stdout
        start = raw.find("{")
        if start != -1:
            return json.loads(raw[start:raw.rindex("}") + 1])
        print("retry %s arr=%d" % (scenario, index), file=sys.stderr)
    raise RuntimeError("%s arr=%d produced no scorecard in 4 attempts" % (scenario, index))

def run_cell(cell):
    s, i = cell
    return cell, run(base, "base", s, i), run(new, "new", s, i)

pool = ThreadPoolExecutor(workers) if workers > 1 else None
results = (list(pool.map(run_cell, cells)) if pool else [run_cell(c) for c in cells])

with open(os.path.join(out, "scorecards.json"), "w") as f:
    json.dump([{"scenario": c[0], "arrangement": c[1], "base": b, "new": n} for c, b, n in results], f)

changed_cells = [c for c, b, n in results if b.get("finalPositionsCrc") != n.get("finalPositionsCrc")]
deltas = {m: [] for m, _ in METRICS}
skipped = {m: 0 for m, _ in METRICS}
for cell, b, n in results:
    same_population = (b.get("unitsArrived") == n.get("unitsArrived")
                       and b.get("unitsNear") == n.get("unitsNear")
                       and b.get("unitsArrived", 0) > 0)
    for m, _ in METRICS:
        if m not in b or m not in n or b[m] == n[m]:
            continue
        if m in JOURNEY and not same_population:
            skipped[m] += 1
            continue
        deltas[m].append((cell, n[m] - b[m], b[m], n[m]))

label = "the same binary twice" if self_mode else "base against new"
print("%d cells, %s" % (len(results), label))
print("final position CRC differs in %d of %d cells" % (len(changed_cells), len(results)))
print()

if self_mode:
    bad = len(changed_cells) or any(deltas[m] for m, _ in METRICS)
    if not bad:
        print("reproducible: every cell produced identical metrics on both runs")
        print("so any difference this gate reports between two binaries is a real one")
        sys.exit(0)
    print("NOT REPRODUCIBLE - this machine cannot judge a simulation change until this is fixed")
    for m, _ in METRICS:
        if deltas[m]:
            print("  %-20s differs in %d cells, ex. %s arr=%d %s to %s"
                  % (m, len(deltas[m]), deltas[m][0][0][0], deltas[m][0][0][1],
                     deltas[m][0][2], deltas[m][0][3]))
    sys.exit(1)

print("%-20s %7s %14s   %s" % ("metric", "cells", "net", "worst cell"))
print("%-20s %7s %14s   %s" % ("-" * 20, "-" * 7, "-" * 14, "-" * 34))
failed = []
for m, better in METRICS:
    d = deltas[m]
    if not d:
        note = "" if not skipped[m] else "  [%d cell(s) left out, not comparable]" % skipped[m]
        print("%-20s %7d %14s   %s%s" % (m, 0, "=", "", note))
        continue
    net = sum(x[1] for x in d)
    # Worst is the largest move in the direction that is not better.
    worst = min(d, key=lambda x: x[1] * better)
    worst_txt = ""
    if worst[1] * better < 0:
        worst_txt = "%s arr=%d  %s to %s" % (worst[0][0], worst[0][1], worst[2], worst[3])
    net_txt = "%+g" % net
    if net * better < 0:
        net_txt += " worse"
    elif net * better > 0:
        net_txt += " better"
    note = "" if not skipped[m] else "  [%d cell(s) left out, not comparable]" % skipped[m]
    print("%-20s %7d %14s   %s%s" % (m, len(d), net_txt, worst_txt, note))
    if m in HARD and net * better < 0:
        failed.append(m)

print()
if failed:
    print("FAIL: %s moved against the run" % ", ".join(failed))
    print("units that did not reach where they were sent are not paid for by anything else")
    sys.exit(1)
print("no unit failed to arrive that arrived before")
print("everything above is a judgement, not a verdict: read net beside cells, and read worst")
PYEOF
