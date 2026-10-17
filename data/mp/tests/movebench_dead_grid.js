include("tests/movebench_common.js");

// Grid readers and objects that died earlier in the same tick.
//
// The object grid is built at the end of a tick,
// so an object that dies during a tick is still in it for the rest of that tick (dead, on the destroyed list).
// Each case below kills an object in a tick where a grid reader that runs later in that tick would find it,
// and prints "deadgrid: <case>: ok" when the reader skipped it, "BUG" when not:
// - squish: two enemy tanks start on the same person.
//   The first squishes him, and the second must not squish him again
//   (moveCheckSquished: experience and the mowed-down score would count him twice).
// - helpbuild: a structure being built is destroyed by a script
//   in the tick a truck first looks for a structure to help build (checkForDamagedStruct).
//   The truck must not start building a new one where the dead one was.
// - repair: the same, with a damaged structure
//   (the truck must not try to repair it: actionDroid asserts on a dead target, "Droid dead").
// - buildpos: a truck waits to build on a tile an enemy droid stands on, and the enemy is destroyed by a script.
//   The truck must start in that tick
//   (actionRemoveDroidsFromBuildPos must not count the dead droid as in the way).
// - gate: a droid standing on an open gate is destroyed by a script, and the gate may close in that tick
//   (not checked here, since scripts can't read a gate's state).
// The script removals are processed right after the scripts run,
// so the objects die before any droid or structure update of that tick.
//
// Not a benchmark.

const ME = 0;
const OTHER = 1;

// squish
const SQUISH_CASE = { x: 6, y: 4 };
// helpbuild: a factory left half built (its builder removed), then destroyed as a new truck arrives
const HELP_CASE = { x: 16, y: 6, struct: "A0LightFactory" };
const HELP_ABANDON_MS = 4000;
const HELP_KILL_MS = 6000;
const HELP_CHECK_MS = 9000;
// repair
const REPAIR_CASE = { x: 26, y: 6, struct: "A0PowerGenerator" };
const REPAIR_KILL_MS = 8000;
// buildpos
const BUILDPOS_CASE = { x: 36, y: 6, struct: "A0HardcreteMk1Wall" };
const BUILDPOS_KILL_MS = 10000;
// gate
const GATE_CASE = { x: 46, y: 6, struct: "A0HardcreteMk1Gate" };
const GATE_KILL_MS = 17000;  // the droid stops on the gate at about 9.3 s, and the gate closes 6 s later unless a droid is on it

var squishers = [];
var helpBuilder = null;
var helpLooker = null;
var helpSite = null;
var repairSite = null;
var blocker = null;
var gateDroid = null;
var buildPosKilledAt = 0;

function report(name, ok, detail)
{
	debug("deadgrid: " + name + ": " + (ok ? "ok" : "BUG") + " (" + detail + ")");
}

function droidById(player, id)
{
	return id === null ? null : getObject(DROID, player, id);
}

function checkSquish()
{
	var exp = [];
	for (var i = 0; i < squishers.length; i++)
	{
		var d = droidById(ME, squishers[i]);
		exp.push(d === null ? -1 : d.experience);
	}
	var squishes = exp.filter(function (e) { return e > 0; }).length;
	report("squish", squishes === 1, "squisher experience " + exp.join(", "));
}

function abandonHelpSite()
{
	var t = droidById(ME, helpBuilder);
	if (t !== null)
	{
		removeObject(t);
	}
	var s = enumStruct(ME, HELP_CASE.struct);
	helpSite = s.length > 0 ? s[0].id : null;
	debug("deadgrid: helpbuild: left " + (s.length > 0 ? s[0].status === BEING_BUILT ? "a structure being built" : "a built structure" : "nothing"));
}

function killHelpSite()
{
	var s = helpSite === null ? null : getObject(STRUCTURE, ME, helpSite);
	if (s === null)
	{
		report("helpbuild", false, "no structure to destroy");
		return;
	}
	helpLooker = benchSpawnFixed(ME, "truck", HELP_CASE.x, HELP_CASE.y + 3, 1, +1, 1, 1)[0].id;
	removeObject(s);
	queue("checkHelpLooker", 300);
}

// The new truck finds no work.
// It must not set off to build where the destroyed structure was (DACTION_BUILD 2, DACTION_MOVETOBUILD 18).
function checkHelpLooker()
{
	var t = droidById(ME, helpLooker);
	var action = t === null ? -1 : t.action;
	report("helpbuild", action !== 2 && action !== 18, "the new truck's action is " + action);
}

function checkHelpSite()
{
	var s = enumStruct(ME, HELP_CASE.struct);
	report("helpbuild-rebuilt", s.length === 0, s.length + " " + HELP_CASE.struct + " after the destroyed one");
}

function killRepairSite()
{
	var s = repairSite === null ? null : getObject(STRUCTURE, ME, repairSite);
	if (s === null)
	{
		report("repair", false, "no structure to destroy");
		return;
	}
	setHealth(s, 50);
	benchSpawnFixed(ME, "truck", REPAIR_CASE.x, REPAIR_CASE.y + 3, 1, +1, 1, 1);
	removeObject(s);
	debug("deadgrid: repair: destroyed a damaged structure next to a new truck (a \"Droid dead\" assert would be the bug)");
}

