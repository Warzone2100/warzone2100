include("tests/movebench_common.js");

// An enemy-held choke under a large attacking force.
//
// Same setup as the enemyblock scenario, with four times the attackers.
// Blocking is not absolute here. Crowd pressure squeezes units sideways into
// blocked terrain, and the leak grows with the size of the pressing force, so a
// number of attackers do get through. That is pre-existing behavior.
//
// This cell is therefore a baseline to hold rather than an invariant to assert:
//
//   - unitsArrived must not rise against the recorded baseline
//   - defenders must not be shuffled aside, and hold position for the whole run
//   - yieldsIssued must be 0, since right-of-way is negotiated only among a
//     player's own or allied units and never across enemy lines
//
// A movement change that lets more attackers squeeze past an enemy wall has
// eroded intentional blocking even if the small-force guard still passes.

const ATTACKER = 0;
const DEFENDER = 1;

const N_ATTACK = 16;
const N_DEFEND = 6;

const Y_NORTH = 24;
const Y_SOUTH = 42;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ATTACKER, "truck");
	benchEnable(DEFENDER, "heavytruck");

	// Defenders plug the 2-wide gap across all three wall rows. They must be
	// heavy-bodied to actually seal it - see the roster note on clearance.
	benchSpawnBlock(DEFENDER, "heavytruck", X_GAP - 1, Y_WALL, 2, +1, N_DEFEND);

	var attackers = benchSpawnBlock(ATTACKER, "truck", X_GAP - 2, Y_NORTH, 4, -1, N_ATTACK);
	hackNetOn();

	// Ordered straight through the held pass. They should not get there.
	benchOrderFanOut(attackers, X_GAP - 4, Y_SOUTH, 8, +1);

	debug("movebench: enemyblock_press, " + attackers.length + " attackers vs " + N_DEFEND + " defenders");
}
