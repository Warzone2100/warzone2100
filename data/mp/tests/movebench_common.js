// Shared helpers for the movement benchmark scenarios.
//
// Scenarios own their geometry and orders. This file owns unit composition and
// the placement patterns they share, so a roster change lands in every scenario
// at once and compositions stay comparable.
//
// Everything is deterministic. Fixed tiles, no script RNG, all orders issued
// from eventStartLevel. The C++ runner owns termination and the scorecard.

const X_GAP = 32;        // gap centre column
const Y_WALL = 32;       // first wall row, wall spans Y_WALL .. Y_WALL+2
const WALL_LEN = 3;

// The north half above the wall is flat open ground, which the open-field
// scenarios use so the whole suite can share one map.
const Y_OPEN = 14;       // a row well clear of both the wall and the map edge

const ROSTER = {
	// Ground model: runs moveUpdateGroundModel, gets pitch from slope, and can
	// squish infantry. The primary composition, because campaign chokes are
	// cliffs with height and only ground-model units feel that.
	medium: { name: "Bench Medium Tank", body: "Body5REC", prop: "tracked01", weap: "MG1Mk1" },
	heavy:  { name: "Bench Heavy Tank",  body: "Body9REC", prop: "tracked01", weap: "MG1Mk1" },
	// Person model: no squish path, no pitch from slope, much smaller radius so
	// a given gap is far roomier. A different code path, not just a smaller unit.
	cyborg: { name: "Bench Cyborg", body: "CyborgLightBody", prop: "CyborgLegs", weap: "CyborgChaingun" },
	// The real-map acceptance pair's tank: the classic early-game clone-wars
	// composition the corridor scenarios were manually tested with.
	lightcannonht: { name: "Bench Light Cannon Halftrack", body: "Body1REC", prop: "HalfTrack", weap: "Cannon1Mk1" },
	// Unarmed, for the enemy-blocking scenario. Two hostile armed blocks would
	// resolve the choke by shooting each other, which measures combat rather
	// than whether enemies remain solid obstacles.
	truck:  { name: "Bench Truck", body: "Body1REC", prop: "wheeled01", weap: "Spade1Mk1" },
	// Unarmed but heavy-bodied, for plugging a gap. Sizing matters: collision
	// radius is 40 for a light body against 60 for a heavy, so two light units
	// in a 2-tile (256 unit) gap leave 96 units of slack - enough for another
	// light unit to squeeze past, which does not test blocking at all. Two
	// heavies leave 16 and actually seal it.
	heavytruck: { name: "Bench Heavy Truck", body: "Body9REC", prop: "tracked01", weap: "Spade1Mk1" },
	// Combat rosters, for the perf_* scenarios. The standard suite fires no shot at all, so
	// anything touching visibility, targeting, line of fire or projectiles is unexercised by it.
	// One kind per projectile movement model, since each takes a different path through
	// proj_InFlightFunc and checkFireLine.
	cannon:  { name: "Bench Cannon Tank",  body: "Body5REC", prop: "tracked01", weap: "Cannon1Mk1" },
	homing:  { name: "Bench Missile Tank", body: "Body5REC", prop: "tracked01", weap: "Missile-A-T" },
	mortar:  { name: "Bench Mortar Tank",  body: "Body5REC", prop: "HalfTrack", weap: "Mortar1Mk1" },
	flamer:  { name: "Bench Flamer Tank",  body: "Body5REC", prop: "tracked01", weap: "Flame1Mk1" },
	// The only way to get a DROID_PERSON into a skirmish, and the only thing moveCheckSquished acts on.
	person:  { name: "Bench Person", body: "B1BaBaPerson01", prop: "BaBaLegs", weap: "BaBaMG" },
	vtol:    { name: "Bench VTOL", body: "Body5REC", prop: "V-Tol", weap: "Rocket-VTOL-LtA-T" },
	// The only DROID_REPAIR in the suite, for the repair-facility scenario.
	repair:  { name: "Bench Repair Turret", body: "Body1REC", prop: "wheeled01", weap: "LightRepair1" },
	// Hover, for its low skid deceleration.
	hover:  { name: "Bench Hover Tank", body: "Body5REC", prop: "hover01", weap: "MG1Mk1" },
	// Trucks for the help-build scenarios, on one body so only the propulsion differs.
	hovertruck:   { name: "Bench Hover Truck",   body: "Body1REC", prop: "hover01",   weap: "Spade1Mk1" },
	trackedtruck: { name: "Bench Tracked Truck", body: "Body1REC", prop: "tracked01", weap: "Spade1Mk1" },
};