function killBlocker()
{
	var d = droidById(OTHER, blocker);
	if (d === null)
	{
		report("buildpos", false, "no blocker");
		return;
	}
	if (buildPosBuilt())
	{
		report("buildpos", false, "already built on the blocker's tile");
		return;
	}
	removeObject(d);
	buildPosKilledAt = gameTime;
	setTimer("pollBuildPos", 100);
}

function buildPosBuilt()
{
	return enumStruct(ME, BUILDPOS_CASE.struct).some(function (s) { return s.x === BUILDPOS_CASE.x && s.y === BUILDPOS_CASE.y; });
}

function pollBuildPos()
{
	if (!buildPosBuilt())
	{
		if (gameTime - buildPosKilledAt > 2000)
		{
			removeTimer("pollBuildPos");
			report("buildpos", false, "not started 2 s after the blocker died");
		}
		return;
	}
	removeTimer("pollBuildPos");
	// Placed in the update of the tick the blocker died in, so the scripts see it in the next tick
	var ticks = (gameTime - buildPosKilledAt) / 100;
	report("buildpos", ticks === 1, "the scripts saw the structure " + ticks + " tick(s) after the blocker died");
}

function killGateDroid()
{
	var d = droidById(ME, gateDroid);
	if (d === null || d.x !== GATE_CASE.x || d.y !== GATE_CASE.y)
	{
		debug("deadgrid: gate: the droid is not on the gate" + (d === null ? "" : " (at " + d.x + ", " + d.y + ", action " + d.action + ")"));
		return;
	}
	removeObject(d);
	debug("deadgrid: gate: destroyed the droid standing on the open gate");
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	benchEnable(ME, "truck");
	benchEnable(OTHER, "person");
	benchEnable(OTHER, "truck");
	enableStructure(HELP_CASE.struct, ME);
	enableStructure(REPAIR_CASE.struct, ME);
	enableStructure(BUILDPOS_CASE.struct, ME);
	enableStructure(GATE_CASE.struct, ME);

	// squish: both tanks on the person, moving off in opposite directions
	benchSpawnFixed(OTHER, "person", SQUISH_CASE.x, SQUISH_CASE.y, 1, +1, 1, 1);
	var tanks = benchSpawnFixed(ME, "medium", SQUISH_CASE.x, SQUISH_CASE.y, 1, 0, 2, 1);  // cols 1, rowDir 0: both on the same tile
	for (var i = 0; i < tanks.length; i++)
	{
		squishers.push(tanks[i].id);
		orderDroidLoc(tanks[i], DORDER_MOVE, SQUISH_CASE.x + (i === 0 ? -4 : 4), SQUISH_CASE.y);
	}

	// helpbuild
	var a = benchSpawnFixed(ME, "truck", HELP_CASE.x, HELP_CASE.y + 3, 1, +1, 1, 1)[0];
	helpBuilder = a.id;
	orderDroidBuild(a, DORDER_BUILD, HELP_CASE.struct, HELP_CASE.x, HELP_CASE.y);

	// repair
	repairSite = addStructure(REPAIR_CASE.struct, ME, REPAIR_CASE.x * 128, REPAIR_CASE.y * 128).id;

	// buildpos: the enemy blocker on the wall's tile, and the builder
	blocker = benchSpawnFixed(OTHER, "truck", BUILDPOS_CASE.x, BUILDPOS_CASE.y, 1, +1, 1, 1)[0].id;
	var d = benchSpawnFixed(ME, "truck", BUILDPOS_CASE.x, BUILDPOS_CASE.y + 3, 1, +1, 1, 1)[0];
	orderDroidBuild(d, DORDER_BUILD, BUILDPOS_CASE.struct, BUILDPOS_CASE.x, BUILDPOS_CASE.y);

	// gate: a wall right behind it, so a droid sent to the wall's tile gets no further than the gate
	var gate = addStructure(GATE_CASE.struct, ME, GATE_CASE.x * 128, GATE_CASE.y * 128);
	addStructure(BUILDPOS_CASE.struct, ME, GATE_CASE.x * 128, (GATE_CASE.y - 1) * 128);
	debug("deadgrid: gate: at " + gate.x + ", " + gate.y);
	var g = benchSpawnFixed(ME, "medium", GATE_CASE.x, GATE_CASE.y + 3, 1, +1, 1, 1)[0];
	gateDroid = g.id;
	orderDroidLoc(g, DORDER_MOVE, GATE_CASE.x, GATE_CASE.y - 1);
	hackNetOn();

	queue("checkSquish", 3000);
	queue("abandonHelpSite", HELP_ABANDON_MS);
	queue("killHelpSite", HELP_KILL_MS);
	queue("checkHelpSite", HELP_CHECK_MS);
	queue("killRepairSite", REPAIR_KILL_MS);
	queue("killBlocker", BUILDPOS_KILL_MS);
	queue("killGateDroid", GATE_KILL_MS);
	debug("movebench: dead_grid");
}
