'use strict';

var mapWidth = 192;
var mapHeight = 192;
var players = 10;
var minFieldSize = 10;
var targetFieldSize = 500;
var maxDifference = 192;

var mapSize = mapWidth*mapHeight;

var verbose = false;

// Maximum and average return values of gameRand().
var RMAX = 0xffffffff;
var RAVG = RMAX/2;


// Random vector with max length maxR.
function randVec(maxR) {
	var s = maxR/RAVG;
	while (true) {  // eslint-disable-line no-constant-condition
		var x = s*(gameRand() - RAVG);
		var y = s*(gameRand() - RAVG);
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
	var dx = b[0] - a[0], dy = b[1] - a[1];
	var mx = a[0] + b[0], my = a[1] + b[1];
	var s = normalRand()/8;
	return [mx/2 - s*dy, my/2 + s*dx];
}

// Generates a slightly fractal loop of points.
function genLoop(subdivLevel) {
	var s = 0.5877852522924731;  // Math.sin(2*Math.PI / players)
	var c = 0.8090169943749475;  // Math.cos(2*Math.PI / players)
	var xy = [-0.9510565162951535, 0.309016994374947];  // [Math.cos(2*Math.PI / players / 2), Math.sin(2*Math.PI / players / 2)]
	var p = [];
	for (var i = 0; i < players; ++i) {
		var r = .4 + .5/RMAX*gameRand();
		p.push([mapWidth*(1 + r*xy[0])/2 | 0, mapHeight*(1 - r*xy[1])/2 | 0]);
		xy = [c*xy[0] + s*xy[1], -s*xy[0] + c*xy[1]];
	}
	for (var subdiv = 0; subdiv < subdivLevel; ++subdiv) {
		var q = [];
		for (var i = 0; i < p.length; ++i) {
			var j = (i + 1)%p.length;
			q.push(p[i]);
			q.push(approxMidpoint(p[i], p[j]));
		}
		p = q;
	}
	return p;
}

// Generates some random points.
function genIslands(count) {
	var p = [];
	for (var i = 0; i < count; ++i) {
		p.push([3 + (mapWidth - 6)/RMAX*gameRand(), 3 + (mapHeight - 6)/RMAX*gameRand()]);
	}
	return p;
}

// Class for drawing circles.
function LayerMap() {
	var edges = this.edges = [];
	edges.length = mapSize;
	var img = this.img = [];
	img.length = mapSize;
	this.base = 0;
	for (var i = 0; i < mapSize; ++i) {
		edges[i] = 0;
		img[i] = 0;
	}
}
LayerMap.prototype = {
	drawCircle: function (x, y, r, plane) {  // 0 ≤ plane - base < 4
		var edges = this.edges;
		var v = 1 << 8*(plane - this.base);
		var yMin = Math.max(Math.ceil(y - r), 0), yMax = Math.min(Math.floor(y + r), mapHeight - 1);
		for (var iy = yMin; iy <= yMax; ++iy) {
			var dy = y - iy;
			var dx = Math.sqrt(r*r - dy*dy);
			var xMin = Math.max(Math.ceil(x - dx), 0), xMax = Math.min(Math.floor(x + dx), mapWidth - 1);
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
		var edges = this.edges;
		for (var y = 0; y < mapHeight; ++y) {
			var c = 0;
			for (var x = 0; x < mapWidth; ++x) {
				var i = mapWidth*y + x;
				c += edges[i];
				if (c > 0) {
					this.img[i] = Math.max(this.img[i], this.base + (c >= 0x100) + (c >= 0x10000) + (c >= 0x1000000));
				}
			}
		}
		for (var i = 0; i < mapSize; ++i) {
			edges[i] = 0;
		}
	}
};

//var ttyp = [[1,4,12,42,89,90,91,92,93,94,95],[0,9,10,11,28,40],[2,3,5,6,7,8,26,36,37,38,39,47,78,79,80,81,82,83,84,85,86,87,88],[23,24,25],[20,21,27,29,34,35,41,43,44,48,49,50,51,52,53,54,60,61,72,73,74],[76],[19,22,57,59,77],[13,14,15,16,17,30,31,32,33],[18,45,46,71,75],[55,56,58,62,63,64,65,66,67,68,69,70],[],[]];

var TILED = [19,22,77];
var RAMP = [20,29];
var HIGHISLAND = [];
var ISLAND = [];
var WATER = [17];
var CLIFFFACE = [18,45,46,71];

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
	var newHeight = [];
	var bits = [0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4];
	for (var y = 0; y < mapHeight; ++y) {
		for (var x = 0; x < mapWidth; ++x) {
			var i = mapWidth*y + x;
			var s = smoothDirs[i];
			var n = ((s & 1) && height[i + 1]) + ((s & 2) && height[i + mapWidth]) + ((s & 4) && height[i - 1]) + ((s & 8) && height[i - mapWidth]);
			var d = bits[s];
			newHeight[i] = (factor*height[i] + n)/(factor + d);
		}
	}
	return newHeight;
}

// Generates terrain and heightmap, including locations.
function genTerrain() {
	var subdivLevel = 6;
	var M = 1 << subdivLevel;
	var loop = genLoop(subdivLevel);
	var islands = genIslands(40);
	var smallIslands = genIslands(40);
	var layers = new LayerMap();
	var startPos = [];
	layers.begin(1);
	for (var i = 0; i < loop.length; ++i) {
		layers.drawCircle(loop[i][0], loop[i][1], i%M < M/8 || i%M >= M - M/8? 8 : 3, 4);
		if (i%M === M/2) {
			layers.drawCircle(loop[i][0], loop[i][1], 6, 3);
		}
		if (i%M === 0) {
			startPos.push(loop[i]);
		}
	}
	for (var i = 0; i < islands.length; ++i) {
		for (var j = gameRand(6); j < 6; ++j) {
			var p = addVec(islands[i], randVec(2));
			var r = 2 + 6/RMAX*gameRand();
			layers.drawCircle(p[0], p[1], r, 1 + gameRand(2));
		}
	}
	for (var i = 0; i < smallIslands.length; ++i) {
		var p = smallIslands[i];
		var r = 2 + 1/RMAX*gameRand();
		layers.drawCircle(p[0], p[1], r, i%4 === 0? 2 : 1);
	}
	layers.end();

	var texture = [];  // Texture of tiles.
	texture.length = mapSize;
	var height = [];  // Height of upper-left corner of tiles.
	height.length = mapSize;
	var smoothDirs = [];  // 1, 2, 4, 8 = right, down, left, up, applies to corners of tiles.
	smoothDirs.length = mapSize;
	for (var i = 0; i < mapSize; ++i) {
		smoothDirs[i] = 0;
	}
	var isCliffOrWater = [];  // Whether texture of tile is cliff/water.
	isCliffOrWater.length = mapSize;
	// 0 = water, 1 = land near water, 2 = hills which form cliffs next to water, 3 = land outside base, 4 = land for bases, 5 = cliffs
	var planeTexture = [WATER, ISLAND, HIGHISLAND, RAMP, TILED, CLIFFFACE];
	var planeHeight = [0, 2, 192, 128, 384];
	for (var y = 0; y < mapHeight; ++y) {
		for (var x = 0; x < mapWidth; ++x) {
			var i00 = mapWidth*y + x, i01 = i00 + (x < mapWidth - 1), i10 = i00 + mapWidth*(y < mapHeight - 1), i11 = i01 + i10 - i00;
			var plane00 = layers.img[i00];
			var plane01 = layers.img[i01];
			var plane10 = layers.img[i10];
			var plane11 = layers.img[i11];
			var planeMin = Math.min(plane00, plane01, plane10, plane11);
			var planeMax = Math.max(plane00, plane01, plane10, plane11);
			//   0 1 2 3 4
			// 0 .
			// 1 . .
			// 2 + . .
			// 3 . . . .
			// 4 + + + + .
			var plane = (planeMax === 4 && planeMin < 3) || (planeMax === 2 && planeMin === 0)? 5 : planeMax;
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
	var height = terrain.height;
	var isCliffOrWater = terrain.isCliffOrWater;

	this.isCliffOrWater = isCliffOrWater;
	this.visit = [];
	this.visit.length = mapSize;
	for (var i = 0; i < mapSize; ++i) {
		this.visit[i] = 0;
	}
	this.visitC = 0;
	this.placeNearFailed = false;

	// Mark tiles occupied, if structures shouldn't be placed on them.
	var occupied = [];
	for (var y = 0; y < mapHeight; ++y) {
		for (var x = 0; x < mapWidth; ++x) {
			var i00 = mapWidth*y + x, i01 = i00 + 1, i10 = i00 + mapWidth, i11 = i01 + i10 - i00;
			if (x < 3 || x >= mapWidth - 3 || y < 3 || y >= mapWidth - 3) {
				occupied[i00] = 2;
				continue;
			}
			var minHeight = Math.min(height[i00], height[i01], height[i10], height[i11]);
			var maxHeight = Math.max(height[i00], height[i01], height[i10], height[i11]);
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
		var occupied = this.occupied;
		for (var t = y - h/2; t < y + h/2; ++t) {
			for (var s = x - w/2; s < x + w/2; ++s) {
				if (occupied[mapWidth*t + s]) {
					return null;
				}
			}
		}
		if (pad) {
			for (var t = y - h/2; t < y + h/2; ++t) {
				if (occupied[mapWidth*t + x - w/2 - 1] >= 2 || occupied[mapWidth*t + x + w/2] >= 2) {
					return null;
				}
			}
			for (var s = x - w/2 - 1; s < x + w/2 + 1; ++s) {
				if (occupied[mapWidth*(y - h/2 - 1) + s] >= 2 || occupied[mapWidth*(y + h/2) + s] >= 2) {
					return null;
				}
			}
		}
		for (var t = y - h/2; t < y + h/2; ++t) {
			for (var s = x - w/2; s < x + w/2; ++s) {
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
		var isCliffOrWater = this.isCliffOrWater;
		x = Math.round(x - w/2) + w/2;
		y = Math.round(y - h/2) + h/2;
		var visit = this.visit;
		var visitC = ++this.visitC;
		function queue(x, y, fromCW) {
			var i = mapWidth*(y | 0) + (x | 0);
			if (visit[i] !== visitC && x > 0 && x < mapWidth && y > 0 && y < mapHeight && (!isCliffOrWater[i] || fromCW)) {
				next.push([x, y]);
				visit[i] = visitC;
			}
		}
		var next = [];
		queue(x, y, true);
		while (next.length) {
			var cur = next;
			next = [];
			for (var i = 0; i < cur.length; ++i) {
				var cx = cur[i][0], cy = cur[i][1];
				var xy = this.consider(cx, cy, w, h, pad, scatter);
				if (xy) {
					return xy;
				}
				var fromCW = isCliffOrWater[mapWidth*(cy | 0) + (cx | 0)];
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
	var spaceFinder = new SpaceFinder(terrain);
	//var placeNear = spaceFinder.placeNear.bind(spaceFinder);
	var placeNear = function (x, y, w, h, pad, scatter) { return spaceFinder.placeNear(x, y, w, h, pad, scatter); };  // QtScript apparently missing even the basics such as `bind`.

	var structures = [];
	var droids = [];
	var features = [];
	for (var q = 0; q < terrain.loop.length; q += 16) {
		var r = terrain.loop[q];
		features.push({name: "OilResource", position: placeNear(r[0], r[1], 1, 1, true), direction: 0});
	}
	for (var q = 0; q < terrain.smallIslands.length; ++q) {
		var r = terrain.smallIslands[q];
		features.push({name: "OilResource", position: placeNear(r[0], r[1], 1, 1, true), direction: 0});
	}
	for (var q = 0; q < terrain.islands.length; ++q) {
		var r = terrain.islands[q];
		for (var i = gameRand(4); i < 4; ++i) {
			features.push({name: "OilResource", position: placeNear(r[0], r[1], 1, 1, true, 3), direction: 0});
		}
	}
	spaceFinder.resetError();
	for (var player = 0; player < players && !spaceFinder.checkError(); ++player) {
		var x = terrain.startPos[player][0], y = terrain.startPos[player][1];
		structures.push({name: "A0CommandCentre", position: placeNear(x, y, 2, 2, true), direction: 0x4000*gameRand(4), modules: 0, player: player});
		structures.push({name: "A0LightFactory", position: placeNear(x, y, 3, 3, true, 4), direction: 0x4000*gameRand(4), modules: 1, player: player});
		structures.push({name: "A0ResearchFacility", position: placeNear(x, y, 2, 2, true, 4), direction: 0x4000*gameRand(4), modules: 1, player: player});
		structures.push({name: "A0ResearchFacility", position: placeNear(x, y, 2, 2, true, 4), direction: 0x4000*gameRand(4), modules: 1, player: player});
		structures.push({name: "A0PowerGenerator", position: placeNear(x, y, 2, 2, true, 4), direction: 0x4000*gameRand(4), modules: 1, player: player});
		structures.push({name: "A0LightFactory", position: placeNear(x, y, 3, 3, true, 4), direction: 0x4000*gameRand(4), modules: 0, player: player});
		var gr = gameRand(4);
		structures.push({name: "A0CyborgFactory", position: placeNear(x, y, 1 + gr%2, 2 - gr%2, true, 4), direction: 0x4000*gr, modules: 0, player: player});
		structures.push({name: "A0RepairCentre3", position: placeNear(x, y, 1, 1, true, 4), direction: 0x4000*gameRand(4), modules: 0, player: player});
		for (var n = 0; n < 3; ++n) {
			var oilPos = placeNear(x, y, 1, 1, true, 20);
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
	var featureTypes = ["Tree1", "Tree2", "Tree3", "Tree1", "Tree2", "Tree3", "LogCabin1", "LogCabin2", "WaterTower"];
	for (var i = 0; i < 150; ++i) {
		features.push({name: sample(featureTypes), position: placeNear(gameRand(mapWidth), gameRand(mapHeight), 1, 1, true), direction: gameRand(0x10000)});
	}

	// Skip unimportant features which couldn't be placed anywhere, maybe was trying to place them in water far from land.
	for (var f = 0; f < features.length; ++f) {
		if (features[f].position === null) {
			features[f] = features[features.length - 1];
			--features.length;
			--f;
		}
	}

	return {structures: structures, droids: droids, features: features};
}

var terrain;
var stuff = null;
while (stuff === null) {
	terrain = genTerrain();
	stuff = placeStuff(terrain);
}

// Mark oil resources with craters.
for (var f = 0; f < stuff.features.length; ++f) {
	if (stuff.features[f].name === 'OilResource') {
		var pos = stuff.features[f].position;
		var x = (pos[0] - 64)/128, y = (pos[1] - 64)/128;
		terrain.texture[mapWidth*y + x] = 0x38 | gameRand()&0xf800;
	}
}
for (var f = 0; f < stuff.structures.length; ++f) {
	if (stuff.structures[f].name === 'A0CommandCentre') {
		var pos = stuff.structures[f].position;
		var x = pos[0]/128, y = pos[1]/128;
		var crater = sample([[0x003f, 0x0040, 0x0041, 0x0042], [0x1041, 0x103f, 0x1042, 0x1040], [0x2042, 0x2041, 0x2040, 0x203f], [0x3040, 0x3042, 0x303f, 0x3041]]);
		terrain.texture[mapWidth*(y - 1) + x - 1] = crater[0];
		terrain.texture[mapWidth*(y - 1) + x] = crater[1];
		terrain.texture[mapWidth*y + x - 1] = crater[2];
		terrain.texture[mapWidth*y + x] = crater[3];
	}
}

// Done.
if (verbose) log('Done.');
setMapData(mapWidth, mapHeight, terrain.texture, terrain.height, stuff.structures, stuff.droids, stuff.features);
