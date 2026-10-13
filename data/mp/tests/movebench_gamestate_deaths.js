include("tests/movebench_common.js");

// Deaths of every shape the GameState "dying" section has to restore,
// for the save / load and rewind tests (tests/gamestate).
// Every few seconds a wave kills, in one tick:
// - several droids, some of them while a repair facility or a repair droid is working on them
// - a commander that leads a group
// - a repair facility while it is repairing, and a wall
// - an oil drum (a feature)
// The eventDestroyed handler then removes one more droid,
// so its tile visibility is still waiting to be removed when the tick ends.
// Survivors keep being wounded, and each wave's losses are replaced, so every wave finds the same shapes.
//
// Not a benchmark: the deaths come from script removals, not from combat (see perf_battle for that).

const ME = 0;

const COLS = 6;
const X0 = X_GAP - 8;
const Y_FAC = 8;
const Y_WOUNDED = 10;   // right by the facilities and the repair droids, so they are being repaired when they die
const Y_REPAIR = 14;
const Y_GROUP = 20;
const Y_SPARE = 24;

const N_FACILITIES = 3;
const N_REPAIR = 6;
const N_WOUNDED = 24;
const N_GROUP = 6;
const N_SPARE = 12;

const WAVE_MS = 3000;
const KILLS_PER_WAVE = 4;
const WOUNDED_PERCENT = 40;

var waves = 0;
var collateral = [];   // droids the eventDestroyed handler removes, one per wave

function spawnWounded(count)
{
	return benchSpawnFixed(ME, "medium", X0, Y_WOUNDED, 2 * COLS, +1, count, 1);
}

function spawnCommander()
{
	var commander = addDroid(ME, X0, Y_GROUP, "Bench Commander", "Body5REC", "tracked01", "", "", "CommandBrain01");
	if (commander === null)
	{
		return;
	}
	var group = benchSpawnFixed(ME, "medium", X0 + 2, Y_GROUP, COLS, +1, N_GROUP, 1);
	for (var i = 0; i < group.length; i++)
	{
		orderDroidObj(group[i], DORDER_COMMANDERSUPPORT, commander);
	}
}

function woundEveryone()
{
	var droids = enumDroid(ME);
	for (var i = 0; i < droids.length; i++)
	{
		if (droids[i].droidType !== DROID_REPAIR && droids[i].droidType !== DROID_COMMAND)
		{
			setHealth(droids[i], WOUNDED_PERCENT);
		}
	}
}

function wave()
{
	waves++;
	var droids = enumDroid(ME, DROID_WEAPON).filter(function (d) { return d.y >= Y_WOUNDED && d.y < Y_REPAIR; });
	// The ones closest to the facilities are the ones being repaired
	droids.sort(function (a, b) { return (a.y - b.y) || (a.x - b.x) || (a.id - b.id); });
	for (var i = 0; i < KILLS_PER_WAVE && i < droids.length; i++)
	{
		removeObject(droids[i], true);
	}
	if (droids.length > KILLS_PER_WAVE)
	{
		collateral.push(droids[droids.length - 1].id);
	}

	var commanders = enumDroid(ME, DROID_COMMAND);
	if (commanders.length > 0 && waves % 2 === 1)
	{
		removeObject(commanders[0], true);
	}

	var walls = enumStruct(ME, WALL);
	if (walls.length > 0)
	{
		removeObject(walls[0], true);
	}
	var facilities = enumStruct(ME, REPAIR_FACILITY);
	if (facilities.length > 0 && waves % 3 === 0)
	{
		removeObject(facilities[waves % facilities.length], true);
	}

	var drums = enumFeature(ALL_PLAYERS, "OilDrum");
	if (drums.length > 0)
	{
		removeObject(drums[0], true);
	}

	queue("replenish", 1000);
}

function eventDestroyed(victim)
{
	if (victim.type !== DROID || victim.player !== ME || collateral.length === 0)
	{
		return;
	}
	var id = collateral.shift();
	var droids = enumDroid(ME);
	for (var i = 0; i < droids.length; i++)
	{
		if (droids[i].id === id)
		{
			removeObject(droids[i], true);
			return;
		}
	}
}

function replenish()
{
	var wounded = enumDroid(ME, DROID_WEAPON).filter(function (d) { return d.y >= Y_WOUNDED && d.y < Y_REPAIR; });
	if (wounded.length < N_WOUNDED)
	{
		spawnWounded(N_WOUNDED - wounded.length);
	}
	if (enumDroid(ME, DROID_COMMAND).length === 0)
	{
		spawnCommander();
	}
	var facilities = enumStruct(ME, REPAIR_FACILITY).length;
	for (var i = facilities; i < N_FACILITIES; i++)
	{
		addStructure("A0RepairCentre3", ME, (X0 + i * 5) * 128, Y_FAC * 128);
	}
	if (enumStruct(ME, WALL).length === 0)
	{
		addStructure("A0HardcreteMk1Wall", ME, (X_GAP + 8) * 128, Y_SPARE * 128);
	}
	if (enumFeature(ALL_PLAYERS, "OilDrum").length === 0)
	{
		addFeature("OilDrum", X_GAP + 10, Y_SPARE);
	}
	woundEveryone();
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	benchEnable(ME, "repair");
	makeComponentAvailable("CommandTurret1", ME);
	makeComponentAvailable("CommandBrain01", ME);
	enableStructure("A0RepairCentre3", ME);
	enableStructure("A0HardcreteMk1Wall", ME);

	benchSpawnFixed(ME, "repair", X0, Y_REPAIR, COLS, +1, N_REPAIR, 2);
	benchSpawnFixed(ME, "medium", X0, Y_SPARE, COLS, +1, N_SPARE, 1);
	replenish();
	hackNetOn();

	setTimer("wave", WAVE_MS);
	debug("movebench: gamestate_deaths");
}
