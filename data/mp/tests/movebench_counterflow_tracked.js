include("tests/movebench_common.js");

// Two blocks of medium tanks belonging to the same player swap sides through a
// 2-wide gap, so they meet head-on. Nothing here is hostile. The units simply
// cannot resolve each other.
//
// Tracked units are used because only ground-model units take pitch from slope.
// Persons and cyborgs skip updateDroidOrientation, so they would not see the
// sloped corridor walls at all.

const ME = 0;
const N_PER_SIDE = 16;

const Y_NORTH = 24;      // north block's row nearest the wall
const Y_SOUTH = 42;      // south block's row nearest the wall

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var north = benchSpawnBlock(ME, "medium", X_GAP - 2, Y_NORTH, 4, -1, N_PER_SIDE);
	var south = benchSpawnBlock(ME, "medium", X_GAP - 2, Y_SOUTH, 4, +1, N_PER_SIDE);
	hackNetOn();

	benchOrderFanOut(north, X_GAP - 4, Y_SOUTH, 8, +1);   // north block, swapping south
	benchOrderFanOut(south, X_GAP - 4, Y_NORTH, 8, -1);   // south block, swapping north

	debug("movebench: counterflow, " + (north.length + south.length) + " units");
}