function benchEnable(player, kind)
{
	var k = ROSTER[kind];
	setPower(1000000, player);
	makeComponentAvailable(k.body, player);
	makeComponentAvailable(k.prop, player);
	makeComponentAvailable(k.weap, player);
}

// Lays `count` units in a `cols`-wide block anchored exactly at (x0, y0), with
// rows receding in `rowDir`. `spacing` (default 1) is the tile pitch between
// units, so a field can be laid out loosely enough to be negotiable rather than
// solid. Returns the droids actually created, in spawn order.
//
// Use this only where the exact tiles matter, ex. units plugging a gap. Most
// scenarios want benchSpawnBlock so their results carry an error bar.
function benchSpawnFixed(player, kind, x0, y0, cols, rowDir, count, spacing)
{
	var k = ROSTER[kind];
	var step = spacing || 1;
	var made = [];
	for (var i = 0; i < count; i++)
	{
		var x = x0 + (i % cols) * step;
		var y = y0 + Math.floor(i / cols) * rowDir * step;
		var d = addDroid(player, x, y, k.name, k.body, k.prop, "", "", k.weap);
		if (d !== null)
		{
			made.push(d);
		}
	}
	return made;
}

// Number of lateral and forward offsets a spawn block can take. Three of each
// gives nine placements per block, so a two-block scenario has 81 arrangements
// in total, which is small enough to enumerate rather than sample.
const ARRANGE_STEPS = 3;

var benchBlockIndex = 0;

// As benchSpawnFixed, but offsets the block according to the arrangement index
// chosen for this run. Successive calls take successive digits of the index, so
// each block moves independently and every combination is reachable by counting
// upward from zero.
//
// This is enumerated rather than random on purpose. Drawing offsets from the
// synchronised RNG covered the same small space unevenly and produced identical
// arrangements from different seeds, which made two seed families disagree about
// a cell's range while both looked like fair samples.
//
// A single arrangement is one trajectory, not a sample. Shifting a spawn block
// by a tile with no other change moves arrival p95 by around a quarter, and some
// cells are bistable and simply resolve or jam depending on it.
function benchSpawnBlock(player, kind, x0, y0, cols, rowDir, count, spacing)
{
	var digit = ARRANGE_STEPS * ARRANGE_STEPS;
	var index = Math.floor(benchArrangement() / Math.pow(digit, benchBlockIndex)) % digit;
	benchBlockIndex++;

	var dx = (index % ARRANGE_STEPS) - Math.floor(ARRANGE_STEPS / 2);
	var dy = Math.floor(index / ARRANGE_STEPS) - Math.floor(ARRANGE_STEPS / 2);
	return benchSpawnFixed(player, kind, x0 + dx, y0 + dy, cols, rowDir, count, spacing);
}

// As benchSpawnBlock, but cycles through a list of kinds so a cluster contains
// units of differing speed and turn rate. Speed heterogeneity is the point:
// faster units catch up to slower ones and stack behind them, which is a
// distinct congestion source from geometry.
function benchSpawnMixedBlock(player, kinds, x0, y0, cols, rowDir, count)
{
	var digit = ARRANGE_STEPS * ARRANGE_STEPS;
	var index = Math.floor(benchArrangement() / Math.pow(digit, benchBlockIndex)) % digit;
	benchBlockIndex++;
	x0 += (index % ARRANGE_STEPS) - Math.floor(ARRANGE_STEPS / 2);
	y0 += Math.floor(index / ARRANGE_STEPS) - Math.floor(ARRANGE_STEPS / 2);

	var made = [];
	for (var i = 0; i < count; i++)
	{
		var k = ROSTER[kinds[i % kinds.length]];
		var x = x0 + (i % cols);
		var y = y0 + Math.floor(i / cols) * rowDir;
		var d = addDroid(player, x, y, k.name, k.body, k.prop, "", "", k.weap);
		if (d !== null)
		{
			made.push(d);
		}
	}
	return made;
}

