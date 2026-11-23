// ---- Performance helpers ----
// Short-lived cache for expensive lookups, so the same enumeration is not repeated many times within a tick.
// Not a var global on purpose: it is rebuilt from scratch after loading a saved game.
const wallyCache = new Map();
const wallyThrottle = new Map();

// Returns the cached value of `key` if it is at most `ttl` ms old (ttl 0 = this game tick only), otherwise recomputes it
function cached(key, ttl, fn) {
	const hit = wallyCache.get(key);
	if (hit !== undefined && gameTime >= hit.t && gameTime - hit.t <= ttl) {
		return hit.v;
	}
	const value = fn();
	wallyCache.set(key, { t: gameTime, v: value });
	return value;
}

function invalidateCache(key) {
	wallyCache.delete(key);
}

// Returns true if `key` already ran less than `ms` ago (the caller should skip its work), otherwise records this run
function throttled(key, ms) {
	const last = wallyThrottle.get(key);
	if (last !== undefined && gameTime >= last && gameTime - last < ms) {
		return true;
	}
	wallyThrottle.set(key, gameTime);
	return false;
}

function wallyLog(message) {
	if (WALLY_DEBUG) {
		debug("[Wally " + me + " @" + Math.floor(gameTime / 1000) + "s] " + message);
	}
}

// Runs `fn` and, in debug mode, reports it when it takes longer than 8 ms of real time
function profiled(name, fn) {
	if (!WALLY_DEBUG) {
		return fn();
	}
	const start = Date.now();
	const result = fn();
	const elapsed = Date.now() - start;
	if (elapsed >= 8) {
		wallyLog("slow tick: " + name + " took " + elapsed + " ms");
	}
	return result;
}

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

// True for an allied slot that is a real, still-participating player (not an empty slot or a spectator)
function isActiveAllySlot(playerIndex) {
	if (playerIndex === undefined || playerIndex === me || playerIndex < 0 || playerIndex >= maxPlayers) {
		return false;
	}
	const data = playerData[playerIndex];
	if (!data || !(data.isHuman || data.isAI) || !allianceExistsBetween(me, playerIndex)) {
		return false;
	}
	return !(typeof isSpectator === "function" && isSpectator(playerIndex));
}

// An ally needs a truck from us when it has no constructor left and no finished factory able to make one
function allyNeedsTruck(playerIndex) {
	if (!isActiveAllySlot(playerIndex) || countDroid(DROID_CONSTRUCT, playerIndex) > 0) {
		return false;
	}
	const factories = enumStruct(playerIndex, FACTORY_STAT).concat(enumStruct(playerIndex, CYBORG_FACTORY_STAT));
	return !factories.some(f => f.status === BUILT);
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

// Counts every oil resource on the map (free or already owned) and divides it by the number of players
function measureMapOilPerPlayer() {
	let total = enumFeature(ALL_PLAYERS, OIL_RES_STAT).length;
	let players = 0;
	for (let i = 0; i < maxPlayers; ++i) {
		total += countStruct(DERRICK_STAT, i);
		if (playerData[i] && (playerData[i].isHuman || playerData[i].isAI)) {
			players++;
		}
	}
	if (scavengerIndex !== undefined) {
		total += enumStruct(scavengerIndex, DERRICK_STAT).length;
	}
	return Math.floor(total / Math.max(1, players));
}

// Classify the map from the oil available per player (same 20+ rule as Cobra) instead of waiting until we own 30 derricks,
// so high-oil maps such as NTW use the high-oil build order from the first second. Once high, it stays high.
function checkIsHighOilMap() {
	if (mapOilPerPlayer === undefined) {
		mapOilPerPlayer = measureMapOilPerPlayer();
	}
	if (isHighOilMap !== true) {
		isHighOilMap = mapOilPerPlayer >= 20 || countStruct(DERRICK_STAT, me) >= 30;
	}
}

// VTOLs (and therefore anti-air) are only relevant when the VTOL factory is not disabled in the game limits
function checkVtolsEnabled() {
	vtolsEnabled = typeof getStructureLimit !== "function" || getStructureLimit(VTOL_FACTORY_STAT, me) > 0;
	if (!vtolsEnabled) {
		enemiesHaveVTOLs = false;
	}
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

// True for enemy/scavenger objects that can actually hurt a truck: ground combat units and finished ground-capable defenses.
// Enemy trucks, sensors, derricks and other unarmed buildings are deliberately not threats, so contested oil is still claimed.
function isArmedEnemyObject(obj) {
	if (!obj || obj.player === me || (!isScavengerPlayer(obj.player) && allianceExistsBetween(me, obj.player))) {
		return false;
	}
	if (obj.type === DROID) {
		return !obj.isVTOL && (obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG || obj.droidType === DROID_PERSON);
	}
	return obj.type === STRUCTURE && obj.stattype === DEFENSE && obj.status === BUILT && obj.canHitGround !== false;
}

// True for our own or allied objects that can protect a truck: ground combat units and finished ground-capable defenses
function isFriendlyGuardObject(obj) {
	if (!obj) {
		return false;
	}
	if (obj.type === DROID) {
		return !obj.isVTOL && (obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG);
	}
	return obj.type === STRUCTURE && obj.stattype === DEFENSE && obj.status === BUILT && obj.canHitGround !== false;
}

// Returns {threats, guards}: armed enemies versus friendly protectors within `range` tiles of (x, y)
function localThreatBalance(x, y, range) {
	const threats = enumRange(x, y, range, ENEMIES, false).filter(isArmedEnemyObject).length;
	if (threats === 0) {
		return { threats: 0, guards: 0 };
	}
	const guards = enumRange(x, y, range, ALLIES, false).filter(isFriendlyGuardObject).length;
	return { threats: threats, guards: guards };
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

// Evaluates if an oil resource at (x, y) is safe to capture.
// Only armed enemies count (an enemy truck or derrick next to the oil does not stop us), and an oil that our own
// units or towers cover at least as well as the enemy threatens it is still worth taking.
function isOilAreaSafe(x, y, range = 7) {
	if (x === undefined || y === undefined) {
		return false;
	}

	const balance = localThreatBalance(x, y, range);
	if (balance.threats > 0) {
		return balance.guards >= balance.threats;
	}

	// An oil well inside an enemy production area is a trap even when no unit is standing there at this moment
	const insideEnemyBase = enumRange(x, y, range, ENEMIES, false).some(obj =>
		obj.type === STRUCTURE && (obj.stattype === FACTORY || obj.stattype === CYBORG_FACTORY || obj.stattype === VTOL_FACTORY)
	);
	if (insideEnemyBase) {
		return false;
	}

	// Nobody is there right now: stay away only while a recent ambush on this very spot is still fresh
	return !isInDangerZone(x, y, 6);
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

		// Enemy VTOLs are ignored here: ground units cannot fight them, so they must not trigger a ground-army recall
		if (obj.type === DROID && !obj.isVTOL &&
			(obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG || obj.droidType === DROID_PERSON)) {
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
