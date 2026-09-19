#!/bin/sh
# Runs the pathfinding benchmark and prints the scorecard, or with --baseline
# records it to tests/pathbench/baseline.json.
#
# Read the two columns differently. Nodes expanded is exact and reproduces run
# to run, so a refactor that leaves the search alone must move it by zero and
# any change at all is a change in behaviour. Microseconds move with load and
# thermal state on any machine, so treat a difference under about ten percent as
# nothing and re-run before believing a larger one.
#
# The pair to watch is reuse against distinct. Both issue the same number of
# requests from the same place, and differ only in whether they share a
# destination. Sharing one lets them share a PathfindContext, so reuse expands a
# fraction of what distinct does. Anything that makes the search cost depend on
# the individual unit collapses that sharing and moves every search into the
# distinct column, which is the regression that got the last rewrite reverted.
#
# Note that data/ changes only take effect after a full `ninja -C build`.

set -e

WZ=${WZ:-build/src/warzone2100}
REPEATS=${REPEATS:-5}
HERE=$(dirname "$0")

if [ ! -x "$WZ" ]; then
	echo "warzone2100 binary not found at $WZ (set WZ=path)" >&2
	exit 1
fi

SUB=""
case "$1" in
	--baseline) SUB="$1"; shift ;;
esac

python3 - "$WZ" "$SUB" "$HERE/baseline.json" "$REPEATS" "$@" <<'PYEOF'
import json, subprocess, sys

wz, sub, out, repeats = sys.argv[1:5]
passthrough = sys.argv[5:]
cmd = [wz, "--pathbench=default", "--pathbenchrepeats=" + repeats] + passthrough
raw = subprocess.run(cmd, capture_output=True, text=True).stdout
card = json.loads(raw[raw.index("{"):raw.rindex("}") + 1])

print("%-12s %8s %12s %10s %8s %8s" % ("CASE", "REQS", "NODES", "us_median", "OK", "NEAREST"))
for name, c in sorted(card["direct"].items()):
    print("%-12s %8d %12d %10d %8d %8d" % (
        name, c["requests"], c["nodesExpanded"], c["micros_median"],
        c["routesFound"], c["routesNearest"]))

q = card["queued"]
print("\nqueued through the worker pool: %d requests drained in %d ticks, %d us"
      % (q["requests"], q["drainTicks"], q["drainMicros"]))

direct = card["direct"]
if "reuse" in direct and "distinct" in direct and direct["reuse"]["nodesExpanded"]:
    ratio = direct["distinct"]["nodesExpanded"] / direct["reuse"]["nodesExpanded"]
    print("context reuse saves %.1fx the nodes of searching per destination" % ratio)

if sub == "--baseline":
    with open(out, "w") as f:
        json.dump({
            "_comment": "Baseline pathfinding benchmark scorecard. Nodes expanded is exact and must not move for a change that is meant to leave the search alone. Microseconds are indicative only. Regenerate with tests/pathbench/run.sh --baseline.",
            "card": card,
        }, f, indent=2, sort_keys=True)
        f.write("\n")
    print("wrote", out, file=sys.stderr)
PYEOF
