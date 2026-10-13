#!/bin/sh
# Runs the GameState fidelity test matrix (see forkcrc.sh) and prints one line per cell.
#
# usage: matrix.sh <warzone2100 binary> [cell filter (grep -E pattern)]
# Environment: OUT (default /tmp/gamestate-forkcrc).
#
# Cells are "<test> <save tick> <end tick> [forkcrc options]".
# "mb:" tests are movement bench scenarios, which run without AI scripts, so they isolate engine restore fidelity.
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
highground 60000 90000 --rewind 5000
highground 60000 130000 --rewind 60000
'

# Double-save cells: savegames written by the fork cells above, loaded and saved again straight away.
DOUBLESAVES='
fork_highground_60000
fork_miza_300000
fork_mb_perf_battle_20000_dying_noloaded
'

echo "== fork cells"
echo "$CELLS" | grep -E "$FILTER" | while read -r cell; do
	[ -n "$cell" ] || continue
	# shellcheck disable=SC2086
	"$HERE/forkcrc.sh" "$BIN" $cell | head -1
done
echo "== double-save cells"
echo "$DOUBLESAVES" | grep -E "$FILTER" | while read -r save; do
	[ -n "$save" ] || continue
	"$HERE/doublesave.sh" "$BIN" "$save" --no-loaded
	"$HERE/doublesave.sh" "$BIN" "$save"
done
