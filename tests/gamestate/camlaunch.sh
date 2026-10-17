#!/bin/sh
# Campaign transporter launch test: launches the player's transporter from the home base,
# as the launch button does (DORDER_TRANSPORTOUT),
# and checks the tile coverage of both worlds every tick (--gamestate-check-coverage=1).
#
# A global test mod (built here from the repository's campaign scripts)
# gives CAM_2A a transporter with a tank aboard, and orders it out at 15 s.
# Two variants:
# - offworld: CAM_2A is made a pre-off-world level, as the levels that set up the player's transporter are,
#   so the transporter's exit wins the level and the game loads the off-world mission SUB_2_1,
#   where the transporter flies in.
#   The home world, which the level change keeps, must not keep any coverage of the transporter.
# - stay: CAM_2A's own conditions, so the exit changes nothing and the transporter stays in the world.
#   Its coverage must be what a recompute gives.
#
# usage: camlaunch.sh <warzone2100 binary>
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 300 seconds),
#              EXTRA (more game options for both runs).
# Exit status: 0 passed, 1 a coverage check found a difference, 2 a run did not get through the launch.
set -u
BIN=${1:?binary}
HERE=$(cd "$(dirname "$0")" && pwd)
CAMSCRIPTS="$HERE/../../data/base/script/campaign"
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-300}
CFG="$OUT/config"

# build_mod <variant>
build_mod() {
	MOD="$OUT/camlaunch-mod-$1"
	rm -rf "$MOD"
	mkdir -p "$MOD/script/campaign" "$CFG/mods/global"
	sed 's/^function eventStartLevel()/function __testOrigStartLevel()/' "$CAMSCRIPTS/cam2-a.js" > "$MOD/script/campaign/cam2-a.js"
	cat >> "$MOD/script/campaign/cam2-a.js" <<EOF

// camlaunch.sh: a transporter with a tank aboard, ordered out at 15 s
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
	var t = getObject(DROID, CAM_HUMAN_PLAYER, __testTransporter);
	debug("camlaunch: launching a transporter with " + enumCargo(t).length + " droid(s) aboard");
	if ("$1" === "offworld")
	{
		camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, cam_levels.beta2.offWorld);
	}
	orderDroidLoc(t, 22, 87, 126);  // DORDER_TRANSPORTOUT, as launchTransporter() gives
}
function eventTransporterExit(t)
{
	if (t.id === __testTransporter)
	{
		debug("camlaunch: the transporter exited at " + gameTime);
	}
}
function eventStartLevel()
{
	__testOrigStartLevel();
	queue("__testSetup", 2000);
	queue("__testLaunch", 15000);
}
EOF
	sed 's/^function eventStartLevel()/function __testOrigStartLevel()/' "$CAMSCRIPTS/cam2-1x.js" > "$MOD/script/campaign/cam2-1x.js"
	cat >> "$MOD/script/campaign/cam2-1x.js" <<'EOF'

// camlaunch.sh
function eventStartLevel()
{
	debug("camlaunch: SUB_2_1 started at " + gameTime);
	__testOrigStartLevel();
}
EOF
	python3 -I - "$MOD" "$CFG/mods/global/camlaunch-$1.wz" <<'EOF'
import os, sys, zipfile
root, dest = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(dest, "w") as z:
    for d, _, files in os.walk(root):
        for f in files:
            z.write(os.path.join(d, f), os.path.relpath(os.path.join(d, f), root))
EOF
}

STATUS=0
for VARIANT in offworld stay; do
	build_mod "$VARIANT"
	LOG="$OUT/camlaunch.$VARIANT.log"
	END=40000
	[ "$VARIANT" = stay ] && END=30000
	timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" --headless --autogame --mod="camlaunch-$VARIANT.wz" --game=CAM_2A \
		--gamestate-test-seed=1 --gamestate-check-coverage=1 --gamestate-exit-at-tick=$END ${EXTRA:-} > "$LOG" 2>&1
	EXITED=$(grep -c 'camlaunch: the transporter exited' "$LOG")
	STARTED=$(grep -c 'camlaunch: SUB_2_1 started' "$LOG")
	CHECKS=$(grep -c 'coverage check (' "$LOG")
	echo "camlaunch $VARIANT: exited $EXITED, SUB_2_1 started $STARTED, $CHECKS coverage check(s) with a difference (log: $LOG)"
	grep 'coverage check (' "$LOG" | head -1 | sed 's/^.*GameState harness: /  /'
	if [ "$EXITED" -ne 1 ] || { [ "$VARIANT" = offworld ] && [ "$STARTED" -ne 1 ]; } || { [ "$VARIANT" = stay ] && [ "$STARTED" -ne 0 ]; }; then
		[ "$STATUS" -eq 0 ] && STATUS=2
	elif [ "$CHECKS" -gt 0 ]; then
		STATUS=1
	fi
done
exit $STATUS
