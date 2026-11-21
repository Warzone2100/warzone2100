function checkWeaponPercentage(enemyTypeLists, weaponLists) {
	if (currentEnemy === undefined) {
		return 0;
	}

	let weaponCount = 0;
	const enemyUnits = enemyTypeLists.flatMap(type => enumDroid(currentEnemy.position, type));
	const totalEnemies = enemyUnits.length;

	if (totalEnemies === 0) {
		return 0;
	}

	const weaponSets = weaponLists.map(list => new Set(list));

	for (const unit of enemyUnits) {
		if (unit.weapons && unit.weapons.length > 0 && unit.weapons[0] && weaponSets.some(set => set.has(unit.weapons[0].name))) {
			weaponCount++;
		}
	}

	return (weaponCount / totalEnemies) * 100;
}

// Specific functions for flame and tank killer
function checkFlameWeapons() {
	return checkWeaponPercentage(
		[DROID_WEAPON, DROID_CYBORG],
		[FLAME_WEAPON_LIST, CYBORG_FLAMERS]
	);
}

function checkTankKillerWeapons() {
	return checkWeaponPercentage(
		[DROID_WEAPON, DROID_CYBORG],
		[ROCKET_WEAPON_LIST, CYBORG_HEAVY_ROCKETS, CYBORG_LIGHT_ROCKETS]
	);
}

// Dynamically adjust Anti-Personnel (MG/Laser) vs Anti-Tank (Rockets) ratio based on enemy unit composition
function checkEnemyAntiPersonnelProbability() {
	if (!currentEnemy || currentEnemy.position === undefined) {
		return 60;
	}

	const enemyTanks = enumDroid(currentEnemy.position, DROID_WEAPON).length;
	const enemyCyborgs = enumDroid(currentEnemy.position, DROID_CYBORG).length;
	const totalCombat = enemyTanks + enemyCyborgs;
	const hasLancerOrBetter = componentAvailable("Rocket-LtA-T") || componentAvailable("Rocket-HvyA-T") || componentAvailable("Missile-A-T");
	const hasMGWeapon = MACHINEGUN_WEAPON_LIST.some(w => componentAvailable(w));

	// In early T1 before Lancers unlock, favor available MG turrets over Mini-Rocket Pods if MG is researched
	if (!hasLancerOrBetter && hasMGWeapon) {
		return 85;
	}

	if (totalCombat < 5) {
		return 65; // Default early game favors MG/Lasers unless anti-tank rockets are needed
	}

	const cyborgRatio = (enemyCyborgs / totalCombat) * 100;
	if (cyborgRatio >= 45) {
		return 70; // Heavy cyborg enemy -> 70% Anti-Personnel (MG/Lasers)
	} else if (cyborgRatio <= 20) {
		return 35; // Tank-heavy enemy -> 35% Anti-Personnel, 65% Anti-Tank Rockets
	}

	return 55;
}

// If the enemies have defenses and our army is close enough to engage them, produce some bunker busters
function checkCurrentEnemyDefenses() {
	if (!currentEnemy || currentEnemy.position === undefined) {
		return 0;
	}

	// Cap Bunker Busters so we don't clog the unit limit with anti-structure only units
	if (enumGroup(bunkerBusterGroup).length >= 12) {
		return 0;
	}

	const enemyIndex = currentEnemy.position;
	const currentEnemyDefenses = enumStruct(enemyIndex, DEFENSE).sort(sortByDistToBase);

	if (currentEnemyDefenses.length === 0) {
		return 0;
	}

	// Only produce Bunker Busters if our main attack army is already reasonably close to the enemy defenses
	const closestDefense = currentEnemyDefenses[0];
	const attackers = enumGroup(attackerGroup);
	if (attackers.length < (isHighOilMap ? 10 : 3)) {
		return 0;
	}

	const closeAttackersCount = attackers.filter(droid =>
		distBetweenTwoPoints(droid.x, droid.y, closestDefense.x, closestDefense.y) <= BASE_THREAT_RANGE + 10
	).length;

	// Require enough combat units (or useBunkerBuster already active) to be near the enemy defense line
	if (!useBunkerBuster && closeAttackersCount < (isHighOilMap ? 4 : 2)) {
		return 0;
	}

	let probability = currentEnemyDefenses.length / (useBunkerBuster ? 1.2 : 2);

	return probability;
}

// Produce long distance support tanks in case we have a huge group of front tanks
function checkLongRangeSupport() {
	let probability = 5;

	const myDroidsSize = enumGroup(attackerGroup)
		.filter(droid => !HOWITZERS_WEAPON_LIST.includes((droid.weapons && droid.weapons[0] && droid.weapons[0].name) ? droid.weapons[0].name : ""))
		.length;

	if (myDroidsSize >= 100) {
		probability = 25;
	} else if (myDroidsSize >= 80) {
		probability = 20;
	} else if (myDroidsSize >= 50) {
		probability = 10;
	}

	return probability;
}
