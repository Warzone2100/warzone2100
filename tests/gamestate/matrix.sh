#!/bin/sh
# Runs the GameState fidelity test matrix (see forkcrc.sh) and prints one line per cell.
#
# usage: matrix.sh <warzone2100 binary> [cell filter (grep -E pattern)]
# Environment: OUT (default /tmp/gamestate-forkcrc).
#              CHECK_COVERAGE (default 10): check the tile coverage every 10 ticks and at every save or snapshot
#              (see forkcrc.sh), and 0 turns it off.
#
# Cells are "<test> <save tick> <end tick> [forkcrc options]".
# "mb:" tests are movement bench scenarios, which run without AI scripts, so they isolate engine restore fidelity:
# - mb:gamestate_deaths kills objects of every shape on purpose
# - mb:gamestate_scripts has script groups and labels a load must restore
# - mb:gamestate_rewind has script state only a rewind can get wrong
# The miza 28102 cells are saved right before the messages for the next tick request a route.
# The betweenticks cell (betweenticks.sh) is saved between two ticks with coverage changes pending.
# "cam:" tests are campaign games, which run campaign scripts but no AI bots
# (keep their end ticks short of the level ending).
# The skirmish tests run bundled AIs.
set -u
BIN=$(cd "$(dirname "${1:?binary}")" && pwd)/$(basename "$1")
FILTER=${2:-.}
HERE=$(cd "$(dirname "$0")" && pwd)
CHECK_COVERAGE=${CHECK_COVERAGE:-10}
export CHECK_COVERAGE

CELLS='
mb:perf_battle 20000 100000 --no-loaded
mb:perf_battle 20000 100000 --no-loaded --dying
mb:perf_artillery 20000 100000 --no-loaded --dying
mb:perf_walled_assault 20000 100000 --no-loaded --dying
mb:perf_vtol_rearm 20000 100000 --no-loaded --dying
mb:perf_repair_facility 20000 100000 --no-loaded
mb:perf_cyborg_blob 20000 100000 --no-loaded
highground 60000 90000
highground 60000 90000 --no-loaded
miza 300000 330000
miza 300000 330000 --no-loaded
miza 300000 360000 --no-loaded --dying
mb:perf_battle 20000 100000 --rewind 5000
mb:perf_battle 20000 100000 --rewind 5000 --dying
mb:perf_vtol_rearm 20000 100000 --rewind 5000 --dying
miza 20000 60000 --no-loaded --dying-filter feature
miza 28102 33102 --no-loaded
miza 28102 33102 --rewind 1000
gamestate_cobra 300000 330000
gamestate_cobra 300000 330000 --no-loaded
gamestate_bonecrusher 300000 330000
gamestate_bonecrusher 300000 330000 --no-loaded
mb:gamestate_deaths 27000 150000 --no-loaded --dying-filter pendingvis
mb:gamestate_deaths 40000 150000 --no-loaded --dying-filter pendingvis
mb:gamestate_deaths 40000 150000 --rewind 5000 --dying-filter pendingvis
mb:gamestate_scripts 30000 100000
mb:gamestate_scripts 30000 100000 --no-loaded
mb:gamestate_scripts 45000 120000 --no-loaded
mb:gamestate_scripts 30000 100000 --rewind 5000
mb:dead_grid 6000 19900 --no-loaded --dying
mb:dead_grid 10000 19900 --no-loaded --dying
mb:dead_grid 17000 19900 --no-loaded --dying
cam:CAM_2A 60000 120000
cam:CAM_2A 60000 120000 --no-loaded
cam:CAM_2A 30000 150000 --no-loaded --dying
cam:CAM_3A 30000 150000 --no-loaded --dying
cam:CAM_3A 60000 150000 --rewind 20000
highground 60000 90000 --rewind 5000
highground 60000 130000 --rewind 60000
mb:gamestate_rewind 20000 60000 --rewind 20000
mb:gamestate_scripts 30000 100000 --rewind 40000
miza 300000 330000 --rewind 5000
gamestate_cobra 300000 330000 --rewind 5000
gamestate_bonecrusher 300000 330000 --rewind 5000
cam:CAM_2A 60000 120000 --rewind 5000
'

