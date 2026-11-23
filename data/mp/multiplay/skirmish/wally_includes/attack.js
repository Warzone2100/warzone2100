var currentEnemyTarget; // Current enemy target Wally is attacking.
var useBunkerBuster = false;
var rallyStartTime = 0; // When the current batch of reinforcements started waiting at base (0 = nobody waiting)
var armyRegroupUntil = 0; // While gameTime is below this, the marching army closes ranks instead of advancing
var armyNoRegroupUntil = 0; // No new regroup is started before this time (prevents endless stop-and-go)
var armyUrgentResponse = false; // True while the army is reacting to a threat at home (no waiting, no regrouping)
var armyCommitted = false; // True once the army decided to take on the current target (see shouldEngageTarget)
var armyRetreatUntil = 0; // After breaking off a losing fight, the army stays away until this time
var lastBaseAttacker = null; // {type, player, id, time}: the last ground enemy that hit one of our base structures

// Rough fighting value of a unit or defense: its build cost scaled by its remaining health
function combatWeight(obj) {
	const cost = (obj.cost !== undefined && obj.cost > 0) ? obj.cost : 100;
	const health = (obj.health !== undefined) ? Math.max(10, Math.min(100, obj.health)) : 100;
	return cost * health / 100;
}

function sumCombatWeight(objects) {
	let total = 0;
	for (let i = 0; i < objects.length; ++i) {
		total += combatWeight(objects[i]);
	}
	return total;
}

// Fighting value of the armed enemies (units and ground defenses, of any enemy player) within `range` tiles of (x, y)
function enemyStrengthNear(x, y, range) {
	return sumCombatWeight(enumRange(x, y, range, ENEMIES, false).filter(isArmedEnemyObject));
}

// Fighting value of our allies' ground units within `range` tiles of (x, y)
function alliedStrengthNear(x, y, range) {
	return sumCombatWeight(enumRange(x, y, range, ALLIES, false).filter(
		o => o.player !== me && o.type === DROID && isFriendlyGuardObject(o)
	));
}

// Decides whether the army should take on (or keep fighting) the enemy force around `target`.
//  - It commits only when it is about as strong as what is waiting there (allies on the spot count for half).
//  - Once committed it tolerates some losses, but when the units actually in the fight are clearly outmatched
//    the whole army breaks off together and stays away for a while instead of feeding units in one by one.
//  - Fights at home, emergencies and a full unit cap always go ahead.
function shouldEngageTarget(attackerDroids, target) {
	if (isMyBaseInTrouble || distBetweenTwoPoints(target.x, target.y, BASE.x, BASE.y) <= BASE_THREAT_RANGE + 14) {
		return true; // At home we fight with our defenses and repair bays behind us
	}
	if (countDroid(DROID_ANY, me) >= 135) {
		return true; // Close to the unit cap: waiting cannot make the army any bigger
	}

	const fighters = attackerDroids.filter(d => d.droidType !== DROID_SENSOR);
	if (fighters.length === 0) {
		return false;
	}

	const enemyPower = cached("enemyPowerAt" + target.id, 1500, () => enemyStrengthNear(target.x, target.y, 12));
	if (enemyPower <= 0) {
		armyCommitted = true; // Undefended target (derrick, truck, lone building)
		return true;
	}

	if (gameTime < armyRetreatUntil) {
		return false;
	}

	const alliedPower = cached("alliedPowerAt" + target.id, 1500, () => alliedStrengthNear(target.x, target.y, 12)) * 0.5;
	const ourPower = sumCombatWeight(fighters);
	const inTheFight = fighters.filter(d => distBetweenTwoPoints(d.x, d.y, target.x, target.y) <= 16);

	if (inTheFight.length > 0) {
		// Units still on their way count for half: they will arrive, but not all at once
		const localPower = sumCombatWeight(inTheFight);
		const effectivePower = localPower + (ourPower - localPower) * 0.5 + alliedPower;
		if (effectivePower < enemyPower * 0.6) {
			armyCommitted = false;
			armyRetreatUntil = gameTime + 25000;
			wallyLog("breaking off: " + Math.round(effectivePower) + " vs " + Math.round(enemyPower) + " at the target");
			return false;
		}
		armyCommitted = true;
		return true;
	}

	// Not in contact yet: commit at 90% of the enemy's strength, keep going as long as we stay above 70%
	const needed = armyCommitted ? 0.7 : 0.9;
	if (ourPower + alliedPower >= enemyPower * needed) {
		armyCommitted = true;
		return true;
	}
	if (armyCommitted) {
		wallyLog("target got too strong on the way: " + Math.round(ourPower + alliedPower) + " vs " + Math.round(enemyPower));
	}
	armyCommitted = false;
	return false;
}

// The armed ground enemy (of ANY enemy player) that threatens our home right now, or null:
// enemy units around the base, enemy defenses built inside it, or whoever just shot at a base structure from further away.
// This is what lets Wally keep one main enemy (see setCurrentEnemy) and still answer every attack on its base.
function getHomeThreatTarget() {
	return cached("homeThreatTarget", 1500, () => {
		const perimeter = BASE_THREAT_RANGE + 14;
		const threats = enumRange(BASE.x, BASE.y, perimeter, ENEMIES, true).filter(obj =>
			isArmedEnemyObject(obj) && (obj.type === DROID || isNearBase(obj.x, obj.y))
		).sort(sortByDistToBase);
		if (threats.length > 0) {
			return threats[0];
		}
		if (lastBaseAttacker && gameTime >= lastBaseAttacker.time && gameTime - lastBaseAttacker.time < 15000) {
			const attacker = getObject(lastBaseAttacker.type, lastBaseAttacker.player, lastBaseAttacker.id);
			if (attacker && attacker.isVTOL !== true) {
				return attacker;
			}
		}
		return null;
	});
}

