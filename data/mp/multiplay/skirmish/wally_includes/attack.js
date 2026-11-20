var currentEnemyTarget; // Current enemy target Wally is attacking.
var useBunkerBuster = false;

function isDroidRepairing(droid) {
	if (droid.order === DORDER_RTR) return true;
	if (typeof DORDER_RTR_SPECIFIED !== "undefined" && droid.order === DORDER_RTR_SPECIFIED) return true;
	return false;
}

function attack() {
	const attackerDroids = enumGroup(attackerGroup).filter(
		attacker => !isDroidRepairing(attacker) && attacker.order !== DORDER_RECOVER
	);

	let MIN_GROUP_SIZE = isMyBaseInTrouble ? 1 : isHighOilMap ? 25 : 10;

	if (attackerDroids.length < MIN_GROUP_SIZE && !isMyBaseInTrouble) {
		retreatToBase(attackerDroids);
	} else if (attackerDroids.length > 0) {
		attackEnemy(attackerDroids);
		attackEnemyDefenses(attackerDroids);
	}

	// Manage VTOL strikes and rearming independently
	manageVTOLs();

	if (isHelpingAlly && currentHelpedAlly) {
		const helperDroids = enumGroup(helperGroup).filter(
			helper => !isDroidRepairing(helper)
		);
		if (helperDroids.length > 0) {
			helpAttackEnemy(helperDroids, currentHelpedAlly.base);
		}
	}
}

// Scores enemy ground units by relevance for VTOL bombers:
// 1 = Mobile Anti-Air / Anti-VTOL tanks (highest threat to our VTOLs!)
// 2 = Long-range Artillery / Howitzers / Sensors / Commanders (high-value backline targets)
// 3 = Main Battle Tanks & Cyborgs (direct frontline support)
// 4 = Constructor Trucks (economy & building denial)
function getVTOLUnitPriority(droid) {
	if (droid.droidType === DROID_CONSTRUCT) {
		return 4;
	}
	if (droid.canHitAir === true) {
		return 1; // Mobile AA tanks or multi-role anti-air units first!
	}
	const weapName = (droid.weapons && droid.weapons[0] && droid.weapons[0].name) ? droid.weapons[0].name : "";
	if (droid.droidType === DROID_SENSOR ||
		droid.droidType === DROID_COMMAND ||
		HOWITZERS_WEAPON_LIST.includes(weapName) ||
		weapName.includes("Mortar") ||
		weapName.includes("Howitzer")) {
		return 2; // Long-range artillery & sensors second!
	}
	return 3; // Standard combat tanks & cyborgs third!
}

