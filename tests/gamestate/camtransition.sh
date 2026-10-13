#!/bin/sh
# Campaign mission transition test: plays a campaign game into an off-world mission and back without a player,
# and checks that no assertion fires and that the transitions are reproducible.
# The transitions free and swap whole worlds between ticks
# (the scripts' loadLevel -> missionDestroyObjects, saveMissionData, restoreMissionData),
# so this covers state that has to be rebuilt or dropped there (ex. the object grid).
# No tick may run from the loadLevel() until the next level starts.
# The game is played twice, with sync-CRC traces, and both runs must trace the same ticks with the same CRCs,
# and start every level at the same gameTime with the same power.
#
# A global test mod (built here from the repository's campaign scripts)
# makes CAM_2A load the off-world mission SUB_2_1 after 20 s, and SUB_2_1 load CAM_2B right away
# (its own loss check would end it within a second, as nobody brings units).
# The game then waits on the mission result screen, which needs a click,
# so the run is stopped once the return has started.
#
# usage: camtransition.sh <warzone2100 binary>
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 300 seconds),
#              EXTRA (more game options for both runs).
# Exit status: 0 passed, 1 an assertion fired or the two runs differ, 2 a run did not get through the transitions.
#
# One unrelated assertion is reported but not counted:
# openLoadFile, for multiplay/skirmish/semperfi.js (--autogame gives the player an AI that campaign data lacks).
set -u
BIN=${1:?binary}
HERE=$(cd "$(dirname "$0")" && pwd)
CAMSCRIPTS="$HERE/../../data/base/script/campaign"
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-300}
CFG="$OUT/config"
MOD="$OUT/camtransition-mod"
LOG="$OUT/camtransition.log"
rm -rf "$MOD"
mkdir -p "$MOD/script/campaign" "$CFG/mods/global"

# make_advancing <script> <level name expression> <delay ms>:
# a copy of a campaign script whose eventStartLevel also queues a call to camNextLevel
make_advancing() {
	sed 's/^function eventStartLevel()/function __testOrigStartLevel()/' "$CAMSCRIPTS/$1" > "$MOD/script/campaign/$1"
	cat >> "$MOD/script/campaign/$1" <<EOF

// camtransition.sh: advance without a player
function eventStartLevel()
{
	debug("camtransition: started $1 at gameTime " + gameTime + ", power " + playerPower(CAM_HUMAN_PLAYER));
	__testOrigStartLevel();
	queue("__testAdvance", $3);
}
function __testAdvance()
{
	debug("camtransition: leaving $1 for " + $2);
	camNextLevel($2);
}
EOF
}
make_advancing cam2-a.js cam_levels.beta2.offWorld 20000
make_advancing cam2-1x.js cam_levels.beta3 300
python3 -I - "$MOD" "$CFG/mods/global/camtransition.wz" <<'EOF'
import os, sys, zipfile
root, dest = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(dest, "w") as z:
    for d, _, files in os.walk(root):
        for f in files:
            z.write(os.path.join(d, f), os.path.relpath(os.path.join(d, f), root))
EOF

# play <log> <trace>: play the game until the return from the off-world mission has freed its objects
# and the next tick has run
play() {
	timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" --headless --autogame --mod=camtransition.wz --game=CAM_2A \
		--gamestate-test-seed=1 --gamestate-crc-trace="$2" ${EXTRA:-} > "$1" 2>&1 &
	PID=$!
	while kill -0 $PID 2> /dev/null; do
		[ "$(grep -c 'missionDestroyObjects.*destroyed objects' "$1")" -ge 2 ] && sleep 5 && break
		sleep 1
	done
	kill $PID 2> /dev/null
	wait $PID 2> /dev/null
}
play "$LOG" "$OUT/camtransition.trace"
play "$OUT/camtransition.2.log" "$OUT/camtransition.2.trace"

LEFT=$(grep -c 'camtransition: leaving' "$LOG")
CLEARED=$(grep -c 'missionDestroyObjects.*destroyed objects' "$LOG")
KNOWN='\[openLoadFile:.*unable to open file'
KNOWNASSERTS=$(cat "$LOG" "$OUT/camtransition.2.log" | grep 'Assert in Warzone' | grep -cE "$KNOWN")
ASSERTS=$(cat "$LOG" "$OUT/camtransition.2.log" | grep 'Assert in Warzone' | grep -cvE "$KNOWN")
echo "camtransition: left $LEFT of 2 levels, $CLEARED of 2 mission clears, $ASSERTS assertion(s), $KNOWNASSERTS known unrelated (log: $LOG)"
grep -h 'camtransition: started' "$LOG" | sed 's/^.*camtransition: /  /'
STATUS=0
if [ "$ASSERTS" -gt 0 ]; then
	cat "$LOG" "$OUT/camtransition.2.log" | grep 'Assert in Warzone' | grep -vE "$KNOWN" | head -5
	STATUS=1
fi
[ "$LEFT" -ge 2 ] && [ "$CLEARED" -ge 2 ] && [ "$(grep -c 'missionDestroyObjects.*destroyed objects' "$OUT/camtransition.2.log")" -ge 2 ] \
	|| exit 2
# Both runs stop on the mission result screen, where game time stops
grep -h 'camtransition: started' "$LOG" | sed 's/^.*camtransition: //' > "$OUT/camtransition.starts"
grep -h 'camtransition: started' "$OUT/camtransition.2.log" | sed 's/^.*camtransition: //' > "$OUT/camtransition.2.starts"
if ! cmp -s "$OUT/camtransition.starts" "$OUT/camtransition.2.starts"; then
	echo "camtransition: the second run started a level at a different time or with different power:"
	sed 's/^/  /' "$OUT/camtransition.2.starts"
	STATUS=1
fi
# Compare the ticks both traces contain (a run is stopped at an arbitrary point),
# and check that they go past the start of the off-world level
LASTSTART=$(sed -n 's/.* at gameTime \([0-9]*\),.*/\1/p' "$OUT/camtransition.starts" | tail -1)
LAST1=$(tail -1 "$OUT/camtransition.trace" | cut -d' ' -f1)
LAST2=$(tail -1 "$OUT/camtransition.2.trace" | cut -d' ' -f1)
RESULT=$(python3 -I "$HERE/cmptrace.py" "$OUT/camtransition.trace" "$OUT/camtransition.2.trace")
echo "camtransition: the two runs' traces: $RESULT (to ticks $LAST1 and $LAST2; last level started at $LASTSTART)"
case "$RESULT" in
	IDENTICAL*) [ "${LAST1:-0}" -gt "${LASTSTART:-0}" ] && [ "${LAST2:-0}" -gt "${LASTSTART:-0}" ] || STATUS=1 ;;
	*) STATUS=1 ;;
esac
exit $STATUS
