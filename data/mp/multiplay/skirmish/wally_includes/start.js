var enemiesList = [];

function eventStartLevel() {
	// Greets the player
	chat(ALLIES, "Hello! I'm Wally, your best defensive ally. I'll help you win this game.");

	// setup groups
	baseBuildersGroup = newGroup();
	defensesBuildersGroup = newGroup();
	attackerGroup = newGroup();
	bunkerBusterGroup = newGroup();
	helperGroup = newGroup();
	vtolGroup = newGroup();

	enumDroid(me).forEach(droid => {
		eventDroidBuilt(droid);
	});

	isSeaMap = isHoverMap();

	// Set Difficulty
	setDifficulty();

	// Check if the Technology level is 4
	isMultiTech4();

	// Recycle old builder if possible
	recycleOldBuilders();

	// Set the allies
	alliesList = getAliveAllyPlayersData();

	// Set the enemies
	enemiesList = getAliveEnemyPlayersData();

	// Set the current enemy
	setCurrentEnemy();

	// Kickstart early game actions immediately at tick 0 (crucial in Low Oil / No Bases)
	// Run build() before lookForOil() so core base structures are started before trucks scatter for oil
	if (reactionSpeedDelay === 0) {
		build();
		lookForOil();
		produceDroids();
		research();
	}

	// Set the timer call randomly so as not to calculate in the same tick.
	setTimer("produceDroids", reactionSpeedDelay + 1500 + ((1 + random(30)) * random(10))); // 1,5 Seconds
	setTimer("research", reactionSpeedDelay + 1500 + ((1 + random(30)) * random(10))); // 1,5 Seconds
	setTimer("build", reactionSpeedDelay + 1500 + ((1 + random(14)) * random(46))); // 1,5 Seconds
	setTimer("attack", reactionSpeedDelay + 1000 + ((1 + random(30)) * random(10))); // 1 Second
	setTimer("checkIsHighOilMap", reactionSpeedDelay + 30000 + ((1 + random(30)) * random(10))); // 30 Seconds
	setTimer("setCurrentEnemy", reactionSpeedDelay + 180000 + ((1 + random(30)) * random(10))); // 3 Minutes
	setTimer("lookForOil", reactionSpeedDelay + 1500 + ((1 + random(30)) * random(10))); // 1,5 Seconds
	setTimer("myBaseInTrouble", reactionSpeedDelay + 5000 + ((1 + random(30)) * random(10))); // 5 Seconds
	setTimer("checkAllyBaseInTrouble", reactionSpeedDelay + 15000 + ((1 + random(30)) * random(10))); // 15 Seconds
	setTimer("recycleOldBuilders", reactionSpeedDelay + 20000 + ((1 + random(30)) * random(10))); // 20 Seconds
	setTimer("checkTrucksCount", reactionSpeedDelay + 120000 + ((1 + random(30)) * random(10))); // 2 Minutes
	setTimer("donateOil", reactionSpeedDelay + 120000 + ((1 + random(30)) * random(10))); // 2 Minutes
	setTimer("checkAllies", reactionSpeedDelay + 120000 + ((1 + random(30)) * random(10))); // 2 Minutes
	setTimer("checkForDefeatedAllies", reactionSpeedDelay + 120000 + ((1 + random(33)) * random(88))); // 2 Minutes
	setTimer("swapBuildersRole", reactionSpeedDelay + 10000 + ((1 + random(30)) * random(10))); // 10 Seconds
	setTimer("callIdleBuildersToBase", reactionSpeedDelay + 5000 + ((1 + random(30)) * random(10))); // 5 Seconds
	setTimer("checkIfDead", reactionSpeedDelay + 15000 + ((1 + random(30)) * random(10))); // 15 Seconds
	setTimer("askForTruck", reactionSpeedDelay + 120000 + ((1 + random(30)) * random(10))); // 2 Minutes
	setTimer("scanForVTOLs", reactionSpeedDelay + 15000 + ((1 + random(30)) * random(10))); // 15 Seconds
	setTimer("recycleObsoleteUnits", reactionSpeedDelay + 45000 + ((1 + random(30)) * random(10))); // 45 Seconds
}

