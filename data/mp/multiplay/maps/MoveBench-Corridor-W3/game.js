// Movement benchmark map. A cliff wall broken by a single narrow pass.
//
// The wall tiles are raised. Height is per-corner, so the passable tiles
// bordering the wall inherit a slope from the wall corners they share. Slope
// feeds rot.pitch and scales speed, so a unit hugging a corridor wall moves
// slower than one down the middle. A flat corridor would not reproduce that.
//
// Geometry only. Units are spawned and ordered by the rules script, because
// map droids cannot specify components.

const mapWidth = 64;
const mapHeight = 64;

const PASS_WIDTH = 3;    // gap width, in tiles
const WALL_LEN = 3;      // wall thickness, i.e. the length of the pass
const X_GAP = 32;        // gap centre column
const Y_WALL = 32;       // first wall row; the wall spans Y_WALL .. Y_WALL+WALL_LEN-1

const TEX_GROUND = 0;    // ttypes.ttp: TER_SAND, passable
const TEX_CLIFF = 18;    // ttypes.ttp: TER_CLIFFFACE, blocking

const CLIFF_HEIGHT = 128;   // one tile width, so wall-adjacent tiles slope steeply

const gapLeft = X_GAP - (PASS_WIDTH >> 1);
const gapRight = gapLeft + PASS_WIDTH;

let texture = new Array(mapWidth * mapHeight);
let height = new Array(mapWidth * mapHeight);

for (let i = 0; i < mapWidth * mapHeight; i++) {
	texture[i] = TEX_GROUND;
	height[i] = 0;
}

for (let t = 0; t < WALL_LEN; t++) {
	const y = Y_WALL + t;
	for (let x = 0; x < mapWidth; x++) {
		if (x >= gapLeft && x < gapRight) {
			continue;   // the pass itself stays ground level
		}
		texture[mapWidth * y + x] = TEX_CLIFF;
		height[mapWidth * y + x] = CLIFF_HEIGHT;
	}
}

setMapData(mapWidth, mapHeight, texture, height, [], [], []);
