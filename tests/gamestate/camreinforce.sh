#!/bin/sh
# Campaign reinforcement test: reinforcements sent during an off-world mission
# must fly in at a tick the game state decides (missionReinforcementUpdate),
# the same headless, with the GUI, and after a save and load while they wait.
#
# A global test mod (built here from the repository's campaign scripts)
# sends the player's transporter from CAM_2A's home base into the off-world mission SUB_2_1
# (as camlaunch.sh's offworld variant does), and gives SUB_2_1 reinforcements every 30 s.
# There the transporter lands, unloads and flies home.
# At <reinforce tick>, --gamestate-test-reinforce puts a home base droid aboard and launches it,
# as the transporter screen does.
# The reinforcements must start flying in (eventTransporterArrived) 10 s before the 30 s are up.
#
#   run A:    headless, to <end tick>, tracing sync CRCs
#   fork:     forkcrc.sh: saved at <save tick> (while the reinforcements wait), loaded, compared with a run A
#   GUI:      the same game with the GUI (xvfb-run, if installed), compared with run A
#
# usage: camreinforce.sh <warzone2100 binary> [<reinforce tick> <save tick> <end tick>]   (default 33000 45000 70000)
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 900 seconds), SEED (default 1).
# Exit status: 0 passed, 1 a check failed, 2 a run failed.
set -u
BIN=${1:?binary}
REINFORCE=${2:-33000}
SAVE=${3:-45000}
END=${4:-70000}
HERE=$(cd "$(dirname "$0")" && pwd)
CAMSCRIPTS="$HERE/../../data/base/script/campaign"
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-900}
SEED=${SEED:-1}
CFG="$OUT/config"
TAG="camreinforce.$REINFORCE.$END"

MOD="$OUT/camreinforce-mod"
rm -rf "$MOD"
mkdir -p "$MOD/script/campaign" "$CFG/mods/global"
sed 's/^function eventStartLevel()/function __testOrigStartLevel()/' "$CAMSCRIPTS/cam2-a.js" > "$MOD/script/campaign/cam2-a.js"
cat >> "$MOD/script/campaign/cam2-a.js" <<'EOF'

// camreinforce.sh: a transporter with a tank aboard, sent out at 15 s. Its exit loads the off-world mission SUB_2_1
var __testTransporter = null;
function __testSetup()
{
	var t = addDroid(CAM_HUMAN_PLAYER, 91, 111, cam_trComps.name, cam_trComps.body, cam_trComps.propulsion, "", "", cam_trComps.weapon);
	setTransporterExit(87, 126, CAM_HUMAN_PLAYER);
	__testTransporter = t.id;
	orderDroidObj(addDroid(CAM_HUMAN_PLAYER, 91, 113, "Cargo Tank", "Body5REC", "tracked01", "", "", "MG1Mk1"), DORDER_EMBARK, t);
}
function __testLaunch()
{
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, cam_levels.beta2.offWorld);
	orderDroidLoc(getObject(DROID, CAM_HUMAN_PLAYER, __testTransporter), 22, 87, 126);  // DORDER_TRANSPORTOUT
}
function eventStartLevel()
{
	__testOrigStartLevel();
	queue("__testSetup", 2000);
	queue("__testLaunch", 15000);
}
EOF
# SUB_2_1 with reinforcements every 30 s, and the transporter's comings and goings logged
sed -e 's/^function eventStartLevel()/function __testOrigStartLevel()/' -e 's/reinforcements: -1,/reinforcements: 30,/' \
	"$CAMSCRIPTS/cam2-1x.js" > "$MOD/script/campaign/cam2-1x.js"
grep -q "reinforcements: 30," "$MOD/script/campaign/cam2-1x.js" || { echo "$TAG: COULDN'T GIVE SUB_2_1 REINFORCEMENTS"; exit 2; }
cat >> "$MOD/script/campaign/cam2-1x.js" <<'EOF'

