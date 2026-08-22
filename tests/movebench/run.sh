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
# of the ordered units reached their destination (unitsNear, six tiles), so a
# mechanism that helps such a cell shows up as a higher fraction rather than as
# a shifted average.
#
# --score reports each metric beside its change against the recorded baseline,
# or against a live reference run when --against='<args>' names one, ex.
#   run.sh --score --markdown --pathfindingbackend=2
# scores that feature set against the plain baseline and prints the table as
# GitHub markdown, ready to paste into a pull request.
#
# --acceptance switches to the real-map acceptance scenarios, judged by summed
# hard stops over their arrangements rather than medians, against
# tests/movebench/acceptance-baseline.json. These are heavy runs.
#
# The acceptance table's two arrival counts answer different questions. `near`
# allows six tiles and says whether a unit got to its destination. `parked`
# allows a tile and a half, the movement code's own slack, so it measures how
# tightly a block settled around a goal tile it cannot all fit on - a change in
# it is packing, not transit.
#
# `settle` reads the journey: the last tick a unit sat outside six tiles of its
# goal, blind to the endgame. `arrival` reads at a tile and a half, so it is
# the only measure that sees the endgame - but it is computed over the units
# that got there, so read it beside `near`: if near falls, a lower arrival may
# be a unit abandoned rather than a tail fixed. `worst arrival` guards exactly
# that by reporting the whole run length whenever any unit finishes outside six
# tiles.
#
# --concurrent=N keeps up to N game processes running at once. Every run is an
# independent deterministic process whose scorecard comes back on its own
# stdout, so concurrency changes wall-clock and nothing else.
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

# Arguments after the mode flags are passed through, so a mechanism can be
# scored against the baseline, ex. run.sh --score --movementspread=field
SUB=""
MARKDOWN=0
ACCEPTANCE=0
HAVE_AGAINST=0
AGAINST=""
CONCURRENT=1
# Mode flags are recognized anywhere on the line. Anything unrecognized is
# collected, in order, as pass-through for the game binary, so a flag under
# test can sit before or after --against without being handed to the game.
REMAINING=$#
while [ $REMAINING -gt 0 ]; do
	arg="$1"
	shift
	case "$arg" in
		--baseline|--check|--score) SUB="$arg" ;;
		--markdown) MARKDOWN=1 ;;
		--acceptance) ACCEPTANCE=1 ;;
		--against=*) AGAINST="${arg#--against=}"; HAVE_AGAINST=1 ;;
		--concurrent=*) CONCURRENT="${arg#--concurrent=}" ;;
		*) set -- "$@" "$arg" ;;
	esac
	REMAINING=$((REMAINING - 1))
done

if [ "$SUB" = "--check" ]; then
	rc=0
	for s in $SCENARIOS; do
		i=0
		while [ "$i" -lt 9 ]; do
			# A run that prints no CRC is the known shutdown flake, a
			# process-level failure to retry, same policy as the sweep runner.
			# Four straight failures is a real break and stops the check.
			a=""; tries=0
			while [ -z "$a" ]; do
				tries=$((tries + 1))
				if [ "$tries" -gt 4 ]; then echo "$s $i produced no CRC in 4 attempts" >&2; exit 1; fi
				a=$("$WZ" --movementbench="$s" --movementarrangement="$i" "$@" 2>/dev/null | grep finalPositionsCrc || true)
			done
			b=""; tries=0
			while [ -z "$b" ]; do
				tries=$((tries + 1))
				if [ "$tries" -gt 4 ]; then echo "$s $i produced no CRC in 4 attempts" >&2; exit 1; fi
				b=$("$WZ" --movementbench="$s" --movementarrangement="$i" "$@" 2>/dev/null | grep finalPositionsCrc || true)
			done
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

python3 - "$WZ" "$SUB" "$HERE/baseline.json" "$ARRANGEMENTS" "$SCENARIOS" \
	"$MARKDOWN" "$ACCEPTANCE" "$HAVE_AGAINST" "$AGAINST" "$HERE/acceptance-baseline.json" "$CONCURRENT" "$@" <<'PYEOF'
import json, statistics, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

