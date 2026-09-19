# Movement and Pathfinding Benchmarks

Two deterministic benchmark harnesses score pathfinding and movement changes
as numbers instead of impressions. Every run is tick-reproducible: the RNG is
seeded per scenario, droid orders apply on fixed ticks, and repeating a run
byte-reproduces it, so any difference between two runs is a real difference
between two builds.

## Movement bench

    tests/movebench/run.sh                 # scorecard table for the current build
    tests/movebench/run.sh --score         # each metric beside its change vs the baseline
    tests/movebench/run.sh --score --markdown   # the same as a GitHub-ready table
    tests/movebench/run.sh --acceptance --score # the heavy real-map scenarios, judged by sums
    tests/movebench/run.sh --check         # verify every arrangement reproduces
    tests/movebench/run.sh --baseline      # regenerate the recorded baseline
    tests/movebench/crcs.sh                # capture every finalPositionsCrc, for refactors

Arguments after the mode flags pass through to the game binary, so a mechanism
under test rides along, ex. `run.sh --score --movementspread=field`. To score
against something other than the recorded baseline, name a live reference:
`--against='<args>'` runs a second sweep with those arguments and reports
changes against it.

Sweeps take `--concurrent=N` to keep that many game processes running at
once. Runs are independent processes collected in order, so the results are
identical and only wall-clock changes. Two or three suits a six-core machine.

A single scenario runs headless with `--movementbench=<name>` and prints a JSON
scorecard, `--movementarrangement=N` picks the spawn arrangement, and
`--movementbenchwatch=<name>` runs one on screen at normal speed. A watched run
bit-reproduces the headless run as long as no input is given, input forks the
sim from that point.

### Reading the numbers

Compare ranges, not numbers. Every cell runs over a family of spawn
arrangements because a single arrangement is one trajectory, not a sample:
shifting a spawn block by one tile moves arrival p95 by around a quarter, and
some cells resolve or jam depending on it. A change is only interesting when it
moves a metric clear of the recorded spread. Some cells are bimodal rather than
spread, a corridor either resolves or it jams, so RESOLVED counts arrangements
where most ordered units arrived instead of averaging across the modes.

