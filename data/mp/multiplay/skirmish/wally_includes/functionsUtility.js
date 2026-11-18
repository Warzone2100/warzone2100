// Objects
function playerDataObject(playerIndex) {
    if (playerIndex === undefined) {
        return
    }

    const player = playerData[playerIndex];
    const baseData = enumStruct(playerIndex, PLAYER_HQ_STAT)[0] ?? startPositions[playerIndex]

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


// Return our real power level
function getRealPower(player) {
	if (player === undefined) {
		player = me;
	}

	return (playerPower(player) - queuedPower(player));
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
    if (scavengers) {
        const hasScavengerStructures = countStruct("A0BaBaFactory", scavengerPlayer) +
                                       countStruct(DERRICK_STAT, scavengerPlayer);
        const hasScavengerDroids = countDroid(DROID_ANY, scavengerPlayer);

        if (hasScavengerStructures > 0 || hasScavengerDroids > 0) {
            scavengerIndex = scavengerPlayer;
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
    let scavengerBase = { x: mapWidth / 2, y: mapHeight / 2 }; // Fallback to center of map
    let scavStructs = enumStruct(scavengerIndex);
    
    if (scavStructs.length > 0) {
        scavengerBase = { x: scavStructs[0].x, y: scavStructs[0].y };
    } else {
        let scavDroids = enumDroid(scavengerIndex);
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

	if (playerIndex === scavengerIndex) {
		return (countStruct("A0BaBaFactory", scavengerIndex) + countStruct(DERRICK_STAT, scavengerIndex) + countDroid(DROID_ANY, scavengerIndex)) > 0;
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
	if (obj.player === me || allianceExistsBetween(me, obj.player)) {
		return false;
	}

	if (obj.type === DROID) {
		return true;
	}

	// Any enemy structure (even a derrick, unfinished defense, or scavenger bunker) means enemy presence
	if (obj.type === STRUCTURE) {
		const dangerousStats = [DEFENSE, FACTORY, CYBORG_FACTORY, VTOL_FACTORY, HQ, RESOURCE_EXTRACTOR];
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

// Evaluates if an area at (x, y) is safe from ALL enemy bases, enemy units/structures, and recent ambushes
function isAreaSafe(x, y, range = 18) {
	if (x === undefined || y === undefined) {
		return false;
	}

	// 1. Check if the location was recently the site of an ambush on our trucks/derricks
	if (isInDangerZone(x, y, range)) {
		return false;
	}

	// 2. Check distance against ALL alive enemy bases (not just currentEnemy!)
	const myDistToPoint = distBetweenTwoPoints(BASE.x, BASE.y, x, y);
	for (const enemy of enemiesList) {
		if (!enemy || !enemy.base) continue;
		const enemyDistToPoint = distBetweenTwoPoints(enemy.base.x, enemy.base.y, x, y);

		// Never send trucks within 28 tiles of any enemy base
		if (enemyDistToPoint < 28) {
			return false;
		}

		// If the point is closer to an enemy base than to our base, only consider it safe if our/allied combat units are nearby guarding it
		if (enemyDistToPoint < myDistToPoint && myDistToPoint > BASE_THREAT_RANGE) {
			const localFriendlyGuards = enumRange(x, y, 14, ALLIES, false)
				.concat(enumRange(x, y, 14, me, false))
				.filter(o => o.type === DROID && (o.droidType === DROID_WEAPON || o.droidType === DROID_CYBORG)).length;
			if (localFriendlyGuards < 3) {
				return false;
			}
		}
	}

	// 3. Check if there are any dangerous enemy droids or structures in range
	const hasUnsafeObjects = enumRange(x, y, range, ENEMIES, false).some(isUnsafeEnemyObject);
	if (hasUnsafeObjects) {
		return false;
	}

	// 4. Local combat balance check (compare local enemies against local friendly units, NOT global map units)
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
    let enemyDroidsCount = 0;

    for (let i = 0, l = enemiesStuff.length; i < l; ++i) {
		let obj = enemiesStuff[i];

		if (obj.type === DROID && (obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG) && obj.droidType !== DROID_CONSTRUCT) {
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