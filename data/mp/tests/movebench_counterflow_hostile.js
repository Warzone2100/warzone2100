include("tests/movebench_common.js");

// The counterflow conflict across enemy lines:
// two blocks of unarmed trucks, one per hostile player, swap sides through the 2-wide gap.
//
// Coordination is negotiated only among a player's own or allied units, so neither side may
// lane up with, queue behind or wait for the other - the encounter resolves by contact, just
// like against any other enemy obstacle.
//
// Trucks rather than tanks for the same reason as enemyblock: armed hostiles would shoot the
// conflict away and we want to measure movement (not combat).

const NORTH = 0;
const SOUTH = 1;
const N_PER_SIDE = 16;

const Y_NORTH = 24;      // north block's row nearest the wall
const Y_SOUTH = 42;      // south block's row nearest the wall

function eventStartLevel()
{
	hackNetOff();
	benchEnable(NORTH, "truck");
	benchEnable(SOUTH, "truck");
	var north = benchSpawnBlock(NORTH, "truck", X_GAP - 2, Y_NORTH, 4, -1, N_PER_SIDE);
	var south = benchSpawnBlock(SOUTH, "truck", X_GAP - 2, Y_SOUTH, 4, +1, N_PER_SIDE);
	hackNetOn();

	benchOrderFanOut(north, X_GAP - 4, Y_SOUTH, 8, +1);   // north block, swapping south
	benchOrderFanOut(south, X_GAP - 4, Y_NORTH, 8, -1);   // south block, swapping north

	debug("movebench: counterflow_hostile, " + (north.length + south.length) + " units");
}
