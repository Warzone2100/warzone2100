include("tests/movebench_common.js");

// Armed units on patrol past a hostile parked block.
//
// A patrolling droid runs the nearest-target search on every tick it has no
// action, unthrottled, which is the one path in orderUpdateDroid that does so.
// The parked block gives it something to find without resolving the patrol.

const SCOUT = 0;
const PARKED = 1;

const N_SCOUT = 30;
const SCOUT_COLS = 10;

const N_PARKED = 20;
const PARKED_COLS = 10;

const X_START = X_GAP - 10;
const Y_START = 8;
const X_END = X_GAP + 10;
const Y_END = 26;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(SCOUT, "medium");
	benchEnable(PARKED, "truck");
	var scouts = benchSpawnBlock(SCOUT, "medium", X_START, Y_START, SCOUT_COLS, +1, N_SCOUT);
	benchSpawnFixed(PARKED, "truck", X_GAP - 5, 16, PARKED_COLS, +1, N_PARKED);
	hackNetOn();

	for (var i = 0; i < scouts.length; i++)
	{
		orderDroidLoc(scouts[i], DORDER_PATROL, X_END, Y_END);
	}

	debug("movebench: perf_scout_patrol, " + scouts.length + " scouts past " + N_PARKED + " parked");
}
