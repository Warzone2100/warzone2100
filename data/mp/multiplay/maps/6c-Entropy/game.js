'use strict';

var mapWidth = 128, mapHeight = 128; //map size (works better if it's a square)
var players = 6; //number of players (excl. scavs)
var richness = 3; //how much oil is in the map, must be >=1
var playerPositioningYStrictness = 0; //higher = player positioning more predictable (vertically). Values too high may cause generation to freeze. 0 = completely random
var trucksPerPlayer = 2; //number of starting trucks for each player, must be >=1
var minFieldSize = 10; //size of smallest areas of terrain
var targetFieldSize = 500; //target size of generated areas of terrain
var maxDifference = 192; //max height difference between connectable areas of terrain
var playerPositioningIters = 400; //how many times we should try to find better positions for the players during map generation. Higher values cause longer generation times. Must be >=1
var decorationMode = 1; //0 = no decorations, 1 = decorate everything except near structures, 2 = decorate everything
var decorationDensity = 0.01; //how much decoration we want in the map. Must be >=0
var decorationFreeRange = 5; //when decorationMode is set to 1, this determines the range around structures that should be free of decorations
var minReachable = 0.75; //how much of the map should be reachable by tanks to be considered acceptable
var noCraters = false; //if set to true, oils will not have a crater texture under them, that way you can't know where they are until they're within sensor range

var mapSize = mapWidth*mapHeight;

var verbose = false;


// Divides map into small regions, giving the centre-of-mass of each region and a matrix saying the size of the border between pairs of regions.
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

	if (verbose) log('Field count = ' + count);

	return {region: simpleFields, count: count, avg: avg, con: connectivityMatrix};
}

//var ttyp = [[0,1,25,34,35,38,74,89],[8,20,27,28,36,49,50,51,52,53,72],[2,3,4,5,6,7,11,21,58,80,81,82,83,84,85,86,87,88],[],[39],[40,43,57],[13,19,22,37,59,60,79],[14,15,16,17,31,32,33],[9,18,29,30,42,44,45,46,54,61,63,68,69,71,76,77,78],[55,56,62],[64,65,70,73,75],[10,12,23,24,26,41,47,48,66,67]];

var TERRAIN0 = [19,22];  // Tiles.
var TERRAIN1 = [40,57,40,57,40,57,40,57,40,57,40, 43];  // Dirt or rocks or something, with occasional white spots.
var TERRAIN2 = [39,39,39,39,39,39,39,39,39,39,39,39,39,39,39, 55];  // Maybe rock or something, with an occasional green spot.
var TERRAIN3 = [2,4,5,7];  // More rocks or dirt or something.
var TERRAIN4 = [3,3,3,3,3,3,3,3,3, 2,21];  // Grass with occasional tiles or dirt.
//var TERRAIN5 = [82];  // White rocks. Broken, apparently sometimes randomly renders as complete black if starting the game a few times! All textures from 80 up seem broken.
var TERRAIN5 = [20,21,20,21, 3];  // Mixture of tiles, dirt and some grass.
var TERRAIN6 = [20,36,49,52,53,20,36,49,52,53,20,36,49,52,53,20,36,49,52,53, 8,27,28,50,51,72];  // Dirt with occasional rocks or tracks or white spots.
var TERRAIN7 = [0,25,0,25,0,25,0,25,0,25,0,25,0,25,0,25, 1,34,74];  // Grass with some dirt and spots.
var TERRAIN8 = [64,65,70,64,65,70,64,65,70,64,65,70,64,65,70, 73,75];  // Snow with occasional grass/dirt showing through.
TERRAIN8.isSnow = true;
var TERRAIN9 = [10,12,23,24,26,41,48,66,10,12,23,24,26,41,48,66, 47,67];  // Light snow on grass/dirt with spots with less/more snow.
TERRAIN9.isSnow = true;
var WATER = [17];
WATER.isWater = true;
var CLIFFFACE = [30];
CLIFFFACE.isCliff = true;
var SNOWCLIFFFACE = [69];
SNOWCLIFFFACE.isCliff = true;
SNOWCLIFFFACE.isSnow = true;
var textureTypes = [TERRAIN0, TERRAIN1, TERRAIN2, TERRAIN3, TERRAIN4, TERRAIN5, TERRAIN6, TERRAIN7, TERRAIN8, TERRAIN9, WATER, WATER];