wz, sub, baseline_path, arrangements, scenarios = sys.argv[1:6]
markdown = sys.argv[6] == "1"
acceptance = sys.argv[7] == "1"
have_against = sys.argv[8] == "1"
against_args = sys.argv[9].split()
acceptance_baseline_path = sys.argv[10]
workers = max(1, int(sys.argv[11]))
passthrough = sys.argv[12:]
scenarios = scenarios.split()

# Stride evenly through the 81 arrangements a two-block scenario can take.
TOTAL_ARRANGEMENTS = 81
n = max(1, min(int(arrangements), TOTAL_ARRANGEMENTS))
indices = [i * TOTAL_ARRANGEMENTS // n for i in range(n)]
FIELDS = ["unitsArrived", "unitsNear", "arrival_p50", "arrival_p95", "hardStops",
          "repaths", "giveUps", "formationSpreadTiles",
          "peakDensity", "density_p95"]
# Reported as a float, so kept out of FIELDS above, which casts its medians to
# int. secPerTile is arrival normalized by leg distance, the number the grind
# column scores against the open-field floor.
FLOAT_FIELDS = ["secPerTile_p95"]

# Share of a cell's ordered units that must arrive for that arrangement to count
# as resolved rather than jammed.
RESOLVED_SHARE = 0.75

# The real-map acceptance pair plus the corner cases, judged by summed hard
# stops across their arrangements, never a single arrangement: several of these
# are bimodal, and a single-arrangement jump can be pure mode reshuffling while
# the sums sit within noise.
ACCEPTANCE_SCENARIOS = ["mountain_chain", "mountain_chain_cross", "rush_turn",
                        "rush_corner", "open_corner"]
ACCEPTANCE_INDICES = [0, 1, 2, 3, 4]
ACCEPTANCE_FIELDS = ["hardStops", "unitsArrived", "unitsNear", "repaths"]
# Aggregated as the max across arrangements, not a sum: these are shape
# signals rather than volumes, and the worst arrangement is the diagnostic one.
ACCEPTANCE_MAX_FIELDS = ["hardStopsTopSharePct", "settle_p95_s", "stallSharePct_p50",
                         "detourPct_p95", "arrival_p95_s", "worstArrival_s"]
# A central measure, so neither summed nor maxed: the median arrangement's
# median arrival is what a run typically costs.
ACCEPTANCE_MEDIAN_FIELDS = ["arrival_p50_s", "settle_p50_s"]

def run(scenario, index, extra):
    # A run that prints no scorecard is a process-level failure, not a sim
    # result: the scenario is deterministic, so a crash or startup hiccup that
    # yields no JSON is a flake to retry, not a number to record. Retrying does
    # not paper over sim nondeterminism, which --check guards separately. A run
    # that fails every attempt is a real break and is left to raise.
    cmd = [wz, "--movementbench=" + scenario, "--movementarrangement=%d" % index] + extra
    for attempt in range(4):
        raw = subprocess.run(cmd, capture_output=True, text=True).stdout
        start = raw.find("{")
        if start != -1:
            return json.loads(raw[start:raw.rindex("}") + 1])
        print("retry %s arr=%d (attempt %d produced no scorecard)"
              % (scenario, index, attempt + 1), file=sys.stderr)
    raise RuntimeError("%s arr=%d produced no scorecard in 4 attempts" % (scenario, index))

# Runs are independent processes collected by index, so the pool changes
# wall-clock and nothing else.
pool = ThreadPoolExecutor(workers) if workers > 1 else None

def run_all(scenario, idxs, extra):
    if pool is None:
        return [run(scenario, i, extra) for i in idxs]
    return list(pool.map(lambda i: run(scenario, i, extra), idxs))

def delta(value, ref):
    # Change against the reference, as a percentage when the reference can
    # carry one and as an absolute step from zero when it cannot. One decimal,
    # truncated toward zero rather than rounded, so -100.0% always means all
    # of it gone and never a nonzero residue rounded away.
    if ref == 0:
        return "(%+.0f)" % (value - ref) if value != ref else "(=)"
    return "(%+.1f%%)" % (int(1000.0 * (value - ref) / ref) / 10.0)

if acceptance:
    def sweep(extra):
        out = {}
        for s in ACCEPTANCE_SCENARIOS:
            cards = run_all(s, ACCEPTANCE_INDICES, extra)
            out[s] = {f: sum(c[f] for c in cards) for f in ACCEPTANCE_FIELDS}
            out[s].update({f: max(c.get(f, 0) for c in cards) for f in ACCEPTANCE_MAX_FIELDS})
            out[s].update({f: statistics.median(c[f] for c in cards)
                           for f in ACCEPTANCE_MEDIAN_FIELDS})
        return out

    results = sweep(passthrough)
    if sub == "--baseline":
        with open(acceptance_baseline_path, "w") as f:
            json.dump({
                "_comment": "Acceptance scenario sums across arrangements 0-4, recorded with no pathfinding features. Regenerate with tests/movebench/run.sh --acceptance --baseline.",
                "arrangements": ACCEPTANCE_INDICES,
                "scenarios": results,
            }, f, indent=2, sort_keys=True)
            f.write("\n")
        print("wrote", acceptance_baseline_path, file=sys.stderr)

    ref = None
    if have_against:
        ref = sweep(against_args)
    elif sub == "--score":
        with open(acceptance_baseline_path) as f:
            ref = json.load(f)["scenarios"]

    def cell(s, field):
        v = results[s][field]
        num = "%g" % v if isinstance(v, float) else "%d" % v
        r = ref[s].get(field) if ref is not None else None
        if r is None:
            # A baseline recorded before the field existed scores without it.
            return num
        return "%s %s" % (num, delta(v, r))

    ACCEPTANCE_COLUMNS = [
        ("hard stops (sum)", "hardStops"),
        ("top share % (max)", "hardStopsTopSharePct"),
        ("arrival p50 s (med)", "arrival_p50_s"),
        ("arrival p95 s (max)", "arrival_p95_s"),
        ("worst arrival s (max)", "worstArrival_s"),
        ("settle p50 s (med)", "settle_p50_s"),
        ("settle p95 s (max)", "settle_p95_s"),
        ("stall p50 % (max)", "stallSharePct_p50"),
        ("detour p95 % (max)", "detourPct_p95"),
        ("near 6t (sum)", "unitsNear"),
        ("parked 1.5t (sum)", "unitsArrived"),
        ("repaths", "repaths"),
    ]
    if markdown:
        print("| scenario | %s |" % " | ".join(h for h, _ in ACCEPTANCE_COLUMNS))
        print("|---%s" % ("|---" * len(ACCEPTANCE_COLUMNS) + "|"))
        for s in ACCEPTANCE_SCENARIOS:
            print("| %s | %s |" % (s, " | ".join(cell(s, f) for _, f in ACCEPTANCE_COLUMNS)))
    else:
        print("%-22s%s" % ("SCENARIO", "".join("%20s" % h for h, _ in ACCEPTANCE_COLUMNS)))
        for s in ACCEPTANCE_SCENARIOS:
            print("%-22s%s" % (s, "".join("%20s" % cell(s, f) for _, f in ACCEPTANCE_COLUMNS)))
    sys.exit(0)

def sweep(extra):
    # Free-travel floor, in seconds per tile, from the open-field scenario over
    # the same arrangements. A cell's grind is its loaded secPerTile against
    # this, so a mechanism only counts when it pulls a pinch back toward free
    # travel rather than merely arriving before the budget runs out.
    floor_cards = run_all("openfield", indices, extra)
    floor = statistics.median(c["secPerTile_p50"] for c in floor_cards)
    results = {}
    for s in scenarios:
        cards = run_all(s, indices, extra)
        results[s] = {f: {"min": min(c[f] for c in cards),
                          "median": int(statistics.median(c[f] for c in cards)),
                          "max": max(c[f] for c in cards)} for f in FIELDS}
        for f in FLOAT_FIELDS:
            results[s][f] = {"min": min(c[f] for c in cards),
                             "median": statistics.median(c[f] for c in cards),
                             "max": max(c[f] for c in cards)}
        # Counted on unitsNear, not unitsArrived: the latter's tile-and-a-half
        # tolerance measures how tightly a block packed onto a taken goal tile,
        # not whether the flow resolved.
        results[s]["resolved"] = {
            "count": sum(1 for c in cards
                         if c["unitsNear"] >= RESOLVED_SHARE * max(1, c["unitsOrdered"])),
            "of": len(cards),
        }
    return floor, results

floor, results = sweep(passthrough)

ref_floor, ref = None, None
if have_against:
    ref_floor, ref = sweep(against_args)
elif sub == "--score":
    with open(baseline_path) as f:
        b = json.load(f)
    ref_floor, ref = b["freeTravelFloorSecPerTile"], b["scenarios"]

def print_table():
    # Time gets equal billing with throughput. Judging on how many units
    # eventually arrive hides a mechanism taking minutes to get them there, and
    # time is what anyone watching actually notices. Arrival p95 is shown in
    # seconds at normal speed, and grind is that time per tile against the
    # free-travel floor, so a slow untangle reads as a large multiple where the
    # raw arrival time cannot tell it apart from a longer route.
    print("free-travel floor: %.2f s/tile (open field)" % floor)
    print("%-20s %9s %14s %14s %12s %16s" % ("SCENARIO", "RESOLVED", "ARRIVAL p95 s",
                                             "GRIND xfloor", "peakDensity", "hardStops"))
    for s in scenarios:
        def rng(field):
            v = results[s][field]
            return "%d [%d-%d]" % (v["median"], v["min"], v["max"])
        def rng_seconds(field):
            v = results[s][field]
            return "%.0f [%.0f-%.0f]" % (v["median"] / 10.0, v["min"] / 10.0, v["max"] / 10.0)
        def rng_grind(field):
            v = results[s][field]
            return "%.1f [%.1f-%.1f]" % (v["median"] / floor, v["min"] / floor, v["max"] / floor)
        r = results[s]["resolved"]
        print("%-20s %9s %14s %14s %12s %16s" % (s, "%d/%d" % (r["count"], r["of"]),
                                                 rng_seconds("arrival_p95"),
                                                 rng_grind("secPerTile_p95"),
                                                 rng("peakDensity"),
                                                 rng("hardStops")))

def print_score():
    # Scored mode trades the ranges for change columns: medians beside their
    # movement against the reference. Ranges still matter for a close call, so
    # rerun without --score when a delta sits near the spread.
    rows = []
    for s in scenarios:
        r = results[s]["resolved"]
        rr = ref[s]["resolved"]
        med = lambda f: results[s][f]["median"]
        rmed = lambda f: ref[s][f]["median"]
        rows.append((s,
                     "%d/%d (ref %d/%d)" % (r["count"], r["of"], rr["count"], rr["of"]),
                     "%.0f %s" % (med("arrival_p95") / 10.0,
                                  delta(med("arrival_p95") / 10.0, rmed("arrival_p95") / 10.0)),
                     "%.1f %s" % (med("secPerTile_p95") / floor,
                                  delta(med("secPerTile_p95") / floor, rmed("secPerTile_p95") / ref_floor)),
                     "%d %s" % (med("peakDensity"), delta(med("peakDensity"), rmed("peakDensity"))),
                     "%d %s" % (med("hardStops"), delta(med("hardStops"), rmed("hardStops")))))
    if markdown:
        print("| scenario | resolved | arrival p95 s | grind xfloor | peak density | hard stops |")
        print("|---|---|---|---|---|---|")
        for row in rows:
            print("| %s | %s | %s | %s | %s | %s |" % row)
    else:
        print("free-travel floor: %.2f s/tile (reference %.2f)" % (floor, ref_floor))
        print("%-20s %18s %16s %14s %16s %18s" % ("SCENARIO", "RESOLVED", "ARR p95 s",
                                                  "GRIND", "peakDensity", "hardStops"))
        for row in rows:
            print("%-20s %18s %16s %14s %16s %18s" % row)

if sub == "--baseline":
    with open(baseline_path, "w") as f:
        json.dump({
            "_comment": "Baseline movement bench scorecards, each metric as min/median/max across spawn arrangements. Regenerate with tests/movebench/run.sh --baseline and review the diff. A difference inside a metric's spread is not a result.",
            "arrangements": indices,
            "freeTravelFloorSecPerTile": floor,
            "scenarios": results,
        }, f, indent=2, sort_keys=True)
        f.write("\n")
    print("wrote", baseline_path, file=sys.stderr)

if ref is not None:
    print_score()
else:
    print_table()
PYEOF
