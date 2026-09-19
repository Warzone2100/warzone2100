include("tests/movebench_common.js");

// Many indirect projectiles in flight at once.
//
// A bank of mortar emplacements shells a parked block well inside their range
// and outside the block's own. Nothing much moves, so the tick is dominated by
// the in-flight collision loop, periodical damage and the structures' target
// selection. This is also the reference scenario for the render-side timers -
// run it with --movementbenchwatch=perf_artillery.

const TARGET = 0;
const GUNNER = 1;

const N_GUNS = 40;
const GUN_COLS = 20;
const Y_GUNS = 10;

const N_TARGETS = 100;
const TARGET_COLS = 20;
const Y_TARGETS = 24;

// Indirect-fire structures shoot only at what a sensor designates for them, and
// a mortar pit's own sensor does not reach this far.
const N_SENSORS = 2;
const Y_SENSORS = 14;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(TARGET, "medium");
	benchEnable(GUNNER, "medium");
	var built = 0;
	for (var i = 0; i < N_GUNS; i++)
	{
		var x = X_GAP - 10 + (i % GUN_COLS);
		var y = Y_GUNS + Math.floor(i / GUN_COLS) * 2;
		if (addStructure("Emplacement-MortarPit01", GUNNER, x * 128, y * 128) !== null)
		{
			built++;
		}
	}
	for (var j = 0; j < N_SENSORS; j++)
	{
		addStructure("Sys-SensoTower02", GUNNER, (X_GAP - 4 + j * 8) * 128, Y_SENSORS * 128);
	}
	var targets = benchSpawnBlock(TARGET, "medium", X_GAP - 10, Y_TARGETS, TARGET_COLS, +1, N_TARGETS);
	hackNetOn();

	// No orders: the block sits where it is and is shelled. Its machineguns
	// cannot reach the emplacements, so the exchange stays one-sided.
	debug("movebench: perf_artillery, " + built + " mortar pits shelling " + targets.length + " tanks");
}
