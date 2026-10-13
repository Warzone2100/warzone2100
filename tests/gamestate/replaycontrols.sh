#!/bin/sh
# GUI test of the replay controls (src/hci/replay_controller.h):
# play a recorded skirmish game's replay back with the GUI under a virtual display (Xvfb),
# and drive the transport bar with the mouse and keyboard (xdotool):
#
#   1. click the timeline near its end: a seek forward, fast-forwarding to the replay's end
#   2. pause, then "back 15 seconds": a seek back from the replay's end (restoring a ring snapshot),
#      which must leave playback paused (no tick runs)
#   3. play, then "back to the start": a seek back to the first snapshot
#   4. click the timeline at three quarters and press Esc at once: the seek stops where it is (or doesn't start)
#   5. click the timeline at a third: a seek back
#
# Every seek must be requested from the controls, at the time clicked,
# and reach its target (or, for 4, be stopped),
# the snapshot ring must run without being asked (the default with the GUI),
# and every tick's CRC (the last time it was simulated) must be the recording's.
# The replay is recorded afresh each run
# (a replay from another build opens a version warning that waits for a click).
#
# The clicks are placed from the transport bar's layout at 1280x800
# (replay_controller.cpp: 60% of the screen width, centered at the bottom,
# and the buttons left to right under the timeline).
# NOTE: a change to that layout must be made here too.
#
# usage: replaycontrols.sh <warzone2100 binary> [<skirmish test> [<end tick>]]   (default highground 90000)
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 600 seconds), SEED (default 1),
#   DISPLAYNUM (default 77).
# Exit status: 0 every check passed, 1 a check failed, 2 a run failed or Xvfb / xdotool are missing.
set -u
BIN=$(cd "$(dirname "${1:?binary}")" && pwd)/$(basename "$1")
TEST=${2:-highground}
ENDTICK=${3:-90000}
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-600}
SEED=${SEED:-1}
DISPLAYNUM=${DISPLAYNUM:-77}
TAG="replaycontrols_$TEST.$ENDTICK"
for tool in Xvfb xdotool; do
	command -v $tool > /dev/null || { echo "$TAG: no $tool"; exit 2; }
done
CFG="$OUT/$TAG.cfg"
LOG="$OUT/$TAG.log"
TRACE="$OUT/$TAG.trace"
rm -rf "$CFG" "$TRACE"
mkdir -p "$CFG"
timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" --headless --autogame --skirmish="$TEST.json" --gamestate-test-seed="$SEED" \
	--gamestate-crc-trace="$OUT/$TAG.rec.trace" --gamestate-exit-at-tick="$ENDTICK" > "$OUT/$TAG.rec.log" 2>&1 \
	|| { echo "$TAG: RECORDING FAILED (see $OUT/$TAG.rec.log)"; exit 2; }
REPLAY=$(ls "$CFG/replay/skirmish/" | head -1)
[ -n "$REPLAY" ] || { echo "$TAG: NO REPLAY RECORDED"; exit 2; }

Xvfb ":$DISPLAYNUM" -screen 0 1280x800x24 > "$OUT/$TAG.xvfb.log" 2>&1 &
XVFB=$!
GAME=""
cleanup() {
	[ -n "$GAME" ] && kill "$GAME" 2> /dev/null
	kill "$XVFB" 2> /dev/null
}
trap cleanup EXIT
sleep 2
DISPLAY=":$DISPLAYNUM" "$BIN" --configdir="$CFG" --nosound --window --resolution=1280x800 --loadreplay="${REPLAY%.wzrp}" \
	--gamestate-crc-trace="$TRACE" > "$LOG" 2>&1 &
GAME=$!

FAIL=""
fail() { FAIL="$FAIL; $1"; }
# wait_log <pattern> <seconds>: wait until the log has a line matching the pattern (counted from the step's start)
wait_log() {
	i=0
	while [ $i -lt "$2" ]; do
		tail -n +"$FROM" "$LOG" | grep -q "$1" && return 0
		kill -0 "$GAME" 2> /dev/null || return 1
		sleep 1
		i=$((i + 1))
	done
	return 1
}
step() { FROM=$(($(wc -l < "$LOG") + 1)); }
# The layout at 1280x800:
# - the bar is 768 wide from x 256, 60 high, 6 from the bottom
# - the timeline is 752 wide from x 264 at y 742 (its track inset by 6 on each side)
# - the buttons are 26 wide, 5 apart, from x 264 at y 760
timeline() { DISPLAY=":$DISPLAYNUM" xdotool mousemove $((270 + 740 * $1 / 100)) 748 click 1; }
button() {
	case $1 in start) n=0;; back) n=1;; pause) n=2;; play) n=3;; forward) n=4;; fastforward) n=5;; esac
	DISPLAY=":$DISPLAYNUM" xdotool mousemove $((277 + 31 * n)) 773 click 1
}
ticks() { wc -l < "$TRACE"; }
# requested <ms>: the last seek requested from the controls was to <ms> (to the second)
requested() { tail -n +"$FROM" "$LOG" | sed -n 's/.*Replay seek to gameTime \([0-9]*\) requested from the replay controls.*/\1/p' | tail -1; }

