include("tests/movebench_common.js");

// Idle construction trucks standing in a field of damaged and half-built structures.
//
// checkForDamagedStruct runs once per tick for every idle truck, and reaches the tests past the
// status check only when a friendly structure within five tiles is damaged or unfinished. Nothing
// else in the suite gets there: the standard scenarios field no construction droids, and
// perf_battle's towers stand clear of the fighting and stay whole.

const ME = 0;

const N_TRUCK = 40;
const N_BUILDER = 6;

const COLS = 7;
const ROWS = 6;
const PITCH = 3;
const X0 = X_GAP - 9;
const Y0 = 7;

// A truck searches only while it is idle, so the period has to outlast the repair the damage
// causes. Damage faster than that parks every truck on one structure for the rest of the run.
const DAMAGED_PERCENT = 95;
const REDAMAGE_MS = 4000;

function damageEverything()
{
	var structures = enumStruct(ME);
	for (var i = 0; i < structures.length; i++)
	{
		if (structures[i].status === BUILT)
		{
			setHealth(structures[i], DAMAGED_PERCENT);
		}
	}
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "truck");
	enableStructure("PillBox1", ME);

	var placed = 0;
	for (var row = 0; row < ROWS; row++)
	{
		for (var col = 0; col < COLS; col++)
		{
			if (addStructure("PillBox1", ME, (X0 + col * PITCH) * 128, (Y0 + row * PITCH) * 128) !== null)
			{
				placed++;
			}
		}
	}

	var trucks = benchSpawnFixed(ME, "truck", X0 + 1, Y0 + 1, COLS, +1, N_TRUCK, PITCH);
	hackNetOn();

	damageEverything();

	// A few trucks put up new structures, so the search takes its being-built branch as well as its
	// damaged one. They build off the end of the field, clear of the grid above.
	for (var i = 0; i < N_BUILDER && i < trucks.length; i++)
	{
		orderDroidBuild(trucks[i], DORDER_BUILD, "PillBox1", X0 + i * PITCH, Y0 + ROWS * PITCH);
	}

	setTimer("damageEverything", REDAMAGE_MS);

	debug("movebench: perf_repair_search, " + trucks.length + " trucks, " + placed + " structures");
}
