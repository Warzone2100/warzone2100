include("tests/movebench_common.js");

// Mass corner head-on across Sk-Rush's north-western quarter: two full
// clusters of cyborgs swap starting positions. The south-eastern cluster
// leaves its corridor at the bend near 10,33 while the north-western one cuts
// the same corner from the other side, turning left across the exit. Both
// flows want the inside line, so a cutting crowd builds against the emerging
// column at the corner and the side corridor entrance at 7,25 backs up behind
// it. Routes hug the shortest line here because nothing prices the corner,
// which makes this the acceptance case for shaping routes around open bends,
// at a scale where the failure cannot hide.

const ME = 0;
const N_UNITS = 136;   // clone wars!!, per side

var a = [];
var b = [];

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "cyborg");
	a = benchSpawnClones(ME, "cyborg", 24, 37, N_UNITS);
	b = benchSpawnClones(ME, "cyborg", 6, 13, N_UNITS);
	hackNetOn();

	benchOrderAllTo(a, 6, 13);
	benchOrderAllTo(b, 24, 37);

	debug("movebench: rush corner, " + (a.length + b.length) + " units");
}
