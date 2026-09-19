include("tests/movebench_common.js");

// VTOLs cycling between a defended target and their rearming pads.
//
// The pads' idle update walks every droid of the owner and then every ally to
// find one waiting to rearm, and each candidate walks the list again to see
// whether a pad is occupied. AA fire keeps the VTOLs damaged so they keep
// coming back, and the AA sites are the only homing-direct projectiles in the
// suite that are aimed upward.

const FLIER = 0;
const DEFENDER = 1;

const N_VTOL = 20;
const VTOL_COLS = 10;
const Y_VTOL = 6;

const N_PADS = 4;
const Y_PADS = 4;

const N_DEFEND = 40;
const DEFEND_COLS = 10;
const Y_DEFEND = 45;

const N_AA = 4;
const Y_AA = 42;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(FLIER, "vtol");
	benchEnable(DEFENDER, "medium");

	var pads = 0;
	for (var i = 0; i < N_PADS; i++)
	{
		if (addStructure("A0VtolPad", FLIER, (X_GAP - 4 + i * 2) * 128, Y_PADS * 128) !== null)
		{
			pads++;
		}
	}
	for (var j = 0; j < N_AA; j++)
	{
		addStructure("AASite-QuadMg1", DEFENDER, (X_GAP - 4 + j * 3) * 128, Y_AA * 128);
	}

	var vtols = benchSpawnFixed(FLIER, "vtol", X_GAP - 5, Y_VTOL, VTOL_COLS, +1, N_VTOL);
	var defenders = benchSpawnBlock(DEFENDER, "medium", X_GAP - 5, Y_DEFEND, DEFEND_COLS, +1, N_DEFEND);
	hackNetOn();

	for (var k = 0; k < vtols.length; k++)
	{
		orderDroidLoc(vtols[k], DORDER_SCOUT, X_GAP - 5 + (k % DEFEND_COLS), Y_DEFEND);
	}

	debug("movebench: perf_vtol_rearm, " + vtols.length + " VTOLs, " + pads + " pads, "
	      + defenders.length + " defenders");
}
