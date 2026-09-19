include("tests/movebench_common.js");

// Mixed-speed cluster rounding the same corner.
//
// Interleaving medium and heavy tanks adds the catch-up pile-up. Faster units
// reach the corner attractor and stack behind slower ones, and a heavy tank's
// slower pivot at the bend stalls everything behind it.
//
// The uniform corner scenario is the control that says any difference came from
// speed heterogeneity rather than from the geometry.

const ME = 0;
const N_UNITS = 24;

const X_STAGE = 27;
const Y_STAGE = 10;
const X_GOAL = 53;
const Y_GOAL = 28;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	benchEnable(ME, "heavy");
	var cluster = benchSpawnMixedBlock(ME, ["medium", "heavy"], X_STAGE, Y_STAGE, 8, +1, N_UNITS);
	hackNetOn();

	benchOrderFanOut(cluster, X_GOAL, Y_GOAL, 8, +1);

	debug("movebench: corner_mixed, " + cluster.length + " units round the bend");
}
