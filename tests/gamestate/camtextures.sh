#!/bin/sh
# Replaced textures across campaign level changes: a game loaded from a save must show the same textures
# as the game that wrote it, on the level it was saved on and on the levels after it.
#
# replaceTexture() loads another file into a texture page, and texture pages last for the whole game,
# so a replacement carries over the levels of a campaign
# (a later level whose models need the page under its original name loads it again).
# A save keeps the replaced pages (the presentation section's "replacedTextures"),
# and a load makes the pages match.
#
# A global test mod (built here from the repository's campaign scripts) plays CAM_2A,
# replaces the scavengers' texture at 2 s
# (the rules already replaced it with the urban one when the level started, so this chains a second replacement),
# and then goes on without a player:
# CAM_2A -> SUB_2_1 (off-world, same data set) -> CAM_2B (home, same data set)
# -> CAM_3A (another data set, whose models load the scavengers' original texture again,
#    and the rules replace it again).
# --gamestate-test-continue-levels goes past the mission result screens
# and logs the replaced textures when each level starts.
#
# Run A plays it all and saves at 10000 (on CAM_2A, after the replacement),
# and run B loads that save and plays the same levels.
# Every level start after the save must log the same replaced textures in both runs,
# and run B's load must log what run A's save has.
# Run B also saves at 20000, on CAM_3A, where the game has two pages replaced the same way
# (the CAM_2 one and the one CAM_3A loaded again), and run C loads that save and saves it again at once.
# Both must keep the same replacements.
#
# All three runs use the GUI under a virtual display (Xvfb):
# a headless game doesn't load the models' textures, so it has nothing to replace.
# The mod turns off the guide topics' pop-ups and the videos, which pause the game with the GUI.
#
# usage: camtextures.sh <warzone2100 binary>
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 300 seconds),
#              EXTRA (more game options for every run), DISPLAYNUM (default 76).
# Exit status: 0 passed, 1 the runs differ, 2 a run did not get through the levels, or Xvfb is missing.
set -u
BIN=${1:?binary}
HERE=$(cd "$(dirname "$0")" && pwd)
CAMSCRIPTS="$HERE/../../data/base/script/campaign"
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-300}
CFG="$OUT/config"
MOD="$OUT/camtextures-mod"
SAVETICK=10000
ENDTICK=25000
SAVE=camtextures_$SAVETICK
SAVETICK2=20000
SAVE2=camtextures_$SAVETICK2
DISPLAYNUM=${DISPLAYNUM:-76}
command -v Xvfb > /dev/null || { echo "camtextures: no Xvfb"; exit 2; }
rm -rf "$MOD" "$CFG/savegames/campaign/$SAVE" "$CFG/savegames/campaign/$SAVE.gam" "$CFG/savegames/campaign/$SAVE2" \
	"$CFG/savegames/campaign/$SAVE2.gam" "$CFG/savegames/campaign/${SAVE2}_resave" "$CFG/savegames/campaign/${SAVE2}_resave.gam"
mkdir -p "$MOD/script/campaign" "$CFG/mods/global"

# make_advancing <script> <level name expression> <delay ms> [<more JS for eventStartLevel>]:
# a copy of a campaign script whose eventStartLevel also queues a call to camNextLevel
make_advancing() {
	sed 's/^function eventStartLevel()/function __testOrigStartLevel()/' "$CAMSCRIPTS/$1" > "$MOD/script/campaign/$1"
	cat >> "$MOD/script/campaign/$1" <<EOF

// camtextures.sh: no guide topic pop-ups or videos (with the GUI, they pause the game)
addGuideTopic = function() {};
camPlayVideos = function() {};

// camtextures.sh: advance without a player
function eventStartLevel()
{
	__testOrigStartLevel();
	queue("__testAdvance", $3);
	${4:-}
}
function __testAdvance()
{
	camNextLevel($2);
}
EOF
}
make_advancing cam2-a.js cam_levels.beta2.offWorld 15000 'queue("__testReplace", 2000);'
cat >> "$MOD/script/campaign/cam2-a.js" <<'EOF'
function __testReplace()
{
	const replaced = replaceTexture("page-7-barbarians-urban.png", "page-7-barbarians-kevlar.png");
	debug("camtextures: replaceTexture at gameTime " + gameTime + ": " + replaced);
}
EOF
make_advancing cam2-1x.js cam_levels.beta3 300
make_advancing cam2-b.js cam_levels.gamma1 300
# CAM_3A only stays
{ cat "$CAMSCRIPTS/cam3-a.js"; printf '\n// camtextures.sh: no guide topic pop-ups or videos (with the GUI, they pause the game)\naddGuideTopic = function() {};\ncamPlayVideos = function() {};\n'; } \
	> "$MOD/script/campaign/cam3-a.js"
python3 -I - "$MOD" "$CFG/mods/global/camtextures.wz" <<'EOF'
import os, sys, zipfile
root, dest = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(dest, "w") as z:
    for d, _, files in os.walk(root):
        for f in files:
            z.write(os.path.join(d, f), os.path.relpath(os.path.join(d, f), root))
