'use strict';

const mapWidth = 192;
const mapHeight = 192;
const players = 10;
const minFieldSize = 10;
const targetFieldSize = 500;
const maxDifference = 192;

const mapSize = mapWidth*mapHeight;

const verbose = false;

// Maximum and average return values of gameRand().
const RMAX = 0xffffffff;
const RAVG = RMAX/2;


// Random vector with max length maxR.
function randVec(maxR) {
	const s = maxR/RAVG;
	while (true) {  // eslint-disable-line no-constant-condition
		const x = s*(gameRand() - RAVG);
		const y = s*(gameRand() - RAVG);
		if (x*x + y*y <= maxR*maxR) {
			return [x, y];
		}
	}
}

// Add vectors.
function addVec(a, b) {
	return [a[0] + b[0], a[1] + b[1]];
}

// Random number with approximate normal distribution.
function normalRand() {
	// gameRand() returns values with average (2**32 - 1)/2 and standard deviation Math.sqrt((2**64 - 1)/12)
	//return (gameRand() + gameRand() + gameRand() + gameRand() + gameRand() + gameRand() - (2**32 - 1)/2*6)/Math.sqrt((2**64 - 1)/12*6);
	return (gameRand() + gameRand() + gameRand() + gameRand() + gameRand() + gameRand() - 12884901885)/3037000500;
}

// Returns a point not too far from a and b's midpoint.
function approxMidpoint(a, b) {
	const dx = b[0] - a[0], dy = b[1] - a[1];
	const mx = a[0] + b[0], my = a[1] + b[1];
	const s = normalRand()/8;
	return [mx/2 - s*dy, my/2 + s*dx];
}

// Generates a slightly fractal loop of points.
function genLoop(subdivLevel) {
	const s = 0.5877852522924731;  // Math.sin(2*Math.PI / players)
	const c = 0.8090169943749475;  // Math.cos(2*Math.PI / players)
	let xy = [-0.9510565162951535, 0.309016994374947];  // [Math.cos(2*Math.PI / players / 2), Math.sin(2*Math.PI / players / 2)]
	let p = [];
	for (let i = 0; i < players; ++i) {
		const r = .4 + .5/RMAX*gameRand();
		p.push([mapWidth*(1 + r*xy[0])/2 | 0, mapHeight*(1 - r*xy[1])/2 | 0]);
		xy = [c*xy[0] + s*xy[1], -s*xy[0] + c*xy[1]];
	}
	for (let subdiv = 0; subdiv < subdivLevel; ++subdiv) {
		const q = [];
		for (let i = 0; i < p.length; ++i) {
			const j = (i + 1)%p.length;
			q.push(p[i]);
			q.push(approxMidpoint(p[i], p[j]));
		}
		p = q;
	}
	return p;
}

// Generates some random points.
function genIslands(count) {
	const p = [];
	for (let i = 0; i < count; ++i) {
		p.push([3 + (mapWidth - 6)/RMAX*gameRand(), 3 + (mapHeight - 6)/RMAX*gameRand()]);
	}
	return p;
}

// Class for drawing circles.
function LayerMap() {
	const edges = this.edges = [];
	edges.length = mapSize;
	const img = this.img = [];
	img.length = mapSize;
	this.base = 0;
	for (let i = 0; i < mapSize; ++i) {
		edges[i] = 0;
		img[i] = 0;
	}
}
LayerMap.prototype = {
	drawCircle: function (x, y, r, plane) {  // 0 ≤ plane - base < 4
		const edges = this.edges;
		const v = 1 << 8*(plane - this.base);
		const yMin = Math.max(Math.ceil(y - r), 0), yMax = Math.min(Math.floor(y + r), mapHeight - 1);
		for (let iy = yMin; iy <= yMax; ++iy) {
			const dy = y - iy;
			const dx = Math.sqrt(r*r - dy*dy);
			const xMin = Math.max(Math.ceil(x - dx), 0), xMax = Math.min(Math.floor(x + dx), mapWidth - 1);
			edges[mapWidth*iy + xMin] += v;
			if (xMax + 1 < mapWidth) {
				edges[mapWidth*iy + xMax + 1] -= v;
			}
		}
	},
	begin: function (base) {
		this.base = base;
	},
	end: function () {
		const edges = this.edges;
		for (let y = 0; y < mapHeight; ++y) {
			let c = 0;
			for (let x = 0; x < mapWidth; ++x) {
				const i = mapWidth*y + x;
				c += edges[i];
				if (c > 0) {
					this.img[i] = Math.max(this.img[i], this.base + (c >= 0x100) + (c >= 0x10000) + (c >= 0x1000000));
				}
			}
		}
		for (let i = 0; i < mapSize; ++i) {
			edges[i] = 0;
		}
	}
};

