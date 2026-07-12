include("tests/movebench_common.js");

// A single cluster rounding a corner into a narrow passage, on the L-shaped map.
//
// One cluster, one direction, no opposing flow, so this is congestion and
// serialisation rather than a head-on conflict.
//
// Corners bunch for a specific reason. moveBestTarget picks its aim point by
// ray-casting line of sight, and a corner wall blocks sight of any waypoint
// past the bend, so every unit's look-ahead stops at the same inside-corner
// tile until it physically rounds it. That tile becomes a point attractor for
// the whole cluster, on top of them all sharing one racing line.
//
// A 2-wide pinch admits only so many units per second, so the goal is an
// orderly, prompt queue rather than the absence of a queue. Watch bumps and
// hardStops rather than raw arrival time.

const ME = 0;
const N_UNITS = 24;

const X_STAGE = 27;      // 8-wide block straddling the corridor mouth at x=30,31
const Y_STAGE = 10;
const X_GOAL = 53;       // inside the goal area east of the corner
const Y_GOAL = 28;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var cluster = benchSpawnBlock(ME, "medium", X_STAGE, Y_STAGE, 8, +1, N_UNITS);
	hackNetOn();

	benchOrderFanOut(cluster, X_GOAL, Y_GOAL, 8, +1);

	debug("movebench: corner, " + cluster.length + " units round the bend");
}
