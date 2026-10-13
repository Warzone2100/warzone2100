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
gamestate_cobra 300000 330000 --no-loaded
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
'

# Double-save cells: savegames written by the fork cells above, loaded and saved again straight away.
DOUBLESAVES='
fork_highground_60000
fork_miza_300000
fork_mb_perf_battle_20000_dying_noloaded
fork_miza_20000_dying_feature_noloaded
fork_mb_gamestate_deaths_27000_dying_pendingvis_noloaded
fork_mb_gamestate_scripts_30000_noloaded
fork_cam_CAM_2A_60000
fork_gamestate_cobra_300000_noloaded
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
if echo "deadgrid" | grep -qE "$FILTER"; then
	"$HERE/deadgrid.sh" "$BIN" | head -1
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