// Pick random element of array.
function sample(array) {
	return array[gameRand(array.length)];
}

// Pick randomly rotated texture from array.
function sampleTexture(array) {
	return sample(array) | gameRand()&0xf800;
}

// Assigns each region a texture distribution and a height.
function genRegions(fields) {
	var tries = 0;
	while (true) {  // eslint-disable-line no-constant-condition
		++tries;

		// Assign regions.
		var regions = [];
		for (var i = 0; i < fields.count; ++i) {
			var textureType = textureTypes[gameRand(textureTypes.length)];
			var height = textureType.isWater? 0 : gameRand(500) + 12;
			regions.push({texture: textureType, height: height, avg: fields.avg[i], reachable: false});
		}

		// Check if most of the map is connected (without needing hover), and without needing narrow passages less than 5 tiles wide.
		var visit = [];
		var next = [];
		function queue(i) {  // eslint-disable-line no-inner-declarations
			if (!visit[i] && !regions[i].texture.isWater) {
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
					if (fields.con[i][j] >= 10 && Math.abs(iHeight - regions[j].height) <= maxDifference) {
						queue(j);
						//log('Connect ' + i + ' → ' + j);
					}
				}
			}
		}

		// Accept region configuration, if it makes at least minReachable% of the map reachable, else retry.
		if (reachableArea/mapSize >= minReachable) {
			if (verbose) log('Region assignment tries = ' + tries + ', reachable area = ' + Math.round((reachableArea/mapSize)*100) + '%');
			return regions;
		}
	}
}

var fields = genFields();
var regions = genRegions(fields);

var texture = [];
var height = [];