// Orders each droid to its own tile in a wide, shallow goal block anchored at
// (x0, y0). Distinct goals are what make "arrived" well defined - if every unit
// shared one goal tile, most could never reach it and the scorecard would
// report a jam that was really just arithmetic. Wide-and-shallow also stops
// later arrivals queueing behind ones that already parked, which would measure
// shuffling rather than whatever the scenario is actually about.
function benchOrderFanOut(droids, x0, y0, cols, rowDir)
{
	for (var i = 0; i < droids.length; i++)
	{
		var x = x0 + (i % cols);
		var y = y0 + Math.floor(i / cols) * rowDir * 2;   // 2-tile row spacing
		orderDroidLoc(droids[i], DORDER_MOVE, x, y);
	}
}

// Packs `count` units in concentric rings around (cx, cy), nearest ring
// first, the shape a clone-wars spawn takes around a selected unit. Use it
// where the open ground is an island, ex. a raised area, so the intended
// tiles crowd the anchor instead of a block's far rows running off the edge.
// Applies the same per-arrangement shift as benchSpawnBlock.
function benchSpawnCluster(player, kind, cx, cy, count)
{
	var digit = ARRANGE_STEPS * ARRANGE_STEPS;
	var index = Math.floor(benchArrangement() / Math.pow(digit, benchBlockIndex)) % digit;
	benchBlockIndex++;
	var ax = cx + (index % ARRANGE_STEPS) - Math.floor(ARRANGE_STEPS / 2);
	var ay = cy + Math.floor(index / ARRANGE_STEPS) - Math.floor(ARRANGE_STEPS / 2);

	var k = ROSTER[kind];
	var made = [];
	for (var r = 0; made.length < count; r++)
	{
		for (var dy = -r; dy <= r && made.length < count; dy++)
		{
			for (var dx = -r; dx <= r && made.length < count; dx++)
			{
				if (Math.max(Math.abs(dx), Math.abs(dy)) != r)
				{
					continue;
				}
				var d = addDroid(player, ax + dx, ay + dy, k.name, k.body, k.prop, "", "", k.weap);
				if (d)
				{
					made.push(d);
				}
			}
		}
	}
	return made;
}

// As benchSpawnCluster, but packed the way the cloning cheat packs a droid
// army: a golden-angle spiral at 50 world-unit radius steps, several units to
// a tile, a dense disc rather than one body per tile. 136 units span roughly
// nine tiles across. Spiral points landing on blocked ground are skipped, the
// same behaviour as the cheat, so the count can fall slightly short there.
// Applies the same per-arrangement shift as benchSpawnBlock.
function benchSpawnClones(player, kind, cx, cy, count)
{
	var digit = ARRANGE_STEPS * ARRANGE_STEPS;
	var index = Math.floor(benchArrangement() / Math.pow(digit, benchBlockIndex)) % digit;
	benchBlockIndex++;
	var ax = cx + (index % ARRANGE_STEPS) - Math.floor(ARRANGE_STEPS / 2);
	var ay = cy + Math.floor(index / ARRANGE_STEPS) - Math.floor(ARRANGE_STEPS / 2);

	var k = ROSTER[kind];
	var made = [];
	for (var i = 0; made.length < count && i < count * 3; i++)
	{
		var ang = (40503 * i % 65536) * Math.PI / 32768;
		var rad = 50 * Math.sqrt(i + 1);
		var tx = Math.floor((ax * 128 + 64 + rad * Math.sin(ang)) / 128);
		var ty = Math.floor((ay * 128 + 64 + rad * Math.cos(ang)) / 128);
		var d = addDroid(player, tx, ty, k.name, k.body, k.prop, "", "", k.weap);
		if (d)
		{
			made.push(d);
		}
	}
	return made;
}

