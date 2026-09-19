include("tests/movebench_common.js");

// A repair facility and a screen of repair droids working on a damaged column.
//
// findSomeoneToRepair() runs from the repair facility's update and from an idle repair droid's, and
// nothing else in the suite reaches either: no scenario builds a repair facility, and no roster fields
// a DROID_REPAIR. So the repair-candidate query, its priority queue and the RTR clearing path all go
// unexercised without this.

const ME = 0;

const N_REPAIR = 8;
const N_WOUNDED = 30;
const COLS = 6;

// A repair facility searches only TILE_UNITS * 5 / 2 around itself and a repair droid REPAIR_MAXDIST,
// so the wounded sit in the droids' reach and on the edge of the facilities'. Packing them onto the
// facilities instead pins every one in Repairing for the whole run and the search stops being called.
const X0 = X_GAP - 8;
const Y_FAC = 8;
const Y_REPAIR = 11;
const Y_WOUNDED = 14;

// Light damage on a period longer than the repair it causes, so the wounded cycle between needing a
// repairer and releasing one. Damage that outruns the repair pins every repairer on one target.
const DAMAGED_PERCENT = 60;
const REDAMAGE_MS = 4000;

function woundEveryone()
{
	var droids = enumDroid(ME);
	for (var i = 0; i < droids.length; i++)
	{
		if (droids[i].droidType !== DROID_REPAIR)
		{
			setHealth(droids[i], DAMAGED_PERCENT);
		}
	}
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	benchEnable(ME, "repair");
	enableStructure("A0RepairCentre3", ME);

	var placed = 0;
	for (var i = 0; i < 3; i++)
	{
		if (addStructure("A0RepairCentre3", ME, (X0 + i * 5) * 128, Y_FAC * 128) !== null)
		{
			placed++;
		}
	}

	var repairers = benchSpawnFixed(ME, "repair", X0, Y_REPAIR, COLS, +1, N_REPAIR, 2);
	var wounded = benchSpawnFixed(ME, "medium", X0, Y_WOUNDED, COLS, +1, N_WOUNDED, 2);
	hackNetOn();

	woundEveryone();
	setTimer("woundEveryone", REDAMAGE_MS);

	debug("movebench: perf_repair_facility, " + placed + " facilities, " + repairers.length
	      + " repairers, " + wounded.length + " wounded");
}
