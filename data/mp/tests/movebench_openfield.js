include("tests/movebench_common.js");

// A wide block translating laterally across open ground.
//
// No choke, no opposing flow, no parked obstacles. Units keep station and only
// make glancing contact, mostly as they converge on their goals.
//
// This is a guard rather than a target. Any non-trivial yielding here is a
// fault, and bumps and arrival times must not regress against the baseline.

const ME = 0;
const N_UNITS = 40;

const X_START = 8;
const X_GOAL = 46;
const Y_ROW = 10;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var block = benchSpawnBlock(ME, "medium", X_START, Y_ROW, 10, +1, N_UNITS);
	hackNetOn();

	// Straight lateral translation: same heading for everyone, so nothing here
	// resembles a head-on stall.
	benchOrderFanOut(block, X_GOAL, Y_ROW, 10, +1);

	debug("movebench: openfield, " + block.length + " units translating");
}