// The closest armed enemy unit (of ANY enemy player) raiding one of our derricks, or null
function getDerrickThreatTarget() {
	return cached("derrickThreatTarget", 2000, () => {
		const derricks = enumStruct(me, DERRICK_STAT);
		for (let i = 0; i < derricks.length; ++i) {
			const raiders = enumRange(derricks[i].x, derricks[i].y, 10, ENEMIES, true)
				.filter(o => o.type === DROID && isArmedEnemyObject(o));
			if (raiders.length > 0) {
				return raiders[0];
			}
		}
		return null;
	});
}

// Middle of a group of droids (median of x and y, so a few stragglers do not drag it away from the main body)
function getArmyCenter(droids) {
	if (!droids || droids.length === 0) {
		return null;
	}
	const xs = droids.map(d => d.x).sort((a, b) => a - b);
	const ys = droids.map(d => d.y).sort((a, b) => a - b);
	const mid = Math.floor(droids.length / 2);
	return { x: xs[mid], y: ys[mid] };
}

// A point `tiles` away from BASE in the direction of the current enemy (or the map centre)
function getPointInFrontOfBase(tiles) {
	const targetPos = (currentEnemy && currentEnemy.base)
		? currentEnemy.base
		: { x: Math.floor(mapWidth / 2), y: Math.floor(mapHeight / 2) };
	const totalDist = distBetweenTwoPoints(BASE.x, BASE.y, targetPos.x, targetPos.y);
	if (totalDist < tiles + 4) {
		return { x: BASE.x, y: BASE.y };
	}
	return {
		x: Math.max(2, Math.min(mapWidth - 3, Math.round(BASE.x + (targetPos.x - BASE.x) / totalDist * tiles))),
		y: Math.max(2, Math.min(mapHeight - 3, Math.round(BASE.y + (targetPos.y - BASE.y) / totalDist * tiles)))
	};
}

// True while the main army is away from home
function isArmyDeployed() {
	const attackers = enumGroup(attackerGroup);
	if (attackers.length < 3) {
		return false;
	}
	const center = getArmyCenter(attackers);
	return distBetweenTwoPoints(center.x, center.y, BASE.x, BASE.y) > BASE_THREAT_RANGE + 10;
}

// Fresh units wait at base (rallyGroup) while the army is away, so they reach the front as a batch and not one by one
function shouldHoldReinforcements() {
	if (isMyBaseInTrouble || armyUrgentResponse || attackerGroup === undefined || rallyGroup === undefined) {
		return false;
	}
	return cached("armyDeployed", 2000, isArmyDeployed);
}

// Releases the waiting reinforcements to the army once the batch is big enough, has waited long enough, or home needs them
function manageReinforcements() {
	const waiting = enumGroup(rallyGroup);
	if (waiting.length === 0) {
		rallyStartTime = 0;
		return;
	}
	if (rallyStartTime === 0 || gameTime < rallyStartTime) {
		rallyStartTime = gameTime;
	}

	const batchSize = isHighOilMap ? 6 : 3;
	if (waiting.length >= batchSize || gameTime - rallyStartTime > 45000 || !shouldHoldReinforcements()) {
		for (const droid of waiting) {
			groupAdd(attackerGroup, droid);
		}
		rallyStartTime = 0;
		return;
	}

	// Keep them together just in front of the base, where they also help against raids
	const rallyPoint = getPointInFrontOfBase(8);
	for (const droid of waiting) {
		if (!isDroidRepairing(droid) && droid.order !== DORDER_MOVE &&
			distBetweenTwoPoints(droid.x, droid.y, rallyPoint.x, rallyPoint.y) > 6) {
			orderDroidLoc(droid, DORDER_MOVE, rallyPoint.x, rallyPoint.y);
		}
	}
}

// On the march, fast units pull ahead and arrive alone. When the army is strung out and still far from its target,
// everybody closes ranks on the middle of the column for a moment. Returns true while the army is regrouping.
function regroupArmyIfSpread(attackingDroids, target) {
	if (isMyBaseInTrouble || armyUrgentResponse) {
		armyRegroupUntil = 0;
		return false;
	}

	const fighters = attackingDroids.filter(d => d.droidType !== DROID_SENSOR);
	if (fighters.length < 4) {
		return false;
	}

	const center = getArmyCenter(fighters);
	const engaged = distBetweenTwoPoints(center.x, center.y, target.x, target.y) < 20;
	const together = fighters.filter(d => distBetweenTwoPoints(d.x, d.y, center.x, center.y) <= 9).length / fighters.length;
	const regrouping = gameTime < armyRegroupUntil;

	if (regrouping) {
		if (engaged || together >= 0.75) {
			armyRegroupUntil = 0;
			armyNoRegroupUntil = gameTime + 30000;
			return false;
		}
	} else {
		if (engaged || together >= 0.6 || gameTime < armyNoRegroupUntil) {
			return false;
		}
		// Start closing ranks: at most 20 seconds, then march on for at least 30 seconds whatever happens
		armyRegroupUntil = gameTime + 20000;
		armyNoRegroupUntil = gameTime + 50000;
	}

	if (!throttled("armyRegroupOrders", 3000)) {
		for (const droid of fighters) {
			if (distBetweenTwoPoints(droid.x, droid.y, center.x, center.y) > 5) {
				orderDroidLoc(droid, DORDER_MOVE, center.x, center.y);
			}
		}
	}
	return true;
}

