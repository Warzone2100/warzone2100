#!/bin/sh
# Runs every movement bench scenario and prints a scorecard table, or with
# --baseline regenerates tests/movebench/baseline.json.
#
# --check runs each scenario twice and fails if finalPositionsCrc differs, which
# catches anything that broke sync, ex. a stray rand(), a float in a position
# path, or a live read from a worker thread.
#
# Note that data/ changes only take effect after a full `ninja -C build`.
# Building just the warzone2100 target leaves the packaged mp.wz stale, and a
# scenario edit will silently appear to have no effect.

set -e

WZ=${WZ:-build/src/warzone2100}
SCENARIOS="counterflow_tracked oneway_tracked counterflow_cyborg counterflow_w1 counterflow_w3 counterflow_w4 crossing separating corner corner_mixed blob parking openfield strafe enemyblock enemyblock_press"
HERE=$(dirname "$0")

if [ ! -x "$WZ" ]; then
	echo "warzone2100 binary not found at $WZ (set WZ=path)" >&2
	exit 1
fi

card() { "$WZ" --movementbench="$1" 2>/dev/null | sed -n '/^{/,/^}/p'; }
field() { echo "$1" | grep "\"$2\"" | sed 's/.*: //;s/,//'; }

case "$1" in
--baseline)
	python3 - "$HERE/baseline.json" $SCENARIOS <<-'EOF'
	import json, subprocess, sys, os
	out, scenarios = sys.argv[1], sys.argv[2:]
	wz = os.environ.get("WZ", "build/src/warzone2100")
	cards = {}
	for s in scenarios:
	    raw = subprocess.run([wz, "--movementbench=" + s], capture_output=True, text=True).stdout
	    body = raw[raw.index("{"):raw.rindex("}") + 1]
	    d = json.loads(body)
	    d.pop("scenario", None)
	    cards[s] = d
	    print("recorded", s, file=sys.stderr)
	doc = {
	    "_comment": "Baseline movement bench scorecards. Regenerate with tests/movebench/run.sh --baseline after an intentional behavior change, and review the diff.",
	    "scenarios": cards,
	}
	with open(out, "w") as f:
	    json.dump(doc, f, indent=2, sort_keys=True)
	    f.write("\n")
	EOF
	echo "wrote $HERE/baseline.json"
	;;
--check)
	rc=0
	for s in $SCENARIOS; do
		a=$(field "$(card "$s")" finalPositionsCrc)
		b=$(field "$(card "$s")" finalPositionsCrc)
		if [ "$a" = "$b" ]; then
			echo "ok       $s crc=$a"
		else
			echo "MISMATCH $s crc=$a vs $b"
			rc=1
		fi
	done
	exit $rc
	;;
*)
	printf '%-20s %5s %5s %6s %6s %8s %6s %6s %5s %s\n' \
		SCENARIO ORD ARR p50 p95 HARDSTOP REPATH GIVEUP STUCK DONE
	for s in $SCENARIOS; do
		r=$(card "$s")
		printf '%-20s %5s %5s %6s %6s %8s %6s %6s %5s %s\n' "$s" \
			"$(field "$r" unitsOrdered)" "$(field "$r" unitsArrived)" \
			"$(field "$r" arrival_p50)" "$(field "$r" arrival_p95)" \
			"$(field "$r" hardStops)" "$(field "$r" repaths)" \
			"$(field "$r" giveUps)" "$(field "$r" stuckRemainingTiles_p50)" \
			"$(field "$r" completed)"
	done
	;;
esac