function manageVTOLs() {
	const vtols = enumGroup(vtolGroup);
	if (vtols.length === 0) return;

	const readyVTOLs = [];
	const hasPads = countStruct(VTOL_PAD_STAT) > 0;

	for (const vtol of vtols) {
		const vtolWeapArmed = (vtol.weapons && vtol.weapons[0] && vtol.weapons[0].armed !== undefined) ? vtol.weapons[0].armed : undefined;
		const armed = vtolWeapArmed !== undefined ? vtolWeapArmed : (vtol.armed !== undefined ? vtol.armed : 100);
		const needsRearm = armed < 20 || vtol.health < 60;

		if (needsRearm) {
			if (hasPads && vtol.order !== DORDER_REARM) {
				orderDroid(vtol, DORDER_REARM);
			} else if (!hasPads && vtol.order !== DORDER_RTB) {
				orderDroid(vtol, DORDER_RTB);
			}
		} else if (vtol.order === DORDER_REARM && (armed < 100 || vtol.health < 100)) {
			// Let it finish rearming and repairing on the pad
			continue;
		} else {
			readyVTOLs.push(vtol);
		}
	}

	const MIN_VTOL_STRIKE_SIZE = isMyBaseInTrouble ? 1 : 4;
	if (readyVTOLs.length < MIN_VTOL_STRIKE_SIZE) {
		return;
	}

	let target = null;

	if (currentEnemy && currentEnemy.position !== undefined) {
		const enemyIdx = currentEnemy.position;

		// Gather all ground enemy tanks, cyborgs, sensors, and trucks (strictly excluding enemy VTOLs!)
		const enemyGroundUnits = enumDroid(enemyIdx, DROID_ANY, false)
			.filter(d => !d.isVTOL && (
				d.droidType === DROID_WEAPON ||
				d.droidType === DROID_CYBORG ||
				d.droidType === DROID_SENSOR ||
				d.droidType === DROID_COMMAND ||
				d.droidType === DROID_CONSTRUCT
			));

		// Sort by VTOL relevance priority first (1: AA tanks -> 2: Artillery/Sensors -> 3: Combat Tanks/Cyborgs -> 4: Trucks),
		// and then by closest distance to our base
		enemyGroundUnits.sort((a, b) => {
			const prioDiff = getVTOLUnitPriority(a) - getVTOLUnitPriority(b);
			if (prioDiff !== 0) return prioDiff;
			return distBetweenTwoPoints(BASE.x, BASE.y, a.x, a.y) - distBetweenTwoPoints(BASE.x, BASE.y, b.x, b.y);
		});

		// Prefer a high-priority ground unit that isn't sitting inside heavy static AA defenses
		const safeUnitTarget = enemyGroundUnits.find(u =>
			enumRange(u.x, u.y, 9, ENEMIES, true).filter(o => o.type === STRUCTURE && o.canHitAir === true).length <= 2
		);

		target = safeUnitTarget || enemyGroundUnits[0] || null;

		// Fallback to enemy structures if the enemy has no ground units left
		if (!target) {
			const enemyFac = findNearestFactory(enemyIdx);
			const enemyDerr = findNearestDerrick(enemyIdx);
			const fallbackTarget = (currentEnemyTarget && !currentEnemyTarget.isVTOL) ? currentEnemyTarget : null;
			const structCandidates = [enemyFac, enemyDerr, fallbackTarget].filter(Boolean);
			target = structCandidates[0] || null;
		}
	}

	if (!target || target.isVTOL) return;

	const realTarget = getObject(target.type, target.player, target.id);
	if (!realTarget) return;

	for (const vtol of readyVTOLs) {
		orderDroidObj(vtol, DORDER_ATTACK, realTarget);
	}
}

function helpAttackEnemy(helperDroids, allyBasePos) {
	// Find visible enemy ground combat units near the ally's base (ignore enemy VTOLs)
	const nearbyEnemies = enumRange(allyBasePos.x, allyBasePos.y, BASE_THREAT_RANGE * 2, ENEMIES, true)
		.filter(obj => obj.type === DROID && !obj.isVTOL && (obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG));

	// Sort enemies by distance to ally base (closest threats first)
	nearbyEnemies.sort((a, b) =>
		distBetweenTwoPoints(a.x, a.y, allyBasePos.x, allyBasePos.y) -
		distBetweenTwoPoints(b.x, b.y, allyBasePos.x, allyBasePos.y)
	);

	for (const droid of helperDroids) {
		if (droidNeedsRepair(droid)) continue;

		const distToAlly = distBetweenTwoPoints(droid.x, droid.y, allyBasePos.x, allyBasePos.y);

		if (nearbyEnemies.length > 0 && distToAlly < BASE_THREAT_RANGE * 2) {
			// Close enough to fight — attack the closest enemy to the ally base
			orderDroidObj(droid, DORDER_ATTACK, nearbyEnemies[0]);
		} else {
			// Not there yet or no visible enemies — move toward the ally's base
			if (droid.order !== DORDER_MOVE && droid.order !== DORDER_ATTACK) {
				orderDroidLoc(droid, DORDER_MOVE, allyBasePos.x, allyBasePos.y);
			}
		}
	}
}