// Armed enemies sitting on the free oil well our trucks want (see getUnassignedSafeOils), closest to our base first
function getContestedOilSquatters() {
	if (!contestedOil || gameTime < contestedOil.time || gameTime - contestedOil.time > 20000) {
		return [];
	}
	return cached("contestedOilSquatters", 2000, () =>
		enumRange(contestedOil.x, contestedOil.y, 8, ENEMIES, false)
			.filter(isArmedEnemyObject)
			.sort(sortByDistToBase));
}

function isDroidRepairing(droid) {
	if (droid.order === DORDER_RTR) return true;
	if (typeof DORDER_RTR_SPECIFIED !== "undefined" && droid.order === DORDER_RTR_SPECIFIED) return true;
	return false;
}

// Dynamically calculates the minimum squad size before launching an offensive,
// reacting immediately to threats near our base or derricks while scaling on low-oil maps
function getMinAttackGroupSize(attackerDroids) {
	armyUrgentResponse = true;
	if (isMyBaseInTrouble) {
		return 1;
	}

	// If enemy ground forces are threatening near our base perimeter, intercept immediately
	if (getHomeThreatTarget() !== null) {
		return 2;
	}

	if (isHighOilMap) {
		armyUrgentResponse = false;
		return gameTime < 480000 ? 15 : 25;
	}

	// If any of our derricks is actively threatened nearby, send a fast response squad
	if (getDerrickThreatTarget() !== null) {
		return 2;
	}

	// From here on nothing at home is burning: the army may wait for reinforcements and close ranks on the march
	armyUrgentResponse = false;

	// Armed enemies are sitting on a free oil well on our side of the map: clear them with a squad just big enough,
	// so the trucks can claim it (this is what wins or loses low-oil maps)
	const squatters = getContestedOilSquatters();
	if (squatters.length > 0) {
		return Math.max(2, Math.min(6, squatters.length + 1));
	}

	// Hysteresis: if we are already engaged near the current target, don't pull survivors back mid-fight
	if (currentEnemyTarget && attackerDroids.length >= 2) {
		const alreadyEngaged = attackerDroids.filter(
			d => distBetweenTwoPoints(d.x, d.y, currentEnemyTarget.x, currentEnemyTarget.y) <= 14
		).length;
		if (alreadyEngaged >= 2) {
			return 2;
		}
	}

	if (!currentEnemy || currentEnemy.position === undefined) {
		return 4;
	}

	// Against Scavengers on low oil, small squads clear camps quickly without waiting too long
	if (isScavengerPlayer(currentEnemy.position)) {
		return 3;
	}

	// Scale low-oil attack threshold dynamically against the current enemy's combat strength (4 to 12)
	const enemyIdx = currentEnemy.position;
	const enemyCombatCount = enumDroid(enemyIdx, DROID_WEAPON, false).filter(d => !d.isVTOL).length +
		enumDroid(enemyIdx, DROID_CYBORG, false).length;
	const enemyDefCount = enumStruct(enemyIdx, DEFENSE).filter(d => d.stattype !== WALL && d.canHitGround).length;
	const dynamicTarget = Math.ceil(enemyCombatCount * 0.6) + Math.min(3, Math.floor(enemyDefCount / 2));

	return Math.max(4, Math.min(12, dynamicTarget));
}

function attack() {
	return profiled("attack", attackTick);
}