# Replay rewinds (replayrewind.sh): a recorded skirmish game played back with an in-process rewind,
# run on to the replay's end.
# The second highground cell seeks back over most of the replay.
REPLAY_CELLS='
highground 60000 90000 5000
highground 3000 90000 50000
miza 300000 330000 20000
'
# Replay rewind faults (replayrewind.sh with FAULT):
# damaged replay read positions must be rejected before the restore changes anything
REPLAY_FAULT_KINDS='replay-position no-replay-positions'

# Replay seeks (replayseek.sh, sharing the recordings above): "<test> <end tick> <ring> [<seeks>]", in order:
# - the ring alone
# - seeks forward (no restore), back to a snapshot's time, back between snapshots and back to the start
# - a small budget that thins the ring out
# - past the replay's end
# - to the very start (the first snapshot)
# - a seek that replaces another still fast-forwarding
# - without the ring (each seek back plays the replay again from the start)
# - miza
SEEK_CELLS='
highground 90000 10000
highground 90000 10000 30000:55000,60000:40002,70000:25000,80000:1000
highground 90000 5000,400 70000:25000,85000:61000
highground 90000 10000 30000:999999
highground 90000 10000 50000:0
highground 90000 10000 30000:80000,40000:20000
highground 90000 off 30000:55000,60000:40002,70000:25000
miza 330000 30000 200000:150000,300000:20000,310000:325000
'
# Replay seek fallbacks (replayseek.sh with FAULT and EXPECT):
# "<fault> <expected fallback> <test> <end tick> <ring> <seeks>".
# A snapshot that is rejected, or whose restore fails half way, is dropped and an older one restored,
# and when none can be, the replay is played again from the start.
# The rings take their snapshots at the same game times in every run (sync).
# With snapshots every 12 s, the highground seek from 60002 to 40002 fails to restore the snapshot at 36002
# and restores 24002 (a failure at pass1 or worlds there must not leave formations behind).
SEEK_FAULT_CELLS='
truncate older highground 90000 12000,65536,3,sync 60000:40002
throw-at:worlds older highground 90000 12000,65536,3,sync 60000:40002
throw-at:pass1 older highground 90000 12000,65536,3,sync 60000:40002
throw-at:sections older highground 90000 12000,65536,3,sync 60000:40002
throw-at:scripting older miza 330000 30000,65536,3,sync 300000:150000,310000:325000
truncate,99 restart highground 90000 12000,65536,3,sync 60000:40002
throw-at:sections,99 restart highground 90000 12000,65536,3,sync 60000:40002
'

# Rewind faults (rewindfault.sh): a damaged snapshot must be rejected before the restore changes anything
# (the live game goes on, IDENTICAL), over-limit must restore,
# and a throw during the restore must stop the game cleanly
FAULT_KINDS='truncate section-version:power bad-stat duplicate-id dangling-ref overlap edge off-map over-limit throw-at:pass1 throw-at:sections'

# Double-save cells: savegames written by the fork cells above, loaded and saved again straight away.
DOUBLESAVES='
fork_highground_60000
fork_miza_300000
fork_mb_perf_battle_20000_dying_noloaded
fork_miza_20000_dying_feature_noloaded
fork_mb_gamestate_deaths_27000_dying_pendingvis_noloaded
fork_mb_gamestate_scripts_30000_noloaded
fork_cam_CAM_2A_60000
fork_gamestate_cobra_300000
fork_gamestate_cobra_300000_noloaded
fork_gamestate_bonecrusher_300000
fork_gamestate_bonecrusher_300000_noloaded
'

echo "== fork cells"
echo "$CELLS" | grep -E "$FILTER" | while read -r cell; do
	[ -n "$cell" ] || continue
	# shellcheck disable=SC2086
	"$HERE/forkcrc.sh" "$BIN" $cell | sed -n '1p;/COVERAGE DIFFERS/,+1p'
done
if echo "betweenticks" | grep -qE "$FILTER"; then
	"$HERE/betweenticks.sh" "$BIN" | sed -n '1p;/COVERAGE DIFFERS/,+1p'
fi
if echo "camlaunch" | grep -qE "$FILTER"; then
	"$HERE/camlaunch.sh" "$BIN" | grep -v '^  '
fi
if echo "camreinforce" | grep -qE "$FILTER"; then
	"$HERE/camreinforce.sh" "$BIN"
