// Movement benchmark map. The Sk-Rush north-west corner in isolation: the
// blocking mass whose south-west corner the traffic rounds, on an otherwise
// flat open field.
//
// The mass keeps the corner's real faces from Sk-Rush - the west face at
// x=11 running north from y=31, the south face at y=31 running east - and
// extends to the north edge so no route ever circles it the other way. The
// side and southern terrain that constrains the approaches on Sk-Rush is
// gone, so no ground here is narrow enough to detect as a corridor and the
// corner behavior stands alone.
//
// Cliff tiles are raised as on the other benchmark maps so passable tiles
// beside them inherit a slope.

const mapWidth = 64;
const mapHeight = 64;

const TEX_GROUND = 0;    // ttypes.ttp: passable
const TEX_CLIFF = 18;    // ttypes.ttp: TER_CLIFFFACE, blocking
const CLIFF_HEIGHT = 128;

// The mass, sharing Sk-Rush's corner tile: west face at x=11, south face at
// y=31, so the rounding bend sits by (10,32) exactly as on the real map.
const MASS_X0 = 11, MASS_X1 = 29;
const MASS_Y1 = 32;

function isOpen(x, y)
{
	return x < MASS_X0 || x >= MASS_X1 || y >= MASS_Y1;
}

let texture = new Array(mapWidth * mapHeight);
let height = new Array(mapWidth * mapHeight);

for (let y = 0; y < mapHeight; y++)
{
	for (let x = 0; x < mapWidth; x++)
	{
		const i = mapWidth * y + x;
		if (isOpen(x, y))
		{
			texture[i] = TEX_GROUND;
			height[i] = 0;
		}
		else
		{
			texture[i] = TEX_CLIFF;
			height[i] = CLIFF_HEIGHT;
		}
	}
}

setMapData(mapWidth, mapHeight, texture, height, [], [], []);
