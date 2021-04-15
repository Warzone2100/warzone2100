'use strict';

const mapWidth = 128, mapHeight = 128; //map size (works better if it's a square)
const players = 6; //number of players (excl. scavs)
const richness = 3; //how much oil is in the map, must be >=1
const playerPositioningYStrictness = 0; //higher = player positioning more predictable (vertically). Values too high may cause generation to freeze. 0 = completely random
const trucksPerPlayer = 2; //number of starting trucks for each player, must be >=1
const minFieldSize = 10; //size of smallest areas of terrain
const targetFieldSize = 500; //target size of generated areas of terrain
const maxDifference = 192; //max height difference between connectable areas of terrain
const playerPositioningIters = 400; //how many times we should try to find better positions for the players during map generation. Higher values cause longer generation times. Must be >=1
const decorationMode = 1; //0 = no decorations, 1 = decorate everything except near structures, 2 = decorate everything
const decorationDensity = 0.01; //how much decoration we want in the map. Must be >=0
const decorationFreeRange = 5; //when decorationMode is set to 1, this determines the range around structures that should be free of decorations
const minReachable = 0.75; //how much of the map should be reachable by tanks to be considered acceptable
const noCraters = false; //if set to true, oils will not have a crater texture under them, that way you can't know where they are until they're within sensor range

const mapSize = mapWidth*mapHeight;

const verbose = false;


