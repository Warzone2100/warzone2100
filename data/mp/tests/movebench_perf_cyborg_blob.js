include("tests/movebench_common.js");

// A cyborg crowd crossing through itself, on the open ground north of the wall.
//
// The movement neighbour queries scale with how many objects sit inside a
// droid's collision radius, so this is the case that makes moveCalcDroidSlide
// and moveCheckSquished dominate a tick. Half the block is sent to the far
// side and half back to the start row, so the two flows interpenetrate at the
// densest point rather than merely converging.
//
// unitsArrived is meaningless here - every unit in a half shares one goal tile.
// Read the perf counters, not the movement metrics.

const ME = 0;
const N_UNITS = 400;
const COLS = 20;

const Y_START = 6;
const Y_GOAL = 28;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "cyborg");
	var blob = benchSpawnBlock(ME, "cyborg", X_GAP - 10, Y_START, COLS, +1, N_UNITS);
	hackNetOn();

	var half = Math.floor(blob.length / 2);
	for (var i = 0; i < blob.length; i++)
	{
		orderDroidLoc(blob[i], DORDER_MOVE, X_GAP, i < half ? Y_GOAL : Y_START);
	}

	debug("movebench: perf_cyborg_blob, " + blob.length + " cyborgs crossing");
}