//const ttyp = [[1,4,12,42,89,90,91,92,93,94,95],[0,9,10,11,28,40],[2,3,5,6,7,8,26,36,37,38,39,47,78,79,80,81,82,83,84,85,86,87,88],[23,24,25],[20,21,27,29,34,35,41,43,44,48,49,50,51,52,53,54,60,61,72,73,74],[76],[19,22,57,59,77],[13,14,15,16,17,30,31,32,33],[18,45,46,71,75],[55,56,58,62,63,64,65,66,67,68,69,70],[],[]];

const TILED = [19,22,77];
const RAMP = [20,29];
const HIGHISLAND = [];
const ISLAND = [];
const WATER = [17];
const CLIFFFACE = [18,45,46,71];

// Pick random element of array.
function sample(array) {
	return array[gameRand(array.length)];
}

// Pick randomly rotated texture from array.
function sampleTexture(array) {
	return sample(array) | gameRand()&0xf800;
}

// Smooths `height` in the places marked by `smoothDirs` by amount `factor`.
function doSmooth(height, smoothDirs, factor) {
	const newHeight = [];
	const bits = [0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4];
	for (let y = 0; y < mapHeight; ++y) {
		for (let x = 0; x < mapWidth; ++x) {
			const i = mapWidth*y + x;
			const s = smoothDirs[i];
			const n = ((s & 1) && height[i + 1]) + ((s & 2) && height[i + mapWidth]) + ((s & 4) && height[i - 1]) + ((s & 8) && height[i - mapWidth]);
			const d = bits[s];
			newHeight[i] = (factor*height[i] + n)/(factor + d);
		}
	}
	return newHeight;
}

