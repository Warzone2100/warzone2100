include("tests/movebench_common.js");

// Conflicting diagonal flows whose paths cross inside the pass.
//
// Both blocks approach the gap from the west and leave to the east while
// swapping sides of the wall. The north block runs north-west to south-east,
// the south block runs south-west to north-east, so their shortest paths
// intersect inside the passage rather than merely meeting head-on.
//
// Lane formation alone cannot resolve this, because the optimal lines genuinely
// cross. Compare against the separating scenario, which is the same map and the
// same two blocks with the diagonals arranged so they do not conflict.

const ME = 0;
const N_PER_SIDE = 16;

const X_WEST = 22;       // approach side for both blocks
const X_EAST = 38;       // exit side for both blocks
const Y_NORTH = 24;
const Y_SOUTH = 42;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var north = benchSpawnBlock(ME, "medium", X_WEST, Y_NORTH, 4, -1, N_PER_SIDE);
	var south = benchSpawnBlock(ME, "medium", X_WEST, Y_SOUTH, 4, +1, N_PER_SIDE);
	hackNetOn();

	benchOrderFanOut(north, X_EAST, Y_SOUTH, 8, +1);   // NW -> SE
	benchOrderFanOut(south, X_EAST, Y_NORTH, 8, -1);   // SW -> NE

	debug("movebench: crossing, " + (north.length + south.length) + " units");
}