function attackEnemy(attackerDroids) {
	if (!currentEnemy || currentEnemy.position === undefined) {
		setCurrentEnemy();
		return;
	}

	const enemyIndex = currentEnemy.position;

	// If the current enemy is dead, choose another one
	if (!isPlayerAlive(enemyIndex)) {
		setCurrentEnemy();
		return;
	}

	let sortFunc = sortByDistToBase;

	// Ground combat units (strictly exclude enemy VTOLs so ground tanks never chase aircraft!)
	const enemyDroid = enumDroid(enemyIndex, DROID_WEAPON, false)
		.concat(enumDroid(enemyIndex, DROID_CYBORG, false))
		.filter(d => !d.isVTOL)
		.sort(sortFunc)[0];

	const enemyTruck = enumDroid(enemyIndex, DROID_CONSTRUCT, false)
		.filter(d => !d.isVTOL)
		.sort(sortByDistToBase)[0];

	const closestDefense = enumStruct(enemyIndex, DEFENSE)
		.filter(def => def.stattype !== WALL) // Ignore walls
		.sort(sortByDistToBase)[0];

	const enemyDerrick = findNearestDerrick(enemyIndex);

	// Get closest among droid, truck, derrick — prefer reachable targets
	const basePos = BASE;
	const candidates = [enemyDroid, enemyTruck, enemyDerrick, closestDefense].filter(Boolean);

	// Use a representative droid to check reachability
	const refDroid = attackerDroids[0];
	const reachableCandidates = refDroid
		? candidates.filter(c => droidCanReach(refDroid, c.x, c.y))
		: candidates;

	// Prefer reachable targets, fall back to all candidates if none are reachable
	const pool = reachableCandidates.length > 0 ? reachableCandidates : candidates;

	currentEnemyTarget = pool.length > 0
		? pool.sort((a, b) =>
			distBetweenTwoPoints(basePos.x, basePos.y, a.x, a.y) -
			distBetweenTwoPoints(basePos.x, basePos.y, b.x, b.y)
		)[0]
		: null;

	// Fallback if no droid/truck/derrick found
	if (!currentEnemyTarget) {
		const enemyFactory = findNearestFactory(enemyIndex);
		if (enemyFactory) {
			currentEnemyTarget = enemyFactory;
		} else if (closestDefense) {
			currentEnemyTarget = closestDefense;
		} else {
			let enemyStructs = enumStruct(enemyIndex);
			if (enemyStructs.length > 0) {
				currentEnemyTarget = enemyStructs.sort(sortByDistToBase)[0];
			} else {
				currentEnemyTarget = enumDroid(enemyIndex, DROID_ANY, false)
					.filter(d => !d.isVTOL)
					.sort(sortByDistToBase)[0];
			}
		}
	}

	// Destroy defenses that are too close to the base
	if (closestDefense && isNearBase(closestDefense.x, closestDefense.y)) {
		currentEnemyTarget = closestDefense;
	}

	// If the enemy is the scavenger, attack all the structures and the visible ground droids
	if (enemyIndex === scavengerIndex) {
		currentEnemyTarget = [...enumStruct(scavengerIndex), ...enumDroid(scavengerIndex, DROID_ANY, true).filter(d => !d.isVTOL)]
			.sort(sortByDistToBase)[0];
	}

	if (!currentEnemyTarget) {
		return;
	}

	let realObject = getObject(currentEnemyTarget.type, enemyIndex, currentEnemyTarget.id);
	if (!realObject) {
		return; // Just for extra precaution
	}

	// Filter attacker droids based on distance
	const { attackingDroids, retreatingDroids } = filterDroidsByDistance(attackerDroids, currentEnemyTarget);

	// Send all the droids to attack if the base is in trouble
	if (isMyBaseInTrouble) {
		const droids = enumDroid(me, DROID_WEAPON).concat(enumDroid(me, DROID_CYBORG));
		sendDroidsToAttackTarget(droids, currentEnemyTarget);
	} else {
		// Retreat to the base (move away) from the enemy target if it is too close
		retreatToBase(retreatingDroids);

		// Send droids that are at a safe distance to attack
		sendDroidsToAttackTarget(attackingDroids, currentEnemyTarget);
	}
}