// Divides map into small regions, giving the centre-of-mass of each region and a matrix saying the size of the border between pairs of regions.
function genFields() {
	const fields = [];
	const fieldSizes = [];
	for (let i = 0; i < mapSize; ++i) {
		fields[i] = i;
		fieldSizes[i] = 1;
	}
	function rep(i) {
		let r = i;
		while (fields[r] != r) {
			r = fields[r];
		}
		while (i != r) {
			const j = i;
			i = fields[i];
			fields[j] = r;
		}
		return r;
	}
	function merge(i, j) {
		const ri = rep(i), rj = rep(j);
		if (ri != rj && (fieldSizes[ri] + fieldSizes[rj] < targetFieldSize || fieldSizes[ri] < minFieldSize || fieldSizes[rj] < minFieldSize)) {
			fields[rj] = ri;
			fieldSizes[ri] += fieldSizes[rj];
		}
	}
	const edges = [];
	for (let y = 0; y < mapHeight - 1; ++y) {
		for (let x = 0; x < mapWidth; ++x) {
			edges.push(x << 8 | y);
		}
	}
	for (let y = 0; y < mapHeight; ++y) {
		for (let x = 0; x < mapWidth - 1; ++x) {
			edges.push(0x10000 | x << 8 | y);
		}
	}
	for (let i = edges.length; i > 0; --i) {
		const j = gameRand(i);
		const e = edges[j];
		edges[j] = edges[i - 1];
		const h = !!(e & 0x10000);
		const x = (e >> 8) & 0xff, y = e & 0xff;
		const x2 = h? x + 1 : x, y2 = h? y : y + 1;
		merge(mapWidth*y + x, mapWidth*y2 + x2);
	}

	let regionCount = 0;
	const regions = {};
	const avg = [];
	const simpleFields = [];
	for (let y = 0; y < mapHeight; ++y) {
		for (let x = 0; x < mapWidth; ++x) {
			const i = mapWidth*y + x;
			const ri = rep(i);
			let reg = regions[ri];
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
	const count = avg.length;
	for (let i = 0; i < count; ++i) {
		const sumX = avg[i][0], sumY = avg[i][1], sum = avg[i][2];
		avg[i] = [sumX/sum, sumY/sum, sum];
	}

	// Initialise connectivity matrix between regions with 0.
	const connectivityMatrix = [];
	for (let j = 0; j < count; ++j) {
		const row = [];
		for (let i = 0; i < count; ++i) {
			row.push(0);
		}
		connectivityMatrix.push(row);
	}

	for (let y = 0; y < mapHeight - 1; ++y) {
		for (let x = 0; x < mapWidth; ++x) {
			const i = mapWidth*y + x, j = i + mapWidth;
			const regi = simpleFields[i], regj = simpleFields[j];
			++connectivityMatrix[regi][regj];
			++connectivityMatrix[regj][regi];
		}
	}
	for (let y = 0; y < mapHeight; ++y) {
		for (let x = 0; x < mapWidth - 1; ++x) {
			const i = mapWidth*y + x, j = i + 1;
			const regi = simpleFields[i], regj = simpleFields[j];
			++connectivityMatrix[regi][regj];
			++connectivityMatrix[regj][regi];
		}
	}

	if (verbose) log('Field count = ' + count);

	return {region: simpleFields, count: count, avg: avg, con: connectivityMatrix};
}

//const ttyp = [[0,1,25,34,35,38,74,89],[8,20,27,28,36,49,50,51,52,53,72],[2,3,4,5,6,7,11,21,58,80,81,82,83,84,85,86,87,88],[],[39],[40,43,57],[13,19,22,37,59,60,79],[14,15,16,17,31,32,33],[9,18,29,30,42,44,45,46,54,61,63,68,69,71,76,77,78],[55,56,62],[64,65,70,73,75],[10,12,23,24,26,41,47,48,66,67]];

const TERRAIN0 = [19,22];  // Tiles.
const TERRAIN1 = [40,57,40,57,40,57,40,57,40,57,40, 43];  // Dirt or rocks or something, with occasional white spots.
const TERRAIN2 = [39,39,39,39,39,39,39,39,39,39,39,39,39,39,39, 55];  // Maybe rock or something, with an occasional green spot.
const TERRAIN3 = [2,4,5,7];  // More rocks or dirt or something.
const TERRAIN4 = [3,3,3,3,3,3,3,3,3, 2,21];  // Grass with occasional tiles or dirt.
//const TERRAIN5 = [82];  // White rocks. Broken, apparently sometimes randomly renders as complete black if starting the game a few times! All textures from 80 up seem broken.
const TERRAIN5 = [20,21,20,21, 3];  // Mixture of tiles, dirt and some grass.
const TERRAIN6 = [20,36,49,52,53,20,36,49,52,53,20,36,49,52,53,20,36,49,52,53, 8,27,28,50,51,72];  // Dirt with occasional rocks or tracks or white spots.
const TERRAIN7 = [0,25,0,25,0,25,0,25,0,25,0,25,0,25,0,25, 1,34,74];  // Grass with some dirt and spots.
const TERRAIN8 = [64,65,70,64,65,70,64,65,70,64,65,70,64,65,70, 73,75];  // Snow with occasional grass/dirt showing through.
TERRAIN8.isSnow = true;
const TERRAIN9 = [10,12,23,24,26,41,48,66,10,12,23,24,26,41,48,66, 47,67];  // Light snow on grass/dirt with spots with less/more snow.
TERRAIN9.isSnow = true;
const WATER = [17];
WATER.isWater = true;
const CLIFFFACE = [30];
CLIFFFACE.isCliff = true;
const SNOWCLIFFFACE = [69];
SNOWCLIFFFACE.isCliff = true;
SNOWCLIFFFACE.isSnow = true;
const textureTypes = [TERRAIN0, TERRAIN1, TERRAIN2, TERRAIN3, TERRAIN4, TERRAIN5, TERRAIN6, TERRAIN7, TERRAIN8, TERRAIN9, WATER, WATER];

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
	let tries = 0;
	while (true) {  // eslint-disable-line no-constant-condition
		++tries;

		// Assign regions.
		const regions = [];
		for (let i = 0; i < fields.count; ++i) {
			const textureType = textureTypes[gameRand(textureTypes.length)];
			const height = textureType.isWater? 0 : gameRand(500) + 12;
			regions.push({texture: textureType, height: height, avg: fields.avg[i], reachable: false});
		}

		// Check if most of the map is connected (without needing hover), and without needing narrow passages less than 5 tiles wide.
		const visit = [];
		let next = [];
		function queue(i) {  // eslint-disable-line no-inner-declarations
			if (!visit[i] && !regions[i].texture.isWater) {
				visit[i] = true;
				next.push(i);
				regions[i].reachable = true;
			}
		}
		queue(gameRand(fields.count));
		let reachableArea = 0;
		while (next.length) {
			const cur = next;
			next = [];
			for (let n = 0; n < cur.length; ++n) {
				const i = cur[n];
				const iHeight = regions[i].height;
				reachableArea += regions[i].avg[2];
				for (let j = 0; j < fields.count; ++j) {
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

const fields = genFields();
const regions = genRegions(fields);

const texture = [];
let height = [];

// Assign textures and assign a map height which is smooth except through cliffs.
const smoothDirs = [];  // 1, 2, 4, 8 = right, down, left, up
const isCliffOrWater = [];
const isSnow = [];
for (let y = 0; y < mapHeight; ++y) {
	for (let x = 0; x < mapWidth; ++x) {
		const i00 = mapWidth*y + x, i01 = i00 + (x < mapWidth - 1), i10 = i00 + mapWidth*(y < mapHeight - 1), i11 = i01 + i10 - i00;
		const reg00 = regions[fields.region[i00]], reg01 = regions[fields.region[i01]], reg10 = regions[fields.region[i10]], reg11 = regions[fields.region[i11]];
		const minHeight = Math.min(reg00.height, reg01.height, reg10.height, reg11.height);
		const maxHeight = Math.max(reg00.height, reg01.height, reg10.height, reg11.height);
		if (maxHeight - minHeight > maxDifference) {
			// Corners have very different heights, so make tile a cliff.
			const nearSnow = !!(reg00.texture.isSnow || reg01.texture.isSnow || reg10.texture.isSnow || reg11.texture.isSnow);
			texture[i00] = sample(nearSnow? SNOWCLIFFFACE : CLIFFFACE);
			isCliffOrWater[i00] = true;
			isSnow[i00] = nearSnow;
		} else {
			// Pick randomly from the corners to set the tile texture.
			const reg = sample([reg00, reg01, reg10, reg11])
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
for (let y = 0; y < mapHeight; ++y) {
	smoothDirs[mapWidth*y + mapWidth - 1] &= ~1;
}
for (let x = 0; x < mapWidth; ++x) {
	smoothDirs[mapWidth*(mapHeight - 1) + x] &= ~2;
}
function doSmooth(factor) {
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
	height = newHeight;
}
doSmooth(.125);
doSmooth(.25);
doSmooth(.5);
doSmooth(1);
doSmooth(2);

// Mark tiles occupied, if structures shouldn't be placed on them.
function makeOccupied() {
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
			occupied[i00] = maxHeight - minHeight > 16 || isCliffOrWater[i00]? 2 : 0;
		}
	}
	return occupied;
}

let occupied;

// Find if a structure can be placed at [x, y], and if so, mark it occupied and return the location in world coords.
function placeAt(x, y, w, h, pad) {
	if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
		return null;
	}
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
}

// Find if a structure can be placed somewhere near [x, y], and if so, mark it occupied and return where in world coords.
const visit = [];
let visitC = 0;
let placeNearFailed;
function placeNear(x, y, w, h, pad, scatter) {
	x = Math.round(x - w/2) + w/2;
	y = Math.round(y - h/2) + h/2;
	function consider(x, y) {
		return (!scatter || !gameRand(scatter)) && placeAt(x, y, w, h, pad);
	}
	/*
	// Place nearby.
	let xy;
	if (xy = consider(x, y)) {
		return xy;
	}
	for (let a = 1; a < 100; ++a) {
		for (let bq = 0; bq < 2*a; ++bq) {
			const b = (bq >> 1) ^ -bq;
			if (xy = consider(x + a, y + b) || consider(x + b, y - a) || consider(x - a, y - b) || consider(x - b, y + a)) {
				return xy;
			}
		}
	}*/
	// Place nearby without crossing cliffs or water.
	++visitC;
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
			const xy = consider(cx, cy);
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
	placeNearFailed = true;
	return null;
}

// Pick random starting positions, which aren't too close.
function genStartPos(fields, regions) {
	function randPos(player) {
		let x, y, i;
		do {
			x = gameRand(mapWidth);
			y = playerPositioningYStrictness==0?gameRand(mapHeight):(~~((((player+1)/players)*mapHeight)+gameRand(~~(mapHeight/playerPositioningYStrictness))-(mapHeight/(playerPositioningYStrictness*2))));
			i = mapWidth*y + x;
		} while (occupied[i] || fields.region[i]===undefined || !regions[fields.region[i]].reachable);
		return [x, y];
	}
	const startPos = [];
	for (let player = 0; player < players; ++player) {
		startPos[player] = randPos(player);
	}
	for (let iter = 0; iter < playerPositioningIters; ++iter) {
		const player = gameRand(players);
		const newPos = randPos(player);
		// Based on very rough slightly-asymmetric pathfinding approximation.
		function scoreAt(pos) {  // eslint-disable-line no-inner-declarations
			let next = []
			const visit = [];
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
				const cur = next;
				next = [];
				for (let n = 0; n < cur.length; ++n) {
					const i = cur[n];
					//log('from i = ' + i);
					const iPos = regions[i].avg;
					const iHeight = regions[i].height;
					const oldPos = visit[i][0];
					const dx = iPos[0] - oldPos[0], dy = iPos[1] - oldPos[1];
					const newDist = Math.sqrt(dx*dx + dy*dy) + visit[i][1];
					for (let j = 0; j < fields.count; ++j) {
						if (i !== j && fields.con[i][j] > 0 && Math.abs(iHeight - regions[j].height) <= maxDifference) {
							queue(j, iPos, newDist);
						}
					}
				}
			}
			let score = 0;
			for (let b = 0; b < players; ++b) {
				if (b !== player) {
					const r = fields.region[mapWidth*startPos[b][1] + startPos[b][0]];
					//log('r = ' + r + ', visit[r] = ' + visit[r] + ', regions[r].reachable = ' + regions[r].reachable);
					const nearPos = visit[r][0], nearDist = visit[r][1];
					const tdx = startPos[b][0] - nearPos[0], tdy = startPos[b][1] - nearPos[1];
					const tankDist = Math.sqrt(tdx*tdx + tdy*tdy) + nearDist;
					const vdx = startPos[b][0] - pos[0], vdy = startPos[b][1] - pos[1];
					const vtolDist2 = vdx*vdx + vdy*vdy;
					// Being 10 tiles further away as the tank drives is as important as being 1 tile further away as the VTOL flies.
					score -= 10/(tankDist*tankDist*tankDist*tankDist) + 1/(vtolDist2*vtolDist2);
				}
			}
			return score;
		}
		const newScore = scoreAt(newPos), oldScore = scoreAt(startPos[player]);
		const deltaScore = newScore - oldScore;
		if (deltaScore >= 0) {
			startPos[player] = newPos;
		}
		//if (verbose) log('score[' + player + '] from ' + oldScore + ' → ' + newScore + ': ' + (deltaScore >= 0? 'accept' : 'reject'));
	}
	return startPos;
}

// Place stuff on map, including a base at each starting position.
function placeStuff(regions, startPos) {
	const structures = [];
	const droids = [];
	const features = [];
	for (let q = 0; q < regions.length; ++q) {
		const r = regions[q];
		features.push({name: "OilResource", position: placeNear(r.avg[0], r.avg[1], 1, 1, true), direction: 0});
	}
	placeNearFailed = false;
	for (let player = 0; player < players && !placeNearFailed; ++player) {
		//const q = gameRand(regions.length);
		//const r = regions[q];
		//const x = r.avg[0], y = r.avg[1];
		const x = startPos[player][0], y = startPos[player][1];
		structures.push({name: "A0CommandCentre", position: placeNear(x, y, 2, 2, true), direction: 0x4000*gameRand(4), modules: 0, player: player});
		structures.push({name: "A0LightFactory", position: placeNear(x, y, 3, 3, true, 4), direction: 0x4000*gameRand(4), modules: 1, player: player});
		structures.push({name: "A0ResearchFacility", position: placeNear(x, y, 2, 2, true, 4), direction: 0x4000*gameRand(4), modules: 1, player: player});
		structures.push({name: "A0ResearchFacility", position: placeNear(x, y, 2, 2, true, 4), direction: 0x4000*gameRand(4), modules: 1, player: player});
		structures.push({name: "A0PowerGenerator", position: placeNear(x, y, 2, 2, true, 4), direction: 0x4000*gameRand(4), modules: 1, player: player});
		structures.push({name: "A0LightFactory", position: placeNear(x, y, 3, 3, true, 4), direction: 0x4000*gameRand(4), modules: 0, player: player});
		const gr = gameRand(4);
		structures.push({name: "A0CyborgFactory", position: placeNear(x, y, 1 + gr%2, 2 - gr%2, true, 4), direction: 0x4000*gr, modules: 0, player: player});
		structures.push({name: "A0RepairCentre3", position: placeNear(x, y, 1, 1, true, 4), direction: 0x4000*gameRand(4), modules: 0, player: player});
		for (let n = 0; n < richness; ++n) {
			const oilPos = placeNear(x, y, 1, 1, true, 20);
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
		const featureTypes = ["Tree1", "Tree2", "Tree3", "Tree1", "Tree2", "Tree3", "LogCabin1", "LogCabin2", "WaterTower"];
		const snowFeatureTypes = ["TreeSnow1", "TreeSnow2", "TreeSnow3", "TreeSnow1", "TreeSnow2", "TreeSnow3", "LogCabin1", "LogCabin2", "WaterTower"];
		for (let i = 0; i < mapSize*decorationDensity; ++i) {
			const pos = placeNear(gameRand(mapWidth), gameRand(mapHeight), 1, 1, true);
			if (pos === null) {
				continue;
			}
			const x = pos[0]/128 | 0, y = pos[1]/128 | 0;
			if(decorationMode==1){
				let nearStructure=false;
				for(let j=0;j<structures.length;j++){
					const s=structures[j];
					if(Math.sqrt(Math.pow(x-s.position[0]/128,2)+Math.pow(y-s.position[1]/128,2))<decorationFreeRange){
						nearStructure=true;
						break;
					}
				}
				if(nearStructure) continue;
			}
			const snow = isSnow[mapWidth*y + x];
			features.push({name: sample(snow? snowFeatureTypes : featureTypes), position: pos, direction: gameRand(0x10000)});
		}
	}
	
	// Add additional oils
	for(let i=0;i<60*(richness-3);i++){
		const oilPos = placeNear(gameRand(mapWidth), gameRand(mapHeight), 1, 1, true, 20);
		if (oilPos !== null) {  // Unlikely to be null, if so placeNearFailed is true.
			features.push({name: "OilResource", position: oilPos, direction: 0});
		}
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

let stuff = null;
while (stuff === null) {
	occupied = makeOccupied();
	const startPos = genStartPos(fields, regions);
	stuff = placeStuff(regions, startPos);
}

// Mark oil resources with craters.
if(!noCraters){
	for (let f = 0; f < stuff.features.length; ++f) {
		if (stuff.features[f].name === 'OilResource') {
			const pos = stuff.features[f].position;
			const x = (pos[0] - 64)/128, y = (pos[1] - 64)/128;
			texture[mapWidth*y + x] = 0x38 | gameRand()&0xf800;
		}
	}
}

// Done.
if (verbose) log('Done.');
setMapData(mapWidth, mapHeight, texture, height, stuff.structures, stuff.droids, stuff.features);
