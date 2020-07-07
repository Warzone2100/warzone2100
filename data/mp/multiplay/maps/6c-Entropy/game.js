'use strict';

var mapWidth = 128;
var mapHeight = 128;
var players = 6;
var minFieldSize = 10;
var targetFieldSize = 500;
var maxDifference = 192;

var mapSize = mapWidth*mapHeight;

function genFields() {
	var fields = [];
	var fieldSizes = [];
	for (var i = 0; i < mapSize; ++i) {
		fields[i] = i;
		fieldSizes[i] = 1;
	}
	function rep(i) {
		var r = i;
		while (fields[r] != r) {
			r = fields[r];
		}
		while (i != r) {
			var j = i;
			i = fields[i];
			fields[j] = r;
		}
		return r;
	}
	function merge(i, j) {
		var ri = rep(i), rj = rep(j);
		if (ri != rj && (fieldSizes[ri] + fieldSizes[rj] < targetFieldSize || fieldSizes[ri] < minFieldSize || fieldSizes[rj] < minFieldSize)) {
			fields[rj] = ri;
			fieldSizes[ri] += fieldSizes[rj];
		}
	}
	var edges = [];
	for (var y = 0; y < mapHeight - 1; ++y) {
		for (var x = 0; x < mapWidth; ++x) {
			edges.push(x << 8 | y);
		}
	}
	for (var y = 0; y < mapHeight; ++y) {
		for (var x = 0; x < mapWidth - 1; ++x) {
			edges.push(0x10000 | x << 8 | y);
		}
	}
	for (var i = edges.length; i > 0; --i) {
		var j = gameRand(i);
		var e = edges[j];
		edges[j] = edges[i - 1];
		var h = !!(e & 0x10000);
		var x = (e >> 8) & 0xff, y = e & 0xff;
		var x2 = h? x + 1 : x, y2 = h? y : y + 1;
		merge(mapWidth*y + x, mapWidth*y2 + x2);
	}

	var regionCount = 0;
	var regions = {};
	var avg = [];
	var simpleFields = [];
	for (var y = 0; y < mapHeight; ++y) {
		for (var x = 0; x < mapWidth; ++x) {
			var i = mapWidth*y + x;
			var ri = rep(i);
			var reg = regions[ri];
			if (reg === undefined) {
				reg = regions[ri] = regionCount++;
				avg[reg] = [0, 0, 0];
			}
			simpleFields[i] = reg;
			avg[reg][0] += x;
			avg[reg][1] += y;
			avg[reg][2] += 1;
		}
	}
	var count = avg.length;
	for (var i = 0; i < count; ++i) {
		var sumX = avg[i][0], sumY = avg[i][1], sum = avg[i][2];
		avg[i] = [sumX/sum, sumY/sum, sum];
	}

	// Initialise connectivity matrix between regions with 0.
	var connectivityMatrix = [];
	for (var j = 0; j < count; ++j) {
		var row = [];
		for (var i = 0; i < count; ++i) {
			row.push(0);
		}
		connectivityMatrix.push(row);
	}

	for (var y = 0; y < mapHeight - 1; ++y) {
		for (var x = 0; x < mapWidth; ++x) {
			var i = mapWidth*y + x, j = i + mapWidth;
			var regi = simpleFields[i], regj = simpleFields[j];
			++connectivityMatrix[regi][regj];
			++connectivityMatrix[regj][regi];
		}
	}
	for (var y = 0; y < mapHeight; ++y) {
		for (var x = 0; x < mapWidth - 1; ++x) {
			var i = mapWidth*y + x, j = i + 1;
			var regi = simpleFields[i], regj = simpleFields[j];
			++connectivityMatrix[regi][regj];
			++connectivityMatrix[regj][regi];
		}
	}

	return {region: simpleFields, count: count, avg: avg, con: connectivityMatrix};
}

var SAND = [0x00, 0x01, 0x22, 0x23, 0x26, 0x59];
var SANDYBRUSH = [0x14, 0x1b, 0x1c, 0x24, 0x31, 0x32, 0x33, 0x34, 0x35, 0x48];
var BAKEDEARTH = [0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x0b, 0x15, 0x3a, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58];
var PINKROCK = [0x28, 0x2b, 0x39];
var WATER = [0x0e, 0x0f, 0x10, 0x11, 0x1f, 0x20, 0x21];
var CLIFFFACE = [0x12, 0x1d, 0x1e, 0x36, 0x3d, 0x3f, 0x44, 0x45, 0x47, 0x4c, 0x4d, 0x4e];
var textureTypes = [SAND, SAND, SAND, SAND, SANDYBRUSH, SANDYBRUSH, SANDYBRUSH, SANDYBRUSH, BAKEDEARTH, BAKEDEARTH, BAKEDEARTH, BAKEDEARTH, PINKROCK, PINKROCK, PINKROCK, PINKROCK, WATER];