function attackTick() {
	// Anti-air tanks that ended up in the main army (older saved game, gift from an ally) move to their own group
	if (!throttled("antiAirMigration", 10000)) {
		for (const droid of enumGroup(attackerGroup)) {
			if (isAntiAirDroid(droid)) {
				groupAdd(aaGroup, droid);
			}
		}
	}

	const attackerDroids = enumGroup(attackerGroup).filter(
		attacker => !isDroidRepairing(attacker) && attacker.order !== DORDER_RECOVER
	);

	const MIN_GROUP_SIZE = getMinAttackGroupSize(attackerDroids);

	manageReinforcements();

	if (attackerDroids.length < MIN_GROUP_SIZE && !isMyBaseInTrouble) {
		armyCommitted = false;
		retreatToBase(filterOutNeedingRepair(attackerDroids));
	} else if (attackerDroids.length > 0) {
		attackEnemy(attackerDroids);
		attackEnemyDefenses(attackerDroids);
	}

	// Mobile anti-air follows the army (and guards the base) instead of attacking with it
	manageAntiAir(attackerDroids);

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

// Positions the mobile anti-air tanks.
// Escorts hold a few tiles behind the middle of the army, between it and our base: their AA range still covers the
// front line, but they are no longer the first units the enemy meets and they never chase a VTOL across the map.
// When the army is at home, or enemy VTOLs bombed the base recently, AA tanks stand where the bombs fell.
function manageAntiAir(attackerDroids) {
	const antiAir = enumGroup(aaGroup).filter(d => !isDroidRepairing(d));
	if (antiAir.length === 0 || throttled("antiAirStations", 2000)) {
		return;
	}

	const repairFacilities = getBuiltRepairFacilities();
	const repairLoadMap = buildRepairLoadMap(repairFacilities);
	const ready = antiAir.filter(d => !droidNeedsRepair(d, repairFacilities, repairLoadMap));
	if (ready.length === 0) {
		return;
	}

	const armyCenter = getArmyCenter(attackerDroids.filter(d => d.droidType !== DROID_SENSOR));
	const armyAway = armyCenter !== null && attackerDroids.length >= 3 &&
		distBetweenTwoPoints(armyCenter.x, armyCenter.y, BASE.x, BASE.y) > BASE_THREAT_RANGE;
	const recentStrike = (lastVtolStrike && gameTime >= lastVtolStrike.time && gameTime - lastVtolStrike.time < 180000)
		? lastVtolStrike
		: null;
	const strikeAtHome = recentStrike !== null &&
		distBetweenTwoPoints(recentStrike.x, recentStrike.y, BASE.x, BASE.y) <= BASE_THREAT_RANGE + 8;

	// Who stays home: everybody when the army is home, a third when the base was bombed recently, nobody otherwise
	let homeCount = ready.length;
	if (armyAway) {
		homeCount = strikeAtHome ? Math.ceil(ready.length / 3) : 0;
	}

	ready.sort(sortByDistToBase);
	const homeGuards = ready.slice(0, homeCount);
	const escorts = ready.slice(homeCount);

	if (homeGuards.length > 0) {
		const homePost = strikeAtHome ? recentStrike : getPointInFrontOfBase(6);
		stationAntiAir(homeGuards, homePost, 7, "home");
	}

	if (escorts.length > 0 && armyCenter !== null) {
		// 4 tiles behind the middle of the army, on the side of our base
		const distToBase = distBetweenTwoPoints(armyCenter.x, armyCenter.y, BASE.x, BASE.y);
		const back = Math.min(4, distToBase);
		const escortPost = distToBase < 1 ? armyCenter : {
			x: Math.round(armyCenter.x + (BASE.x - armyCenter.x) / distToBase * back),
			y: Math.round(armyCenter.y + (BASE.y - armyCenter.y) / distToBase * back)
		};
		stationAntiAir(escorts, escortPost, 4, "escort");
	}
}

// droid id -> "home" / "escort": the duty each AA tank had last time (rebuilt after loading a saved game)
const antiAirDuties = new Map();

// Sends each AA tank that strayed more than `leash` tiles from its post back to a slot around that post.
// Tanks already in place are left alone: standing still, they engage every VTOL in weapon range on their own.
// A tank whose duty just changed always gets a fresh order, so it does not keep driving to its previous post.
function stationAntiAir(droids, post, leash, duty) {
	const slots = [[0, 0], [2, 0], [-2, 0], [0, 2], [0, -2], [2, 2], [-2, -2], [2, -2], [-2, 2]];
	if (antiAirDuties.size > 300) {
		antiAirDuties.clear();
	}
	for (let i = 0; i < droids.length; ++i) {
		const droid = droids[i];
		const sameDuty = antiAirDuties.get(droid.id) === duty;
		antiAirDuties.set(droid.id, duty);
		if (sameDuty && distBetweenTwoPoints(droid.x, droid.y, post.x, post.y) <= leash) {
			continue;
		}
		const slot = slots[i % slots.length];
		const sx = Math.max(1, Math.min(mapWidth - 2, post.x + slot[0]));
		const sy = Math.max(1, Math.min(mapHeight - 2, post.y + slot[1]));
		orderDroidLoc(droid, DORDER_MOVE, sx, sy);
	}
}

// Scores enemy ground units by relevance for VTOL bombers (lower = attacked first):
// 1 = Long-range Artillery / Howitzers / Sensors / Commanders (high-value backline targets)
// 2 = Main Battle Tanks & Cyborgs (direct frontline support)
// 3 = Constructor Trucks (economy & building denial)
// 5 = Mobile Anti-Air: the units built to kill our VTOLs are the last thing we fly into
function getVTOLUnitPriority(droid) {
	if (droid.canHitAir === true) {
		return 5;
	}
	if (droid.droidType === DROID_CONSTRUCT) {
		return 3;
	}
	const weapName = (droid.weapons && droid.weapons[0] && droid.weapons[0].name) ? droid.weapons[0].name : "";
	if (droid.droidType === DROID_SENSOR ||
		droid.droidType === DROID_COMMAND ||
		HOWITZERS_WEAPON_LIST.includes(weapName) ||
		weapName.includes("Mortar") ||
		weapName.includes("Howitzer")) {
		return 1;
	}
	return 2;
}

// Number of enemy anti-air guns (static sites AND mobile AA units) covering the area around (x, y)
function countEnemyAntiAirNear(x, y) {
	return enumRange(x, y, 9, ENEMIES, true).filter(
		o => (o.type === STRUCTURE || o.type === DROID) && o.canHitAir === true
	).length;
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

	const MIN_VTOL_STRIKE_SIZE = isMyBaseInTrouble ? 1 : (isHighOilMap ? 4 : 2);
	if (readyVTOLs.length < MIN_VTOL_STRIKE_SIZE) {
		return;
	}

	let unitTargets = [];
	let structureTarget = null;
	let findSafeEnemyStruct = null;

	if (currentEnemy && currentEnemy.position !== undefined) {
		const enemyIdx = currentEnemy.position;

		// Gather all ground enemy tanks, cyborgs, scavenger infantry, sensors, and trucks (strictly excluding enemy VTOLs!)
		const enemyGroundUnits = enumDroid(enemyIdx, DROID_ANY, false)
			.filter(d => !d.isVTOL && (
				d.droidType === DROID_WEAPON ||
				d.droidType === DROID_CYBORG ||
				d.droidType === DROID_PERSON ||
				d.droidType === DROID_SENSOR ||
				d.droidType === DROID_COMMAND ||
				d.droidType === DROID_CONSTRUCT
			));

		// Sort by VTOL relevance priority first (artillery/sensors -> combat tanks/cyborgs -> trucks -> anti-air last),
		// and then by closest distance to our base
		enemyGroundUnits.sort((a, b) => {
			const prioDiff = getVTOLUnitPriority(a) - getVTOLUnitPriority(b);
			if (prioDiff !== 0) return prioDiff;
			return distBetweenTwoPoints(BASE.x, BASE.y, a.x, a.y) - distBetweenTwoPoints(BASE.x, BASE.y, b.x, b.y);
		});

		// Prefer high-priority ground units that aren't covered by more than 2 anti-air guns, static or mobile
		// (only the first 12 candidates are checked, up to 4 are kept for target splitting)
		const safeUnitTargets = enemyGroundUnits.slice(0, 12).filter(u => countEnemyAntiAirNear(u.x, u.y) <= 2).slice(0, 4);

		unitTargets = safeUnitTargets.length > 0 ? safeUnitTargets : enemyGroundUnits.slice(0, 4);

		findSafeEnemyStruct = () => {
			const enemyStructs = enumStruct(enemyIdx, DEFENSE, false)
				.concat(enumStruct(enemyIdx, FACTORY, false))
				.concat(enumStruct(enemyIdx, RESOURCE_EXTRACTOR, false));
			const safeStruct = enemyStructs.find(s => countEnemyAntiAirNear(s.x, s.y) <= 2);
			const fallbackTarget = (currentEnemyTarget && !currentEnemyTarget.isVTOL && currentEnemyTarget.type === STRUCTURE)
				? currentEnemyTarget
				: null;
			return safeStruct || findNearestFactory(enemyIdx) || findNearestDerrick(enemyIdx) || fallbackTarget || null;
		};

		// Fallback to enemy structures if the enemy has no ground units left
		if (unitTargets.length === 0) {
			structureTarget = findSafeEnemyStruct();
		}
	}

	if (unitTargets.length === 0 && (!structureTarget || structureTarget.isVTOL)) return;

	// Split VTOL strikes so rocket/missile VTOLs don't overkill a single ground unit,
	// while Bunker Buster VTOLs prioritize structures and heavy bombs focus top armored targets.
	let rocketIdx = 0;
	let bombIdx = 0;
	for (const vtol of readyVTOLs) {
		const weapName = (vtol.weapons && vtol.weapons[0] && vtol.weapons[0].name) ? vtol.weapons[0].name : "";
		const isBomb = weapName.startsWith("Bomb");
		const isBB = BUNKER_BUSTER_WEAPON.includes(weapName);

		let chosen = null;
		if (isBB) {
			if (!structureTarget && findSafeEnemyStruct) {
				structureTarget = findSafeEnemyStruct();
			}
			chosen = (structureTarget && !structureTarget.isVTOL) ? structureTarget : (unitTargets[0] || null);
		} else if (unitTargets.length > 0) {
			if (isBomb) {
				const bombTargets = Math.min(2, unitTargets.length);
				chosen = unitTargets[Math.floor(bombIdx / 3) % bombTargets];
				bombIdx++;
			} else {
				chosen = unitTargets[Math.floor(rocketIdx / 2) % unitTargets.length];
				rocketIdx++;
			}
		} else {
			chosen = structureTarget;
		}

		if (!chosen || chosen.isVTOL) continue;
		const realTarget = getObject(chosen.type, chosen.player, chosen.id);
		if (realTarget) {
			orderDroidObj(vtol, DORDER_ATTACK, realTarget);
		}
	}
}

function helpAttackEnemy(helperDroids, allyBasePos) {
	// Phase 1: gather on the staging point chosen in helpAlly() until 70% of the group is there or time is up
	if (helpStage) {
		const gathered = helperDroids.filter(
			d => distBetweenTwoPoints(d.x, d.y, helpStage.x, helpStage.y) <= 7
		).length;
		const expired = gameTime >= helpStage.until || gameTime < helpStage.until - 60000;
		if (!expired && gathered < Math.ceil(helperDroids.length * 0.7)) {
			for (const droid of helperDroids) {
				if (droid.order !== DORDER_MOVE && distBetweenTwoPoints(droid.x, droid.y, helpStage.x, helpStage.y) > 4) {
					orderDroidLoc(droid, DORDER_MOVE, helpStage.x, helpStage.y);
				}
			}
			return;
		}
		helpStage = null;
	}

	// Phase 2: advance together
	// Find visible enemy ground combat units near the ally's base (ignore enemy VTOLs)
	const nearbyEnemies = enumRange(allyBasePos.x, allyBasePos.y, BASE_THREAT_RANGE * 2, ENEMIES, true)
		.filter(obj => obj.type === DROID && !obj.isVTOL && (obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG || obj.droidType === DROID_PERSON));

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
			// Not there yet or no visible enemies — attack-move toward the ally's base (fights whatever it meets on the way)
			if (droid.order !== DORDER_SCOUT && droid.order !== DORDER_ATTACK) {
				orderDroidLoc(droid, DORDER_SCOUT, allyBasePos.x, allyBasePos.y);
			}
		}
	}
}

