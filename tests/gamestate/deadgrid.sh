#!/bin/sh
# Dead grid readers test: runs the dead_grid movement bench scenario,
# in which objects die in a tick where a grid reader that runs later in that tick would find them
# (the grid keeps them until the end of the tick), and checks that each reader skipped them.
# The scenario prints "deadgrid: <case>: ok" or "BUG", and the repair case would assert ("Droid dead").
# The gate case isn't checked (scripts can't read a gate's state).
#
# usage: deadgrid.sh <warzone2100 binary>
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 300 seconds).
# Exit status: 0 passed, 1 a reader read a dead object, 2 the run failed.
set -u
BIN=${1:?binary}
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-300}
CFG="$OUT/config"
LOG="$OUT/deadgrid.log"
mkdir -p "$CFG"

timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" --movementbench=dead_grid --gamestate-test-seed=1 > "$LOG" 2>&1 \
	|| { echo "deadgrid: RUN FAILED (see $LOG)"; exit 2; }

OK=$(grep -c 'deadgrid: [a-z-]*: ok' "$LOG")
BUGS=$(grep -c 'deadgrid: [a-z-]*: BUG' "$LOG")
DEADTARGET=$(grep -c 'actionDroidBase:[0-9]*\] Droid dead' "$LOG")
echo "deadgrid: $OK ok, $BUGS BUG, $DEADTARGET dead action target assert(s) (log: $LOG)"
grep 'deadgrid: [a-z-]*: BUG' "$LOG" | sed 's/^.*deadgrid: /  /'
# squish, helpbuild, helpbuild-rebuilt, buildpos
[ "$OK" -eq 4 ] && [ "$BUGS" -eq 0 ] && [ "$DEADTARGET" -eq 0 ] || exit 1
exit 0
