include("tests/movebench_common.js");

// Two hostile mixed forces meeting on the open ground north of the wall.
//
// The standard suite fires no shot: every hostile scenario in it uses unarmed
// trucks, and the single-player ones have no enemy. This one covers what that
// leaves out - visibility of enemies, target selection, line of fire past
// structures, and one projectile of every movement model in flight at once.
//
// Each side gets a mixed block ordered to attack-move at the other, a row of
// persons (the only thing moveCheckSquished acts on), two towers and two wall
// segments to put structures on the line of fire, and trucks parked by the
// towers so the damaged-structure repair search runs. The towers stand clear of
// the fighting and stay whole, so that search always comes up empty - see
// perf_repair_search for the case that finds something.

const NORTH = 0;
const SOUTH = 1;

const N_MIXED = 60;
const N_PERSON = 10;
const N_TRUCK = 3;
const COLS = 10;
const KINDS = ["medium", "cannon", "homing", "mortar", "flamer"];

const X_LEFT = X_GAP - 5;   // left column of every block
const Y_MEET = 17;          // where both attack-move orders send their side

function setUpSide(player, towerY, wallY)
{
	for (var i = 0; i < KINDS.length; i++)
	{
		benchEnable(player, KINDS[i]);
	}
	benchEnable(player, "person");
	benchEnable(player, "truck");

	addStructure("WallTower01", player, (X_GAP - 6) * 128, towerY * 128);
	addStructure("WallTower01", player, (X_GAP + 6) * 128, towerY * 128);
	addStructure("A0HardcreteMk1Wall", player, (X_GAP - 1) * 128, wallY * 128);
	addStructure("A0HardcreteMk1Wall", player, (X_GAP + 1) * 128, wallY * 128);
}

function orderSideTo(droids, y)
{
	for (var i = 0; i < droids.length; i++)
	{
		orderDroidLoc(droids[i], DORDER_SCOUT, X_LEFT + (i % COLS), y);
	}
}

function eventStartLevel()
{
	hackNetOff();
	setUpSide(NORTH, 3, 5);
	setUpSide(SOUTH, 31, 29);

	var northTrucks = benchSpawnFixed(NORTH, "truck", X_GAP - 7, 6, 3, +1, N_TRUCK);
	var southTrucks = benchSpawnFixed(SOUTH, "truck", X_GAP - 7, 28, 3, -1, N_TRUCK);
	var northPeople = benchSpawnFixed(NORTH, "person", X_LEFT, 7, COLS, +1, N_PERSON);
	var southPeople = benchSpawnFixed(SOUTH, "person", X_LEFT, 27, COLS, -1, N_PERSON);
	var north = benchSpawnMixedBlock(NORTH, KINDS, X_LEFT, 9, COLS, +1, N_MIXED);
	var south = benchSpawnMixedBlock(SOUTH, KINDS, X_LEFT, 25, COLS, -1, N_MIXED);
	hackNetOn();

	orderSideTo(north.concat(northPeople), Y_MEET);
	orderSideTo(south.concat(southPeople), Y_MEET + 1);

	debug("movebench: perf_battle, " + (north.length + northPeople.length + northTrucks.length)
	      + " vs " + (south.length + southPeople.length + southTrucks.length));
}
