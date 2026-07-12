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
};

function benchEnable(player, kind)
{
	var k = ROSTER[kind];
	setPower(1000000, player);
	makeComponentAvailable(k.body, player);
	makeComponentAvailable(k.prop, player);
	makeComponentAvailable(k.weap, player);
}

// Lays `count` units in a `cols`-wide block anchored at (x0, y0), with rows
// receding in `rowDir`. `spacing` (default 1) is the tile pitch between units,
// so a field can be laid out loosely enough to be negotiable rather than solid.
// Returns the droids actually created, in spawn order.
function benchSpawnBlock(player, kind, x0, y0, cols, rowDir, count, spacing)
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

// As benchSpawnBlock, but cycles through a list of kinds so a cluster contains
// units of differing speed and turn rate. Speed heterogeneity is the point:
// faster units catch up to slower ones and stack behind them, which is a
// distinct congestion source from geometry.
function benchSpawnMixedBlock(player, kinds, x0, y0, cols, rowDir, count)
{
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

// Orders every droid to a single destination. Used by the blob scenario, where
// converging on one point is the phenomenon under test rather than an artifact.
function benchOrderAllTo(droids, x, y)
{
	for (var i = 0; i < droids.length; i++)
	{
		orderDroidLoc(droids[i], DORDER_MOVE, x, y);
	}
}
