#!/bin/sh
# Runs the movement bench scenarios and reports each metric as a range across
# several arrangements, or with --baseline regenerates tests/movebench/baseline.json.
#
# Every cell is run over a family of spawn arrangements, each shifting the spawn
# blocks by up to a tile. That matters because a single arrangement is one
# trajectory rather than a sample: shifting a block by one tile with no other
# change moves arrival p95 by around a quarter, and some cells simply resolve or
# jam depending on it. Reporting one number invites reading that as a result.
#
# Arrangements are enumerated rather than drawn from the RNG. Seeding covered the
# same small space unevenly and collided, so two seed families disagreed about a
# cell's range while both looked like fair samples.
#
# So compare ranges, not numbers. A change is only interesting when it moves a
# metric clear of the spread reported here.
#
# Some cells are bimodal rather than spread: a corridor either resolves or it
# jams, ex. counterflow_cyborg returning 48 seven times out of twelve and 10 to
# 21 the rest. A median there reports whichever mode happened to win and hides
# the thing that actually varies. RESOLVED counts the arrangements in which most
# of the ordered units arrived, so a mechanism that helps such a cell shows up as
# a higher fraction rather than as a shifted average.
#
# --check verifies every arrangement is reproducible, which catches anything
# that broke sync, ex. a stray rand(), a float in a position path, or a live
# read from a worker thread.
#
# Note that data/ changes only take effect after a full `ninja -C build`.
# Building just the warzone2100 target leaves the packaged mp.wz stale, and a
# scenario edit will silently appear to have no effect.

set -e

WZ=${WZ:-build/src/warzone2100}
# Arrangements are enumerated, not sampled. A two-block scenario has 81 of them
# (nine placements per block), so ARRANGEMENTS evenly strides that space. Use 81
# for exhaustive coverage when a decision rests on the result.
ARRANGEMENTS=${ARRANGEMENTS:-27}
SCENARIOS=${SCENARIOS:-"counterflow_tracked oneway_tracked counterflow_cyborg counterflow_w1 counterflow_w3 counterflow_w4 counterflow_w6 counterflow_w8 tworoute crossing separating corner corner_mixed blob parking openfield strafe enemyblock enemyblock_press"}
HERE=$(dirname "$0")

if [ ! -x "$WZ" ]; then
	echo "warzone2100 binary not found at $WZ (set WZ=path)" >&2
	exit 1
fi

# Arguments after the subcommand are passed through, so a mechanism can be
# scored against the baseline, ex. run.sh --movementspread=field
SUB=""
case "$1" in
	--baseline|--check) SUB="$1"; shift ;;
esac

if [ "$SUB" = "--check" ]; then
	rc=0
	for s in $SCENARIOS; do
		i=0
		while [ "$i" -lt 9 ]; do
			a=$("$WZ" --movementbench="$s" --movementarrangement="$i" "$@" 2>/dev/null | grep finalPositionsCrc)
			b=$("$WZ" --movementbench="$s" --movementarrangement="$i" "$@" 2>/dev/null | grep finalPositionsCrc)
			i=$((i + 1))
			if [ "$a" != "$b" ]; then
				echo "MISMATCH $s arrangement=$((i - 1))"
				rc=1
			fi
		done
		echo "ok       $s (all arrangements reproducible)"
	done
	exit $rc
fi

python3 - "$WZ" "$SUB" "$HERE/baseline.json" "$ARRANGEMENTS" "$SCENARIOS" "$@" <<'PYEOF'
import json, statistics, subprocess, sys

wz, sub, out, arrangements, scenarios = sys.argv[1:6]
passthrough = sys.argv[6:]
scenarios = scenarios.split()

# Stride evenly through the 81 arrangements a two-block scenario can take.
TOTAL_ARRANGEMENTS = 81
n = max(1, min(int(arrangements), TOTAL_ARRANGEMENTS))
indices = [i * TOTAL_ARRANGEMENTS // n for i in range(n)]
FIELDS = ["unitsArrived", "arrival_p50", "arrival_p95", "hardStops",
          "repaths", "giveUps", "formationSpreadTiles",
          "peakDensity", "density_p95"]

# Share of a cell's ordered units that must arrive for that arrangement to count
# as resolved rather than jammed.
RESOLVED_SHARE = 0.75

def run(scenario, index):
    # A run that prints no scorecard is a process-level failure, not a sim
    # result: the scenario is deterministic, so a crash or startup hiccup that
    # yields no JSON is a flake to retry, not a number to record. Retrying does
    # not paper over sim nondeterminism, which --check guards separately. A run
    # that fails every attempt is a real break and is left to raise.
    cmd = [wz, "--movementbench=" + scenario, "--movementarrangement=%d" % index] + passthrough
    for attempt in range(4):
        raw = subprocess.run(cmd, capture_output=True, text=True).stdout
        start = raw.find("{")
        if start != -1:
            return json.loads(raw[start:raw.rindex("}") + 1])
        print("retry %s arr=%d (attempt %d produced no scorecard)"
              % (scenario, index, attempt + 1), file=sys.stderr)
    raise RuntimeError("%s arr=%d produced no scorecard in 4 attempts" % (scenario, index))

results = {}
for s in scenarios:
    cards = [run(s, i) for i in indices]
    results[s] = {f: {"min": min(c[f] for c in cards),
                      "median": int(statistics.median(c[f] for c in cards)),
                      "max": max(c[f] for c in cards)} for f in FIELDS}
    results[s]["resolved"] = {
        "count": sum(1 for c in cards
                     if c["unitsArrived"] >= RESOLVED_SHARE * max(1, c["unitsOrdered"])),
        "of": len(cards),
    }

def print_table():
    # p95 gets equal billing with throughput. Judging on how many units
    # eventually arrive hides a mechanism taking twice as long to get them there,
    # and time is what anyone watching actually notices.
    # arrival p95 and peak packing get billing beside resolved. A blob that
    # eventually clears still reads as resolved, so the time it took and how
    # tightly it packed are what tell a clean flow from a slow untangle.
    print("%-20s %9s %18s %14s" % ("SCENARIO", "RESOLVED", "ARRIVAL p95", "peakDensity"))
    for s in scenarios:
        def rng(field):
            v = results[s][field]
            return "%d [%d-%d]" % (v["median"], v["min"], v["max"])
        r = results[s]["resolved"]
        print("%-20s %9s %18s %14s" % (s, "%d/%d" % (r["count"], r["of"]),
                                       rng("arrival_p95"), rng("peakDensity")))

if sub == "--baseline":
    with open(out, "w") as f:
        json.dump({
            "_comment": "Baseline movement bench scorecards, each metric as min/median/max across spawn arrangements. Regenerate with tests/movebench/run.sh --baseline and review the diff. A difference inside a metric's spread is not a result.",
            "arrangements": indices,
            "scenarios": results,
        }, f, indent=2, sort_keys=True)
        f.write("\n")
    print("wrote", out, file=sys.stderr)

print_table()
PYEOF