EOF

Xvfb ":$DISPLAYNUM" -screen 0 1024x768x24 > "$OUT/camtextures.xvfb.log" 2>&1 &
XVFB=$!
trap 'kill $XVFB 2> /dev/null' EXIT
sleep 2
export DISPLAY=":$DISPLAYNUM"
GUI="--nosound --window --resolution=1024x768"
# shellcheck disable=SC2086
timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" $GUI --autogame --mod=camtextures.wz --game=CAM_2A --gamestate-test-seed=1 \
	--gamestate-test-continue-levels --gamestate-save-at-tick="$SAVETICK:savegames/campaign/$SAVE.gam" \
	--gamestate-exit-at-tick="$ENDTICK" ${EXTRA:-} > "$OUT/camtextures.A.log" 2>&1
[ -f "$CFG/savegames/campaign/$SAVE/gamestate.wz" ] || { echo "camtextures: RUN A WROTE NO SAVE (see $OUT/camtextures.A.log)"; exit 2; }
# shellcheck disable=SC2086
timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" $GUI --autogame --mod=camtextures.wz --loadcampaign="$SAVE" \
	--gamestate-test-continue-levels --gamestate-save-at-tick="$SAVETICK2:savegames/campaign/$SAVE2.gam" \
	--gamestate-exit-at-tick="$ENDTICK" ${EXTRA:-} > "$OUT/camtextures.B.log" 2>&1
# shellcheck disable=SC2086
timeout "$RUN_TIMEOUT" "$BIN" --configdir="$CFG" $GUI --autogame --mod=camtextures.wz --loadcampaign="$SAVE2" \
	--gamestate-resave-on-load="savegames/campaign/${SAVE2}_resave.gam" ${EXTRA:-} > "$OUT/camtextures.C.log" 2>&1

# levels <log>: the level starts after the save, as "<level> [<replaced textures>]"
levels() {
	sed -n 's/.*GameState harness: level \([^ ]*\) started at gameTime \([0-9]*\); replaced textures: \(.*\)/\2 \1 \3/p' "$1" \
		| awk -v t="$SAVETICK" '$1 > t + 2 { $1 = ""; print substr($0, 2) }'
}
LA=$(levels "$OUT/camtextures.A.log")
LB=$(levels "$OUT/camtextures.B.log")
FAIL=""
grep -q "camtextures: replaceTexture at gameTime [0-9]*: true" "$OUT/camtextures.A.log" || FAIL="$FAIL; run A's replaceTexture didn't replace"
echo "$LA" | grep -q "^CAM_3A " || FAIL="$FAIL; run A didn't reach CAM_3A (see $OUT/camtextures.A.log)"
[ "$LA" = "$LB" ] || FAIL="$FAIL; the level starts after the save differ"
LOADED=$(sed -n 's/.*GameState harness: level [^ ]* started at gameTime [0-9]*; replaced textures: \(.*\)/\1/p' "$OUT/camtextures.B.log" | head -1)
# saved <save>: the replaced textures a save has
saved() {
	python3 -I -c '
import json, sys, zipfile
d = json.loads(zipfile.ZipFile(sys.argv[1]).read("gamestate.json"))
print("[" + ", ".join(a + " -> " + b for a, b in d["gameState"]["presentation"].get("replacedTextures", [])) + "]")' \
		"$CFG/savegames/campaign/$1/gamestate.wz" 2> /dev/null || echo "(no save)"
}
SAVED=$(saved "$SAVE")
[ "$LOADED" = "$SAVED" ] || FAIL="$FAIL; run B loaded $LOADED, the save has $SAVED"
SAVED2=$(saved "$SAVE2")
RESAVED2=$(saved "${SAVE2}_resave")
[ "$SAVED2" != "(no save)" ] && [ "$SAVED2" = "$RESAVED2" ] || FAIL="$FAIL; the CAM_3A save has $SAVED2, saved again $RESAVED2"
# Not counted:
# - openLoadFile for multiplay/skirmish/semperfi.js (--autogame gives the player an AI that campaign data lacks)
# - widgDisplayScreen's invalid screen pointer when a game with the GUI starts from the command line
# - in run C, which --gamestate-resave-on-load quits before its first frame,
#   the two text labels the renderer reports not cleaned up
for run in A B C; do
	grep "Assert in Warzone" "$OUT/camtextures.$run.log" | grep -v "frame.cpp:152\|widget.cpp:1791" \
		| { [ $run = C ] && grep -v "gfx_api_gl.cpp:[0-9]* (debugLiveTextures.empty())" || cat; } | grep -q . \
		&& FAIL="$FAIL; assertion(s) in run $run"
done

echo "camtextures (saved on CAM_2A $SAVED, on CAM_3A $SAVED2, again $RESAVED2):"
echo "$LA" | sed 's/^/  A: /'
echo "$LB" | sed 's/^/  B: /'
[ -z "$FAIL" ] && { echo "camtextures: IDENTICAL"; exit 0; }
echo "camtextures: FAILED:${FAIL#;}"
exit 1