// Generates terrain and heightmap, including locations.
function genTerrain() {
	const subdivLevel = 6;
	const M = 1 << subdivLevel;
	const loop = genLoop(subdivLevel);
	const islands = genIslands(40);
	const smallIslands = genIslands(40);
	const layers = new LayerMap();
	const startPos = [];
	layers.begin(1);
	for (let i = 0; i < loop.length; ++i) {
		layers.drawCircle(loop[i][0], loop[i][1], i%M < M/8 || i%M >= M - M/8? 8 : 3, 4);
		if (i%M === M/2) {
			layers.drawCircle(loop[i][0], loop[i][1], 6, 3);
		}
		if (i%M === 0) {
			startPos.push(loop[i]);
		}
	}
	for (let i = 0; i < islands.length; ++i) {
		for (let j = gameRand(6); j < 6; ++j) {
			const p = addVec(islands[i], randVec(2));
			const r = 2 + 6/RMAX*gameRand();
			layers.drawCircle(p[0], p[1], r, 1 + gameRand(2));
		}
	}
	for (let i = 0; i < smallIslands.length; ++i) {
		const p = smallIslands[i];
		const r = 2 + 1/RMAX*gameRand();
		layers.drawCircle(p[0], p[1], r, i%4 === 0? 2 : 1);
	}
	layers.end();

	const texture = [];  // Texture of tiles.
	texture.length = mapSize;
	let height = [];  // Height of upper-left corner of tiles.
	height.length = mapSize;
	const smoothDirs = [];  // 1, 2, 4, 8 = right, down, left, up, applies to corners of tiles.
	smoothDirs.length = mapSize;
	for (let i = 0; i < mapSize; ++i) {
		smoothDirs[i] = 0;
	}
	const isCliffOrWater = [];  // Whether texture of tile is cliff/water.
	isCliffOrWater.length = mapSize;
	// 0 = water, 1 = land near water, 2 = hills which form cliffs next to water, 3 = land outside base, 4 = land for bases, 5 = cliffs
	const planeTexture = [WATER, ISLAND, HIGHISLAND, RAMP, TILED, CLIFFFACE];
	const planeHeight = [0, 2, 192, 128, 384];
	for (let y = 0; y < mapHeight; ++y) {
		for (let x = 0; x < mapWidth; ++x) {
			const i00 = mapWidth*y + x, i01 = i00 + (x < mapWidth - 1), i10 = i00 + mapWidth*(y < mapHeight - 1), i11 = i01 + i10 - i00;
			const plane00 = layers.img[i00];
			const plane01 = layers.img[i01];
			const plane10 = layers.img[i10];
			const plane11 = layers.img[i11];
			const planeMin = Math.min(plane00, plane01, plane10, plane11);
			const planeMax = Math.max(plane00, plane01, plane10, plane11);
			//   0 1 2 3 4
			// 0 .
			// 1 . .
			// 2 + . .
			// 3 . . . .
			// 4 + + + + .
			const plane = (planeMax === 4 && planeMin < 3) || (planeMax === 2 && planeMin === 0)? 5 : planeMax;
			texture[i00] = sampleTexture(planeTexture[plane]);
			height[i00] = planeHeight[plane00];
			isCliffOrWater[i00] = plane === 0 || plane === 5;
			if (!isCliffOrWater[i00] && i00 !== i01 && i00 !== i10) {
				if (plane00 != 0) {
					smoothDirs[i00] |= 3;
				}
				if (plane01 != 0) {
					smoothDirs[i01] |= 6;
				}
				if (plane10 != 0) {
					smoothDirs[i10] |= 9;
				}
				if (plane11 != 0) {
					smoothDirs[i11] |= 12;
				}
			}
		}
	}

	height = doSmooth(doSmooth(doSmooth(doSmooth(doSmooth(height, smoothDirs, .125), smoothDirs, .25), smoothDirs, .5), smoothDirs, 1), smoothDirs, 2);

	return {texture: texture, height: height, smoothDirs: smoothDirs, isCliffOrWater: isCliffOrWater, loop: loop, startPos: startPos, islands: islands, smallIslands: smallIslands};
}