The headline metrics: hardStops counts contact chain-stops (the "hit more than
one droid, stop dead" branch), grind is arrival seconds per tile against the
free-travel floor from the open-field scenario in the same run, and peak
density is the largest cluster of tracked droids within a tile and a half.

Two fields read as the feel of a run rather than its totals. Settle time is
when each share of the force came within six tiles of its goal for the last
time, so it tracks the visual moment a crowd looks "mostly there" where the
strict arrival tolerance does not, and `unitsNear` counts who ended inside
that radius. Stall share is the fraction of each unit's far-from-goal time
spent effectively stationary: hard stops count collisions, stall share counts
waiting, and a force can post few hard stops while spending most of its
journey standing in a queue. Time parked in the crowd around the destination
does not count. Time on a longer route counts as moving, so a
detour scores by its travel and the two together separate "slower but flowing"
from "faster but standing in line".

Detour compares the distance each unit drove against the straight line of its
leg, as a percentage, with `unitsDetoured` counting units half again over it.
Near-geodesic travel reads around 100 no matter how long it took, so the pair
of detour and stall share fingerprints how a config resolved contention:
coordinated threading is high stall with detour near the map's route floor,
rerouting around the problem is high detour with low stall, and churning
wander is high on both. A hard jam also reads high stall with low detour,
since a wedged unit barely drives, so settle time is what separates a queue
that drains from a jam that never does. Every route on a map carries that
map's winding floor over the straight line, so compare detour within a map
rather than against 100. Whether a high detour is a win depends on the map -
a spare route put to use is the point of tworoute, while a force splitting
onto a long loop to dodge a corner may read as pathfinding taking the scenic
route.

Each scorecard also reports hard-stop concentration: the distinct droids that
hard-stopped, the worst droid's count, and that count's share of the total.
hardStops flat while the share collapses means fewer stuck units, not less
congestion, and a high share fingers a single wedged unit inflating the count.
The acceptance tables print the worst arrangement's share beside the sums.

Five cells read inverted. enemyblock and enemyblock_press park units to deny a
chokepoint and measure that the denial holds, so a low RESOLVED count is the
pass condition. blob and strafe cannot reach the resolved threshold by
construction. counterflow_w1 is the ceiling case, opposing columns in a
single-file pass with nowhere to yield.

### The acceptance tier

`--acceptance` switches to the real-map scenarios: opposing cyborg and tank
flows with mid-transit re-orders through Sk-Mountain's chained passes
(mountain_chain, mountain_chain_cross) and Sk-Rush's center corridor
(rush_turn), plus the mass corner cases (rush_corner on Sk-Rush, open_corner
isolated on flat ground). These encode the manual test conditions for congested
opposing flows, they are heavy, and they are judged by hard stops summed across
arrangements, never a single arrangement, because several are bimodal and a
single-arrangement jump can be pure mode reshuffling. The converging
destinations park most units outside the arrival tolerance, the known base-game
parking behavior, so judge by density and hard stops, not RESOLVED.

### Adding a scenario

A scenario is a script and a config in data/mp/tests/, ex.
movebench_crossing.js with movebench_crossing.json, plus a row in the
BenchScenario table in src/movebench.cpp naming its config, tick budget and
seed. Scenarios run on shipped skirmish maps or on purpose-built ones under
data/mp/multiplay/maps/. A purpose-built bench map is registered in
data/mp/addon.lev with `type 15`, LEVEL_TYPE::SKIRMISH_HIDDEN, deliberately
not 14: the multiplayer map list accepts only the SKIRMISH and
MULTI_SKIRMISH2-4 types, so a hidden map stays out of skirmish and
multiplayer map selection while levFindDataSet() still resolves it by name
for the bench configs.

## Pathfinding bench

    tests/pathbench/run.sh                 # scorecard for canned route requests
    tests/pathbench/run.sh --baseline      # record it

Nodes expanded is exact and reproduces run to run, so it can say a change moved
nothing at all, which is the claim a planner refactor needs to make.
Microsecond timings are indicative only and move with machine load. The reuse
and distinct cases guard PathfindContext sharing, anything that makes search
cost depend on the individual unit collapses that sharing and shows up as the
pair diverging.

## Invariants for any change

A change meant to alter nothing must prove it: capture
`tests/movebench/crcs.sh` before and after, and an identical set means every
unit finished on the tile it finished on before, across the whole suite. A
change meant to alter behavior must still reproduce, `run.sh --check` runs
every arrangement twice and flags any mismatch, which catches a stray rand(), a
float in a position path, or a live read from a worker thread.

Scenario data lives in data/mp/tests/ and only takes effect after a full
`ninja -C build`. Building just the warzone2100 target leaves the packaged
mp.wz stale, and a scenario edit silently appears to have no effect.

## The pathfinding feature bitmask

Congestion features live behind independent bits of the synced
`game.pathfindingBackend` setting, declared in src/pathfinding_backend.h.
A new game starts with every feature on, zero is the legacy planner with none
of them, every client in a game runs the same value, and it travels with the
game everywhere simulation-affecting options do (saves, replays, etc).
The bits are independent so mechanisms can be measured alone and in
combination, and so a regression can be attributed to the bit that causes it.

`--pathfindingbackend=` forces a feature set for a run. It takes a combined
mask or a comma-separated list of values OR'd together, so a known stack plus
the flag under test needs no arithmetic, ex. `--pathfindingbackend=7,8`.
Values with bits no feature defines are rejected with the list of valid ones.

## Adding a feature

Append an enum value in src/pathfinding_backend.h with an accessor beside the
existing ones, and add the new value to the valid list, help text and rejection
message for `--pathfindingbackend` in clparse.cpp. The bit values are
serialized, so they are fixed once released, only append. Put every behavior
change behind the accessor: with the bit clear the simulation must be
byte-identical, which crcs.sh proves, and with it set the run must reproduce,
which --check proves.

## Producing results for a pull request

Score the synthetic suite and the acceptance tier, as markdown, against the
plain baseline:

    tests/movebench/run.sh --score --markdown --pathfindingbackend=<features>
    tests/movebench/run.sh --acceptance --score --markdown --pathfindingbackend=<features>

To show the marginal effect of one flag on top of an existing stack, score
against that stack instead:

    tests/movebench/run.sh --acceptance --score --markdown \
        --pathfindingbackend=<stack>,<flag> --against='--pathfindingbackend=<stack>'

The reference sweep runs with only the arguments named inside --against, so
anything the main sweep also needs, ex. a --configdir, is named again there.

Quote both tables. The synthetic cells respond to different mechanisms than the
real maps do, and a change that helps one repeatedly turns out to tax the
other.
