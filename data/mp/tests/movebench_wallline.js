include("tests/movebench_common.js");

// An army shuffled back and forth behind a line of spaced wall segments, on
// flat open ground. The manual report this encodes: on 4.7.0 a clone-wars
// army moved back and forth behind such a line stays behind it, while later
// builds let units wander out through the gaps in front of it.
//
// Every goal is behind the line, so no unit ever has a reason to pass
// through a gap. The scenario reports on stderr how many did, per leg and in
// total, as lines starting "wallline:": the distinct droids that ever stood
// in front of the line, the peak count at once, and droid-seconds spent
// there. Lower is better, but 4.7.0-like behavior is not zero: a crowd
// pressed against the line overflows into the gaps, and a droid turning
// around in a gap can arc out in front of it. tests/movebench/wallline.sh
// sums the totals over every layout and arrangement.
//
// The walls are structures, placed after corridor detection, so none of the
// gaps is a corridor and no lane machinery is involved. The south half of
// MoveBench-OpenCorner is open flat ground the full width of the map.

const ME = 0;
const N_UNITS = 41;   // the reported clone-wars army
const MIXED = ["lightcannonht", "medium", "hover"];   // the kinds a mixed army cycles through

// The line: wall segments with gaps across the field. The side with smaller y
// is "in front", the enemy side.
const WALL_Y = 42;
const WALL_X0 = 4, WALL_X1 = 60;

// arrangement / 9 picks a layout, arrangement % 9 the spawn shift, so the
// scenario has 234 arrangements. Each layout names its wall pattern and the
// goals the army is cycled through, all behind the line, and optionally the
// army's size (`n`) and a mix of kinds cycled through it (`kinds`). The army
// starts three tiles behind the second goal, so the first leg is a real drive.
// With two-tile segments and gaps from x = 4 the pattern repeats every four
// tiles, so x = 22 and 42 sit behind gaps and x = 24 and 44 behind segments.
const LAYOUTS = [
	{ seg: 2, gap: 2, goals: [[32, 52], [32, 43]] },   // depth, goal behind a segment
	{ seg: 2, gap: 2, goals: [[31, 52], [31, 43]] },   // depth, goal behind a gap
	{ seg: 2, gap: 2, goals: [[22, 44], [42, 44]] },   // lateral along the line
	{ seg: 2, gap: 2, goals: [[22, 43], [42, 43]] },   // lateral, tight to the line
	{ seg: 2, gap: 1, goals: [[32, 52], [32, 43]] },   // depth, one-tile gaps
	{ seg: 2, gap: 2, goals: [[24, 52], [40, 43]] },   // diagonal
	// Lateral shuffles standing off the line, as in the report. The crowd
	// around a goal is three or four tiles deep, so the nearer of these still
	// brush the line.
	{ seg: 2, gap: 2, goals: [[22, 45], [42, 45]] },   // lateral, 3 tiles back
	{ seg: 2, gap: 2, goals: [[22, 46], [42, 46]] },   // lateral, 4 tiles back
	{ seg: 2, gap: 2, goals: [[22, 47], [42, 47]] },   // lateral, 5 tiles back
	{ seg: 2, gap: 2, goals: [[22, 48], [42, 48]] },   // lateral, 6 tiles back
	// The shuffle at 3 and 4 tiles back, varying one thing at a time against
	// layouts 6 and 7.
	{ seg: 2, gap: 2, goals: [[26, 45], [38, 45]] },   // 3 back, 12-tile span
	{ seg: 2, gap: 2, goals: [[26, 46], [38, 46]] },   // 4 back, 12-tile span
	{ seg: 2, gap: 2, goals: [[16, 45], [48, 45]] },   // 3 back, 32-tile span
	{ seg: 2, gap: 2, goals: [[16, 46], [48, 46]] },   // 4 back, 32-tile span
	{ seg: 2, gap: 1, goals: [[22, 45], [42, 45]] },   // 3 back, one-tile gaps
	{ seg: 2, gap: 1, goals: [[22, 46], [42, 46]] },   // 4 back, one-tile gaps
	{ seg: 2, gap: 3, goals: [[22, 45], [42, 45]] },   // 3 back, three-tile gaps
	{ seg: 2, gap: 3, goals: [[22, 46], [42, 46]] },   // 4 back, three-tile gaps
	{ seg: 2, gap: 2, goals: [[22, 45], [42, 45]], n: 61 },   // 3 back, larger army
	{ seg: 2, gap: 2, goals: [[22, 46], [42, 46]], n: 61 },   // 4 back, larger army
	{ seg: 2, gap: 2, goals: [[22, 45], [42, 45]], kinds: MIXED },   // 3 back, mixed propulsion
	{ seg: 2, gap: 2, goals: [[22, 46], [42, 46]], kinds: MIXED },   // 4 back, mixed propulsion
	{ seg: 2, gap: 2, goals: [[23, 45], [43, 45]] },   // 3 back, goals one tile along, still behind gaps
	{ seg: 2, gap: 2, goals: [[23, 46], [43, 46]] },   // 4 back, goals one tile along, still behind gaps
	{ seg: 2, gap: 2, goals: [[24, 45], [44, 45]] },   // 3 back, goals behind segments
	{ seg: 2, gap: 2, goals: [[24, 46], [44, 46]] },   // 4 back, goals behind segments
];
const LAYOUT = Math.floor(benchArrangement() / 9) % LAYOUTS.length;
const L = LAYOUTS[LAYOUT];
const SEG_LEN = L.seg, GAP_LEN = L.gap;