function attackEnemyDefenses(attackerDroids) {
	if (!currentEnemy || !currentEnemyTarget) return;

	// Get the current list of Bunker Busters
	let bunkerBusterDroids = enumGroup(bunkerBusterGroup);
	if (bunkerBusterDroids.length === 0) return;

	// Send all to attack anything if the base is in trouble
	if (isMyBaseInTrouble) {
		sendDroidsToAttackTarget(bunkerBusterDroids, currentEnemyTarget);
		return;
	}

	const enemyIndex = currentEnemy.position;
	const enemyStructures = enumStruct(enemyIndex, DEFENSE).sort(sortByDistToBase);
	const closestEnemyStructure = enemyStructures[0];

	if (!closestEnemyStructure) {
		useBunkerBuster = false;
		if (bunkerBusterDroids.length >= 5) {
			let realObject = getObject(currentEnemyTarget.type, enemyIndex, currentEnemyTarget.id);
			if (realObject) {
				sendDroidsToAttackTarget(bunkerBusterDroids, currentEnemyTarget);
			}
		}
		return;
	}

	// Count how many of our main attack droids are actually close to the enemy defense to escort the Bunker Busters
	const escortCount = attackerDroids.filter(droid =>
		distBetweenTwoPoints(droid.x, droid.y, closestEnemyStructure.x, closestEnemyStructure.y) <= BASE_THREAT_RANGE
	).length;

	// Only engage with Bunker Busters if at least 5 attack units are in range of the enemy defense (or it's right at our base)
	useBunkerBuster = escortCount >= 5 || isNearBase(closestEnemyStructure.x, closestEnemyStructure.y);

	if (!useBunkerBuster) {
		// Pull Bunker Busters back to base so they don't wander alone and die in vain
		retreatToBase(bunkerBusterDroids);
		return;
	}

	// If there are less than 5 Bunker Busters, keep them safe at base until a strike group is ready
	if (bunkerBusterDroids.length < 5) {
		retreatToBase(bunkerBusterDroids);
		return;
	}

	// Ensure the structure still exists
	let realObject = getObject(closestEnemyStructure.type, enemyIndex, closestEnemyStructure.id);
	if (!realObject) return;

	for (const bunkerBuster of bunkerBusterDroids) {
		orderDroidObj(bunkerBuster, DORDER_ATTACK, closestEnemyStructure);
	}
}


function filterDroidsByDistance(attackerDroids, enemyTarget) {
	const attackingDroids = [];
	const retreatingDroids = [];

	const DISTANCE_LIMITS = {
		ROCKETS: getRocketRange(),
		MACHINEGUN: getMachinegunRange(),
		SENSOR: componentAvailable("Sensor-WideSpec") ? 17 : 12,
		HOWITZERS: getHowitzerRange()
	};


	const weaponToLimit = new Map([
		...ROCKET_WEAPON_LIST.map(w => [w, DISTANCE_LIMITS.ROCKETS]),
		...MACHINEGUN_WEAPON_LIST.map(w => [w, DISTANCE_LIMITS.MACHINEGUN]),
		...HOWITZERS_WEAPON_LIST.map(w => [w, DISTANCE_LIMITS.HOWITZERS])
	]);

	for (const droid of attackerDroids) {
		const { x, y, weapons, droidType } = droid;
		const distance = distBetweenTwoPoints(x, y, enemyTarget.x, enemyTarget.y);

		const weaponName = (weapons && weapons[0]) ? weapons[0].name : undefined;
		let limit = weaponToLimit.get(weaponName);

		if (limit === undefined) {
			if (droidType === DROID_SENSOR) {
				limit = DISTANCE_LIMITS.SENSOR;
			} else if (droidType === DROID_CYBORG) {
				limit = getCyborgRange(weaponName);
			} else {
				limit = 5; // Default limit for unknown droids
			}
		}

		if (limit !== undefined && distance < limit) {
			retreatingDroids.push(droid);
		} else {
			attackingDroids.push(droid);
		}
	}

	return { attackingDroids, retreatingDroids };
}

function getRocketRange() {
	let range;

	switch (true) {
		case componentAvailable("Missile-A-T"):
			range = 14;
			break;
		case componentAvailable("Rocket-HvyA-T"):
			range = 10;
			break;
		case componentAvailable("Rocket-LtA-T"):
			range = 9;
			break;
		case componentAvailable("Rocket-Pod"):
			range = 8;
			break;
		default:
			range = 8;
	}

	return range;
}

function getCyborgRange(weaponName) {
	let range;

	switch (weaponName) {
		case "CyborgChaingun":
			range = 6;
			break;
		case "CyborgRotMG":
			range = 7;
			break;
		case "Cyb-Wpn-Laser":
			range = 12;
			break;
		case "Cyb-Hvywpn-PulseLsr":
			range = 14;
			break;
		case "CyborgRocket":
			range = 7;
			break;
		case "Cyb-Wpn-Atmiss":
			range = 13;
			break;
		case "Cyb-Hvywpn-TK":
			range = 10;
			break;
		case "Cyb-Hvywpn-A-T":
			range = 14;
			break;
		default:
			range = 6;
	}

	return range;
}