fi
if echo "camtextures" | grep -qE "$FILTER"; then
	if command -v Xvfb > /dev/null; then
		"$HERE/camtextures.sh" "$BIN" | grep -v '^  '
	else
		echo "camtextures: skipped (no Xvfb)"
	fi
fi
if echo "deadgrid" | grep -qE "$FILTER"; then
	"$HERE/deadgrid.sh" "$BIN" | head -1
fi
if echo "replayrewind" | grep -qE "$FILTER"; then
	echo "$REPLAY_CELLS" | while read -r cell; do
		[ -n "$cell" ] || continue
		# shellcheck disable=SC2086
		"$HERE/replayrewind.sh" "$BIN" $cell
	done
	for kind in $REPLAY_FAULT_KINDS; do
		FAULT=$kind "$HERE/replayrewind.sh" "$BIN" highground 60000 90000 5000
	done
fi
# GUI cells (guistress.sh): a replay with seeks, and a skirmish game with and without the stress
GUI_CELLS='
replay highground 90000 10000 30000:55000,60000:40002,70000:25000,80000:1000 20
skirmish highground 90000 10
'
if echo "guistress" | grep -qE "$FILTER"; then
	if command -v xvfb-run > /dev/null; then
		echo "$GUI_CELLS" | while read -r cell; do
			[ -n "$cell" ] || continue
			# shellcheck disable=SC2086
			"$HERE/guistress.sh" "$BIN" $cell
		done
	else
		echo "guistress: skipped (no xvfb-run)"
	fi
fi
if echo "replayseek" | grep -qE "$FILTER"; then
	echo "$SEEK_CELLS" | while read -r cell; do
		[ -n "$cell" ] || continue
		# shellcheck disable=SC2086
		"$HERE/replayseek.sh" "$BIN" $cell | head -2
	done
	echo "$SEEK_FAULT_CELLS" | while read -r fault expect cell; do
		[ -n "$fault" ] || continue
		# shellcheck disable=SC2086
		FAULT=$fault EXPECT=$expect "$HERE/replayseek.sh" "$BIN" $cell | head -1
	done
fi
if echo "replaycontrols" | grep -qE "$FILTER"; then
	if command -v Xvfb > /dev/null && command -v xdotool > /dev/null; then
		"$HERE/replaycontrols.sh" "$BIN"
	else
		echo "replaycontrols: skipped (no Xvfb or xdotool)"
	fi
fi
if echo "rewindfault" | grep -qE "$FILTER"; then
	for kind in $FAULT_KINDS; do
		"$HERE/rewindfault.sh" "$BIN" mb:perf_battle 20000 30000 5000 "$kind"
	done
fi
# Campaign level changes (camtransition.sh, which also builds the test mod the cells below need):
# CAM_2A loads the off-world mission SUB_2_1 in tick 20002.
# The 15000 cells run through the level change.
# The 20002 cell asks for a save in the tick of the loadLevel() call, which the game can't save,
# so it is taken in SUB_2_1's first tick.
CAMTRANSITION_CELLS='
15000 20250
15000 20250 --no-loaded
20002 20250 --no-loaded
'
if echo "camtransition" | grep -qE "$FILTER"; then
	"$HERE/camtransition.sh" "$BIN" | grep -v '^  started'
	echo "$CAMTRANSITION_CELLS" | while read -r cell; do
		[ -n "$cell" ] || continue
		# shellcheck disable=SC2086
		EXTRA="--mod=camtransition.wz" "$HERE/forkcrc.sh" "$BIN" cam:CAM_2A $cell | sed -n '1p;/COVERAGE DIFFERS/,+1p'
	done
fi
echo "== double-save cells"
echo "$DOUBLESAVES" | grep -E "$FILTER" | while read -r save; do
	[ -n "$save" ] || continue
	"$HERE/doublesave.sh" "$BIN" "$save" --no-loaded
	"$HERE/doublesave.sh" "$BIN" "$save"
done
if echo "camtransition" | grep -qE "$FILTER"; then
	EXTRA="--mod=camtransition.wz" "$HERE/doublesave.sh" "$BIN" fork_cam_CAM_2A_20002_noloaded --no-loaded
	EXTRA="--mod=camtransition.wz" "$HERE/doublesave.sh" "$BIN" fork_cam_CAM_2A_20002_noloaded
fi