FROM=1
wait_log "took a snapshot at gameTime 2 " 120 || { echo "$TAG: THE PLAYBACK DIDN'T START, OR THE RING ISN'T RUNNING (see $LOG)"; exit 2; }
END=$(sed -n 's/.*which ends at gameTime \([0-9]*\).*/\1/p' "$LOG" | head -1)
[ -n "$END" ] && [ "$END" -gt 0 ] || { echo "$TAG: THE REPLAY'S END IS UNKNOWN (see $LOG)"; exit 2; }
# expect <percent>: the time a timeline click at <percent> seeks to (whole seconds)
expect() { echo $(( (END * $1 / 100) / 1000 * 1000 )); }
near() { [ -n "$1" ] && [ $(( $1 - $2 )) -le 1000 ] && [ $(( $2 - $1 )) -le 1000 ]; }

# 1. A seek forward to near the end
step; timeline 99
wait_log "Replay seek to gameTime [0-9]*: reached" 60 || fail "1: the seek forward didn't reach its target"
near "$(requested)" "$(expect 99)" || fail "1: seek requested to $(requested), not near $(expect 99)"
wait_log "Replay ended at gameTime" 60 || fail "1: playback didn't reach the replay's end"

# 2. Pause, back 15 seconds: restores, and stays paused
step; button pause; sleep 1; button back
wait_log "Replay seek to gameTime [0-9]*: reached" 60 || fail "2: the seek back from the end didn't reach its target"
tail -n +"$FROM" "$LOG" | grep -q "Replay seek to gameTime [0-9]*: restored" || fail "2: the seek back didn't restore a snapshot"
sleep 1; T1=$(ticks); sleep 3; T2=$(ticks)
[ "$T1" = "$T2" ] || fail "2: playback didn't stay paused after the seek ($T1 then $T2 ticks traced)"

# 3. Play, back to the start
step; button play; sleep 2; button start
wait_log "Replay seek to gameTime [0-9]*: reached" 60 || fail "3: the seek to the start didn't reach its target"
[ "$(requested)" = "0" ] || fail "3: seek requested to $(requested), not 0"
sleep 1; T1=$(ticks); sleep 2; T2=$(ticks)
[ "$T1" != "$T2" ] || fail "3: playback didn't go on after play"

# 4. A long seek forward, stopped with Esc at once
step; timeline 75; DISPLAY=":$DISPLAYNUM" xdotool key Escape
wait_log "Replay seek to gameTime [0-9]*: \(stopped\|cancelled\|reached\)" 60 || fail "4: the seek was neither stopped nor done"
STOPPED=$(tail -n +"$FROM" "$LOG" | grep -c "Replay seek to gameTime [0-9]*: \(stopped\|cancelled\)")
# (If the seek was done before the Esc, the Esc opened the in-game menu: close it)
[ "$STOPPED" -eq 0 ] && { sleep 1; DISPLAY=":$DISPLAYNUM" xdotool key Escape; }

# 5. A seek back to a third
step; sleep 2; timeline 33
wait_log "Replay seek to gameTime [0-9]*: reached" 60 || fail "5: the seek back didn't reach its target"
near "$(requested)" "$(expect 33)" || fail "5: seek requested to $(requested), not near $(expect 33)"
sleep 3

grep -q "Replay seek to gameTime [0-9]* failed" "$LOG" && fail "a seek failed"
grep -q "Synch error" "$LOG" && fail "sync error(s) logged"
grep -q "Snapshot ring: " "$LOG" && fail "snapshot ring assertion(s)"
kill "$GAME" 2> /dev/null; wait "$GAME" 2> /dev/null; GAME=""
CMP=$(python3 -I "$HERE/cmptrace.py" "$OUT/$TAG.rec.trace" "$TRACE" --after 0 --last)
CMPRC=$?
SEEKS=$(grep -c "requested from the replay controls" "$LOG")
RESTORES=$(grep -c "Replay seek to gameTime [0-9]*: restored" "$LOG")
echo "$TAG ($SEEKS seek(s) from the controls, $RESTORES restore(s), $STOPPED stopped with Esc): playback vs recording: $CMP${FAIL:+; FAILED:${FAIL#;}}"
[ $CMPRC -eq 0 ] && [ -z "$FAIL" ] && exit 0
exit 1