// Re-initialize state and groups when loading a saved game
function eventGameLoaded() {
	if (typeof dangerZones === "undefined" || !(dangerZones instanceof Map)) {
		dangerZones = new Map();
	}
	if (typeof pendingBuildOrders === "undefined" || !(pendingBuildOrders instanceof Map)) {
		pendingBuildOrders = new Map();
	}
	if (typeof truckIdleTracker === "undefined" || !(truckIdleTracker instanceof Map)) {
		truckIdleTracker = new Map();
	}

	if (baseBuildersGroup === undefined) baseBuildersGroup = newGroup();
	if (defensesBuildersGroup === undefined) defensesBuildersGroup = newGroup();
	if (attackerGroup === undefined) attackerGroup = newGroup();
	if (bunkerBusterGroup === undefined) bunkerBusterGroup = newGroup();
	if (helperGroup === undefined) helperGroup = newGroup();
	if (vtolGroup === undefined) vtolGroup = newGroup();

	isSeaMap = isHoverMap();
	setDifficulty();
	isMultiTech4();

	alliesList = getAliveAllyPlayersData();
	enemiesList = getAliveEnemyPlayersData();
	if (!currentEnemy || !isPlayerAlive(currentEnemy.position)) {
		setCurrentEnemy();
	}

	// Ensure all droids belong to their appropriate group
	swapBuildersRole();
	const assignedCombatIds = new Set([
		...enumGroup(attackerGroup).map(d => d.id),
		...enumGroup(bunkerBusterGroup).map(d => d.id),
		...enumGroup(helperGroup).map(d => d.id),
		...enumGroup(vtolGroup).map(d => d.id)
	]);
	for (const droid of enumDroid(me)) {
		if (droid.droidType !== DROID_CONSTRUCT && !assignedCombatIds.has(droid.id)) {
			eventDroidBuilt(droid);
		}
	}
}

// Balance builders between baseBuildersGroup (up to 4) and defensesBuildersGroup (at least 1 when >= 3 trucks)
// without endlessly swapping trucks between groups every 10 seconds
function swapBuildersRole() {
	const allBuilders = enumDroid(me, DROID_CONSTRUCT);
	const baseBuilders = enumGroup(baseBuildersGroup);
	const defBuilders = enumGroup(defensesBuildersGroup);

	const baseIds = new Set(baseBuilders.map(d => d.id));
	const defIds = new Set(defBuilders.map(d => d.id));

	// Assign any unassigned builder first
	for (const builder of allBuilders) {
		if (!baseIds.has(builder.id) && !defIds.has(builder.id)) {
			eventDroidBuilt(builder);
			baseIds.add(builder.id);
		}
	}

	const updatedBase = enumGroup(baseBuildersGroup);
	const updatedDef = enumGroup(defensesBuildersGroup);

	// If we have 3+ trucks total and 0 defense builders, move 1 base builder to defense builders
	if (allBuilders.length >= 3 && updatedDef.length === 0 && updatedBase.length > 0) {
		groupAdd(defensesBuildersGroup, updatedBase[updatedBase.length - 1]);
		return;
	}

	// If base builders dropped below 4 and defense builders has more than 1, move excess back to base builders
	if (updatedBase.length < 4 && updatedDef.length > 1) {
		const needed = Math.min(4 - updatedBase.length, updatedDef.length - 1);
		for (let i = 0; i < needed; i++) {
			groupAdd(baseBuildersGroup, updatedDef[i]);
		}
	}
}

// Check and update the alive allies
function checkAllies() {
	alliesList = getAliveAllyPlayersData();
}

// Check if Wally has been defeated
function checkIfDead() {
	currentlyDead = !isPlayerAlive(me);
}

// Ask for a truck if Wally is dead
function askForTruck() {
	if (currentlyDead) {
		chat(ALLIES, "need truck");
	}
}

// Set the nearest enemy player as enemy to attack
function setCurrentEnemy(enemyPlayerData) {
	enemiesList = getAliveEnemyPlayersData();

	if (enemyPlayerData && enemyPlayerData.position !== undefined) {
		currentEnemy = enemyPlayerData;
	} else if (enemiesList.length > 0) {
		// Prioritize normal players over scavengers if available, and pick the closest enemy to our base
		const regularEnemies = enemiesList.filter(e => e.position !== scavengerIndex);
		const pool = regularEnemies.length > 0 ? regularEnemies : enemiesList;

		pool.sort((a, b) => {
			if (!a.base || !b.base) return 0;
			return distBetweenTwoPoints(BASE.x, BASE.y, a.base.x, a.base.y) -
				   distBetweenTwoPoints(BASE.x, BASE.y, b.base.x, b.base.y);
		});

		currentEnemy = pool[0];
	} else {
		areAllEnemiesDead = true;
		shutdownScripts();
	}
}

