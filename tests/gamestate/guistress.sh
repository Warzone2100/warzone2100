#!/bin/sh
# GUI test of the UI's references to game objects, under a virtual display (xvfb-run).
# With --gamestate-ui-stress, the UI is pointed every <stress> ticks at the droid most likely to die next
# (camera tracking, sensor view, selection, and the local player's forms in turn),
# and every frame the game checks that no UI reference is to a freed object.
#
#   replay mode:   records a headless skirmish game to <end tick>
#                  (a fresh recording each run, since a replay recorded by another build opens a version warning
#                  that waits for a click), then plays it back with the GUI, the snapshot ring and the seeks.
#                  Every seek must reach its target,
#                  every tick's CRC (the last time it was simulated) must be the recording's,
#                  and no frame may find a reference to a freed object.
#   skirmish mode: plays the skirmish game with the GUI twice, with and without the stress.
#                  The two must be IDENTICAL (the stress doesn't change the game),
#                  the one without must be IDENTICAL to the same game played headless
#                  (the GUI doesn't change the game either),
#                  and no frame may find a reference to a freed object.
#
# usage: guistress.sh <warzone2100 binary> replay <skirmish test> <end tick> <ring> <seeks> <stress ticks>
#        guistress.sh <warzone2100 binary> skirmish <skirmish test> <end tick> <stress ticks>
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 900 seconds), SEED (default 1).
# Exit status: 0 every check passed, 1 a check failed, 2 a run failed (or no xvfb-run).
set -u
BIN=${1:?binary}
MODE=${2:?mode}
TEST=${3:?skirmish test}
ENDTICK=${4:?end tick}
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-900}
SEED=${SEED:-1}
command -v xvfb-run > /dev/null || { echo "guistress: no xvfb-run"; exit 2; }
gui() { # log, args...
	log=$1; shift
	timeout "$RUN_TIMEOUT" xvfb-run -a -s "-screen 0 1280x800x24" "$BIN" --nosound --window --resolution=1024x768 "$@" > "$log" 2>&1
}
stale() { grep -c "reference(s) to freed objects" "$1"; }
checked() { grep -q "checking the UI's object references every frame" "$1"; }

case "$MODE" in
replay)
	RING=${5:?ring}
	SEEKS=${6:?seeks}
	STRESS=${7:?stress ticks}
	TAG="guistress_replay_$TEST.ring_$(echo "$RING" | tr ',' '_').seek_$(echo "$SEEKS" | tr ',:' '_-').stress$STRESS"
	CFG="$OUT/$TAG.cfg"
	rm -rf "$CFG"
	mkdir -p "$CFG"
	timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" --headless --autogame --skirmish="$TEST.json" --gamestate-test-seed="$SEED" \
		--gamestate-crc-trace="$OUT/$TAG.rec.trace" --gamestate-exit-at-tick="$ENDTICK" > "$OUT/$TAG.rec.log" 2>&1 \
		|| { echo "$TAG: RECORDING FAILED (see $OUT/$TAG.rec.log)"; exit 2; }
	REPLAY=$(ls "$CFG/replay/skirmish/" | head -1)
	[ -n "$REPLAY" ] || { echo "$TAG: NO REPLAY RECORDED"; exit 2; }
	# (The GUI doesn't quit at the replay's end: stop a little before it)
	gui "$OUT/$TAG.log" --configdir="$CFG" --loadreplay="${REPLAY%.wzrp}" --gamestate-crc-trace="$OUT/$TAG.trace" \
		--gamestate-exit-at-tick=$((ENDTICK - 1000)) --gamestate-snapshot-ring="$RING" --gamestate-seek-test="$SEEKS" \
		--gamestate-ui-stress="$STRESS" || { echo "$TAG: GUI PLAYBACK FAILED (exit $?; see $OUT/$TAG.log)"; exit 2; }
	FAIL=""
	checked "$OUT/$TAG.log" || FAIL="$FAIL; the UI check didn't run"
	NSEEKS=$(echo "$SEEKS" | tr ',' '\n' | wc -l)
	REACHED=$(grep -c "Replay seek to gameTime [0-9]*: reached" "$OUT/$TAG.log")
	[ "$REACHED" -eq "$NSEEKS" ] || FAIL="$FAIL; $REACHED of $NSEEKS seek(s) reached their target"
	STALE=$(stale "$OUT/$TAG.log")
	[ "$STALE" -eq 0 ] || FAIL="$FAIL; $STALE frame(s) found references to freed objects"
	STRESSED=$(grep -c "UI stress at" "$OUT/$TAG.log")
	CMP=$(python3 -I "$HERE/cmptrace.py" "$OUT/$TAG.rec.trace" "$OUT/$TAG.trace" --after 0 --last)
	CMPRC=$?
	echo "$TAG ($REACHED seek(s), $STRESSED UI stress step(s), $STALE stale frame(s)): GUI playback vs recording: $CMP${FAIL:+; FAILED:${FAIL#;}}"
	[ $CMPRC -eq 0 ] && [ -z "$FAIL" ] && exit 0
	exit 1
	;;
