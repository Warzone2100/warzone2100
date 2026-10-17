#!/bin/sh
# Between-ticks save test: a fork cell saved between two ticks,
# right after the game loop has processed a message that changed a player's sensor range,
# so the tile coverage changes it implies are still pending at the save.
#
# A global test mod (built here from the repository's movement bench script)
# makes mb:perf_battle send a script sync request at 20 s,
# and its eventSyncRequest handler (run between two ticks, when the request comes back)
# raises player 0's sensor ranges.
# Run A saves at the first point after 20 s where coverage changes are pending (--coverage-pending),
# and run B loads that save.
#
# usage: betweenticks.sh <warzone2100 binary>
# Environment: as forkcrc.sh (OUT, RUN_TIMEOUT, SEED, CHECK_COVERAGE).
# Exit status: as forkcrc.sh.
set -u
BIN=${1:?binary}
HERE=$(cd "$(dirname "$0")" && pwd)
TESTS="$HERE/../../data/mp/tests"
OUT=${OUT:-/tmp/gamestate-forkcrc}
CFG="$OUT/config"
MOD="$OUT/betweenticks-mod"
rm -rf "$MOD"
mkdir -p "$MOD/tests" "$CFG/mods/global"

sed 's/^function eventStartLevel()/function __testOrigStartLevel()/' "$TESTS/movebench_perf_battle.js" > "$MOD/tests/movebench_perf_battle.js"
cat >> "$MOD/tests/movebench_perf_battle.js" <<'EOF'

// betweenticks.sh: change a player's sensor ranges between two ticks
function eventStartLevel()
{
	__testOrigStartLevel();
	queue("__testRequest", 20000);
}
function __testRequest()
{
	syncRequest(4242, 1, 1);
}
function eventSyncRequest(from, reqId, x, y, obj1, obj2)
{
	if (reqId !== 4242)
	{
		return;
	}
	for (var name in Upgrades[0].Sensor)
	{
		Upgrades[0].Sensor[name].Range = Upgrades[0].Sensor[name].Range + 512;
	}
	debug("betweenticks: raised player 0's sensor ranges at gameTime " + gameTime);
}
EOF
python3 -I - "$MOD" "$CFG/mods/global/betweenticks.wz" <<'EOF'
import os, sys, zipfile
root, dest = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(dest, "w") as z:
    for d, _, files in os.walk(root):
        for f in files:
            z.write(os.path.join(d, f), os.path.relpath(os.path.join(d, f), root))
EOF

EXTRA="--mod=betweenticks.wz" OUT="$OUT" exec "$HERE/forkcrc.sh" "$BIN" mb:perf_battle 20000 60000 --no-loaded --coverage-pending
