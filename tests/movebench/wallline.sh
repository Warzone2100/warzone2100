#!/bin/sh
# Runs the wallline scenario over every layout and spawn arrangement and sums
# how much of the army strayed in front of the wall line, per layout and in
# total. Lower is better. Arguments pass through to the game binary, so two
# feature sets compare as two invocations, ex.
#   tests/movebench/wallline.sh
#   tests/movebench/wallline.sh --pathfindingbackend=65531
#
# A single arrangement is one trajectory and its result is noisy: a droid
# turning around in a gap either arcs out or it does not. Compare the sums
# over all 234 arrangements, and treat differences of a few tens of percent
# with suspicion unless they hold per layout.
#
# FIRST=N starts at arrangement N, ex. FIRST=54 to skip the first six layouts.
#
# --concurrent=N keeps N game processes running at once. Scenario data only
# takes effect after a full `ninja -C build`.

WZ=${WZ:-build/src/warzone2100}
ARRANGEMENTS=${ARRANGEMENTS:-234}
FIRST=${FIRST:-0}
CONCURRENT=1
REMAINING=$#
while [ $REMAINING -gt 0 ]; do
	arg="$1"
	shift
	case "$arg" in
		--concurrent=*) CONCURRENT="${arg#--concurrent=}" ;;
		*) set -- "$@" "$arg" ;;
	esac
	REMAINING=$((REMAINING - 1))
done

if [ ! -x "$WZ" ]; then
	echo "warzone2100 binary not found at $WZ (set WZ=path)" >&2
	exit 1
fi

seq "$FIRST" $((ARRANGEMENTS - 1)) | xargs -P "$CONCURRENT" -I{} sh -c \
	'"$0" --movementbench=wallline --movementarrangement={} "$@" 2>&1 >/dev/null | grep "^wallline: total"' "$WZ" "$@" |
awk '
{
	for (i = 1; i <= NF; i++) { split($i, kv, "="); v[kv[1]] = kv[2] }
	l = v["layout"]; runs[l]++; ever[l] += v["everFront"]; secs[l] += v["frontSeconds"]; deep[l] += v["deepSeconds"]
	if (v["peakFront"] > peak[l]) peak[l] = v["peakFront"]
	if (v["maxDepth"] > depth[l]) depth[l] = v["maxDepth"]
}
END {
	printf "%-7s %5s %10s %8s %13s %9s %12s\n", "LAYOUT", "RUNS", "everFront", "maxPeak", "frontSeconds", "maxDepth", "deepSeconds"
	for (l = 0; l < 64; l++) if (runs[l]) {
		printf "%-7s %5d %10d %8d %13.1f %9d %12.1f\n", l, runs[l], ever[l], peak[l], secs[l], depth[l], deep[l]
		tr += runs[l]; te += ever[l]; ts += secs[l]; td += deep[l]
		if (depth[l] > tdepth) tdepth = depth[l]
	}
	printf "%-7s %5d %10d %8s %13.1f %9d %12.1f\n", "total", tr, te, "", ts, tdepth, td
}'
