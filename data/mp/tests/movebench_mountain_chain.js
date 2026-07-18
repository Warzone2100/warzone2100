include("tests/movebench_common.js");

// Opposing tank and cyborg flows through Sk-Mountain's chained passes, the
// winding corridor system between the western and north-eastern entrances.
// The chain holds every hazard the synthetic maps cannot: adjoining member
// mouths across small pockets, a one-tile pinch that a cyborg and a tank can
// thread together but two tanks cannot, S-bends against cliff corners, and a
// junction pocket mid-chain. Cyborgs run west to north-east, tanks the
// reverse, and the cyborgs are re-ordered to the same destination twice while
// mid-transit, which regenerates their paths from inside coordinated space -
// historically the sharpest stress on route-grounded classification.

const ME = 0;
const N_UNITS = 41;   // one clone wars worth per side, as manually tested
const REORDER_MS = 25000;

var cyborgs = [];
var reorders = 0;

const CYB_TARGET_X = 81, CYB_TARGET_Y = 73;

function mountainReorder()
{
	// Twice only. Re-ordering forever would keep issuing degenerate short legs
	// after arrival and pollute the per-leg statistics.
	if (++reorders > 2)
	{
		removeTimer("mountainReorder");
		return;
	}
	benchOrderAllTo(cyborgs, CYB_TARGET_X, CYB_TARGET_Y);
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "lightcannonht");
	benchEnable(ME, "cyborg");
	var tanks = benchSpawnBlock(ME, "lightcannonht", 78, 70, 6, +1, N_UNITS);
	cyborgs = benchSpawnBlock(ME, "cyborg", 58, 82, 5, +1, N_UNITS);
	hackNetOn();

	benchOrderAllTo(tanks, 60, 83);
	benchOrderAllTo(cyborgs, CYB_TARGET_X, CYB_TARGET_Y);
	setTimer("mountainReorder", REORDER_MS);

	debug("movebench: mountain chain, " + (tanks.length + cyborgs.length) + " units");
}
