var enemiesList = [];

function eventStartLevel() {
	// Greets the player
	chat(ALLIES, "Hello! I'm Wally, your best defensive ally. I'll help you win this game.");

	// setup groups
	baseBuildersGroup = newGroup();
	oilGrabbersGroup = newGroup();
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

	// Initialize high/low oil classification at tick 0 instead of waiting 30s
	checkIsHighOilMap();

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

	// Set responsive timers (matched to Cobra cadence for fast reaction on low oil / no bases)
	setTimer("produceDroids", reactionSpeedDelay + 450 + ((1 + random(15)) * random(5))); // ~0.5 Seconds
	setTimer("research", reactionSpeedDelay + 800 + ((1 + random(15)) * random(8))); // ~0.8 Seconds
	setTimer("build", reactionSpeedDelay + 350 + ((1 + random(12)) * random(10))); // ~0.4 Seconds
	setTimer("attack", reactionSpeedDelay + 800 + ((1 + random(20)) * random(8))); // ~0.8 Seconds
	setTimer("checkIsHighOilMap", reactionSpeedDelay + 30000 + ((1 + random(30)) * random(10))); // 30 Seconds
	setTimer("setCurrentEnemy", reactionSpeedDelay + 180000 + ((1 + random(30)) * random(10))); // 3 Minutes
	setTimer("lookForOil", reactionSpeedDelay + 900 + ((1 + random(15)) * random(8))); // ~0.9 Seconds
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
	if (oilGrabbersGroup === undefined) oilGrabbersGroup = newGroup();
	if (defensesBuildersGroup === undefined) defensesBuildersGroup = newGroup();
	if (attackerGroup === undefined) attackerGroup = newGroup();
	if (bunkerBusterGroup === undefined) bunkerBusterGroup = newGroup();
	if (helperGroup === undefined) helperGroup = newGroup();
	if (vtolGroup === undefined) vtolGroup = newGroup();

	isSeaMap = isHoverMap();
	setDifficulty();
	isMultiTech4();
	checkIsHighOilMap();

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

// Balance builders cleanly across baseBuildersGroup (2-4), dedicated oilGrabbersGroup (up to 2-3), and defensesBuildersGroup
function swapBuildersRole() {
	const allBuilders = enumDroid(me, DROID_CONSTRUCT);
	const baseBuilders = enumGroup(baseBuildersGroup);
	const oilGrabbers = enumGroup(oilGrabbersGroup);
	const defBuilders = enumGroup(defensesBuildersGroup);

	const baseIds = new Set(baseBuilders.map(d => d.id));
	const oilIds = new Set(oilGrabbers.map(d => d.id));
	const defIds = new Set(defBuilders.map(d => d.id));

	// Assign any unassigned builder first
	for (const builder of allBuilders) {
		if (!baseIds.has(builder.id) && !oilIds.has(builder.id) && !defIds.has(builder.id)) {
			eventDroidBuilt(builder);
			return;
		}
	}

	const updatedBase = enumGroup(baseBuildersGroup);
	const updatedOil = enumGroup(oilGrabbersGroup);
	const updatedDef = enumGroup(defensesBuildersGroup);

	// Always keep up to 2 base builders when we have 2+ trucks
	if (allBuilders.length >= 2 && updatedBase.length < 2) {
		const donor = updatedDef.length > 0 ? updatedDef[updatedDef.length - 1] : updatedOil[updatedOil.length - 1];
		if (donor) {
			groupAdd(baseBuildersGroup, donor);
		}
		return;
	}

	// Ensure at least 1 dedicated oil grabber as soon as we have 3+ trucks
	if (allBuilders.length >= 3 && updatedOil.length === 0) {
		const donor = updatedDef.length > 0 ? updatedDef[updatedDef.length - 1] : (updatedBase.length > 2 ? updatedBase[updatedBase.length - 1] : null);
		if (donor) {
			groupAdd(oilGrabbersGroup, donor);
		}
		return;
	}

	// Once we have 5+ trucks, ensure at least 1 dedicated defense builder
	if (allBuilders.length >= 5 && updatedDef.length === 0) {
		const donor = updatedBase.length > 2 ? updatedBase[updatedBase.length - 1] : (updatedOil.length > 2 ? updatedOil[updatedOil.length - 1] : null);
		if (donor) {
			groupAdd(defensesBuildersGroup, donor);
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

// Set the nearest enemy player (including scavengers) as enemy to attack
function setCurrentEnemy(enemyPlayerData) {
	enemiesList = getAliveEnemyPlayersData();

	if (enemyPlayerData && enemyPlayerData.position !== undefined) {
		currentEnemy = enemyPlayerData;
	} else if (enemiesList.length > 0) {
		// Treat all enemies (including scavengers) equally and pick the closest enemy to our base
		enemiesList.sort((a, b) => {
			if (!a.base || !b.base) return 0;
			return distBetweenTwoPoints(BASE.x, BASE.y, a.base.x, a.base.y) -
				   distBetweenTwoPoints(BASE.x, BASE.y, b.base.x, b.base.y);
		});

		currentEnemy = enemiesList[0];
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
		const oilGrabbersCount = enumGroup(oilGrabbersGroup).length;
		const defBuildersCount = enumGroup(defensesBuildersGroup).length;
		const targetEarlyOilGrabbers = isHighOilMap ? 3 : 2;

		// Keep first 2 trucks at base, dedicate trucks 3-4 (+6th) to oil capture, and 5th+ to defenses/base
		if (baseBuildersCount < 2) {
			groupAdd(baseBuildersGroup, droid);
		} else if (oilGrabbersCount < targetEarlyOilGrabbers) {
			groupAdd(oilGrabbersGroup, droid);
		} else if (defBuildersCount === 0) {
			groupAdd(defensesBuildersGroup, droid);
		} else if (oilGrabbersCount < 3) {
			groupAdd(oilGrabbersGroup, droid);
		} else if (baseBuildersCount < 4) {
			groupAdd(baseBuildersGroup, droid);
		} else {
			groupAdd(defensesBuildersGroup, droid);
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
		const enemyData = playerDataObject(targetPlayer);
		if (enemyData) {
			setCurrentEnemy(enemyData);
		}
	}
}

// Chain nearby oil resources immediately after finishing a derrick and defend remote derricks (Cobra & SemperFi strategy)
function eventStructureBuilt(structure, droid) {
	if (!droid || droid.player !== me || droid.droidType !== DROID_CONSTRUCT) {
		return;
	}
	if (typeof pendingBuildOrders !== "undefined") {
		pendingBuildOrders.delete(droid.id);
	}

	// 1. Cobra strategy: check for nearby free oil resources within 10 tiles of the truck and build immediately
	const nearbyOils = enumRange(droid.x, droid.y, 10, ALL_PLAYERS, false)
		.filter(obj =>
			obj.type === FEATURE &&
			(obj.stat === OIL_RES_STAT || (typeof OIL_RESOURCE !== "undefined" && obj.stattype === OIL_RESOURCE)) &&
			isOilAreaSafe(obj.x, obj.y) &&
			!isOilTargetedByPendingBuild(obj.x, obj.y)
		)
		.sort((a, b) => distBetweenTwoPoints(droid.x, droid.y, a.x, a.y) - distBetweenTwoPoints(droid.x, droid.y, b.x, b.y));

	if (nearbyOils.length > 0) {
		const nextOil = nearbyOils[0];
		if (orderDroidBuild(droid, DORDER_BUILD, DERRICK_STAT, nextOil.x, nextOil.y)) {
			if (typeof pendingBuildOrders !== "undefined") {
				pendingBuildOrders.set(droid.id, {
					stat: DERRICK_STAT,
					x: nextOil.x,
					y: nextOil.y,
					time: gameTime
				});
			}
			return;
		}
	}

	// 2. Cobra & SemperFi strategy: if we just built a remote derrick outside our base, defend it right on the spot
	if (structure && !isNearBase(structure.x, structure.y)) {
		scanAndDefendPosition(structure, droid);
	}
}

// Contest oil wells stolen by enemy trucks right in front of us, or grab nearby oil (Cobra & SemperFi strategy)
function eventDroidIdle(droid) {
	if (!droid || droid.player !== me || droid.droidType !== DROID_CONSTRUCT) {
		return;
	}
	if (typeof pendingBuildOrders !== "undefined") {
		pendingBuildOrders.delete(droid.id);
	}

	// 1. Check if there is another free safe oil resource within 8 tiles of this truck
	const nearbyOils = enumRange(droid.x, droid.y, 8, ALL_PLAYERS, false)
		.filter(obj =>
			obj.type === FEATURE &&
			(obj.stat === OIL_RES_STAT || (typeof OIL_RESOURCE !== "undefined" && obj.stattype === OIL_RESOURCE)) &&
			isOilAreaSafe(obj.x, obj.y) &&
			!isOilTargetedByPendingBuild(obj.x, obj.y)
		)
		.sort((a, b) => distBetweenTwoPoints(droid.x, droid.y, a.x, a.y) - distBetweenTwoPoints(droid.x, droid.y, b.x, b.y));

	if (nearbyOils.length > 0) {
		const nextOil = nearbyOils[0];
		if (orderDroidBuild(droid, DORDER_BUILD, DERRICK_STAT, nextOil.x, nextOil.y)) {
			if (typeof pendingBuildOrders !== "undefined") {
				pendingBuildOrders.set(droid.id, {
					stat: DERRICK_STAT,
					x: nextOil.x,
					y: nextOil.y,
					time: gameTime
				});
			}
			return;
		}
	}

	// 2. Most likely an enemy truck grabbed the oil before us: build a defense tower right next to it!
	let nearbyEnemies = enumRange(droid.x, droid.y, 7, ENEMIES, false);
	if (scavengerIndex !== undefined) {
		nearbyEnemies = nearbyEnemies.concat(enumRange(droid.x, droid.y, 7, scavengerIndex, false));
	}
	const hasEnemyDerrickOrTruck = nearbyEnemies.some(obj =>
		(obj.type === STRUCTURE && obj.stattype === RESOURCE_EXTRACTOR) ||
		(obj.type === DROID && obj.droidType === DROID_CONSTRUCT)
	);

	if (hasEnemyDerrickOrTruck && !isNearBase(droid.x, droid.y)) {
		scanAndDefendPosition(undefined, droid);
	}
}
