#!/bin/sh
# GameState rewind fault test: a rewind test (see forkcrc.sh --rewind)
# whose snapshot is damaged right before it is restored (--gamestate-rewind-fault),
# to see where the in-process restore fails and what it leaves behind.
#
#   run A: an uninterrupted run, tracing sync CRCs (the reference)
#   run B: the same run with --gamestate-rewind-test=<tick>:<k> and the fault
#
# Prints one line: what the fault did, and the outcome:
#   - "rejected, live game went on": the restore failed before it changed any live state,
#     and run B went on with the live game.
#     Its trace is compared with run A's over the whole run (it must be IDENTICAL).
#   - "restored": the restore succeeded anyway.
#     The ticks after the snapshot are compared with run A's,
#     and the number of references the restore could not resolve is shown.
#   - "failed at stage <stage>": the restore failed after it had changed live state.
#     Shows run B's exit status, the last tick it traced, and the assertions logged after the failure
#     (with ",continue" in the kind, the half-restored game is ticked on instead of quitting).
#
# usage: rewindfault.sh <warzone2100 binary> <test> <tick> <end tick> <k> <fault kind>
#   <test> as forkcrc.sh, <fault kind> as --gamestate-rewind-fault (see src/gamestate_harness.h)
# Environment: OUT, RUN_TIMEOUT, SEED, EXTRA as forkcrc.sh.
# Exit status: 0 the outcome is one a restore should give (rejected with an identical run, or restored identical),
#              1 anything else, 2 a run failed to start or the fault was not applied.
set -u
BIN=${1:?binary}
TEST=${2:?test}
TICK=${3:?tick}
ENDTICK=${4:?end tick}
K=${5:?ticks later}
FAULT=${6:?fault kind}
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-600}
SEED=${SEED:-1}
CFG="$OUT/config"
mkdir -p "$CFG"
case "$TEST" in
	mb:*) START="--movementbench=${TEST#mb:}" ;;
	cam:*) START="--game=${TEST#cam:}" ;;
	*) START="--skirmish=$TEST.json" ;;
esac
COMMON="--configdir=$CFG --headless --autogame --gamestate-test-seed=$SEED ${EXTRA:-}"
case "$TEST" in mb:*) COMMON="--configdir=$CFG --gamestate-test-seed=$SEED ${EXTRA:-}" ;; esac
TAG="$(echo "$TEST" | tr ':' '_').$TICK.rewind$K.fault_$(echo "$FAULT" | tr ':,' '__')"

# The reference run is shared by every fault kind of the same cell
REF="$OUT/$(echo "$TEST" | tr ':' '_').$TICK.$ENDTICK.reference"
if [ ! -s "$REF.trace" ]; then
	# shellcheck disable=SC2086
	timeout "$RUN_TIMEOUT" "$BIN" $COMMON "$START" --gamestate-crc-trace="$REF.trace" --gamestate-exit-at-tick="$ENDTICK" \
		> "$REF.log" 2>&1 || { rm -f "$REF.trace"; echo "$TAG: REFERENCE RUN FAILED"; exit 2; }
fi
# shellcheck disable=SC2086
timeout "$RUN_TIMEOUT" "$BIN" $COMMON "$START" --gamestate-crc-trace="$OUT/$TAG.B.trace" --gamestate-exit-at-tick="$ENDTICK" \
	--gamestate-rewind-test="$TICK:$K" --gamestate-rewind-fault="$FAULT" > "$OUT/$TAG.B.log" 2>&1
STATUS=$?
LOG="$OUT/$TAG.B.log"
WHAT=$(sed -n 's/.*GameState harness: rewind fault [^ ]*: \(.*\)$/\1/p' "$LOG" | head -1)
if [ -z "$WHAT" ]; then
	echo "$TAG: FAULT NOT APPLIED (exit $STATUS; see $LOG)"
	grep 'rewind fault' "$LOG" | head -1 | sed 's/^.*GameState harness: /  /'
	exit 2
fi
AT=$(sed -n 's/.*rewind snapshot taken at gameTime \([0-9]*\).*/\1/p' "$LOG" | head -1)
LAST=$(tail -1 "$OUT/$TAG.B.trace" 2>/dev/null | cut -d' ' -f1)
if grep -q 'rewind restore FAILED before changing any live state' "$LOG"; then
	ERR=$(sed -n 's/.*FAILED before changing any live state ([^)]*): \(.*\); the live game goes on$/\1/p' "$LOG" | head -1)
	RESULT=$(python3 -I "$HERE/cmptrace.py" "$REF.trace" "$OUT/$TAG.B.trace" --after 0 --last)
	RC=$?
	echo "$TAG: $WHAT -> rejected, live game went on (\"$ERR\"); run vs reference: $RESULT"
	exit $RC
elif grep -q 'rewound from gameTime' "$LOG"; then
	UNRESOLVED=$(sed -n 's/.*rewound from gameTime [0-9]* to gameTime [0-9]* (\([0-9]*\) unresolved.*/\1/p' "$LOG" | head -1)
	RESULT=$(python3 -I "$HERE/cmptrace.py" "$REF.trace" "$OUT/$TAG.B.trace" --after "$AT" --last)
	RC=$?
	echo "$TAG: $WHAT -> restored ($UNRESOLVED unresolved reference(s)); after the snapshot at $AT vs reference: $RESULT"
	exit $RC
else
	STAGE=$(sed -n 's/.*rewind restore FAILED at stage \([a-z0-9]*\),.*/\1/p' "$LOG" | head -1)
	ERR=$(sed -n 's/.*after the live state was changed ([^)]*): \(.*\)$/\1/p' "$LOG" | head -1)
	ASSERTS=$(sed -n '/rewind restore FAILED at stage/,$p' "$LOG" | grep -c 'Assert in Warzone')
	FIRSTASSERT=$(sed -n '/rewind restore FAILED at stage/,$p' "$LOG" | grep -m1 'Assert in Warzone' | sed 's/^[^[]*\[\([^]]*\)\].*Assert in Warzone: [^ ]*\/\([^/ ]*:[0-9]*\) \(.*\), last script.*/\1 \2 \3/')
	CRASH=
	[ "$STATUS" -ge 128 ] && CRASH=" (signal $((STATUS - 128)))"
	echo "$TAG: $WHAT -> failed at stage ${STAGE:-?} (\"$ERR\"); exit $STATUS$CRASH, last traced tick ${LAST:-none}, $ASSERTS assert(s) after the failure${FIRSTASSERT:+; first: $FIRSTASSERT}"
	exit 1
fi
