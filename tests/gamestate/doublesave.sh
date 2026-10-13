#!/bin/sh
# GameState double-save test: load a savegame and save it again straight away, once the load has completed
# and before the first tick (--gamestate-resave-on-load).
# The two GameState documents should be identical.
# Any difference is state the load did not restore exactly, or state that load-time code changed afterwards.
#
# usage: doublesave.sh <warzone2100 binary> <skirmish savegame name> [--no-loaded]
#   The savegame must already exist in $OUT/config/savegames/skirmish/ (ex. written by forkcrc.sh).
#   --no-loaded  load with --gamestate-test-suppress-loaded-event (no eventGameLoaded / research replay)
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 600 seconds).
# Exit status: 0 identical, 1 differs, 2 a run failed.
set -u
BIN=${1:?binary}
SAVE=${2:?savegame name}
NOLOADED=
[ "${3:-}" = "--no-loaded" ] && NOLOADED=--gamestate-test-suppress-loaded-event
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-600}
CFG="$OUT/config"
RESAVE="${SAVE}_resave${NOLOADED:+_noloaded}"
rm -rf "$CFG/savegames/skirmish/$RESAVE"
[ -f "$CFG/savegames/skirmish/$SAVE/gamestate.wz" ] || { echo "$SAVE: no such new-format savegame in $CFG"; exit 2; }

timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" --headless --autogame --loadskirmish="$SAVE" $NOLOADED \
	--gamestate-resave-on-load="savegames/skirmish/$RESAVE.gam" > "$OUT/$RESAVE.log" 2>&1
[ -f "$CFG/savegames/skirmish/$RESAVE/gamestate.wz" ] || { echo "$SAVE${NOLOADED:+ (no eventGameLoaded)}: RESAVE FAILED (see $OUT/$RESAVE.log)"; exit 2; }
echo "$SAVE${NOLOADED:+ (no eventGameLoaded)}: load + immediate re-save:"
python3 -I "$HERE/cmpsave.py" "$CFG/savegames/skirmish/$SAVE" "$CFG/savegames/skirmish/$RESAVE"