function sample(array) {
	return array[gameRand(array.length)];
}

var fields = genFields();

function genRegions() {
	while (true) {
		// Assign regions.
		var regions = [];
		for (var i = 0; i < fields.count; ++i) {
			var textureType = textureTypes[gameRand(textureTypes.length)];
			var height = textureType === WATER? 0 : gameRand(500) + 12;
			regions.push({texture: textureType, height: height, avg: fields.avg[i], reachable: false});
		}

		// Check if most of the map is connected (without needing hover), and without needing narrow passages less than 5 tiles wide.
		var visit = [];
		var next = [gameRand(fields.count)];
		function queue(i) {
			if (!visit[i] && regions[i].texture != WATER) {
				visit[i] = true;
				next.push(i);
				regions[i].reachable = true;
			}
		}
		queue(gameRand(fields.count));
		var reachableArea = 0;
		while (next.length) {
			var cur = next;
			next = [];
			for (var n = 0; n < cur.length; ++n) {
				var i = cur[n];
				var iHeight = regions[i].height;
				reachableArea += regions[i].avg[2];
				for (var j = 0; j < fields.count; ++j) {
					if (fields.con[i][j] >= 5 && Math.abs(iHeight - regions[j].height) <= maxDifference) {
						queue(j);
					}
				}
			}
		}

		// Accept region configuration, if it makes at least ¾ of the map reachable, else retry.
		if (4*reachableArea >= mapSize*3) {
			return regions;
		}
	}
}
var regions = genRegions();

var texture = [];
var height = [];
var smoothDirs = [];  // 1, 2, 4, 8 = right, down, left, up
var isCliffOrWater = [];
for (var y = 0; y < mapHeight; ++y) {
	for (var x = 0; x < mapWidth; ++x) {
		var i00 = mapWidth*y + x, i01 = i00 + (x < mapWidth - 1), i10 = i00 + mapWidth*(y < mapHeight - 1), i11 = i01 + i10 - i00;
		var reg00 = regions[fields.region[i00]], reg01 = regions[fields.region[i01]], reg10 = regions[fields.region[i10]], reg11 = regions[fields.region[i11]];
		var minHeight = Math.min(reg00.height, reg01.height, reg10.height, reg11.height);
		var maxHeight = Math.max(reg00.height, reg01.height, reg10.height, reg11.height);
		if (maxHeight - minHeight > maxDifference) {
			// Corners have very different heights, so make tile a cliff.
			texture[i00] = sample(CLIFFFACE);
			isCliffOrWater[i00] = true;
		} else {
			// Pick randomly from the corners to set the tile texture.
			var reg = sample([reg00, reg01, reg10, reg11])
			texture[i00] = sample(reg.texture);
			smoothDirs[i00] |= 3;
			smoothDirs[i01] |= 6;
			smoothDirs[i10] |= 9;
			smoothDirs[i11] |= 12;
			isCliffOrWater[i00] = reg.texture === WATER;
		}
		height[i00] = reg00.height;
	}
}
for (var y = 0; y < mapHeight; ++y) {
	smoothDirs[mapWidth*y + mapWidth - 1] &= ~1;
}
for (var x = 0; x < mapWidth; ++x) {
	smoothDirs[mapWidth*(mapHeight - 1) + x] &= ~2;
}
function doSmooth(factor) {
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
	height = newHeight;
}
doSmooth(.125);
doSmooth(.25);
doSmooth(.5);
doSmooth(1);
doSmooth(2);

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
		occupied[i00] = maxHeight - minHeight > 16 || isCliffOrWater[i00]? 2 : 0;
	}
}

