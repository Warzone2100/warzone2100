// Objects
function isScavengerPlayer(playerIndex) {
	return playerIndex !== undefined && (
		(typeof scavengerPlayer !== "undefined" && scavengerPlayer >= 0 && playerIndex === scavengerPlayer) ||
		(scavengerIndex !== undefined && playerIndex === scavengerIndex)
	);
}

function playerDataObject(playerIndex) {
	if (playerIndex === undefined || playerIndex < 0) {
		return undefined;
	}

	if (isScavengerPlayer(playerIndex)) {
		return createScavengerPlayerData(playerIndex);
	}

	const player = playerData[playerIndex];
	if (!player) {
		return undefined;
	}

	const baseData = enumStruct(playerIndex, PLAYER_HQ_STAT)[0] || startPositions[playerIndex] || BASE || { x: Math.floor(mapWidth / 2), y: Math.floor(mapHeight / 2) };

	return {
		"difficulty": player.difficulty,
		"colour": player.colour,
		"position": playerIndex,
		"isAI": player.isAI,
		"isHuman": player.isHuman,
		"name": player.name,
		"team": player.team,
		"base": {
			x: baseData.x,
			y: baseData.y
		}
	};
}

// Functions

function isHoverMap() {
	for (let i = 0; i < maxPlayers; ++i) {
		if (i === me || !startPositions[i]) continue;

		// If wheeled propulsion cannot reach player i's starting position, but hover can, it's a hover/water map
		if (!propulsionCanReach("wheeled01", BASE.x, BASE.y, startPositions[i].x, startPositions[i].y)) {
			if (propulsionCanReach("hover01", BASE.x, BASE.y, startPositions[i].x, startPositions[i].y)) {
				return true;
			}
		}
	}

	return false;
}

// Return a number in the range of 0 to (max - 1).
function random(max) {
	return (Math.random() * max) >>> 0;
}


// Return our real power level (also deducting uncommitted pending build orders of walking trucks)
function getRealPower(player) {
	if (player === undefined) {
		player = me;
	}

	let power = playerPower(player) - queuedPower(player);
	if (player === me && typeof pendingBuildOrders !== "undefined" && pendingBuildOrders instanceof Map) {
		for (const [, info] of pendingBuildOrders.entries()) {
			if (gameTime - info.time <= 25000 &&
				info.stat &&
				!info.stat.startsWith("__") &&
				info.stat !== DERRICK_STAT &&
				info.stat !== "A0PowMod1") {
				const alreadyPlaced = enumStruct(me, info.stat).some(
					s => distBetweenTwoPoints(s.x, s.y, info.x, info.y) <= 2
				);
				if (!alreadyPlaced) {
					power -= (info.stat === POW_GEN_STAT) ? 50 : 100;
				}
			}
		}
	}
	return power;
}

// Sort an object according to the distance from the base
function sortByDistToPlayerBase(obj1, obj2, playerIndex) {
	if (playerIndex === undefined) {
		return 0;
	}

	const ally = alliesList.find(a => a.position === playerIndex);
	const playerBase = ally ? ally.base : startPositions[playerIndex];
	if (!playerBase) return 0;

	let dist1 = distBetweenTwoPoints(playerBase.x, playerBase.y, obj1.x, obj1.y);
	let dist2 = distBetweenTwoPoints(playerBase.x, playerBase.y, obj2.x, obj2.y);
	return (dist1 - dist2);
}

// Sort an object from farthest to closest to the player's base
function sortByDistToFarthestPlayerBase(obj1, obj2, playerIndex) {
	if (playerIndex === undefined) {
		return 0;
	}

	const ally = alliesList.find(a => a.position === playerIndex);
	const playerBase = ally ? ally.base : startPositions[playerIndex];
	if (!playerBase) return 0;

	let dist1 = distBetweenTwoPoints(playerBase.x, playerBase.y, obj1.x, obj1.y);
	let dist2 = distBetweenTwoPoints(playerBase.x, playerBase.y, obj2.x, obj2.y);

	return (dist2 - dist1);
}

// Sort an object according to the distance from the base
function sortByDistToBase(obj1, obj2) {
	let dist1 = distBetweenTwoPoints(BASE.x, BASE.y, obj1.x, obj1.y);
	let dist2 = distBetweenTwoPoints(BASE.x, BASE.y, obj2.x, obj2.y);
	return (dist1 - dist2);
}