function SpaceFinder(terrain) {
	const height = terrain.height;
	const isCliffOrWater = terrain.isCliffOrWater;

	this.isCliffOrWater = isCliffOrWater;
	this.visit = [];
	this.visit.length = mapSize;
	for (let i = 0; i < mapSize; ++i) {
		this.visit[i] = 0;
	}
	this.visitC = 0;
	this.placeNearFailed = false;

	// Mark tiles occupied, if structures shouldn't be placed on them.
	const occupied = [];
	for (let y = 0; y < mapHeight; ++y) {
		for (let x = 0; x < mapWidth; ++x) {
			const i00 = mapWidth*y + x, i01 = i00 + 1, i10 = i00 + mapWidth, i11 = i01 + i10 - i00;
			if (x < 3 || x >= mapWidth - 3 || y < 3 || y >= mapWidth - 3) {
				occupied[i00] = 2;
				continue;
			}
			const minHeight = Math.min(height[i00], height[i01], height[i10], height[i11]);
			const maxHeight = Math.max(height[i00], height[i01], height[i10], height[i11]);
			occupied[i00] = maxHeight - minHeight > 48 || isCliffOrWater[i00]? 2 : 0;
		}
	}
	this.occupied = occupied;
}
SpaceFinder.prototype = {
	// Find if a structure can be placed at [x, y], and if so, mark it occupied and return the location in world coords.
	placeAt: function (x, y, w, h, pad) {
		if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
			return null;
		}
		const occupied = this.occupied;
		for (let t = y - h/2; t < y + h/2; ++t) {
			for (let s = x - w/2; s < x + w/2; ++s) {
				if (occupied[mapWidth*t + s]) {
					return null;
				}
			}
		}
		if (pad) {
			for (let t = y - h/2; t < y + h/2; ++t) {
				if (occupied[mapWidth*t + x - w/2 - 1] >= 2 || occupied[mapWidth*t + x + w/2] >= 2) {
					return null;
				}
			}
			for (let s = x - w/2 - 1; s < x + w/2 + 1; ++s) {
				if (occupied[mapWidth*(y - h/2 - 1) + s] >= 2 || occupied[mapWidth*(y + h/2) + s] >= 2) {
					return null;
				}
			}
		}
		for (let t = y - h/2; t < y + h/2; ++t) {
			for (let s = x - w/2; s < x + w/2; ++s) {
				occupied[mapWidth*t + s] = pad? 2 : 1;
			}
		}
		return [x*128, y*128];
	},
	// Consider placing at x, y, with probability 1/scatter.
	consider: function (x, y, w, h, pad, scatter) {
		return (!scatter || !gameRand(scatter)) && this.placeAt(x, y, w, h, pad);
	},
	// Find if a structure can be placed somewhere near [x, y] without crossing cliffs or water, and if so, mark it occupied and return where in world coords.
	placeNear: function (x, y, w, h, pad, scatter) {
		const isCliffOrWater = this.isCliffOrWater;
		x = Math.round(x - w/2) + w/2;
		y = Math.round(y - h/2) + h/2;
		const visit = this.visit;
		const visitC = ++this.visitC;
		function queue(x, y, fromCW) {
			const i = mapWidth*(y | 0) + (x | 0);
			if (visit[i] !== visitC && x > 0 && x < mapWidth && y > 0 && y < mapHeight && (!isCliffOrWater[i] || fromCW)) {
				next.push([x, y]);
				visit[i] = visitC;
			}
		}
		let next = [];
		queue(x, y, true);
		while (next.length) {
			const cur = next;
			next = [];
			for (let i = 0; i < cur.length; ++i) {
				const cx = cur[i][0], cy = cur[i][1];
				const xy = this.consider(cx, cy, w, h, pad, scatter);
				if (xy) {
					return xy;
				}
				const fromCW = isCliffOrWater[mapWidth*(cy | 0) + (cx | 0)];
				queue(cx + 1, cy, fromCW);
				queue(cx - 1, cy, fromCW);
				queue(cx, cy + 1, fromCW);
				queue(cx, cy - 1, fromCW);
			}
		}
		if (verbose) log('placeNear(' + x + ', ' + y + ', ' + w + ', ' + h + ', ' + pad + ', ' + scatter + ') couldn\'t find spot.');
		this.placeNearFailed = true;
		return null;
	},
	// Reset error status.
	resetError: function () {
		this.placeNearFailed = false;
	},
	// Check if anything failed since resetError.
	checkError: function () {
		return this.placeNearFailed;
	}
};

