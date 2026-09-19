include("tests/movebench_common.js");

// An enemy-held choke must stay solid.
//
// Player 0 is ordered through the pass. Player 1, non-allied, parks unarmed
// units in it and is given no orders. Walling a choke with units is a
// legitimate tactic, so this asserts three things:
//
//   - attackers must not get through. Spawn placement varies per arrangement
//     and a stray attacker occasionally slips the plug, so the criterion is a
//     median of 2 or fewer arrivals across arrangements rather than a hard zero
//   - defenders must not be shuffled aside, and hold position for the whole run
//   - yieldsIssued must be 0, since right-of-way is negotiated only among a
//     player's own or allied units and never across enemy lines
//
// Both sides are unarmed on purpose. Two armed hostile blocks would shoot each
// other and clear the choke, measuring combat rather than blocking.
//
// The attacking force is deliberately small. Blocking is not absolute here.
// Crowd pressure squeezes units sideways into blocked terrain, and the leak
// grows with the size of the pressing force. At this size the invariant holds
// cleanly. The enemyblock_press scenario covers the leak itself.

const ATTACKER = 0;
const DEFENDER = 1;

const N_ATTACK = 4;
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
	benchSpawnFixed(DEFENDER, "heavytruck", X_GAP - 1, Y_WALL, 2, +1, N_DEFEND);

	var attackers = benchSpawnBlock(ATTACKER, "truck", X_GAP - 2, Y_NORTH, 4, -1, N_ATTACK);
	hackNetOn();

	// Ordered straight through the held pass. They should not get there.
	benchOrderFanOut(attackers, X_GAP - 4, Y_SOUTH, 8, +1);

	debug("movebench: enemyblock, " + attackers.length + " attackers vs " + N_DEFEND + " defenders");
}
