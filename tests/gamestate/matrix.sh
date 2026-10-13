#!/bin/sh
# Runs the GameState fidelity test matrix (see forkcrc.sh) and prints one line per cell.
#
# usage: matrix.sh <warzone2100 binary> [cell filter (grep -E pattern)]
# Environment: OUT (default /tmp/gamestate-forkcrc).
#
# Cells are "<test> <save tick> <end tick> [forkcrc options]".
# "mb:" tests are movement bench scenarios, which run without AI scripts, so they isolate engine restore fidelity:
# - mb:gamestate_deaths kills objects of every shape on purpose
# - mb:gamestate_scripts has script groups and labels a load must restore
# "cam:" tests are campaign games, which run campaign scripts but no AI bots
# (keep their end ticks short of the level ending).
# The skirmish tests run bundled AIs.
set -u
BIN=$(cd "$(dirname "${1:?binary}")" && pwd)/$(basename "$1")
FILTER=${2:-.}
HERE=$(cd "$(dirname "$0")" && pwd)

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
	"$HERE/forkcrc.sh" "$BIN" $cell | head -1
done
if echo "deadgrid" | grep -qE "$FILTER"; then
	"$HERE/deadgrid.sh" "$BIN" | head -1
fi
echo "== double-save cells"
echo "$DOUBLESAVES" | grep -E "$FILTER" | while read -r save; do
	[ -n "$save" ] || continue
	"$HERE/doublesave.sh" "$BIN" "$save" --no-loaded
	"$HERE/doublesave.sh" "$BIN" "$save"
done