// Scavengers only: every scavenger object is a target (fighters, cranes and other vehicles, bunkers and towers,
// factories, derricks, generators, huts...), strictly in order of distance from our base, so their camps are
// cleared completely one after the other. Helicopters are left to the anti-air; walls and gates are only shot
// at when nothing else is left. Returns the nearest target our army can reach, or null.
function getScavengerTarget(scavIndex, refDroid) {
	const targets = cached("scavengerTargets", 2000, () => {
		const droids = enumDroid(scavIndex, DROID_ANY, false).filter(d => !d.isVTOL);
		const structs = enumStruct(scavIndex);
		const solid = structs.filter(s => s.stattype !== WALL && s.stattype !== GATE);
		const list = droids.concat((solid.length > 0 || droids.length > 0) ? solid : structs);
		return list.sort(sortByDistToBase);
	});

	if (targets.length === 0) {
		return null;
	}
	if (refDroid) {
		// Only the nearest few are tested for reachability (cliffs, water): enough to skip an island camp
		for (let i = 0; i < Math.min(targets.length, 10); ++i) {
			if (droidCanReach(refDroid, targets[i].x, targets[i].y)) {
				return targets[i];
			}
		}
	}
	return targets[0];
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
		.concat(enumDroid(enemyIndex, DROID_PERSON, false))
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

	if (isScavengerPlayer(enemyIndex)) {
		// Scavengers: not just their fighters. Units, buildings and derricks alike, nearest to our base first.
		currentEnemyTarget = getScavengerTarget(enemyIndex, refDroid);
	} else {
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
	}

	// Fallback if no droid/truck/derrick found
	if (!currentEnemyTarget) {
		const enemyFactory = findNearestFactory(enemyIndex);
		if (enemyFactory) {
			currentEnemyTarget = enemyFactory;
		} else if (closestDefense) {
			currentEnemyTarget = closestDefense;
		} else {
			let enemyStructs = enumStruct(enemyIndex).filter(s => s.stattype !== WALL);
			if (enemyStructs.length === 0) {
				enemyStructs = enumStruct(enemyIndex);
			}
			if (enemyStructs.length > 0) {
				currentEnemyTarget = enemyStructs.sort(sortByDistToBase)[0];
			} else {
				currentEnemyTarget = enumDroid(enemyIndex, DROID_ANY, false)
					.filter(d => !d.isVTOL)
					.sort(sortByDistToBase)[0];
			}
		}
	}

	// Whoever threatens our home comes first, whichever enemy player it belongs to: Wally keeps one main enemy
	// for its offensives, but always answers an attack on its base or derricks.
	const homeThreat = getHomeThreatTarget();
	const derrickThreat = homeThreat ? null : getDerrickThreatTarget();
	if (homeThreat) {
		currentEnemyTarget = homeThreat;
	} else if (closestDefense && isNearBase(closestDefense.x, closestDefense.y)) {
		// Destroy defenses that are too close to the base
		currentEnemyTarget = closestDefense;
	} else if (derrickThreat && !isMyBaseInTrouble) {
		currentEnemyTarget = derrickThreat;
	} else if (!isMyBaseInTrouble) {
		// Armed enemies (of any enemy player) holding a free oil well on our side of the map come first,
		// unless the regular target is even closer to home
		const squatters = getContestedOilSquatters();
		if (squatters.length > 0 && squatters.length <= attackerDroids.length &&
			(!currentEnemyTarget || sortByDistToBase(squatters[0], currentEnemyTarget) < 0)) {
			currentEnemyTarget = squatters[0];
		}
	}

	if (!currentEnemyTarget) {
		return;
	}

	let realObject = getObject(currentEnemyTarget.type, currentEnemyTarget.player, currentEnemyTarget.id);
	if (!realObject) {
		return; // Just for extra precaution
	}

	// Judge the fight before (and while) taking it: do not walk into a force we cannot beat, and break off together when losing
	const wasCommitted = armyCommitted;
	if (!shouldEngageTarget(attackerDroids, currentEnemyTarget)) {
		retreatToBase(filterOutNeedingRepair(attackerDroids));
		return;
	}

	// Tell the team where the offensive is going (beacon + quick chat), once per push
	if (!wasCommitted && armyCommitted && !armyUrgentResponse &&
		distBetweenTwoPoints(currentEnemyTarget.x, currentEnemyTarget.y, BASE.x, BASE.y) > BASE_THREAT_RANGE + 14) {
		notifyAllies("attacking", currentEnemyTarget.x, currentEnemyTarget.y);
	}

	// Strung-out column far from the target: close ranks first instead of arriving one by one
	if (regroupArmyIfSpread(attackerDroids, currentEnemyTarget)) {
		return;
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
	const minBunkerBusters = isHighOilMap ? 5 : 2;
	const minEscortCount = isHighOilMap ? 5 : 2;

	if (!closestEnemyStructure) {
		useBunkerBuster = false;
		if (bunkerBusterDroids.length >= minBunkerBusters) {
			let realObject = getObject(currentEnemyTarget.type, currentEnemyTarget.player, currentEnemyTarget.id);
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

	// Only engage with Bunker Busters if enough attack units are in range of the enemy defense (or it's right at our base)
	useBunkerBuster = escortCount >= minEscortCount || isNearBase(closestEnemyStructure.x, closestEnemyStructure.y);

	const readyBunkerBusters = filterOutNeedingRepair(bunkerBusterDroids);

	if (!useBunkerBuster) {
		// Pull Bunker Busters back to base so they don't wander alone and die in vain
		retreatToBase(readyBunkerBusters);
		return;
	}

	// Keep Bunker Busters safe at base until a strike group is ready
	if (readyBunkerBusters.length < minBunkerBusters) {
		retreatToBase(readyBunkerBusters);
		return;
	}

	// Ensure the structure still exists
	let realObject = getObject(closestEnemyStructure.type, enemyIndex, closestEnemyStructure.id);
	if (!realObject) return;

	for (const bunkerBuster of readyBunkerBusters) {
		orderDroidObj(bunkerBuster, DORDER_ATTACK, closestEnemyStructure);
	}
}


function getWeaponRangeMap() {
	const rocketRange = getRocketRange();
	const mgRange = getMachinegunRange();
	const howitzerRange = getHowitzerRange();
	const rocketArrayRanges = {
		"Missile-MdArt": 12,
		"Rocket-MRL-Hvy": 10,
		"Rocket-MRL": 9
	};

	return new Map([
		...ROCKET_WEAPON_LIST.map(w => [w, rocketRange]),
		...ROCKET_ARRAY_WEAPON_LIST.map(w => [w, rocketArrayRanges[w] || rocketRange]),
		...MACHINEGUN_WEAPON_LIST.map(w => [w, mgRange]),
		...HOWITZERS_WEAPON_LIST.map(w => [w, howitzerRange])
	]);
}

function filterDroidsByDistance(attackerDroids, enemyTarget) {
	const attackingDroids = [];
	const retreatingDroids = [];

	const SENSOR_RANGE = componentAvailable("Sensor-WideSpec") ? 17 : 12;
	const weaponToLimit = getWeaponRangeMap();

	for (const droid of attackerDroids) {
		const { x, y, weapons, droidType } = droid;
		const distance = distBetweenTwoPoints(x, y, enemyTarget.x, enemyTarget.y);

		const weaponName = (weapons && weapons[0]) ? weapons[0].name : undefined;
		let limit = weaponToLimit.get(weaponName);

		if (limit === undefined) {
			if (droidType === DROID_SENSOR) {
				limit = SENSOR_RANGE;
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

function getBuiltRepairFacilities() {
	return enumStruct(me, REPAIR_FACILITY_STAT).filter(r => r.status === BUILT);
}

// Count only droids already close to returning or actively wounded around each Repair Facility
function buildRepairLoadMap(repairFacilities) {
	const repairLoadMap = new Map();
	for (const fac of repairFacilities) {
		const waiting = enumRange(fac.x, fac.y, 5, me, false)
			.filter(o => o.type === DROID && (o.order === DORDER_RTR || o.health < 75)).length;
		repairLoadMap.set(fac.id, waiting);
	}
	return repairLoadMap;
}

function filterOutNeedingRepair(droids) {
	const repairFacilities = getBuiltRepairFacilities();
	if (repairFacilities.length === 0) return droids;
	const repairLoadMap = buildRepairLoadMap(repairFacilities);
	return droids.filter(d => !droidNeedsRepair(d, repairFacilities, repairLoadMap));
}

// Does a droid need to repair? Distributes damaged droids across available Repair Facilities to avoid traffic jams.
function droidNeedsRepair(droid, repairFacilities, loadMap) {
	// It's a sensor or it's invalid
	if (!droid || droid.droidType === DROID_SENSOR) {
		return false;
	}

	// If the base is in trouble, send also damaged droids to defend
	if (isMyBaseInTrouble) {
		return false;
	}

	const basePercent = droid.propulsion === "hover01" ? 60 : 55;
	const percent = (droid.experience !== undefined && droid.experience > 2) ? basePercent + 5 : basePercent;

	if (droid.health < 99) {
		if (isDroidRepairing(droid)) {
			return true;
		}
		// Keep repairing if the droid was moving to a repair facility and is already within 6 tiles of one
		const facilities = repairFacilities !== undefined ? repairFacilities : getBuiltRepairFacilities();

		for (const fac of facilities) {
			if (distBetweenTwoPoints(droid.x, droid.y, fac.x, fac.y) <= 6 && droid.health < 95) {
				if (droid.order !== DORDER_RTR) {
					orderDroid(droid, DORDER_RTR);
				}
				return true;
			}
		}
	}

	if (droid.health < percent) {
		const facilities = repairFacilities !== undefined ? repairFacilities : getBuiltRepairFacilities();

		if (facilities.length === 0) {
			return false;
		}

		// Pick the best Repair Facility balancing distance and active queue load without starving idle facilities
		let bestFacility = null;
		let bestScore = 999999;

		for (const fac of facilities) {
			if (!droidCanReach(droid, fac.x, fac.y)) continue;

			const dist = distBetweenTwoPoints(droid.x, droid.y, fac.x, fac.y);
			// Count active/queued droids around this facility + droids assigned during this tick
			const nearbyDamaged = loadMap && loadMap.has(fac.id)
				? loadMap.get(fac.id)
				: enumRange(fac.x, fac.y, 5, me, false)
					.filter(o => o.type === DROID && (o.order === DORDER_RTR || o.health < 75)).length;

			// Moderate penalty (6 tiles per queued droid) avoids bottlenecks while keeping idle bays fed
			const score = dist + (nearbyDamaged * 6);
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
			} else if (distBetweenTwoPoints(droid.x, droid.y, bestFacility.x, bestFacility.y) <= 12) {
				orderDroid(droid, DORDER_RTR);
			} else {
				// Move toward the least-crowded repair facility; within 6 tiles DORDER_RTR snaps cleanly to it
				orderDroidLoc(droid, DORDER_MOVE, bestFacility.x, bestFacility.y);
			}
			return true;
		}
	}

	return false;
}

// Function to order droids to attack the target
function sendDroidsToAttackTarget(droids, target) {
	const repairFacilities = getBuiltRepairFacilities();
	const repairLoadMap = buildRepairLoadMap(repairFacilities);

	const sensors = enumDroid(me, DROID_SENSOR);
	const activeSensor = sensors.length > 0 ? sensors[0] : null;

	// Look for nearby wounded ground enemies around the target (within 8 tiles) so we can finish off low-HP units
	// while strictly respecting each droid's minimum weapon range and ignoring enemy VTOLs
	const localEnemies = enumRange(target.x, target.y, 8, ENEMIES, true)
		.filter(obj => (obj.type === DROID && !obj.isVTOL) || (obj.type === STRUCTURE && obj.stattype !== WALL))
		.sort((a, b) => (a.health !== undefined ? a.health : 100) - (b.health !== undefined ? b.health : 100));

	const weaponToLimit = getWeaponRangeMap();

	for (let j = 0; j < droids.length; j++) {
		const droid = droids[j];
		// Anti-air tanks are positioned by manageAntiAir(): they never get ground attack orders
		if (isAntiAirDroid(droid)) {
			continue;
		}
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

// Return the nearest factory (normal factory has precedence, supports both player and scavenger factories)
function findNearestFactory(player) {
	let facs = enumStruct(player, FACTORY).sort(sortByDistToBase);
	let cybFacs = enumStruct(player, CYBORG_FACTORY).sort(sortByDistToBase);
	let vtolFacs = enumStruct(player, VTOL_FACTORY).sort(sortByDistToBase);
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
	let derr = enumStruct(player, RESOURCE_EXTRACTOR, false).sort(sortByDistToBase);

	if (derr.length > 0) {
		target = derr[0];
	}

	return target;
}

function eventAttacked(victim, attacker) {
	if (!attacker || !victim || attacker.player === me || allianceExistsBetween(attacker.player, me)) return;

	// Enemy VTOLs: remember where the bombs fell so anti-air goes there. They are not a job for the ground army,
	// so they neither raise the "base in trouble" alarm nor blacklist the area for trucks.
	if (attacker.type === DROID && attacker.isVTOL === true) {
		if (vtolsEnabled) {
			enemiesHaveVTOLs = true;
		}
		if (victim.player === me) {
			lastVtolStrike = { x: victim.x, y: victim.y, time: gameTime };
		}
		return;
	}

	// If a constructor truck or a remote derrick/structure outside our base is attacked,
	// mark that sector as a danger zone and pull back any vulnerable trucks in that area!
	if (victim.player === me && !isNearBase(victim.x, victim.y)) {
		if ((victim.type === DROID && victim.droidType === DROID_CONSTRUCT) ||
			(victim.type === STRUCTURE && victim.stattype === RESOURCE_EXTRACTOR)) {
			markDangerZone(victim.x, victim.y, 90000);
			if (victim.type === DROID && victim.order !== DORDER_RTB) {
				orderDroid(victim, DORDER_RTB);
			}
		}
	}

	// Check if the base is in trouble.
	// The main enemy is no longer swapped for whoever fired last (that made Wally change opponent all the time):
	// the attacker is remembered instead, and the army deals with it through getHomeThreatTarget() whoever it belongs to.
	if (attacker.type === DROID || attacker.type === STRUCTURE) {
		if ((victim.type === STRUCTURE && isBaseStructure(victim.stattype)) ||
			(victim.stattype === DEFENSE && isNearBase(attacker.x, attacker.y))) {
			lastBaseAttacker = { type: attacker.type, player: attacker.player, id: attacker.id, time: gameTime };
			isMyBaseInTrouble = true;
		}
	}
}

function eventDestroyed(object) {
	if (!object || object.player !== me) return;

	// If one of our trucks or derricks outside the base was destroyed, blacklist that area for 2.5 minutes
	if (!isNearBase(object.x, object.y)) {
		if ((object.type === DROID && object.droidType === DROID_CONSTRUCT) ||
			(object.type === STRUCTURE && object.stattype === RESOURCE_EXTRACTOR)) {
			markDangerZone(object.x, object.y, 120000);
		}
	}
}

// Check if enemies have VTOLs (or VTOL factories/pads) so we can prepare AA defenses in advance
function scanForVTOLs() {
	if (enemiesHaveVTOLs || !vtolsEnabled) {
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