// Get all the allies alive players data
function getAliveAllyPlayersData() {
	const allies = [];

	for (let i = 0; i < maxPlayers; ++i) {
		if (i !== me && allianceExistsBetween(me, i) && isPlayerAlive(i)) {
			const allyPlayerData = playerDataObject(i);
			if (allyPlayerData) {
				allies.push(allyPlayerData);
			}
		}
	}

	return allies;
}

// Get all the allies defeated players data
function getDefeatedAllyPlayersData() {
	const allies = [];

	for (let i = 0; i < maxPlayers; ++i) {
		if (i !== me && allianceExistsBetween(me, i) && !isPlayerAlive(i)) {
			const allyPlayerData = playerDataObject(i);
			if (allyPlayerData) {
				allies.push(allyPlayerData);
			}
		}
	}

	return allies;
}

// Return all enemy players that are still alive
function getAliveEnemyPlayersData() {
	const enemies = [];

	for (let i = 0; i < maxPlayers; ++i) {
		if (i !== me && !allianceExistsBetween(me, i) && isPlayerAlive(i)) {
			const enemyPlayerData = playerDataObject(i);
			if (enemyPlayerData) {
				enemies.push(enemyPlayerData);
			}
		}
	}

	// Check for scavenger player
	if (scavengers && typeof scavengerPlayer !== "undefined" && scavengerPlayer >= 0) {
		scavengerIndex = scavengerPlayer;
		const hasScavengerStructures = enumStruct(scavengerPlayer).filter(s => s.stattype !== WALL).length;
		const hasScavengerDroids = countDroid(DROID_ANY, scavengerPlayer);

		if (hasScavengerStructures > 0 || hasScavengerDroids > 0) {
			const scavengerData = createScavengerPlayerData(scavengerPlayer);
			if (scavengerData) {
				enemies.push(scavengerData);
			}
		}
	}

	return enemies;
}

/**
 * Create a minimal PlayerData object for the scavenger player
 * @param {number} scavengerIndex - The index of the scavenger
 * @returns {PlayerData}
 */
function createScavengerPlayerData(scavengerIndex) {
	let scavengerBase = { x: Math.floor(mapWidth / 2), y: Math.floor(mapHeight / 2) }; // Fallback to center of map
	let scavStructs = enumStruct(scavengerIndex).filter(s => s.stattype !== WALL).sort(sortByDistToBase);

	if (scavStructs.length > 0) {
		scavengerBase = { x: scavStructs[0].x, y: scavStructs[0].y };
	} else {
		let scavDroids = enumDroid(scavengerIndex).sort(sortByDistToBase);
		if (scavDroids.length > 0) {
			scavengerBase = { x: scavDroids[0].x, y: scavDroids[0].y };
		}
	}

	return {
		difficulty: null,
		colour: null,
		position: scavengerIndex, // only position index is filled
		isAI: true,
		isHuman: false,
		name: "Scavenger",
		team: -1,
		base: scavengerBase
	};
}


// Check if a player is still alive
function isPlayerAlive(playerIndex) {
	if (playerIndex === undefined) {
		return false;
	}

	if (isScavengerPlayer(playerIndex)) {
		return (enumStruct(playerIndex).filter(s => s.stattype !== WALL).length + countDroid(DROID_ANY, playerIndex)) > 0;
	}

	if (countDroid(DROID_CONSTRUCT, playerIndex) + countStruct(FACTORY_STAT, playerIndex) + countStruct(CYBORG_FACTORY_STAT, playerIndex) > 0) {
		return true;
	}

	return false;
}

// Check if a player is an ally
function friendlyPlayer(player) {
	if (player === undefined) {
		return false;
	}

	return (player === me || allianceExistsBetween(me, player));
}

// Get the player name
function getPlayerName(player) {
	if (player === undefined || !playerData[player]) {
		return "Scavenger";
	}

	return playerData[player].name;
}

// Check if we have a lot of derricks
function checkIsHighOilMap() {
	isHighOilMap = countStruct(DERRICK_STAT, me) >= 30;
}

// Check if an object is a derrick
function isDerrick(obj) {
	return (obj.type === STRUCTURE && obj.stattype === RESOURCE_EXTRACTOR);
}

