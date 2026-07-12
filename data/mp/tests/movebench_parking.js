include("tests/movebench_common.js");

// A moving column through a field of parked units.
//
// Exercises moveShuffleDroid, which fires only against an ally already sitting
// at MOVEINACTIVE. The parked units are given no orders at all, so the runner
// never tracks them and unitsOrdered counts only the column.

const ME = 0;
const N_COLUMN = 8;
const N_PARKED = 24;
const PARK_SPACING = 2;   // loose field, not a solid wall of hulls

const Y_COLUMN = 8;      // column starts north of the parked field
const Y_PARKED = 18;     // parked field, 4 rows deep
const Y_GOAL = 28;       // goal is south of the parked field, so it must pass through

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	// Parked first, so the column has something to negotiate from tick 0. The
	// field is deliberately loose: packed shoulder to shoulder it is not
	// negotiable at all, which measures jamming rather than shuffle behavior.
	benchSpawnBlock(ME, "medium", X_GAP - 11, Y_PARKED, 12, +1, N_PARKED, PARK_SPACING);
	var column = benchSpawnBlock(ME, "medium", X_GAP - 2, Y_COLUMN, 4, -1, N_COLUMN);
	hackNetOn();

	benchOrderFanOut(column, X_GAP - 4, Y_GOAL, 8, +1);

	debug("movebench: parking, " + column.length + " moving past " + N_PARKED + " idle");
}