function shutdownScripts() {
	removeTimer("produceDroids");
	removeTimer("research");
	removeTimer("build");
	removeTimer("attack");
	removeTimer("checkIsHighOilMap");
	removeTimer("setCurrentEnemy");
	removeTimer("lookForOil");
	removeTimer("myBaseInTrouble");
	removeTimer("checkAllyBaseInTrouble");
	removeTimer("recycleOldBuilders");
	removeTimer("checkTrucksCount");
	removeTimer("donateOil");
	removeTimer("checkAllies");
	removeTimer("checkForDefeatedAllies");
	removeTimer("swapBuildersRole");
	removeTimer("callIdleBuildersToBase");
	removeTimer("checkIfDead");
	removeTimer("askForTruck");
	removeTimer("scanForVTOLs");
	removeTimer("recycleObsoleteUnits");
}

function setDifficulty() {
	switch (difficulty) {
		case EASY:
			reactionSpeedDelay = 15000; // 15 Seconds
			break;
		case MEDIUM:
			reactionSpeedDelay = 5000; // 5 Seconds
			break;
		case HARD:
		case INSANE:
			reactionSpeedDelay = 0; // 0 Seconds - normal speed
			break;
		default:
			break;
	}
}

function isMultiTech4() {
	if (getMultiTechLevel() === 4) {
		researchDone = true;
		recycleOldBuilders();
	}
}

function eventDroidBuilt(droid) {
	if (!droid || droid.type != DROID) {
		return;
	}

	if (droid.droidType === DROID_CONSTRUCT) {
		const baseBuildersCount = enumGroup(baseBuildersGroup).length;
		const defBuildersCount = enumGroup(defensesBuildersGroup).length;

		// Ensure we always have at least 1 dedicated defense builder as soon as we have 3+ trucks,
		// and assign all trucks beyond 4 to defensesBuildersGroup so base defenses are never neglected
		if (baseBuildersCount >= 4 || (baseBuildersCount >= 2 && defBuildersCount === 0)) {
			groupAdd(defensesBuildersGroup, droid);
		} else {
			groupAdd(baseBuildersGroup, droid);
		}

		return;
	}

	if (droid.droidType === DROID_WEAPON && droid.weapons && droid.weapons[0] && droid.weapons[0].name === BUNKER_BUSTER_WEAPON[0]) {
		groupAdd(bunkerBusterGroup, droid);
		retreatToBase([droid]);
		return;
	}

	if (droid.isVTOL === true) {
		groupAdd(vtolGroup, droid);
		return;
	}

	if (droid.droidType === DROID_WEAPON || droid.droidType === DROID_SENSOR || droid.droidType === DROID_CYBORG) {
		groupAdd(attackerGroup, droid);
		return;
	}
}

function eventObjectTransfer(obj, from) {
	if (obj.type === DROID && from !== me) {
		// If Wally was defeated, update the base position to the ally who donated the truck
		if (currentlyDead) {
			BASE = startPositions[from];
		}
		let droid = obj;
		eventDroidBuilt(droid);
		recycleOldBuilders();
		chat(from, "Thank you for the gift <3");
	}
}

// The parameter "to" is always "me"
function eventBeacon(x, y, from, to, message) {
	if (from === me || !friendlyPlayer(from)) {
		return;
	}

	// Check if the beacon is near an ally's base (ally requesting defense)
	const ally = alliesList.find(a => a.position === from);
	if (ally && ally.base && distBetweenTwoPoints(x, y, ally.base.x, ally.base.y) <= BASE_THREAT_RANGE * 1.5) {
		if (!isHelpingAlly && !isMyBaseInTrouble) {
			helpAlly(ally);
		}
		return;
	}

	// Otherwise, check if the beacon points at an enemy base or enemy units (ally requesting coordinated attack)
	const nearbyEnemies = enumRange(x, y, 20, ENEMIES, false);
	if (nearbyEnemies.length > 0) {
		const targetPlayer = nearbyEnemies[0].player;
		const enemyData = isScavengerPlayer(targetPlayer) ? createScavengerPlayerData(targetPlayer) : playerDataObject(targetPlayer);
		if (enemyData) {
			setCurrentEnemy(enemyData);
		}
	}
}
