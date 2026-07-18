include("tests/movebench_common.js");

// Opposing flows through Sk-Rush's centre with a mid-corridor departure: the
// cyborgs run the long diagonal corridor south-west and peel off south toward
// the centre partway down, the tanks run the full corridor the other way. The
// cyborgs' turn is the lane handedness acceptance case: their departure bend
// sits at an inner junction, often with its vertex corner-cut a tile inside
// the corridor being left, and the correct assignment puts them on the inside
// of their turn with the tanks outside, so the flows never cross at it. The
// cyborgs are re-ordered to the same destination mid-transit, regenerating
// paths from inside coordinated space.

const ME = 0;
const N_UNITS = 41;   // one clone wars worth per side, as manually tested
const REORDER_MS = 25000;

var cyborgs = [];
var reorders = 0;

const CYB_TARGET_X = 46, CYB_TARGET_Y = 48;

function rushReorder()
{
	// Twice only. Re-ordering forever would keep issuing degenerate short legs
	// after arrival and pollute the per-leg statistics.
	if (++reorders > 2)
	{
		removeTimer("rushReorder");
		return;
	}
	benchOrderAllTo(cyborgs, CYB_TARGET_X, CYB_TARGET_Y);
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "lightcannonht");
	benchEnable(ME, "cyborg");
	// The tanks pack the small raised area, ringed around mid-ridge so the
	// overflow crowds the rise instead of a block's far rows running down the
	// south slope below it.
	var tanks = benchSpawnCluster(ME, "lightcannonht", 35, 45, N_UNITS);
	cyborgs = benchSpawnBlock(ME, "cyborg", 57, 31, 4, +1, N_UNITS);
	hackNetOn();

	benchOrderAllTo(tanks, 57, 31);
	benchOrderAllTo(cyborgs, CYB_TARGET_X, CYB_TARGET_Y);
	setTimer("rushReorder", REORDER_MS);

	debug("movebench: rush turn, " + (tanks.length + cyborgs.length) + " units");
}
