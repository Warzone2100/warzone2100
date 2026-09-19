// Spawns the units the pathfinding benchmark pushes through the worker pool.
//
// The direct measurement builds its requests by hand and needs nothing here.
// These exist only for the queued measurement, which has to go through
// fpathDroidRoute and therefore needs real droids to route.
//
// They are placed above the first band of the serpentine and never ordered
// anywhere by this script, so whatever they do is the benchmark's doing. Wheeled
// to match the propulsion the direct requests use, so both measurements search
// the same blocking map.

const N_UNITS = 32;
const SPAWN_X = 8;       // clear of the west gap, so the block is not sitting in it
const SPAWN_Y = 4;
const COLS = 8;

const UNIT = { name: "PathBench Unit", body: "Body5REC", prop: "wheeled01", weap: "MG1Mk1" };

function eventStartLevel()
{
	setPower(1000000, me);
	makeComponentAvailable(UNIT.body, me);
	makeComponentAvailable(UNIT.prop, me);
	makeComponentAvailable(UNIT.weap, me);

	for (var i = 0; i < N_UNITS; i++)
	{
		var x = SPAWN_X + (i % COLS);
		var y = SPAWN_Y + Math.floor(i / COLS);
		addDroid(me, x, y, UNIT.name, UNIT.body, UNIT.prop, "", "", UNIT.weap);
	}
	debug("pathbench: spawned " + N_UNITS + " units");
}
