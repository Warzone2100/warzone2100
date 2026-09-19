include("tests/movebench_common.js");

// Cyborg counter-flow through the same narrow pass.
//
// Covers the other movement model rather than simply a smaller unit. Cyborgs
// run moveUpdateCyborgModel into moveUpdatePersonModel with no squish path, and
// updateDroidOrientation early-returns for them, so they take no pitch from the
// sloped corridor walls. Their radius is 30 against a tank's 50, so the same
// gap is far roomier and the density is much higher.

const ME = 0;
const N_PER_SIDE = 24;   // denser than the tracked blocks, since they are smaller

const Y_NORTH = 24;
const Y_SOUTH = 42;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "cyborg");
	var north = benchSpawnBlock(ME, "cyborg", X_GAP - 2, Y_NORTH, 4, -1, N_PER_SIDE);
	var south = benchSpawnBlock(ME, "cyborg", X_GAP - 2, Y_SOUTH, 4, +1, N_PER_SIDE);
	hackNetOn();

	benchOrderFanOut(north, X_GAP - 4, Y_SOUTH, 8, +1);
	benchOrderFanOut(south, X_GAP - 4, Y_NORTH, 8, -1);

	debug("movebench: counterflow cyborg, " + (north.length + south.length) + " units");
}
