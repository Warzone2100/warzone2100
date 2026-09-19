include("tests/movebench_common.js");

// Opposite diagonals that separate without coordination.
//
// The north block stays east, the south block stays west. Path cost is genuine
// shortest path, so each block hugs the side of the pass it was already biased
// toward and the two streams self-separate into distinct lanes.
//
// That corner-hugging is doing real work here, which is why this cell is much
// easier than crossing despite the same map, unit count and head-on setup. It
// guards against a counter-flow change destroying the free separation.

const ME = 0;
const N_PER_SIDE = 16;

const X_WEST = 22;
const X_EAST = 38;
const Y_NORTH = 24;
const Y_SOUTH = 42;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var north = benchSpawnBlock(ME, "medium", X_EAST, Y_NORTH, 4, -1, N_PER_SIDE);
	var south = benchSpawnBlock(ME, "medium", X_WEST, Y_SOUTH, 4, +1, N_PER_SIDE);
	hackNetOn();

	benchOrderFanOut(north, X_EAST, Y_SOUTH, 8, +1);   // NE -> SE, stays east
	benchOrderFanOut(south, X_WEST, Y_NORTH, 8, -1);   // SW -> NW, stays west

	debug("movebench: separating, " + (north.length + south.length) + " units");
}