// camreinforce.sh
function eventStartLevel()
{
	debug("camreinforce: SUB_2_1 started at " + gameTime);
	__testOrigStartLevel();
}
function eventTransporterLanded(t) { debug("camreinforce: transporter landed at " + gameTime); }
function eventTransporterExit(t) { debug("camreinforce: transporter exited at " + gameTime); }
function eventTransporterArrived(t) { debug("camreinforce: reinforcements arrived at " + gameTime); }
EOF
python3 -I - "$MOD" "$CFG/mods/global/camreinforce.wz" <<'EOF'
import os, sys, zipfile
root, dest = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(dest, "w") as z:
    for d, _, files in os.walk(root):
        for f in files:
            z.write(os.path.join(d, f), os.path.relpath(os.path.join(d, f), root))
EOF

GAME="--mod=camreinforce.wz --game=CAM_2A --gamestate-test-seed=$SEED --gamestate-test-reinforce=$REINFORCE"
arrived() { sed -n 's/.*camreinforce: reinforcements arrived at \([0-9]*\).*/\1/p' "$1" | tr '\n' ' ' | sed 's/ $//'; }
FAIL=""

# Run A
# shellcheck disable=SC2086
timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" --headless --autogame $GAME --gamestate-crc-trace="$OUT/$TAG.A.trace" \
	--gamestate-exit-at-tick="$END" > "$OUT/$TAG.A.log" 2>&1 || { echo "$TAG: RUN A FAILED (see $OUT/$TAG.A.log)"; exit 2; }
grep -q "GameState harness: reinforcements launched" "$OUT/$TAG.A.log" || { echo "$TAG: REINFORCEMENTS NOT LAUNCHED (see $OUT/$TAG.A.log)"; exit 2; }
ARRIVED=$(arrived "$OUT/$TAG.A.log")
# 30 s after the launch at the first capture point at or after REINFORCE, less the 10 s lead-in: the tick after
EXPECTED=$(( (REINFORCE / 100 * 100 + 2) + 30000 - 10000 + 100 ))
[ "$ARRIVED" = "$EXPECTED" ] || FAIL="$FAIL; headless arrival at '$ARRIVED', expected $EXPECTED"

# Fork: saved while the reinforcements wait
FORK=$(EXTRA="--mod=camreinforce.wz --gamestate-test-reinforce=$REINFORCE" OUT="$OUT" sh "$HERE/forkcrc.sh" "$BIN" cam:CAM_2A "$SAVE" "$END" --no-loaded | head -1)
echo "$FORK" | grep -q ": IDENTICAL" || FAIL="$FAIL; fork: $FORK"
FORKLOG=$(ls -t "$OUT"/cam_CAM_2A."$SAVE"*.B.log 2>/dev/null | head -1)
FORKARRIVED=$( [ -n "$FORKLOG" ] && arrived "$FORKLOG")
[ "$FORKARRIVED" = "$EXPECTED" ] || FAIL="$FAIL; loaded game's arrival at '$FORKARRIVED'"

# GUI
GUIRESULT="skipped (no xvfb-run)"
if command -v xvfb-run > /dev/null; then
	# shellcheck disable=SC2086
	timeout "$RUN_TIMEOUT" xvfb-run -a -s "-screen 0 1280x800x24" "$BIN" --configdir="$CFG" --nosound --window --resolution=1024x768 \
		--autogame $GAME --gamestate-crc-trace="$OUT/$TAG.gui.trace" --gamestate-exit-at-tick="$END" > "$OUT/$TAG.gui.log" 2>&1 \
		|| { echo "$TAG: GUI RUN FAILED (see $OUT/$TAG.gui.log)"; exit 2; }
	GUIARRIVED=$(arrived "$OUT/$TAG.gui.log")
	[ "$GUIARRIVED" = "$EXPECTED" ] || FAIL="$FAIL; GUI arrival at '$GUIARRIVED'"
	GUIRESULT=$(python3 -I "$HERE/cmptrace.py" "$OUT/$TAG.A.trace" "$OUT/$TAG.gui.trace" --after 0 --until "$END") || FAIL="$FAIL; GUI vs headless differs"
fi

echo "$TAG (reinforcements launched at the first tick from $REINFORCE, arrived at ${ARRIVED:-never}; expected $EXPECTED): fork saved at $SAVE: ${FORK#*: }; GUI vs headless: $GUIRESULT${FAIL:+; FAILED:${FAIL#;}}"
[ -z "$FAIL" ] && exit 0
exit 1
