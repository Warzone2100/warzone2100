include("tests/movebench_common.js");

// Script state that a GameState save has to restore, for the save / load tests (tests/gamestate):
// - a script group, some of whose members embark into a transporter,
//   so that when the game is saved the group has members that are not on the map
//   (the transporter flies off and drops them again after the save)
// - object labels armed for one player only (resetLabel(label, player)), on objects first seen after the save:
//   one seen by a droid of that player (which must trigger eventObjectSeen),
//   one by a droid of another player (which must not)
// The script turns a lost group member or a wrong eventObjectSeen into a different game:
// it spawns a droid for each, and orders the group's members (as the group has them) to move.
//
// Not a benchmark.

const ME = 0;
const OTHER = 1;

const X_CARGO = X_GAP - 14;
const Y_CARGO = 4;
const N_CARGO = 4;      // embark
const N_STAY = 2;       // stay on the map

const X_DROP = X_GAP - 6;
const Y_DROP = 8;
const DROP_MS = 40000;
const REGROUP_MS = 70000;

// A power generator (unarmed) of each player, labeled and armed for OTHER only,
// and an unarmed scout of each player that sees the other's generator only after SCOUT_MS
const Y_SCOUT = 22;
const X_OTHER_GEN = X_GAP + 22;
const X_MY_GEN = X_GAP - 22;
const SCOUT_MS = 35000;

var group = 0;
var groupMembers = 0;   // members the script put in the group
var transporter = null;
var scouts = [];
var losses = 0;
var seenEvents = 0;

function checkGroup()
{
	var size = groupSize(group);
	if (size < groupMembers)
	{
		losses += groupMembers - size;
		debug("movebench: gamestate_scripts: the group lost " + (groupMembers - size) + " member(s)");
		groupMembers = size;
		benchSpawnFixed(ME, "medium", X_GAP + 2 + losses, Y_CARGO, 1, +1, 1, 1);
	}
}

function drop()
{
	var t = getObject(DROID, ME, transporter);
	if (t !== null)
	{
		orderDroidLoc(t, DORDER_DISEMBARK, X_DROP, Y_DROP);
	}
}

function regroup()
{
	var members = enumGroup(group);
	var t = getObject(DROID, ME, transporter);
	debug("movebench: gamestate_scripts: regroup " + members.length + " of " + groupSize(group) + ", transporter at "
	      + (t === null ? "none" : t.x + "," + t.y + " carrying " + enumCargo(t).length));
	for (var i = 0; i < members.length; i++)
	{
		orderDroidLoc(members[i], DORDER_MOVE, X_DROP + 2 * (i % 4), Y_DROP + 4 + 2 * Math.floor(i / 4));
	}
}

function sendScouts()
{
	var mine = getObject(DROID, ME, scouts[0]);
	if (mine !== null)
	{
		orderDroidLoc(mine, DORDER_MOVE, X_OTHER_GEN - 4, Y_SCOUT);
	}
	var theirs = getObject(DROID, OTHER, scouts[1]);
	if (theirs !== null)
	{
		orderDroidLoc(theirs, DORDER_MOVE, X_MY_GEN + 4, Y_SCOUT);
	}
}

function eventObjectSeen(viewer, seen)
{
	seenEvents++;
	debug("movebench: gamestate_scripts: player " + viewer.player + " saw object " + seen.id + " of player " + seen.player);
	benchSpawnFixed(ME, "medium", X_GAP + 2 + seenEvents, Y_CARGO + 2, 1, +1, 1, 1);
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	benchEnable(ME, "truck");
	benchEnable(OTHER, "truck");
	makeComponentAvailable("SuperTransportBody", ME);
	makeComponentAvailable("V-Tol", ME);
	makeComponentAvailable("ZNULLECM", ME);
	enableStructure("A0PowerGenerator", ME);
	enableStructure("A0PowerGenerator", OTHER);

	var t = addDroid(ME, X_CARGO, Y_CARGO, "Bench Transporter", "SuperTransportBody", "V-Tol", "", "", "ZNULLECM");
	if (t === null || t.droidType !== DROID_SUPERTRANSPORTER)
	{
		debug("movebench: gamestate_scripts: no transporter");
		return;
	}
	transporter = t.id;

	group = newGroup();
	var droids = benchSpawnFixed(ME, "medium", X_CARGO, Y_CARGO + 3, N_CARGO + N_STAY, +1, N_CARGO + N_STAY, 1);
	for (var i = 0; i < droids.length; i++)
	{
		groupAdd(group, droids[i]);
		if (i < N_CARGO)
		{
			orderDroidObj(droids[i], DORDER_EMBARK, t);
		}
	}
	groupMembers = groupSize(group);

	var otherGen = addStructure("A0PowerGenerator", OTHER, X_OTHER_GEN * 128, Y_SCOUT * 128);
	var myGen = addStructure("A0PowerGenerator", ME, X_MY_GEN * 128, Y_SCOUT * 128);
	addLabel(otherGen, "otherGen");
	resetLabel("otherGen", OTHER);   // OTHER owns it, so it never sees it: ME's scout must not trigger it
	addLabel(myGen, "myGen");
	resetLabel("myGen", OTHER);      // OTHER's scout triggers it
	scouts.push(benchSpawnFixed(ME, "truck", X_GAP + 2, Y_SCOUT, 1, +1, 1, 1)[0].id);
	scouts.push(benchSpawnFixed(OTHER, "truck", X_GAP - 2, Y_SCOUT, 1, +1, 1, 1)[0].id);
	hackNetOn();

	setTimer("checkGroup", 2000);
	queue("sendScouts", SCOUT_MS);
	queue("drop", DROP_MS);
	queue("regroup", REGROUP_MS);
	debug("movebench: gamestate_scripts, group of " + groupMembers);
}
