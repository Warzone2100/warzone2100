#!/bin/sh
# Compares the per-tick sync CRC trace of two binaries over the movement bench suite.
#
# This is the strong identity check. crcs.sh only compares where every unit ended up. The trace
# compared here is the same syncDebug() CRC multiplayer uses to detect a desync, so it covers
# everything the simulation logs.
#
# A change that is meant to alter nothing must be IDENTICAL on every cell. Anything else means the
# change is not result-identical, and the tick number printed is where to start looking.
#
# The standard scenarios are movement-only: no shot is ever fired in them. Combat, visibility,
# targeting and projectile changes need the perf_* scenarios.
#
# Both binaries must be built with the same compiler and flags, as for crcs.sh.
#
# NOTE: data/ changes only take effect after a full `ninja -C build`.
set -e

BASE=$1
NEW=$2
if [ -z "$BASE" ] || [ -z "$NEW" ]; then
	echo "usage: $0 <base-binary> <new-binary>" >&2
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
OUT=${OUT:-/tmp/synccrc}
mkdir -p "$OUT"

status=0

# A run that dies without producing a trace is a process-level failure, not a simulation result, so it
# is retried. Scenarios are deterministic: a real difference reproduces, while a startup hiccup does not.
run_one() {
	bin=$1; tag=$2; scenario=$3; arrangement=$4
	attempt=0
	while [ "$attempt" -lt 3 ]; do
		if "$bin" --movementbench="$scenario" --movementarrangement="$arrangement" \
			--gamestate-crc-trace="$OUT/$scenario.$arrangement.$tag.txt" >/dev/null 2>&1 \
			&& [ -s "$OUT/$scenario.$arrangement.$tag.txt" ]; then
			return 0
		fi
		attempt=$((attempt + 1))
	done
	return 1
}

compare_suite() {
	arrangements=$1
	shift
	for s in "$@"; do
		i=0
		while [ "$i" -lt "$arrangements" ]; do
			if ! run_one "$BASE" base "$s" "$i" || ! run_one "$NEW" new "$s" "$i"; then
				echo "$s $i RUN FAILED"
				status=1
				i=$((i + 1))
				continue
			fi
			b="$OUT/$s.$i.base.txt"
			n="$OUT/$s.$i.new.txt"
			if cmp -s "$b" "$n"; then
				echo "$s $i IDENTICAL ($(wc -l < "$b" | tr -d ' ') ticks)"
			else
				tick=$(diff "$b" "$n" | sed -n 's/^< \([0-9]*\) .*/\1/p' | head -1)
				echo "$s $i DIFFERS at tick ${tick:-?}"
				status=1
			fi
			i=$((i + 1))
		done
	done
}

compare_suite "$ARRANGEMENTS" $SCENARIOS
compare_suite "$PERF_ARRANGEMENTS" $PERF_SCENARIOS

exit $status