skirmish)
	STRESS=${5:?stress ticks}
	TAG="guistress_skirmish_$TEST.$ENDTICK.stress$STRESS"
	CFG="$OUT/$TAG.cfg"
	mkdir -p "$CFG"
	REF="$OUT/guistress_skirmish_$TEST.$ENDTICK.plain"
	if [ ! -s "$REF.trace" ]; then
		gui "$REF.log" --configdir="$CFG" --autogame --skirmish="$TEST.json" --gamestate-test-seed="$SEED" \
			--gamestate-crc-trace="$REF.trace" --gamestate-exit-at-tick="$ENDTICK" \
			|| { rm -f "$REF.trace"; echo "$TAG: GUI REFERENCE RUN FAILED (see $REF.log)"; exit 2; }
	fi
	HREF="$OUT/guistress_skirmish_$TEST.$ENDTICK.headless"
	if [ ! -s "$HREF.trace" ]; then
		timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" --headless --autogame --skirmish="$TEST.json" --gamestate-test-seed="$SEED" \
			--gamestate-crc-trace="$HREF.trace" --gamestate-exit-at-tick="$ENDTICK" > "$HREF.log" 2>&1 \
			|| { rm -f "$HREF.trace"; echo "$TAG: HEADLESS REFERENCE RUN FAILED (see $HREF.log)"; exit 2; }
	fi
	gui "$OUT/$TAG.log" --configdir="$CFG" --autogame --skirmish="$TEST.json" --gamestate-test-seed="$SEED" \
		--gamestate-crc-trace="$OUT/$TAG.trace" --gamestate-exit-at-tick="$ENDTICK" --gamestate-ui-stress="$STRESS" \
		|| { echo "$TAG: GUI RUN FAILED (exit $?; see $OUT/$TAG.log)"; exit 2; }
	FAIL=""
	checked "$OUT/$TAG.log" || FAIL="$FAIL; the UI check didn't run"
	STALE=$(stale "$OUT/$TAG.log")
	[ "$STALE" -eq 0 ] || FAIL="$FAIL; $STALE frame(s) found references to freed objects"
	STRESSED=$(grep -c "UI stress at" "$OUT/$TAG.log")
	FORMS=$(grep -c "UI stress at.*, form" "$OUT/$TAG.log")
	CMP=$(python3 -I "$HERE/cmptrace.py" "$REF.trace" "$OUT/$TAG.trace" --after 0)
	CMPRC=$?
	HCMP=$(python3 -I "$HERE/cmptrace.py" "$HREF.trace" "$REF.trace" --after 0)
	HCMPRC=$?
	echo "$TAG ($STRESSED UI stress step(s), $FORMS with a form, $STALE stale frame(s)): with the stress vs without: $CMP; GUI vs headless: $HCMP${FAIL:+; FAILED:${FAIL#;}}"
	[ $CMPRC -eq 0 ] && [ $HCMPRC -eq 0 ] && [ -z "$FAIL" ] && exit 0
	exit 1
	;;
*)
	echo "guistress: unknown mode $MODE"
	exit 2
	;;
esac
