include("tests/movebench_common.js");

// Many units ordered to a single destination, on the open ground north of the
// wall.
//
// Every unit is ordered to the same tile, which makes the destination a point
// attractor. Each unit runs its own path to the identical tile and follows the
// same centreline, so they converge single-file instead of spreading.
//
// unitsArrived is inherently low here and is not a failure. A hundred units
// cannot fit within arrival tolerance of one tile, so read stuckRemainingTiles
// for how tightly the blob packed in, and bumps and repaths for thrash.

const ME = 0;
const N_UNITS = 100;

const Y_START = 6;
const Y_GOAL = 28;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var blob = benchSpawnBlock(ME, "medium", X_GAP - 5, Y_START, 10, +1, N_UNITS);
	hackNetOn();

	benchOrderAllTo(blob, X_GAP, Y_GOAL);

	debug("movebench: blob, " + blob.length + " units to one tile");
}
