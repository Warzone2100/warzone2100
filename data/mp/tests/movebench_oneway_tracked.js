include("tests/movebench_common.js");

// A single tracked block through the pass.
//
// Identical map, spawn block, goals and composition to the counter-flow
// scenario, with the opposing block removed. Proves the pass is traversable, so
// a bad counter-flow result can be attributed to the head-on conflict rather
// than to geometry the units could never negotiate.

const ME = 0;
const N_UNITS = 16;

const Y_NORTH = 24;
const Y_SOUTH = 42;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var north = benchSpawnBlock(ME, "medium", X_GAP - 2, Y_NORTH, 4, -1, N_UNITS);
	hackNetOn();

	benchOrderFanOut(north, X_GAP - 4, Y_SOUTH, 8, +1);

	debug("movebench: oneway, " + north.length + " units");
}
