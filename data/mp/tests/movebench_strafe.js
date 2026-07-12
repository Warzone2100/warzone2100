include("tests/movebench_common.js");

// A large same-speed mass strafing back and forth across open ground.
//
// Selecting a mass of units and walking them repeatedly across open terrain is
// core play, and it is the behavior most exposed to collateral damage from
// changes aimed at chokepoints. This is a guard rather than a target. Nothing
// here should improve, and nothing should regress.
//
// The reversal is the risk moment. When the mass turns around, trailing units
// become leading ones and briefly face the rest of their own formation, which
// looks like opposing headings plus contact. A single translation never
// produces that, so the reorders matter more than the distance covered.
//
// What to watch: yieldsIssued should stay at zero, and bumps, giveUps and
// formationSpreadTiles must not rise against the baseline. The runner is told
// not to stop on completion, since the mass arrives at the end of every leg.

const ME = 0;
const N_UNITS = 60;

const X_WEST = 8;
const X_EAST = 44;
const Y_ROW = 6;
const COLS = 10;

const LEG_MS = 15000;    // reversal interval, in game time

var headingEast = true;

function strafeReverse()
{
	headingEast = !headingEast;
	benchOrderFanOut(enumDroid(ME), headingEast ? X_EAST : X_WEST, Y_ROW, COLS, +1);
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var mass = benchSpawnBlock(ME, "medium", X_WEST, Y_ROW, COLS, +1, N_UNITS);
	hackNetOn();

	benchOrderFanOut(mass, X_EAST, Y_ROW, COLS, +1);
	setTimer("strafeReverse", LEG_MS);

	debug("movebench: strafe, " + mass.length + " units");
}