// Assign textures and assign a map height which is smooth except through cliffs.
var smoothDirs = [];  // 1, 2, 4, 8 = right, down, left, up
var isCliffOrWater = [];
var isSnow = [];
for (var y = 0; y < mapHeight; ++y) {
	for (var x = 0; x < mapWidth; ++x) {
		var i00 = mapWidth*y + x, i01 = i00 + (x < mapWidth - 1), i10 = i00 + mapWidth*(y < mapHeight - 1), i11 = i01 + i10 - i00;
		var reg00 = regions[fields.region[i00]], reg01 = regions[fields.region[i01]], reg10 = regions[fields.region[i10]], reg11 = regions[fields.region[i11]];
		var minHeight = Math.min(reg00.height, reg01.height, reg10.height, reg11.height);
		var maxHeight = Math.max(reg00.height, reg01.height, reg10.height, reg11.height);
		if (maxHeight - minHeight > maxDifference) {
			// Corners have very different heights, so make tile a cliff.
			var nearSnow = !!(reg00.texture.isSnow || reg01.texture.isSnow || reg10.texture.isSnow || reg11.texture.isSnow);
			texture[i00] = sample(nearSnow? SNOWCLIFFFACE : CLIFFFACE);
			isCliffOrWater[i00] = true;
			isSnow[i00] = nearSnow;
		} else {
			// Pick randomly from the corners to set the tile texture.
			var reg = sample([reg00, reg01, reg10, reg11]);
			texture[i00] = sampleTexture(reg.texture);
			smoothDirs[i00] |= 3;
			smoothDirs[i01] |= 6;
			smoothDirs[i10] |= 9;
			smoothDirs[i11] |= 12;
			isCliffOrWater[i00] = !!reg.texture.isWater;
			isSnow[i00] = !!reg.texture.isSnow;
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

// Mark tiles occupied, if structures shouldn't be placed on them.
function makeOccupied() {
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
	return occupied;
}

var occupied;

// Find if a structure can be placed at [x, y], and if so, mark it occupied and return the location in world coords.
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

// Find if a structure can be placed somewhere near [x, y], and if so, mark it occupied and return where in world coords.
var visit = [], visitC = 0;
var placeNearFailed;
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
			var xy = consider(cx, cy);
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
	placeNearFailed = true;
	return null;
}

// Pick random starting positions, which aren't too close.
function genStartPos(fields, regions) {
	function randPos(player) {
		var x, y, i;
		do {
			x = gameRand(mapWidth);
			y = playerPositioningYStrictness==0?gameRand(mapHeight):(~~((((player+1)/players)*mapHeight)+gameRand(~~(mapHeight/playerPositioningYStrictness))-(mapHeight/(playerPositioningYStrictness*2))));
			i = mapWidth*y + x;
		} while (occupied[i] || fields.region[i]===undefined || !regions[fields.region[i]].reachable);
		return [x, y];
	}
	var startPos = [];
	for (var player = 0; player < players; ++player) {
		startPos[player] = randPos(player);
	}
	for (var iter = 0; iter < playerPositioningIters; ++iter) {
		var player = gameRand(players);
		var newPos = randPos(player);
		// Based on very rough slightly-asymmetric pathfinding approximation.
		function scoreAt(pos) {  // eslint-disable-line no-inner-declarations
			var next = [];
			var visit = [];
			function queue(i, pos, dist) {
				if (regions[i].texture === WATER) {
					return;
				}
				if (!visit[i]) {
					//log('visiting i = ' + i);
					next.push(i);
					visit[i] = [pos, dist];
				} else if (visit[i][1] > dist) {
					visit[i] = [pos, dist];
				}
			}
			queue(fields.region[mapWidth*pos[1] + pos[0]], pos, 0);
			while (next.length) {
				var cur = next;
				next = [];
				for (var n = 0; n < cur.length; ++n) {
					var i = cur[n];
					//log('from i = ' + i);
					var iPos = regions[i].avg;
					var iHeight = regions[i].height;
					var oldPos = visit[i][0];
					var dx = iPos[0] - oldPos[0], dy = iPos[1] - oldPos[1];
					var newDist = Math.sqrt(dx*dx + dy*dy) + visit[i][1];
					for (var j = 0; j < fields.count; ++j) {
						if (i !== j && fields.con[i][j] > 0 && Math.abs(iHeight - regions[j].height) <= maxDifference) {
							queue(j, iPos, newDist);
						}
					}
				}
			}
			var score = 0;
			for (var b = 0; b < players; ++b) {
				if (b !== player) {
					var r = fields.region[mapWidth*startPos[b][1] + startPos[b][0]];
					//log('r = ' + r + ', visit[r] = ' + visit[r] + ', regions[r].reachable = ' + regions[r].reachable);
					var nearPos = visit[r][0], nearDist = visit[r][1];
					var tdx = startPos[b][0] - nearPos[0], tdy = startPos[b][1] - nearPos[1];
					var tankDist = Math.sqrt(tdx*tdx + tdy*tdy) + nearDist;
					var vdx = startPos[b][0] - pos[0], vdy = startPos[b][1] - pos[1];
					var vtolDist2 = vdx*vdx + vdy*vdy;
					// Being 10 tiles further away as the tank drives is as important as being 1 tile further away as the VTOL flies.
					score -= 10/(tankDist*tankDist*tankDist*tankDist) + 1/(vtolDist2*vtolDist2);
				}
			}
			return score;
		}
		var newScore = scoreAt(newPos), oldScore = scoreAt(startPos[player]);
		var deltaScore = newScore - oldScore;
		if (deltaScore >= 0) {
			startPos[player] = newPos;
		}
		//if (verbose) log('score[' + player + '] from ' + oldScore + ' → ' + newScore + ': ' + (deltaScore >= 0? 'accept' : 'reject'));
	}
	return startPos;
}

// Place stuff on map, including a base at each starting position.
function placeStuff(regions, startPos) {
	var structures = [];
	var droids = [];
	var features = [];
	for (var q = 0; q < regions.length; ++q) {
		var r = regions[q];
		features.push({name: "OilResource", position: placeNear(r.avg[0], r.avg[1], 1, 1, true), direction: 0});
	}
	placeNearFailed = false;
	for (var player = 0; player < players && !placeNearFailed; ++player) {
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
		for (var n = 0; n < richness; ++n) {
			var oilPos = placeNear(x, y, 1, 1, true, 20);
			if (oilPos === null) {  // Unlikely to be null, if so placeNearFailed is true.
				break;
			}
			features.push({name: "OilResource", position: oilPos, direction: 0});
			structures.push({name: "A0ResourceExtractor", position: oilPos, direction: 0x4000*gameRand(4), modules: 0, player: player});
			structures.push({name: "WallTower01", position: placeNear(oilPos[0]/128, oilPos[1]/128, 1, 1, false, 4), direction: 0x4000*gameRand(4), modules: 0, player: player});
		}
		for(var i=0;i<trucksPerPlayer;i++){
			droids.push({name: "ConstructionDroid", position: placeNear(x, y, 1, 1, false, 4), direction: gameRand(0x10000), player: player});
		}
	}
	if (placeNearFailed) {
		if (verbose) log('Base placement failed!');
		return null;  // Failed to place something important.
	}
	if(decorationMode!=0){
		var featureTypes = ["Tree1", "Tree2", "Tree3", "Tree1", "Tree2", "Tree3", "LogCabin1", "LogCabin2", "WaterTower"];
		var snowFeatureTypes = ["TreeSnow1", "TreeSnow2", "TreeSnow3", "TreeSnow1", "TreeSnow2", "TreeSnow3", "LogCabin1", "LogCabin2", "WaterTower"];
		for (var i = 0; i < mapSize*decorationDensity; ++i) {
			var pos = placeNear(gameRand(mapWidth), gameRand(mapHeight), 1, 1, true);
			if (pos === null) {
				continue;
			}
			var x = pos[0]/128 | 0, y = pos[1]/128 | 0;
			if(decorationMode==1){
				var nearStructure=false;
				for(var j=0;j<structures.length;j++){
					var s=structures[j];
					if(Math.sqrt(Math.pow(x-s.position[0]/128,2)+Math.pow(y-s.position[1]/128,2))<decorationFreeRange){
						nearStructure=true;
						break;
					}
				}
				if(nearStructure) continue;
			}
			var snow = isSnow[mapWidth*y + x];
			features.push({name: sample(snow? snowFeatureTypes : featureTypes), position: pos, direction: gameRand(0x10000)});
		}
	}
	
	// Add additional oils
	for(var i=0;i<60*(richness-3);i++){
		var oilPos = placeNear(gameRand(mapWidth), gameRand(mapHeight), 1, 1, true, 20);
		if (oilPos !== null) {  // Unlikely to be null, if so placeNearFailed is true.
			features.push({name: "OilResource", position: oilPos, direction: 0});
		}
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

var stuff = null;
while (stuff === null) {
	occupied = makeOccupied();
	var startPos = genStartPos(fields, regions);
	stuff = placeStuff(regions, startPos);
}

// Mark oil resources with craters.
if(!noCraters){
	for (var f = 0; f < stuff.features.length; ++f) {
		if (stuff.features[f].name === 'OilResource') {
			var pos = stuff.features[f].position;
			var x = (pos[0] - 64)/128, y = (pos[1] - 64)/128;
			texture[mapWidth*y + x] = 0x38 | gameRand()&0xf800;
		}
	}
}

// Done.
if (verbose) log('Done.');
setMapData(mapWidth, mapHeight, texture, height, stuff.structures, stuff.droids, stuff.features);