function getHowitzerRange() {
	let range;

	switch (true) {
		case componentAvailable("Howitzer150Mk1"):
			range = 64;
			break;
		case componentAvailable("Howitzer03-Rot"):
			range = 55;
			break;
		case componentAvailable("Howitzer-Incendiary"):
			range = 40;
			break;
		case componentAvailable("Howitzer105Mk1"):
			range = 55;
			break;
		default:
			range = 40;
	}

	return range;
}

function getMachinegunRange() {
	let range;

	switch (true) {
		case componentAvailable("HeavyLaser"):
			range = 16;
			break;
		case componentAvailable("Laser2PULSEMk1"):
			range = 14;
			break;
		case componentAvailable("MG5TWINROTARY"):
			range = 9;
			break;
		case componentAvailable("MG4ROTARYMk1"):
			range = 9;
			break;
		case componentAvailable("MG3Mk1"):
			range = 7;
			break;
		case componentAvailable("MG2Mk1"):
			range = 6;
			break;
		default:
			range = 6;
	}

	return range;
}


function retreatToBase(droids) {
	for (const droid of droids) {
		if (!isDroidRepairing(droid) && droid.order !== DORDER_RTB) {
			orderDroid(droid, DORDER_RTB);
		}
	}
}

// Does a droid need to repair? Distributes damaged droids across available Repair Facilities to avoid traffic jams.
function droidNeedsRepair(droid, repairFacilities, loadMap) {
	// It's a sensor or it's invalid
	if (!droid || droid.droidType === DROID_SENSOR) {
		return false;
	}

	if (droid.weapons && droid.weapons[0] && droid.weapons[0].name === BUNKER_BUSTER_WEAPON[0]) {
		return false;
	}

	// If the base is in trouble, send also damaged droids to defend
	if (isMyBaseInTrouble) {
		return false;
	}

	const percent = droid.propulsion === "hover01" ? 45 : 30;

	if (droid.health < 99) {
		if (isDroidRepairing(droid)) {
			return true;
		}
		// Also keep repairing if the droid was moving to a repair facility and is already within 5 tiles of one
		const facilities = repairFacilities !== undefined
			? repairFacilities
			: enumStruct(me, REPAIR_FACILITY_STAT).filter(r => r.status === BUILT);

		for (const fac of facilities) {
			if (distBetweenTwoPoints(droid.x, droid.y, fac.x, fac.y) <= 5 && droid.health < 95) {
				if (droid.order !== DORDER_RTR) {
					orderDroid(droid, DORDER_RTR);
				}
				return true;
			}
		}
	}

	if (droid.health < percent) {
		const facilities = repairFacilities !== undefined
			? repairFacilities
			: enumStruct(me, REPAIR_FACILITY_STAT).filter(r => r.status === BUILT);

		if (facilities.length === 0) {
			return false;
		}

		// Pick the best Repair Facility balancing distance and current queue load (avoids crowding a single facility)
		let bestFacility = null;
		let bestScore = 999999;

		for (const fac of facilities) {
			if (!droidCanReach(droid, fac.x, fac.y)) continue;

			const dist = distBetweenTwoPoints(droid.x, droid.y, fac.x, fac.y);
			// Count already-queued droids around this facility + droids assigned during this tick
			const nearbyDamaged = loadMap && loadMap.has(fac.id)
				? loadMap.get(fac.id)
				: enumRange(fac.x, fac.y, 8, me, false).filter(o => o.type === DROID && o.health < 95).length;

			// Each waiting droid adds an equivalent penalty of 18 tiles of distance
			const score = dist + (nearbyDamaged * 18);
			if (score < bestScore) {
				bestScore = score;
				bestFacility = fac;
			}
		}

		if (bestFacility) {
			if (loadMap) {
				loadMap.set(bestFacility.id, (loadMap.has(bestFacility.id) ? loadMap.get(bestFacility.id) : 0) + 1);
			}
			if (typeof DORDER_RTR_SPECIFIED !== "undefined") {
				orderDroidObj(droid, DORDER_RTR_SPECIFIED, bestFacility);
			} else {
				// Move directly to the chosen least-crowded repair facility; once within 5 tiles, DORDER_RTR snaps to it
				orderDroidLoc(droid, DORDER_MOVE, bestFacility.x, bestFacility.y);
			}
			return true;
		}
	}

	return false;
}