// Place stuff on map, including a base at each starting position.
function placeStuff(terrain) {
	const spaceFinder = new SpaceFinder(terrain);
	//const placeNear = spaceFinder.placeNear.bind(spaceFinder);
	const placeNear = function (x, y, w, h, pad, scatter) { return spaceFinder.placeNear(x, y, w, h, pad, scatter); };  // QtScript apparently missing even the basics such as `bind`.

	const structures = [];
	const droids = [];
	const features = [];
	for (let q = 0; q < terrain.loop.length; q += 16) {
		const r = terrain.loop[q];
		features.push({name: "OilResource", position: placeNear(r[0], r[1], 1, 1, true), direction: 0});
	}
	for (let q = 0; q < terrain.smallIslands.length; ++q) {
		const r = terrain.smallIslands[q];
		features.push({name: "OilResource", position: placeNear(r[0], r[1], 1, 1, true), direction: 0});
	}
	for (let q = 0; q < terrain.islands.length; ++q) {
		const r = terrain.islands[q];
		for (let i = gameRand(4); i < 4; ++i) {
			features.push({name: "OilResource", position: placeNear(r[0], r[1], 1, 1, true, 3), direction: 0});
		}
	}
	spaceFinder.resetError();
	for (let player = 0; player < players && !spaceFinder.checkError(); ++player) {
		const x = terrain.startPos[player][0], y = terrain.startPos[player][1];
		structures.push({name: "A0CommandCentre", position: placeNear(x, y, 2, 2, true), direction: 0x4000*gameRand(4), modules: 0, player: player});
		structures.push({name: "A0LightFactory", position: placeNear(x, y, 3, 3, true, 4), direction: 0x4000*gameRand(4), modules: 1, player: player});
		structures.push({name: "A0ResearchFacility", position: placeNear(x, y, 2, 2, true, 4), direction: 0x4000*gameRand(4), modules: 1, player: player});
		structures.push({name: "A0ResearchFacility", position: placeNear(x, y, 2, 2, true, 4), direction: 0x4000*gameRand(4), modules: 1, player: player});
		structures.push({name: "A0PowerGenerator", position: placeNear(x, y, 2, 2, true, 4), direction: 0x4000*gameRand(4), modules: 1, player: player});
		structures.push({name: "A0LightFactory", position: placeNear(x, y, 3, 3, true, 4), direction: 0x4000*gameRand(4), modules: 0, player: player});
		const gr = gameRand(4);
		structures.push({name: "A0CyborgFactory", position: placeNear(x, y, 1 + gr%2, 2 - gr%2, true, 4), direction: 0x4000*gr, modules: 0, player: player});
		structures.push({name: "A0RepairCentre3", position: placeNear(x, y, 1, 1, true, 4), direction: 0x4000*gameRand(4), modules: 0, player: player});
		for (let n = 0; n < 3; ++n) {
			const oilPos = placeNear(x, y, 1, 1, true, 20);
			if (oilPos === null) {  // Unlikely to be null, if so placeNearFailed is true.
				break;
			}
			features.push({name: "OilResource", position: oilPos, direction: 0});
			structures.push({name: "A0ResourceExtractor", position: oilPos, direction: 0x4000*gameRand(4), modules: 0, player: player});
			structures.push({name: "WallTower01", position: placeNear(oilPos[0]/128, oilPos[1]/128, 1, 1, false, 4), direction: 0x4000*gameRand(4), modules: 0, player: player});
		}
		droids.push({name: "ConstructionDroid", position: placeNear(x, y, 1, 1, false, 4), direction: gameRand(0x10000), player: player});
		droids.push({name: "ConstructionDroid", position: placeNear(x, y, 1, 1, false, 4), direction: gameRand(0x10000), player: player});
	}
	if (spaceFinder.checkError()) {
		if (verbose) log('Base placement failed!');
		return null;  // Failed to place something important.
	}
	const featureTypes = ["Tree1", "Tree2", "Tree3", "Tree1", "Tree2", "Tree3", "LogCabin1", "LogCabin2", "WaterTower"];
	for (let i = 0; i < 150; ++i) {
		features.push({name: sample(featureTypes), position: placeNear(gameRand(mapWidth), gameRand(mapHeight), 1, 1, true), direction: gameRand(0x10000)});
	}

	// Skip unimportant features which couldn't be placed anywhere, maybe was trying to place them in water far from land.
	for (let f = 0; f < features.length; ++f) {
		if (features[f].position === null) {
			features[f] = features[features.length - 1];
			--features.length;
			--f;
		}
	}

	return {structures: structures, droids: droids, features: features};
}

let terrain;
let stuff = null;
while (stuff === null) {
	terrain = genTerrain();
	stuff = placeStuff(terrain);
}

// Mark oil resources with craters.
for (let f = 0; f < stuff.features.length; ++f) {
	if (stuff.features[f].name === 'OilResource') {
		const pos = stuff.features[f].position;
		const x = (pos[0] - 64)/128, y = (pos[1] - 64)/128;
		terrain.texture[mapWidth*y + x] = 0x38 | gameRand()&0xf800;
	}
}
for (let f = 0; f < stuff.structures.length; ++f) {
	if (stuff.structures[f].name === 'A0CommandCentre') {
		const pos = stuff.structures[f].position;
		const x = pos[0]/128, y = pos[1]/128;
		const crater = sample([[0x003f, 0x0040, 0x0041, 0x0042], [0x1041, 0x103f, 0x1042, 0x1040], [0x2042, 0x2041, 0x2040, 0x203f], [0x3040, 0x3042, 0x303f, 0x3041]]);
		terrain.texture[mapWidth*(y - 1) + x - 1] = crater[0];
		terrain.texture[mapWidth*(y - 1) + x] = crater[1];
		terrain.texture[mapWidth*y + x - 1] = crater[2];
		terrain.texture[mapWidth*y + x] = crater[3];
	}
}

// Done.
if (verbose) log('Done.');
setMapData(mapWidth, mapHeight, terrain.texture, terrain.height, stuff.structures, stuff.droids, stuff.features);
