include("tests/movebench_common.js");

// Script state that an in-process restore (a rewind) has to roll back, for the rewind tests (tests/gamestate):
// state that only appears after the snapshot,
// which a restore of the snapshot's saved globals alone doesn't remove.
// Rewind with the snapshot at 20 s (forkcrc.sh mb:gamestate_rewind 20000 <end> --rewind <k>, k >= 6000),
// so the changes made at CHANGE_MS are live at the restore.
// At CHECK_MS, before the changes are made again,
// the script checks each case and prints "rewindscript: <case>: ok" or "STALE".
// A stale case also spawns a droid, so the rewound game differs from an uninterrupted one.
// The cases:
// - global: a global the script creates at CHANGE_MS (never declared)
// - declared: a declared global with no value until CHANGE_MS
// - groupsize: groupSizes[] of a group that is empty at the snapshot and gets a member at CHANGE_MS
// - groupmember: a group that has a member at the snapshot and loses it at CHANGE_MS
//   (control: the restore puts the member back)
// - timer: a timer set at CHANGE_MS (it must not run after the restore before it is set again)
//
// Not a benchmark.

const ME = 0;
const CHANGE_MS = 25000;
const CHECK_MS = 22000;
const SPAWN_X = X_GAP - 10;
const SPAWN_Y = 6;

var declaredLater;
var filledLater = 0;    // group, empty until CHANGE_MS
var emptiedLater = 0;   // group, one member until CHANGE_MS
var emptiedMember = null;
var spawned = 0;
var checks = 0;

function report(name, ok, detail)
{
	debug("rewindscript: " + name + ": " + (ok ? "ok" : "STALE") + " (" + detail + ")");
	if (!ok)
	{
		spawned++;
		benchSpawnFixed(ME, "medium", SPAWN_X + spawned, SPAWN_Y, 1, +1, 1, 1);
	}
}

function lateTimer()
{
	if (gameTime < CHANGE_MS)
	{
		report("timer", false, "a timer set at " + CHANGE_MS + " ran at " + gameTime);
	}
}

function check()
{
	checks++;
	report("global", typeof globalThis.lateGlobal === "undefined", "typeof lateGlobal is " + typeof globalThis.lateGlobal);
	report("declared", typeof declaredLater === "undefined", "declaredLater is " + declaredLater);
	report("groupsize", !groupSizes[filledLater], "groupSizes[filledLater] is " + groupSizes[filledLater] + ", groupSize() " + groupSize(filledLater));
	report("groupmember", groupSize(emptiedLater) === 1, "groupSize(emptiedLater) is " + groupSize(emptiedLater) + ", groupSizes[] " + groupSizes[emptiedLater]);
}

function change()
{
	globalThis.lateGlobal = gameTime;
	declaredLater = gameTime;
	groupAdd(filledLater, benchSpawnFixed(ME, "medium", SPAWN_X, SPAWN_Y + 4, 1, +1, 1, 1)[0]);
	var d = getObject(DROID, ME, emptiedMember);
	if (d !== null)
	{
		removeObject(d);
	}
	setTimer("lateTimer", 1000);
	debug("rewindscript: changed at " + gameTime);
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	filledLater = newGroup();
	emptiedLater = newGroup();
	var d = benchSpawnFixed(ME, "medium", SPAWN_X, SPAWN_Y + 2, 1, +1, 1, 1)[0];
	groupAdd(emptiedLater, d);
	emptiedMember = d.id;
	hackNetOn();

	queue("check", CHECK_MS);
	queue("change", CHANGE_MS);
	debug("movebench: gamestate_rewind");
}
