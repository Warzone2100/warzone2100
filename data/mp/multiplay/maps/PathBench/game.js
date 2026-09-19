// Pathfinding benchmark map. A serpentine of walls, an open field, and a sealed
// chamber, on a map the size the perf budget actually targets.
//
// 256x256 because A* cost scales with the area it can explore, so a smaller map
// cannot show a regression that only bites at scale. The three regions each
// exist to drive one kind of search:
//
// - The serpentine forces a corner-to-corner route to cross the full width once
//   per band, so a long route expands a large fraction of the map instead of
//   running down a straight line.
// - The open field south of it gives short routes that resolve in a handful of
//   expansions, which is where per-call overhead shows up rather than search
//   cost.
// - The sealed chamber is ground enclosed by cliff. Its interior tiles are
//   valid and non-blocking, so a route to one is a legitimate request that
//   cannot be satisfied, and A* has to exhaust everything reachable before it
//   can say so. That worst case is the one to watch, since it is where a
//   pathfinder without a search cap explodes.
//
// Geometry only, and no script RNG, so the terrain is identical every run.

const mapWidth = 256;
const mapHeight = 256;

const TEX_GROUND = 0;    // ttypes.ttp: TER_SAND, passable
const TEX_CLIFF = 18;    // ttypes.ttp: TER_CLIFFFACE, blocking

const CLIFF_HEIGHT = 128;   // one tile width, matching the movement bench maps

// Serpentine. Eight bands of wall, each with one gap, the gap alternating sides
// so a route has to run the width of the map to reach the next band.
const BAND_COUNT = 8;
const BAND_FIRST_Y = 24;    // top wall row of the first band
const BAND_PITCH = 22;      // rows between one band and the next
const BAND_THICKNESS = 2;
const GAP_WIDTH = 4;
const GAP_MARGIN = 2;       // tiles between the gap and the map edge

// Sealed chamber, in the open ground below the serpentine.
const CHAMBER_X0 = 200, CHAMBER_X1 = 240;
const CHAMBER_Y0 = 200, CHAMBER_Y1 = 240;
const CHAMBER_WALL = 2;

let texture = new Array(mapWidth * mapHeight);
let height = new Array(mapWidth * mapHeight);

for (let i = 0; i < mapWidth * mapHeight; i++) {
	texture[i] = TEX_GROUND;
	height[i] = 0;
}

function setCliff(x, y) {
	if (x < 0 || y < 0 || x >= mapWidth || y >= mapHeight) {
		return;
	}
	texture[mapWidth * y + x] = TEX_CLIFF;
	height[mapWidth * y + x] = CLIFF_HEIGHT;
}

for (let b = 0; b < BAND_COUNT; b++) {
	const yTop = BAND_FIRST_Y + b * BAND_PITCH;
	// Even bands leave their gap at the west edge, odd bands at the east, so a
	// route alternates sides on the way down.
	const gapLeft = (b % 2 === 0) ? GAP_MARGIN : mapWidth - GAP_MARGIN - GAP_WIDTH;
	const gapRight = gapLeft + GAP_WIDTH;

	for (let t = 0; t < BAND_THICKNESS; t++) {
		const y = yTop + t;
		for (let x = 0; x < mapWidth; x++) {
			if (x >= gapLeft && x < gapRight) {
				continue;   // the gap stays ground level
			}
			setCliff(x, y);
		}
	}
}

// Ring the chamber, leaving its interior as ground.
for (let y = CHAMBER_Y0; y <= CHAMBER_Y1; y++) {
	for (let x = CHAMBER_X0; x <= CHAMBER_X1; x++) {
		const insideX = x >= CHAMBER_X0 + CHAMBER_WALL && x <= CHAMBER_X1 - CHAMBER_WALL;
		const insideY = y >= CHAMBER_Y0 + CHAMBER_WALL && y <= CHAMBER_Y1 - CHAMBER_WALL;
		if (insideX && insideY) {
			continue;
		}
		setCliff(x, y);
	}
}

setMapData(mapWidth, mapHeight, texture, height, [], [], []);
