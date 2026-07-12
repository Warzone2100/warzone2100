// Movement benchmark map. A cliff wall broken by two separate passes.
//
// Every other bench map offers a single route, which is the worst case and is
// what the reported bottleneck describes. Real maps usually give a choice. This
// map exists so that can be tested, both to see whether counter-flow deadlock
// survives having an alternative and to give congestion-aware routing somewhere
// to route to.
//
// The two gaps are equal width and equidistant from the spawn columns, so
// neither is the obvious choice on path cost alone.
//
// As on the other maps, cliff tiles are raised so the passable tiles beside
// them inherit a slope.

const mapWidth = 64;
const mapHeight = 64;

const PASS_WIDTH = 2;    // gap width, in tiles
const WALL_LEN = 3;      // wall thickness, i.e. the length of the pass
const X_GAP = 32;        // gap centre column
const Y_WALL = 32;       // first wall row; the wall spans Y_WALL .. Y_WALL+WALL_LEN-1

const TEX_GROUND = 0;    // ttypes.ttp: TER_SAND, passable
const TEX_CLIFF = 18;    // ttypes.ttp: TER_CLIFFFACE, blocking

const CLIFF_HEIGHT = 128;   // one tile width, so wall-adjacent tiles slope steeply

const GAP_SEPARATION = 16;   // tiles between the two gap centres

const gapALeft = X_GAP - GAP_SEPARATION / 2 - (PASS_WIDTH >> 1);
const gapBLeft = X_GAP + GAP_SEPARATION / 2 - (PASS_WIDTH >> 1);

let texture = new Array(mapWidth * mapHeight);
let height = new Array(mapWidth * mapHeight);

for (let i = 0; i < mapWidth * mapHeight; i++) {
	texture[i] = TEX_GROUND;
	height[i] = 0;
}

for (let t = 0; t < WALL_LEN; t++) {
	const y = Y_WALL + t;
	for (let x = 0; x < mapWidth; x++) {
		const inGapA = (x >= gapALeft && x < gapALeft + PASS_WIDTH);
		const inGapB = (x >= gapBLeft && x < gapBLeft + PASS_WIDTH);
		if (inGapA || inGapB) {
			continue;   // the passes themselves stay ground level
		}
		texture[mapWidth * y + x] = TEX_CLIFF;
		height[mapWidth * y + x] = CLIFF_HEIGHT;
	}
}

setMapData(mapWidth, mapHeight, texture, height, [], [], []);
