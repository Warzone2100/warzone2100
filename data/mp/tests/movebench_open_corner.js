include("tests/movebench_common.js");

// The rush_corner head-on with the corridors stripped away. Same corner, same
// clusters, same relative spots: the mass's south-west corner matches
// Sk-Rush's, the clusters spawn at the identical coordinates and swap
// positions, but the flanking terrain is gone so nothing here detects as a
// corridor. Whatever corner scrum forms is the route layer's own doing, with
// no lane machinery involved on any backend.

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

	debug("movebench: open corner, " + (a.length + b.length) + " units");
}