// Function to order droids to attack the target
function sendDroidsToAttackTarget(droids, target) {
	const repairFacilities = enumStruct(me, REPAIR_FACILITY_STAT).filter(r => r.status === BUILT);
	const repairLoadMap = new Map();
	for (const fac of repairFacilities) {
		const waiting = enumRange(fac.x, fac.y, 8, me, false)
			.filter(o => o.type === DROID && o.health < 95).length;
		repairLoadMap.set(fac.id, waiting);
	}

	const sensors = enumDroid(me, DROID_SENSOR);
	const activeSensor = sensors.length > 0 ? sensors[0] : null;

	// Look for nearby wounded ground enemies around the target (within 8 tiles) so we can finish off low-HP units
	// while strictly respecting each droid's minimum weapon range and ignoring enemy VTOLs
	const localEnemies = enumRange(target.x, target.y, 8, ENEMIES, true)
		.filter(obj => (obj.type === DROID && !obj.isVTOL) || (obj.type === STRUCTURE && obj.stattype !== WALL))
		.sort((a, b) => (a.health !== undefined ? a.health : 100) - (b.health !== undefined ? b.health : 100));

	const DISTANCE_LIMITS = {
		ROCKETS: getRocketRange(),
		MACHINEGUN: getMachinegunRange(),
		HOWITZERS: getHowitzerRange()
	};

	const weaponToLimit = new Map([
		...ROCKET_WEAPON_LIST.map(w => [w, DISTANCE_LIMITS.ROCKETS]),
		...MACHINEGUN_WEAPON_LIST.map(w => [w, DISTANCE_LIMITS.MACHINEGUN]),
		...HOWITZERS_WEAPON_LIST.map(w => [w, DISTANCE_LIMITS.HOWITZERS])
	]);

	for (let j = 0; j < droids.length; j++) {
		const droid = droids[j];
		if (droid.droidType === DROID_SENSOR) {
			if (droid.order !== DORDER_OBSERVE) {
				orderDroidObj(droid, DORDER_OBSERVE, target);
			}
		} else if (droid.order !== DORDER_RECYCLE && !droidNeedsRepair(droid, repairFacilities, repairLoadMap)) {
			const weaponName = (droid.weapons && droid.weapons[0]) ? droid.weapons[0].name : undefined;

			// Link long-range howitzers to our active sensor droid if available
			if (activeSensor && HOWITZERS_WEAPON_LIST.includes(weaponName)) {
				if (droid.order !== DORDER_FIRESUPPORT) {
					orderDroidObj(droid, DORDER_FIRESUPPORT, activeSensor);
				}
				continue;
			}

			// Determine minimum safe distance for this droid's weapon
			let minSafeRange = weaponToLimit.get(weaponName);
			if (minSafeRange === undefined) {
				minSafeRange = droid.droidType === DROID_CYBORG ? getCyborgRange(weaponName) : 5;
			}

			// Pick the lowest-health enemy near the target that stays at or beyond our safe range
			let bestTarget = target;
			if (!isMyBaseInTrouble && localEnemies.length > 0) {
				for (const candidate of localEnemies) {
					const distToCandidate = distBetweenTwoPoints(droid.x, droid.y, candidate.x, candidate.y);
					if (distToCandidate >= minSafeRange && distToCandidate <= minSafeRange + 8) {
						bestTarget = candidate;
						break;
					}
				}
			}

			if (droidCanReach(droid, bestTarget.x, bestTarget.y)) {
				orderDroidObj(droid, DORDER_ATTACK, bestTarget);
			} else if (droid.order !== DORDER_MOVE) {
				// Can't reach target — move as close as possible instead of getting stuck
				orderDroidLoc(droid, DORDER_MOVE, bestTarget.x, bestTarget.y);
			}
		}
	}
}