const LEG_MS = 20000;
const N_LEGS = 12;
const SAMPLE_MS = 500;
// A droid this many tiles or more in front of the line has left the gap it
// came through rather than poking into it.
const DEEP_TILES = 3;

var army = [];
var leg = 0;

var everFront = {};     // droid id -> true, over the whole run
var legEverFront = {};  // the same, this leg
var legPeak = 0;
var legFrontSamples = 0;
var totalPeak = 0;
var totalFrontSamples = 0;
var legMaxDepth = 0;    // tiles in front of the line, deepest sample
var totalMaxDepth = 0;
var legDeepSamples = 0;
var totalDeepSamples = 0;

function buildWallLine()
{
	for (var x = WALL_X0; x <= WALL_X1; x += SEG_LEN + GAP_LEN)
	{
		for (var k = 0; k < SEG_LEN; k++)
		{
			addStructure("A0HardcreteMk1Wall", ME, (x + k) * 128 + 64, WALL_Y * 128 + 64);
		}
	}
}

function liveArmy()
{
	var ids = {};
	for (var i = 0; i < army.length; i++)
	{
		ids[army[i].id] = true;
	}
	return enumDroid(ME).filter(function (d) { return ids[d.id] === true; });
}

function wallSample()
{
	var droids = liveArmy();
	var front = 0;
	for (var i = 0; i < droids.length; i++)
	{
		if (droids[i].y < WALL_Y)
		{
			front++;
			everFront[droids[i].id] = true;
			legEverFront[droids[i].id] = true;
			var depth = WALL_Y - droids[i].y;
			legMaxDepth = Math.max(legMaxDepth, depth);
			totalMaxDepth = Math.max(totalMaxDepth, depth);
			if (depth >= DEEP_TILES)
			{
				legDeepSamples++;
				totalDeepSamples++;
			}
		}
	}
	legPeak = Math.max(legPeak, front);
	totalPeak = Math.max(totalPeak, front);
	legFrontSamples += front;
	totalFrontSamples += front;
}

function closeLeg()
{
	if (leg === 0)
	{
		return;
	}
	var behind = 0;
	var droids = liveArmy();
	for (var i = 0; i < droids.length; i++)
	{
		if (droids[i].y > WALL_Y)
		{
			behind++;
		}
	}
	debug("wallline: leg " + leg
	      + " everFront=" + Object.keys(legEverFront).length
	      + " peakFront=" + legPeak
	      + " frontSeconds=" + (legFrontSamples * SAMPLE_MS / 1000)
	      + " maxDepth=" + legMaxDepth
	      + " deepSeconds=" + (legDeepSamples * SAMPLE_MS / 1000)
	      + " behindAtEnd=" + behind + "/" + droids.length);
	legEverFront = {};
	legPeak = 0;
	legFrontSamples = 0;
	legMaxDepth = 0;
	legDeepSamples = 0;
}

function nextLeg()
{
	closeLeg();
	if (leg >= N_LEGS)
	{
		removeTimer("nextLeg");
		removeTimer("wallSample");
		debug("wallline: total layout=" + LAYOUT + " everFront=" + Object.keys(everFront).length
		      + " peakFront=" + totalPeak
		      + " frontSeconds=" + (totalFrontSamples * SAMPLE_MS / 1000)
		      + " maxDepth=" + totalMaxDepth
		      + " deepSeconds=" + (totalDeepSamples * SAMPLE_MS / 1000));
		return;
	}
	leg++;
	var g = L.goals[(leg - 1) % L.goals.length];
	benchOrderAllTo(liveArmy(), g[0], g[1]);
}

function eventStartLevel()
{
	hackNetOff();
	var kinds = L.kinds || ["lightcannonht"];
	for (var i = 0; i < kinds.length; i++)
	{
		benchEnable(ME, kinds[i]);
	}
	buildWallLine();
	if (L.kinds)
	{
		army = benchSpawnMixedClones(ME, L.kinds, L.goals[1][0], L.goals[1][1] + 3, L.n || N_UNITS);
	}
	else
	{
		army = benchSpawnClones(ME, "lightcannonht", L.goals[1][0], L.goals[1][1] + 3, L.n || N_UNITS);
	}
	hackNetOn();

	nextLeg();
	setTimer("nextLeg", LEG_MS);
	setTimer("wallSample", SAMPLE_MS);

	debug("movebench: wallline, " + army.length + " units");
}
