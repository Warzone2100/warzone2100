#!/bin/sh
# Replay seek test: play a recorded skirmish game's replay back headless with the snapshot ring
# (--gamestate-snapshot-ring) and replay seeks (--gamestate-seek-test), and compare the playback with the recording.
#
#   recording: a headless skirmish game to <end tick>, tracing sync CRCs (shared with replayrewind.sh)
#   seeking:   the replay played back to its end with the ring and the seeks.
#              Each seek must reach its target, the playback must reach the replay's end and quit with status 0,
#              the game must log no sync error,
#              and every tick's CRC (the last time it was simulated) must be the recording's.
#
# usage: replayseek.sh <warzone2100 binary> <skirmish test> <end tick> <ring> [<seeks>]
#   <ring>  as --gamestate-snapshot-ring: <interval ms>[,<budget KB>[,<zlib level>]]
#   <seeks> as --gamestate-seek-test: <at>:<target>[,<at>:<target>...] (none: the ring alone)
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 600 seconds), SEED (default 1).
# Exit status: 0 identical and every check passed, 1 a difference or a failed check, 2 a run failed.
set -u
BIN=${1:?binary}
TEST=${2:?skirmish test}
ENDTICK=${3:?end tick}
RING=${4:?ring}
SEEKS=${5:-}
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-600}
SEED=${SEED:-1}
TAG="replayseek_$TEST.ring_$(echo "$RING" | tr ',' '_')${SEEKS:+.seek_$(echo "$SEEKS" | tr ',:' '_-')}"
# The recording, as replayrewind.sh makes it (and shared with it)
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
LOG="$OUT/$TAG.log"
timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" --headless --loadreplay="${REPLAY%.wzrp}" --gamestate-crc-trace="$OUT/$TAG.trace" \
	--gamestate-snapshot-ring="$RING" ${SEEKS:+--gamestate-seek-test="$SEEKS"} > "$LOG" 2>&1 \
	|| { echo "$TAG: PLAYBACK FAILED (exit $?; see $LOG)"; exit 2; }
FAIL=""
NSEEKS=0
[ -n "$SEEKS" ] && NSEEKS=$(echo "$SEEKS" | tr ',' '\n' | wc -l)
REACHED=$(grep -c "Replay seek to gameTime [0-9]*: reached" "$LOG")
[ "$REACHED" -eq "$NSEEKS" ] || FAIL="$FAIL; $REACHED of $NSEEKS seek(s) reached their target"
grep -q "Replay seek to gameTime [0-9]* failed" "$LOG" && FAIL="$FAIL; a seek failed"
grep -q "Synch error" "$LOG" && FAIL="$FAIL; sync error(s) logged"
grep -q "Snapshot ring: " "$LOG" && FAIL="$FAIL; snapshot ring assertion(s)"
ENDED=$(sed -n 's/.*Replay ended at gameTime \([0-9]*\).*/\1/p' "$LOG" | tail -1)
[ -n "$ENDED" ] || FAIL="$FAIL; the playback didn't reach the replay's end"
CMP=$(python3 -I "$HERE/cmptrace.py" "$REC.trace" "$OUT/$TAG.trace" --after 0 --last)
CMPRC=$?
RESTORES=$(grep -c "Replay seek to gameTime [0-9]*: restored" "$LOG")
RINGSTATS=$(sed -n 's/.*Snapshot ring stopped: //p' "$LOG" | tail -1)
echo "$TAG (replay ended at ${ENDED:-?}; $REACHED seek(s), $RESTORES restore(s)): playback vs recording: $CMP${FAIL:+; FAILED:${FAIL#;}}"
[ -n "$RINGSTATS" ] && echo "  ring: $RINGSTATS"
grep "Replay seek to gameTime [0-9]*: reached" "$LOG" | sed 's/^.*Replay seek to /  seek to /'
[ $CMPRC -eq 0 ] && [ -z "$FAIL" ] && exit 0
exit 1
