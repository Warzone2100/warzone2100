#!/bin/sh
# Replay rewind test: record a skirmish game, play its replay back with an in-process rewind
# (--gamestate-rewind-test during --loadreplay), and compare the playback with the recording.
#
#   recording: a headless skirmish game to <end tick>, tracing sync CRCs
#              (it records a replay as any skirmish game does)
#   playback:  the replay played back, plainly. It must reproduce the recording's CRCs.
#   rewound:   the replay played back with a snapshot at <tick>, restored <k> ticks later.
#              After the snapshot its CRCs must be the recording's.
#              It runs with no exit tick,
#              so it must reach the replay's end after the rewind and quit with status 0.
#
# The recording is shared by every cell of the same test and end tick.
#
# With FAULT set to a --gamestate-rewind-fault kind (replay-position, no-replay-positions),
# the snapshot is damaged before the restore.
# The restore must be rejected before it changes anything,
# and the rewound playback (no rewind then) must reproduce the recording over the whole run
# and still reach the replay's end.
#
# usage: replayrewind.sh <warzone2100 binary> <skirmish test> <tick> <end tick> <k>
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 600 seconds), SEED (default 1), FAULT.
# Exit status: 0 both identical, 1 a difference, 2 a run failed.
set -u
BIN=${1:?binary}
TEST=${2:?skirmish test}
TICK=${3:?tick}
ENDTICK=${4:?end tick}
K=${5:?ticks later}
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-600}
SEED=${SEED:-1}
FAULT=${FAULT:-}
TAG="replay_$TEST.$TICK.rewind$K${FAULT:+.fault_$FAULT}"
CFG="$OUT/replaycfg_$TEST.$ENDTICK"
REC="$OUT/replay_$TEST.$ENDTICK.rec"
if [ ! -s "$REC.trace" ] || [ -z "$(ls "$CFG/replay/skirmish/" 2>/dev/null)" ]; then
	rm -rf "$CFG"
	mkdir -p "$CFG"
	timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" --headless --autogame --skirmish="$TEST.json" --gamestate-test-seed="$SEED" \
		--gamestate-crc-trace="$REC.trace" --gamestate-exit-at-tick="$ENDTICK" > "$REC.log" 2>&1 \
		|| { rm -f "$REC.trace"; echo "$TAG: RECORDING FAILED (see $REC.log)"; exit 2; }
fi
REPLAY=$(ls "$CFG/replay/skirmish/" | head -1)
[ -n "$REPLAY" ] || { echo "$TAG: NO REPLAY RECORDED"; exit 2; }
if [ ! -s "$REC.play.trace" ]; then
	timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" --headless --loadreplay="${REPLAY%.wzrp}" --gamestate-crc-trace="$REC.play.trace" \
		--gamestate-exit-at-tick="$ENDTICK" > "$REC.play.log" 2>&1 || { rm -f "$REC.play.trace"; echo "$TAG: PLAYBACK FAILED (see $REC.play.log)"; exit 2; }
fi
timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" --headless --loadreplay="${REPLAY%.wzrp}" --gamestate-crc-trace="$OUT/$TAG.trace" \
	--gamestate-rewind-test="$TICK:$K" ${FAULT:+--gamestate-rewind-fault="$FAULT"} > "$OUT/$TAG.log" 2>&1 \
	|| { echo "$TAG: REWOUND PLAYBACK FAILED (see $OUT/$TAG.log)"; exit 2; }
AT=$(sed -n 's/.*rewind snapshot taken at gameTime \([0-9]*\).*/\1/p' "$OUT/$TAG.log" | head -1)
[ -n "$AT" ] || { echo "$TAG: NO REWIND SNAPSHOT"; exit 2; }
if [ -n "$FAULT" ]; then
	grep -q "rewind fault $FAULT: " "$OUT/$TAG.log" || { echo "$TAG: FAULT NOT APPLIED (see $OUT/$TAG.log)"; exit 2; }
	grep -q "rewind restore FAILED before changing any live state" "$OUT/$TAG.log" \
		|| { echo "$TAG: RESTORE NOT REJECTED (see $OUT/$TAG.log)"; exit 1; }
	CMPAFTER=0; CMPLAST=
else
	grep -q "rewound from gameTime" "$OUT/$TAG.log" || { echo "$TAG: REWIND RESTORE DID NOT HAPPEN"; exit 2; }
	CMPAFTER=$AT; CMPLAST=--last
fi
ENDED=$(sed -n 's/.*Replay ended at gameTime \([0-9]*\).*/\1/p' "$OUT/$TAG.log" | tail -1)
[ -n "$ENDED" ] || { echo "$TAG: REWOUND PLAYBACK DID NOT REACH THE REPLAY'S END"; exit 2; }
PLAY=$(python3 -I "$HERE/cmptrace.py" "$REC.trace" "$REC.play.trace" --after 0 --last)
PLAYRC=$?
RW=$(python3 -I "$HERE/cmptrace.py" "$REC.trace" "$OUT/$TAG.trace" --after "$CMPAFTER" $CMPLAST)
RWRC=$?
echo "$TAG (snapshot at $AT, replay ended at $ENDED${FAULT:+, restore rejected}): playback vs recording: $PLAY; rewound playback vs recording: $RW"
[ $PLAYRC -eq 0 ] && [ $RWRC -eq 0 ] && exit 0
exit 1