// Used for deciding if a truck will capture oil or build/recover in an area.
function isUnsafeEnemyObject(obj) {
	if (!obj || obj.player === me || (!isScavengerPlayer(obj.player) && allianceExistsBetween(me, obj.player))) {
		return false;
	}

	if (obj.type === DROID) {
		return true;
	}

	// Any enemy or scavenger structure (including scavenger camps, walls/lookout towers, bunkers, factories, derricks) means enemy presence
	if (obj.type === STRUCTURE) {
		if (isScavengerPlayer(obj.player)) {
			return true;
		}
		const dangerousStats = [
			DEFENSE, FACTORY, CYBORG_FACTORY, VTOL_FACTORY, HQ, RESOURCE_EXTRACTOR,
			POWER_GEN, RESEARCH_LAB, REPAIR_FACILITY, REARM_PAD, LASSAT, SAT_UPLINK, COMMAND_CONTROL
		];
		return dangerousStats.includes(obj.stattype);
	}

	return false;
}

// Records a dangerous zone for 2 minutes when a truck or remote structure is attacked/destroyed
function markDangerZone(x, y, durationMs = 120000) {
	if (x === undefined || y === undefined || isNearBase(x, y)) return;
	// Quantize to 8x8 tile sectors
	const key = `${Math.floor(x / 8)}_${Math.floor(y / 8)}`;
	dangerZones.set(key, gameTime + durationMs);
}

// Checks if (x, y) is near a recently recorded danger zone
function isInDangerZone(x, y, radiusTiles = 16) {
	if (!dangerZones || dangerZones.size === 0) return false;
	for (const [key, expiry] of dangerZones.entries()) {
		if (gameTime > expiry) {
			dangerZones.delete(key);
			continue;
		}
		const [gx, gy] = key.split("_").map(Number);
		const zx = gx * 8 + 4;
		const zy = gy * 8 + 4;
		if (distBetweenTwoPoints(x, y, zx, zy) <= radiusTiles) {
			return true;
		}
	}
	return false;
}

// Evaluates if an oil resource at (x, y) is safe to capture (Cobra & SemperFi strategy, including Scavengers)
function isOilAreaSafe(x, y, range = 7) {
	if (x === undefined || y === undefined) {
		return false;
	}

	if (isInDangerZone(x, y, 10)) {
		return false;
	}

	let nearby = enumRange(x, y, range, ENEMIES, false);
	if (typeof scavengers !== "undefined" && scavengers && typeof scavengerPlayer !== "undefined" && scavengerPlayer >= 0) {
		scavengerIndex = scavengerPlayer;
	}
	if (scavengerIndex !== undefined) {
		nearby = nearby.concat(enumRange(x, y, range, scavengerIndex, false));
	}

	return !nearby.some(obj => {
		if (!obj || obj.player === me || (!isScavengerPlayer(obj.player) && allianceExistsBetween(me, obj.player))) {
			return false;
		}
		if (obj.type === DROID) {
			return true;
		}
		// Only armed enemy/scavenger defenses or factories block oil capture (undefended enemy derricks do NOT block us!)
		return obj.type === STRUCTURE && (
			(obj.stattype === DEFENSE && obj.status === BUILT) ||
			obj.stattype === FACTORY ||
			obj.stattype === CYBORG_FACTORY ||
			obj.stattype === VTOL_FACTORY
		);
	});
}