// Return the nearest factory (normal factory has precedence)
function findNearestFactory(player) {
	let facs = enumStruct(player, FACTORY_STAT).sort(sortByDistToBase);
	let cybFacs = enumStruct(player, CYBORG_FACTORY_STAT).sort(sortByDistToBase);
	let vtolFacs = enumStruct(player, VTOL_FACTORY_STAT).sort(sortByDistToBase);
	let target;

	if (facs.length > 0) {
		target = facs[0];
	} else if (cybFacs.length > 0) {
		target = cybFacs[0];
	} else if (vtolFacs.length > 0) {
		target = vtolFacs[0];
	}

	return target;
}

// Return closest player derrick ID
function findNearestDerrick(player) {
	let target;
	let derr = enumStruct(player, DERRICK_STAT, true).sort(sortByDistToBase);

	if (derr.length > 0) {
		target = derr[0];
	}

	return target;
}

function eventAttacked(victim, attacker) {
	if (!attacker || !victim || attacker.player === me || allianceExistsBetween(attacker.player, me)) return;

	// If a constructor truck or a remote derrick/structure outside our base is attacked,
	// mark that sector as a danger zone and pull back any vulnerable trucks in that area!
	if (victim.player === me && !isNearBase(victim.x, victim.y)) {
		if ((victim.type === DROID && victim.droidType === DROID_CONSTRUCT) ||
			(victim.type === STRUCTURE && victim.stattype === RESOURCE_EXTRACTOR)) {
			markDangerZone(victim.x, victim.y, 120000);
			if (victim.type === DROID && victim.order !== DORDER_RTB) {
				orderDroid(victim, DORDER_RTB);
			}
		}
	}

	// Check if the base is in trouble
	if (attacker.type === DROID || attacker.type === STRUCTURE) {
		if (attacker.type === DROID && attacker.isVTOL === true) {
			enemiesHaveVTOLs = true;
		}

		if ((victim.type === STRUCTURE && isBaseStructure(victim.stattype)) ||
			(victim.stattype === DEFENSE && isNearBase(attacker.x, attacker.y))) {
			const enemy = isScavengerPlayer(attacker.player) ? createScavengerPlayerData(attacker.player) : playerDataObject(attacker.player);
			if (enemy) {
				setCurrentEnemy(enemy);
			}
			isMyBaseInTrouble = true;
		}

		if (!isHelpingAlly && !isMyBaseInTrouble) {
			if (isScavengerPlayer(attacker.player)) {
				const scavenger = createScavengerPlayerData(attacker.player);
				if (scavenger) {
					setCurrentEnemy(scavenger);
				}
				return;
			}

			const enemy = playerDataObject(attacker.player);
			if (enemy) {
				setCurrentEnemy(enemy);
			}
		}
	}
}

function eventDestroyed(object) {
	if (!object || object.player !== me) return;

	// If one of our trucks or derricks outside the base was destroyed, blacklist that area for 2.5 minutes
	if (!isNearBase(object.x, object.y)) {
		if ((object.type === DROID && object.droidType === DROID_CONSTRUCT) ||
			(object.type === STRUCTURE && object.stattype === RESOURCE_EXTRACTOR)) {
			markDangerZone(object.x, object.y, 150000);
		}
	}
}

// Check if enemies have VTOLs (or VTOL factories/pads) so we can prepare AA defenses in advance
function scanForVTOLs() {
	if (enemiesHaveVTOLs) {
		removeTimer("scanForVTOLs");
		return; // Already aware of VTOLs
	}

	for (const enemy of enemiesList) {
		if (enemy.position === undefined) continue;

		// Detect if enemy already built a VTOL Factory or VTOL Rearm Pad
		if (countStruct(VTOL_FACTORY_STAT, enemy.position) > 0 || countStruct(VTOL_PAD_STAT, enemy.position) > 0) {
			enemiesHaveVTOLs = true;
			return;
		}

		const enemyDroids = enumDroid(enemy.position, DROID_WEAPON, false);
		for (let i = 0, l = enemyDroids.length; i < l; ++i) {
			if (enemyDroids[i].isVTOL === true) {
				enemiesHaveVTOLs = true;
				return;
			}
		}
	}
}
