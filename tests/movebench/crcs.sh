#!/bin/sh
# Captures finalPositionsCrc for every scenario and arrangement, one per line.
#
# Use this on a change that is meant to alter nothing. Capture before,
# capture after, diff the two. An identical set means every unit finished every
# scenario on exactly the tile it finished on before, which is a statement about
# the whole simulation trajectory rather than about the metrics run.sh reports.
# Two runs can post the same arrival counts while having moved differently.
#
# Use it on refactors, where the expected result is no diff at all. A change
# meant to improve movement will differ here by design, and run.sh is what
# scores whether it improved.
#
# It compares a binary against another build of itself on one machine, so it
# says nothing about whether the two agree with a different compiler or a
# different architecture. Only running the suite on those can answer that.
#
# Note that data/ changes only take effect after a full `ninja -C build`.
set -e

WZ=${WZ:-build/src/warzone2100}
SCENARIOS=${SCENARIOS:-"counterflow_tracked oneway_tracked counterflow_cyborg counterflow_w1 counterflow_w3 counterflow_w4 counterflow_w6 counterflow_w8 tworoute crossing separating corner corner_mixed blob parking openfield strafe enemyblock enemyblock_press"}
ARRANGEMENTS=${ARRANGEMENTS:-9}

if [ ! -x "$WZ" ]; then
	echo "warzone2100 binary not found at $WZ (set WZ=path)" >&2
	exit 1
fi

for s in $SCENARIOS; do
	i=0
	while [ "$i" -lt "$ARRANGEMENTS" ]; do
		crc=$("$WZ" --movementbench="$s" --movementarrangement="$i" "$@" 2>/dev/null \
			| sed -n 's/.*"finalPositionsCrc": *\([0-9]*\).*/\1/p')
		if [ -z "$crc" ]; then
			echo "$s $i FAILED" >&2
			exit 1
		fi
		echo "$s $i $crc"
		i=$((i + 1))
	done
done