// Orders every droid to a single destination. Used by the blob scenario, where
// converging on one point is the phenomenon under test rather than an artifact.
function benchOrderAllTo(droids, x, y)
{
	for (var i = 0; i < droids.length; i++)
	{
		orderDroidLoc(droids[i], DORDER_MOVE, x, y);
	}
}

// The parking scenario's column through a field of parked units. With `stale` the parked units are
// first sent south and stopped a second later, so each carries a stale sMove.target ahead of the column.
const PARK_N_COLUMN = 8;
const PARK_N_PARKED = 24;
const PARK_PITCH = 2;
const PARK_Y_COLUMN = 8;
const PARK_Y_PARKED = 18;
const PARK_Y_GOAL = 28;
const PARK_SETTLE_MS = 1000;

var benchParkColumnIds = [];

function benchParkingRelease()
{
	var droids = enumDroid(0);
	var column = [];
	for (var i = 0; i < droids.length; i++)
	{
		if (benchParkColumnIds.indexOf(droids[i].id) >= 0)
		{
			column.push(droids[i]);
		}
		else
		{
			orderDroid(droids[i], DORDER_STOP);
		}
	}
	benchOrderFanOut(column, X_GAP - 4, PARK_Y_GOAL, 8, +1);
}

function benchParkingStart(columnKind, stale)
{
	hackNetOff();
	benchEnable(0, "medium");
	benchEnable(0, columnKind);
	var parked = benchSpawnBlock(0, "medium", X_GAP - 11, PARK_Y_PARKED, 12, +1, PARK_N_PARKED, PARK_PITCH);
	var column = benchSpawnBlock(0, columnKind, X_GAP - 2, PARK_Y_COLUMN, 4, -1, PARK_N_COLUMN);
	hackNetOn();

	benchParkColumnIds = [];
	for (var i = 0; i < column.length; i++)
	{
		benchParkColumnIds.push(column[i].id);
	}
	if (stale)
	{
		for (var j = 0; j < parked.length; j++)
		{
			orderDroidLoc(parked[j], DORDER_SCOUT, parked[j].x, PARK_Y_GOAL + 12);
		}
	}
	queue("benchParkingRelease", PARK_SETTLE_MS);

	debug("movebench: parking " + columnKind + (stale ? " stale" : "") + ", " + column.length
	      + " moving past " + parked.length + " idle");
}

// Two trucks ordered to build the same structure, A from six tiles off and B from thirteen down the same
// line, so B joins A as a helper. `slot` swaps the factory for a one-tile tower walled in on every side
// but the approach, so the only tile to build from is the one A stands on.
const HELP_X_SITE = X_GAP;
const HELP_Y_SITE = Y_OPEN + 4;
const HELP_STRUCT = "A0LightFactory";
const HELP_SLOT_STRUCT = "GuardTower1";
const HELP_WALL = "A0HardcreteMk1Wall";

function benchHelpBuildStart(truckKind, slot)
{
	hackNetOff();
	benchEnable(0, truckKind);
	var structName = slot ? HELP_SLOT_STRUCT : HELP_STRUCT;
	enableStructure(structName, 0);
	if (slot)
	{
		enableStructure(HELP_WALL, 0);
		var ring = [[-1, -1], [1, -1], [-1, 0], [1, 0], [-1, 1], [0, 1], [1, 1]];
		for (var i = 0; i < ring.length; i++)
		{
			addStructure(HELP_WALL, 0, (HELP_X_SITE + ring[i][0]) * 128, (HELP_Y_SITE + ring[i][1]) * 128);
		}
	}
	var a = benchSpawnBlock(0, truckKind, HELP_X_SITE, HELP_Y_SITE - 6, 1, -1, 1);
	var b = benchSpawnBlock(0, truckKind, HELP_X_SITE, HELP_Y_SITE - 13, 1, -1, 1);
	hackNetOn();

	orderDroidBuild(a[0], DORDER_BUILD, structName, HELP_X_SITE, HELP_Y_SITE);
	orderDroidBuild(b[0], DORDER_BUILD, structName, HELP_X_SITE, HELP_Y_SITE);

	debug("movebench: helpbuild " + truckKind + (slot ? " slot" : ""));
}
