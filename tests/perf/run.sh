#!/bin/sh
# Runs the perf_* scenarios with --perfcounters and prints the per-tick mean of
# every counter over each run. Needs a binary configured with
# -DWZ_PERF_COUNTERS=ON, which is off by default.
#
# The mean covers the whole run by default. These scenarios are not steady state
# - units die and structures fall, so a trailing window measures the aftermath
# rather than the thing the scenario is for. Set WINDOW=N for the last N ticks.
#
# Read timers and counts differently. Counts (C_*) are exact and reproduce run
# to run, so a change meant to remove work must move them and a change meant to
# remove none must leave them alone. Timers (T_*) are nanoseconds and move with
# load and thermal state, so treat a difference under about ten percent as
# nothing.
#
# The raw per-tick CSVs are left in $OUT, so set OUT=<dir> to keep the run a
# later one is compared against.
#
# Note that data/ changes only take effect after a full `ninja -C build`.
set -e

WZ=${WZ:-build/src/warzone2100}
SCENARIOS=${SCENARIOS:-"perf_cyborg_blob perf_battle perf_walled_assault perf_artillery perf_vtol_rearm perf_scout_patrol perf_repair_search perf_repair_facility"}
WINDOW=${WINDOW:-0}
OUT=${OUT:-/tmp/perfcounters}

if [ ! -x "$WZ" ]; then
	echo "warzone2100 binary not found at $WZ (set WZ=path)" >&2
	exit 1
fi

mkdir -p "$OUT"

for s in $SCENARIOS; do
	"$WZ" --movementbench="$s" --perfcounters="$OUT/$s.csv" "$@" >/dev/null 2>&1 || {
		echo "$s: run failed" >&2
		exit 1
	}
done

python3 - "$OUT" "$WINDOW" $SCENARIOS <<'PYEOF'
import csv, sys

out, window = sys.argv[1], int(sys.argv[2])
scenarios = sys.argv[3:]

means = {}
for s in scenarios:
    with open("%s/%s.csv" % (out, s)) as f:
        rows = list(csv.DictReader(f))
    tail = rows[-window:] if window else rows
    means[s] = {k: sum(int(r[k]) for r in tail) / len(tail) for k in rows[0] if k != "gameTime"}

columns = list(means[scenarios[0]].keys())
print("%-30s %s" % ("COUNTER", " ".join("%14s" % s for s in scenarios)))
for c in columns:
    values = [means[s][c] for s in scenarios]
    if not any(values):
        continue
    print("%-30s %s" % (c, " ".join("%14.1f" % v for v in values)))
PYEOF