// Evaluates if an area at (x, y) is safe from ALL enemy bases (including Scavengers), enemy/scavenger units/structures, and recent ambushes
function isAreaSafe(x, y, range = 18) {
	if (x === undefined || y === undefined) {
		return false;
	}

	// 1. Check if the location was recently the site of an ambush on our trucks/derricks
	if (isInDangerZone(x, y, range)) {
		return false;
	}

	// 2. Check distance against ALL alive enemy bases AND all Scavenger factories/bases
	const myDistToPoint = distBetweenTwoPoints(BASE.x, BASE.y, x, y);
	const enemyBasesToCheck = [];
	for (const enemy of enemiesList) {
		if (enemy && enemy.base) {
			enemyBasesToCheck.push(enemy.base);
		}
	}
	if (typeof scavengers !== "undefined" && scavengers && typeof scavengerPlayer !== "undefined" && scavengerPlayer >= 0) {
		scavengerIndex = scavengerPlayer;
	}
	if (scavengerIndex !== undefined) {
		const scavBases = enumStruct(scavengerIndex, FACTORY)
			.concat(enumStruct(scavengerIndex, VTOL_FACTORY));
		for (const sb of scavBases) {
			enemyBasesToCheck.push({ x: sb.x, y: sb.y });
		}
	}

	for (const enemyBase of enemyBasesToCheck) {
		const enemyDistToPoint = distBetweenTwoPoints(enemyBase.x, enemyBase.y, x, y);

		// Never send trucks outside our base within 28 tiles of any enemy or scavenger base
		if (!isNearBase(x, y) && enemyDistToPoint < 28) {
			return false;
		}

		// If the point is closer to an enemy/scavenger base than to our base, only consider it safe if our/allied combat units are nearby guarding it
		if (enemyDistToPoint < myDistToPoint && myDistToPoint > BASE_THREAT_RANGE) {
			const localFriendlyGuards = enumRange(x, y, 14, ALLIES, false)
				.concat(enumRange(x, y, 14, me, false))
				.filter(o => o.type === DROID && (o.droidType === DROID_WEAPON || o.droidType === DROID_CYBORG)).length;
			if (localFriendlyGuards < 3) {
				return false;
			}
		}
	}

	// 3. Check if there are any dangerous enemy or scavenger droids or structures in range
	let nearbyObjects = enumRange(x, y, range, ENEMIES, false);
	if (scavengerIndex !== undefined) {
		nearbyObjects = nearbyObjects.concat(enumRange(x, y, range, scavengerIndex, false));
	}
	if (nearbyObjects.some(isUnsafeEnemyObject)) {
		return false;
	}

	// 4. Local combat balance check (compare local enemies & scavengers against local friendly units)
	return numWeapObjectsInRange(x, y, ENEMIES, me, range, false);
}

function numWeapObjectsInRange(x, y, enemies, allyIndex, scanRadius, visible) {
	let isSafe = true;

	if (x === undefined || y === undefined) {
		return isSafe; // Seems to be the best fail-safe
	}

	if (!scanRadius) {
		scanRadius = 5;
	}

	if (visible === undefined) {
		visible = false;
	}

	// Count only friendly combat units that are actually within defensive range of (x, y)
	const defenseScanRadius = Math.max(scanRadius, BASE_THREAT_RANGE);
	const allyObjects = enumRange(x, y, defenseScanRadius, allyIndex, false);
	let allyDroidsCount = 0;
	for (let i = 0, l = allyObjects.length; i < l; ++i) {
		const o = allyObjects[i];
		if (o.type === DROID && (o.droidType === DROID_WEAPON || o.droidType === DROID_CYBORG)) {
			allyDroidsCount++;
		}
	}

	let enemiesStuff = enumRange(x, y, scanRadius, enemies, visible);
	if (enemies === ENEMIES && scavengerIndex !== undefined) {
		enemiesStuff = enemiesStuff.concat(enumRange(x, y, scanRadius, scavengerIndex, visible));
	}
	let enemyDroidsCount = 0;
	const seenIds = new Set();

	for (let i = 0, l = enemiesStuff.length; i < l; ++i) {
		let obj = enemiesStuff[i];
		if (!obj || seenIds.has(obj.id)) continue;
		seenIds.add(obj.id);

		if (obj.type === DROID &&
			(obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG || obj.droidType === DROID_PERSON) &&
			obj.droidType !== DROID_CONSTRUCT) {
			enemyDroidsCount++;
		} else if (obj.type === STRUCTURE && obj.stattype === DEFENSE && obj.status === BUILT) {
			enemyDroidsCount++;
		}
	}

	if (enemyDroidsCount > allyDroidsCount) {
		isSafe = false;
	}

	return isSafe;
}

function isBaseStructure(type) {
	return [FACTORY, CYBORG_FACTORY, VTOL_FACTORY, POWER_GEN, RESEARCH_LAB, HQ].includes(type);
}

function isNearBase(x, y) {
	return distBetweenTwoPoints(x, y, BASE.x, BASE.y) <= BASE_THREAT_RANGE;
}

// Fisher-Yates shuffle for better randomization
function shuffleArray(array) {
	for (let i = array.length - 1; i > 0; i--) {
		const j = Math.floor(Math.random() * (i + 1));
		[array[i], array[j]] = [array[j], array[i]]; // Swap elements
	}
	return array;
}
