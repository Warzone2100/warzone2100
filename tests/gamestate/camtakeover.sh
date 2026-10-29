#!/bin/sh
# Campaign structure takeover test.
# A test mod builds the player a cyborg factory on CAM_3A's map, rotated by 90 degrees, on the shore of deep water,
# and donates it to NEXUS (a campaign game rebuilds a donated structure as a takeover does).
# A forkcrc.sh cell then saves after the takeover.
# The save must be complete (its legacy files too), and cmptakeover.py checks its GameState document.
# The save is then loaded and saved again (doublesave.sh).
#
# usage: camtakeover.sh <warzone2100 binary>
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 600 seconds), EXTRA (more game options).
# Exit status: 0 passed, 1 failed, 2 a run failed or no site was found.
set -u
BIN=${1:?binary}
HERE=$(cd "$(dirname "$0")" && pwd)
CAMSCRIPTS="$HERE/../../data/base/script/campaign"
OUT=${OUT:-/tmp/gamestate-forkcrc}
CFG="$OUT/config"
MOD="$OUT/camtakeover-mod"
SAVETICK=20000
ENDTICK=40000
rm -rf "$MOD"
mkdir -p "$MOD/script/campaign" "$CFG/mods/global"

sed 's/^function eventStartLevel()/function __testOrigStartLevel()/' "$CAMSCRIPTS/cam3-a.js" > "$MOD/script/campaign/cam3-a.js"
cat >> "$MOD/script/campaign/cam3-a.js" <<'EOF'

// camtakeover.sh
const __TEST_WATER = 7; // TER_WATER
function __testIsWater(x, y)
{
	return terrainType(x, y) === __TEST_WATER;
}
function __testIsLand(x, y)
{
	const t = terrainType(x, y);
	return t !== __TEST_WATER && t !== 8; // not water, not cliff
}
function eventStartLevel()
{
	__testOrigStartLevel();
	queue("__testBuildFactory", 2000);
}
function __testBuildFactory()
{
	// Facing 90 degrees, the factory covers tiles (x - 1, y) and (x, y), with deep water north of it
	for (let strict = 1; strict >= 0; --strict)
	{
		for (let y = 6; y < mapHeight - 6; ++y)
		{
			for (let x = 6; x < mapWidth - 6; ++x)
			{
				if (!__testIsLand(x - 1, y) || !__testIsLand(x, y) || !__testIsLand(x - 1, y + 1) || !__testIsLand(x, y + 1)
				    || !__testIsWater(x, y - 1) || !__testIsWater(x + 1, y - 1) || !__testIsWater(x, y - 2) || !__testIsWater(x + 1, y - 2)
				    || (strict && (!__testIsWater(x - 1, y - 1) || !__testIsWater(x - 1, y - 2))))
				{
					continue;
				}
				if (enumRange(x, y, 4, ALL_PLAYERS, false).length > 0)
				{
					continue;
				}
				const factory = addStructure("A0CyborgFactory", CAM_HUMAN_PLAYER, x * 128, y * 128 + 64, 90);
				if (!factory)
				{
					continue;
				}
				debug("camtakeover: site " + x + " " + y);
				setStructureLimits("A0CyborgFactory", 50, CAM_NEXUS);
				if (!donateObject(factory, CAM_NEXUS))
				{
					debug("camtakeover: donateObject failed");
				}
				__testSite = {x: x, y: y};
				queue("__testCheckFactory", 2000);
				return;
			}
		}
	}
	debug("camtakeover: no site found");
}
var __testSite;
function __testCheckFactory()
{
	const factories = enumStruct(CAM_NEXUS, "A0CyborgFactory").filter((s) => (Math.abs(s.x - __testSite.x) <= 1 && Math.abs(s.y - __testSite.y) <= 1));
	debug("camtakeover: NEXUS has " + factories.length + " cyborg factory(ies) at the site" + factories.map((s) => (" (" + s.x + "," + s.y + ")")).join(""));
}
EOF
python3 -I - "$MOD" "$CFG/mods/global/camtakeover.wz" <<'EOF'
import os, sys, zipfile
root, dest = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(dest, "w") as z:
    for d, _, files in os.walk(root):
        for f in files:
            z.write(os.path.join(d, f), os.path.relpath(os.path.join(d, f), root))
EOF

STATUS=0
EXTRA="--mod=camtakeover.wz ${EXTRA:-}" OUT="$OUT" "$HERE/forkcrc.sh" "$BIN" cam:CAM_3A $SAVETICK $ENDTICK --no-loaded
FORK=$?
[ $FORK -eq 2 ] && exit 2
[ $FORK -ne 0 ] && STATUS=1
TAG="cam_CAM_3A.$SAVETICK.noloaded"
SAVENAME="fork_$(echo "$TAG" | tr '.' '_')"
SAVE="$CFG/savegames/campaign/$SAVENAME"
SITE=$(sed -n 's/.*camtakeover: site \([0-9]*\) \([0-9]*\).*/\1 \2/p' "$OUT/$TAG.A.log" | head -1)
grep -h 'camtakeover:' "$OUT/$TAG.A.log" | sed 's/^.*camtakeover: /  /'
[ -n "$SITE" ] || { echo "camtakeover: NO SITE (see $OUT/$TAG.A.log)"; exit 2; }

for f in game.map struct.json ttypes.ttp gamestate.wz; do
	[ -f "$SAVE/$f" ] || { echo "camtakeover: the save has no $f (see $OUT/$TAG.A.log)"; STATUS=1; }
done
BAD=$(grep -hE 'Clamping tile|Legacy savegame files .* are incomplete|exceeds maximum tile height' "$OUT/$TAG.A.log" "$OUT/$TAG.B.log")
[ -z "$BAD" ] || { echo "camtakeover: saving went wrong:"; echo "$BAD" | head -3 | sed 's/^/  /'; STATUS=1; }

# shellcheck disable=SC2086 # SITE is "x y"
python3 -I "$HERE/cmptakeover.py" "$SAVE" $SITE || STATUS=1

OUT="$OUT" EXTRA="--mod=camtakeover.wz" "$HERE/doublesave.sh" "$BIN" "$SAVENAME" || STATUS=1
exit $STATUS