function placeAt(x, y, w, h, pad) {
	if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
		return null;
	}
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
}
var visit = [], visitC = 0;
function placeNear(x, y, w, h, pad, scatter) {
	x = Math.round(x - w/2) + w/2;
	y = Math.round(y - h/2) + h/2;
	function consider(x, y) {
		return (!scatter || !gameRand(scatter)) && placeAt(x, y, w, h, pad);
	}
	/*
	// Place nearby.
	var xy;
	if (xy = consider(x, y)) {
		return xy;
	}
	for (var a = 1; a < 100; ++a) {
		for (var bq = 0; bq < 2*a; ++bq) {
			var b = (bq >> 1) ^ -bq;
			if (xy = consider(x + a, y + b) || consider(x + b, y - a) || consider(x - a, y - b) || consider(x - b, y + a)) {
				return xy;
			}
		}
	}*/
	// Place nearby without crossing cliffs or water.
	++visitC;
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
			var xy;
			if (xy = consider(cx, cy)) {
				return xy;
			}
			var fromCW = isCliffOrWater[mapWidth*(cy | 0) + (cx | 0)];
			queue(cx + 1, cy, fromCW);
			queue(cx - 1, cy, fromCW);
			queue(cx, cy + 1, fromCW);
			queue(cx, cy - 1, fromCW);
		}
	}
	//log('placeNear(' + x + ', ' + y + ', ' + w + ', ' + h + ', ' + pad + ', ' + scatter + ') couldn\'t find spot.');
	return null;
}

function genStartPos() {
	function randPos() {
		var x, y, i;
		do {
			x = gameRand(mapWidth);
			y = gameRand(mapHeight);
			i = mapWidth*y + x;
		} while (occupied[i] || !regions[fields.region[i]].reachable);
		return [x, y];
	}
	var startPos = [];
	for (var player = 0; player < players; ++player) {
		startPos[player] = randPos();
	}
	for (var i = 0; i < 100; ++i) {
		var player = gameRand(players);
		var newPos = randPos();
		var deltaScore = 0;
		for (var b = 0; b < players; ++b) {
			if (b !== player) {
				var dx = startPos[b][0] - startPos[player][0], dy = startPos[b][1] - startPos[player][1];
				var ndx = startPos[b][0] - newPos[0], ndy = startPos[b][1] - newPos[1];
				var dist2 = dx*dx + dy*dy;
				var ndist2 = ndx*ndx + ndy*ndy;
				deltaScore += 1/dist2 - 1/ndist2;
			}
		}
		if (deltaScore >= 0) {
			startPos[player] = newPos;
		}
	}
	return startPos;
}
var startPos = genStartPos();

var structures = [];
var droids = [];
var features = [];
for (var q = 0; q < regions.length; ++q) {
	var r = regions[q];
	features.push({name: "OilResource", position: placeNear(r.avg[0], r.avg[1], 1, 1, true), direction: 0});
}
for (var player = 0; player < players; ++player) {
	//var q = gameRand(regions.length);
	//var r = regions[q];
	//var x = r.avg[0], y = r.avg[1];
	var x = startPos[player][0], y = startPos[player][1];
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
		features.push({name: "OilResource", position: oilPos, direction: 0});
		structures.push({name: "A0ResourceExtractor", position: oilPos, direction: 0x4000*gameRand(4), modules: 0, player: player});
		structures.push({name: "WallTower01", position: placeNear(oilPos[0]/128, oilPos[1]/128, 1, 1, false, 4), direction: 0x4000*gameRand(4), modules: 0, player: player});
	}
	droids.push({name: "ConstructionDroid", position: placeNear(x, y, 1, 1, false, 4), direction: gameRand(0x10000), player: player});
	droids.push({name: "ConstructionDroid", position: placeNear(x, y, 1, 1, false, 4), direction: gameRand(0x10000), player: player});
}
var featureTypes = ["Tree1", "Tree2", "Tree3", "TreeSnow1", "TreeSnow2", "TreeSnow3", "LogCabin1", "LogCabin2", "WaterTower"];
for (var i = 0; i < 150; ++i) {
	features.push({name: sample(featureTypes), position: placeNear(gameRand(mapWidth), gameRand(mapHeight), 1, 1, true), direction: gameRand(0x10000)});
}

// Skip features which couldn't be placed anywhere, maybe was trying to place them in water far from land.
for (var f = 0; f < features.length; ++f) {
	if (features[f].position === null) {
		features[f] = features[features.length - 1];
		--features.length;
		--f;
	}
}

setMapData(mapWidth, mapHeight, texture, height, structures, droids, features);
