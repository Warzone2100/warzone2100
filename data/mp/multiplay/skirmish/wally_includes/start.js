var enemiesList = [];
var scanForVTOLsTimerActive = false;

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
	aaGroup = newGroup();
	rallyGroup = newGroup();

	// Classify the map from its oil per player at tick 0 (before any truck is assigned a role)
	checkIsHighOilMap();
	checkVtolsEnabled();

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

	// Set responsive timers (matched to Cobra cadence for fast reaction on low oil / no bases)
	setTimer("produceDroids", reactionSpeedDelay + 450 + ((1 + random(15)) * random(5))); // ~0.5 Seconds
	setTimer("research", reactionSpeedDelay + 800 + ((1 + random(15)) * random(8))); // ~0.8 Seconds
	setTimer("build", reactionSpeedDelay + 350 + ((1 + random(12)) * random(10))); // ~0.4 Seconds
	setTimer("attack", reactionSpeedDelay + 800 + ((1 + random(20)) * random(8))); // ~0.8 Seconds
	setTimer("checkIsHighOilMap", reactionSpeedDelay + 30000 + ((1 + random(30)) * random(10))); // 30 Seconds
	setTimer("setCurrentEnemy", reactionSpeedDelay + 60000 + ((1 + random(30)) * random(10))); // 1 Minute
	setTimer("lookForOil", reactionSpeedDelay + 900 + ((1 + random(15)) * random(8))); // ~0.9 Seconds
	setTimer("myBaseInTrouble", reactionSpeedDelay + 5000 + ((1 + random(30)) * random(10))); // 5 Seconds
	setTimer("checkAllyBaseInTrouble", reactionSpeedDelay + 15000 + ((1 + random(30)) * random(10))); // 15 Seconds
	setTimer("recycleOldBuilders", reactionSpeedDelay + 20000 + ((1 + random(30)) * random(10))); // 20 Seconds
	setTimer("checkTrucksCount", reactionSpeedDelay + 120000 + ((1 + random(30)) * random(10))); // 2 Minutes
	setTimer("donateOil", reactionSpeedDelay + 120000 + ((1 + random(30)) * random(10))); // 2 Minutes
	setTimer("checkAllies", reactionSpeedDelay + 120000 + ((1 + random(30)) * random(10))); // 2 Minutes
	setTimer("checkForDefeatedAllies", reactionSpeedDelay + 20000 + ((1 + random(33)) * random(88))); // 20 Seconds
	setTimer("swapBuildersRole", reactionSpeedDelay + 10000 + ((1 + random(30)) * random(10))); // 10 Seconds
	setTimer("callIdleBuildersToBase", reactionSpeedDelay + 5000 + ((1 + random(30)) * random(10))); // 5 Seconds
	setTimer("checkIfDead", reactionSpeedDelay + 15000 + ((1 + random(30)) * random(10))); // 15 Seconds
	setTimer("askForTruck", reactionSpeedDelay + 120000 + ((1 + random(30)) * random(10))); // 2 Minutes
	setTimer("scanForVTOLs", reactionSpeedDelay + 15000 + ((1 + random(30)) * random(10))); // 15 Seconds
	scanForVTOLsTimerActive = true;
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
	if (aaGroup === undefined) aaGroup = newGroup();
	if (rallyGroup === undefined) rallyGroup = newGroup();

	isSeaMap = isHoverMap();
	setDifficulty();
	isMultiTech4();
	checkIsHighOilMap();
	checkVtolsEnabled();

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
		...enumGroup(vtolGroup).map(d => d.id),
		...enumGroup(aaGroup).map(d => d.id),
		...enumGroup(rallyGroup).map(d => d.id)
	]);
	for (const droid of enumDroid(me)) {
		if (droid.droidType !== DROID_CONSTRUCT && !assignedCombatIds.has(droid.id)) {
			eventDroidBuilt(droid);
		}
	}
}

// Returns target truck split across Group A (base), Group B (oil), and Group C (forward army support)
function getTargetBuilderSplit(total) {
	if (total <= 1) {
		return { base: total, oil: 0, defense: 0 };
	}
	const oil = (isHighOilMap && total >= 10) ? 3 : (total >= 7 ? 2 : 1);
	const defense = total >= 11 ? 3 : (total >= 8 ? 2 : (total >= 5 ? 1 : 0));
	const base = Math.max(1, total - oil - defense);
	return { base: base, oil: oil, defense: defense };
}

function isTruckInGroup(droid, groupId) {
	return Boolean(droid && groupId !== undefined && enumGroup(groupId).some(d => d.id === droid.id));
}

// Balance builders cleanly across Group A (baseBuildersGroup), Group B (oilGrabbersGroup), and Group C (defensesBuildersGroup)
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
	const split = getTargetBuilderSplit(allBuilders.length);

	const pickDonor = (list) => {
		if (!list || list.length === 0) return null;
		return list.find(d => typeof isTruckBusy === "function" && !isTruckBusy(d)) || list[list.length - 1];
	};

	if (updatedBase.length < split.base) {
		let donor = null;
		if (updatedDef.length > split.defense) {
			donor = pickDonor(updatedDef);
		} else if (updatedOil.length > split.oil) {
			donor = pickDonor(updatedOil);
		} else if (updatedBase.length < Math.min(2, split.base)) {
			if (updatedDef.length > 0) {
				donor = pickDonor(updatedDef);
			} else if (updatedOil.length > 1) {
				donor = pickDonor(updatedOil);
			}
		}
		if (donor) {
			groupAdd(baseBuildersGroup, donor);
		}
		return;
	}

	if (updatedOil.length < split.oil) {
		const donor = updatedBase.length > split.base
			? pickDonor(updatedBase)
			: (updatedDef.length > split.defense ? pickDonor(updatedDef) : null);
		if (donor) {
			groupAdd(oilGrabbersGroup, donor);
		}
		return;
	}

	if (updatedDef.length < split.defense) {
		const donor = updatedBase.length > split.base
			? pickDonor(updatedBase)
			: (updatedOil.length > split.oil ? pickDonor(updatedOil) : null);
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
		// Human teammates read the quick chat line more easily than free text
		if (typeof quickChat === "function" && typeof WzQuickChatMessages !== "undefined") {
			quickChat(ALLIES, WzQuickChatMessages.TEAM_REQUEST_TRANSFER_REBUILD);
		}
	}
}

// While gameTime is below this, the main enemy was chosen by a teammate (beacon) and is not re-evaluated
var enemyLockUntil = 0;

// Allied (not our own) ground units operating around an enemy's base: the sign that our team is already fighting that enemy
function countAlliedPressure(enemy) {
	if (!enemy || !enemy.base) {
		return 0;
	}
	return enumRange(enemy.base.x, enemy.base.y, 22, ALLIES, false).filter(
		o => o.type === DROID && o.player !== me && !o.isVTOL && (o.droidType === DROID_WEAPON || o.droidType === DROID_CYBORG)
	).length;
}

// Lower is better: the distance from our base, minus up to 40 tiles for an enemy our allies are already attacking.
// Scavengers receive a large +5000 penalty so real enemy players always take priority.
function scoreEnemy(enemy) {
	const dist = (enemy && enemy.base) ? distBetweenTwoPoints(BASE.x, BASE.y, enemy.base.x, enemy.base.y) : 9999;
	const scavPenalty = (enemy && isScavengerPlayer(enemy.position)) ? 5000 : 0;
	return dist + scavPenalty - Math.min(10, countAlliedPressure(enemy)) * 4;
}

// Chooses the main enemy for our offensives.
// With an argument (ally beacon / explicit request) that enemy is taken, optionally locked for `lockMs`
// (unless it is Scavengers while real enemy players remain alive).
// Without one, Wally picks from living real enemy players first and only falls back to Scavengers once no real enemies remain.
function setCurrentEnemy(enemyPlayerData, lockMs) {
	enemiesList = getAliveEnemyPlayersData();
	const realEnemies = enemiesList.filter(e => !isScavengerPlayer(e.position));
	const candidatePool = realEnemies.length > 0 ? realEnemies : enemiesList;

	if (enemyPlayerData && enemyPlayerData.position !== undefined) {
		if (!isScavengerPlayer(enemyPlayerData.position) || realEnemies.length === 0) {
			currentEnemy = enemyPlayerData;
			if (typeof lockMs === "number" && lockMs > 0) {
				enemyLockUntil = gameTime + lockMs;
			}
			return;
		}
	}

	if (candidatePool.length === 0) {
		areAllEnemiesDead = true;
		shutdownScripts();
		return;
	}

	const current = (currentEnemy && currentEnemy.position !== undefined)
		? candidatePool.find(e => e.position === currentEnemy.position)
		: undefined;
	if (current && gameTime < enemyLockUntil && gameTime > enemyLockUntil - 600000) {
		currentEnemy = current;
		return;
	}

	let best = candidatePool[0];
	let bestScore = scoreEnemy(best);
	for (let i = 1; i < candidatePool.length; ++i) {
		const score = scoreEnemy(candidatePool[i]);
		if (score < bestScore) {
			best = candidatePool[i];
			bestScore = score;
		}
	}

	// Refresh the data of the current enemy (its base position can change); switch only for a clear improvement
	if (current && (best.position === current.position || bestScore > scoreEnemy(current) - 15)) {
		currentEnemy = current;
	} else {
		currentEnemy = best;
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
	if (scanForVTOLsTimerActive) {
		removeTimer("scanForVTOLs");
		scanForVTOLsTimerActive = false;
	}
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
		const totalBuilders = enumDroid(me, DROID_CONSTRUCT).length;
		const split = getTargetBuilderSplit(totalBuilders);
		const baseBuildersCount = enumGroup(baseBuildersGroup).length;
		const oilGrabbersCount = enumGroup(oilGrabbersGroup).length;
		const defBuildersCount = enumGroup(defensesBuildersGroup).length;

		// Assign 1st truck to base, keep small Group B for oils, majority to Group A (base), remaining split to Group C (forward support)
		if (baseBuildersCount >= 1 && oilGrabbersCount < split.oil) {
			groupAdd(oilGrabbersGroup, droid);
		} else if (baseBuildersCount < split.base) {
			groupAdd(baseBuildersGroup, droid);
		} else if (defBuildersCount < split.defense) {
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

	// Dedicated anti-air tanks escort the army from behind instead of charging with it (see manageAntiAir in attack.js)
	if (isAntiAirDroid(droid)) {
		groupAdd(aaGroup, droid);
		return;
	}

	if (droid.droidType === DROID_WEAPON || droid.droidType === DROID_SENSOR || droid.droidType === DROID_CYBORG) {
		// While the army is away, fresh units wait at base and join the front as a batch instead of one by one
		if (droid.droidType !== DROID_SENSOR && shouldHoldReinforcements()) {
			groupAdd(rallyGroup, droid);
		} else {
			groupAdd(attackerGroup, droid);
		}
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
		// A human teammate is always followed. Between AI teammates the lower player number leads,
		// so two bots announcing different targets do not keep swapping enemies with each other.
		if (playerData[from] && playerData[from].isAI && from > me) {
			return;
		}
		const nonScavObj = nearbyEnemies.find(obj => !isScavengerPlayer(obj.player));
		const hasRealEnemies = enemiesList.some(e => !isScavengerPlayer(e.position) && isPlayerAlive(e.position));
		if (!nonScavObj && hasRealEnemies) {
			return; // Never lock the main offensive onto Scavengers while real enemy players are still alive
		}
		const targetPlayer = (nonScavObj || nearbyEnemies[0]).player;
		const enemyData = playerDataObject(targetPlayer);
		if (enemyData) {
			setCurrentEnemy(enemyData, 300000); // Stay on the requested enemy for 5 minutes
		}
	}
}

// Chain nearby oil resources immediately after finishing a derrick and defend remote derricks (Cobra & SemperFi strategy)
function eventStructureBuilt(structure, droid) {
	// A finished Research Lab (or its module) starts working at once instead of waiting for the research timer
	if (structure && structure.player === me && structure.stattype === RESEARCH_LAB && reactionSpeedDelay === 0) {
		research();
	}

	if (!droid || droid.player !== me || droid.droidType !== DROID_CONSTRUCT) {
		return;
	}
	if (typeof pendingBuildOrders !== "undefined") {
		pendingBuildOrders.delete(droid.id);
	}

	const inOilGroup = isTruckInGroup(droid, oilGrabbersGroup) ||
		(countDroid(DROID_CONSTRUCT, me) === 1 && countStruct(DERRICK_STAT) === 0);

	if (inOilGroup) {
		// 1. Group B chains nearby free oil resources within 10 tiles immediately
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
				setPendingOrder(droid, DERRICK_STAT, nextOil.x, nextOil.y);
				return;
			}
		}

		// 2. Defend contested derricks immediately, and on low-oil maps also fortify uncontested oil derricks beyond core base
		if (structure && structure.stattype === RESOURCE_EXTRACTOR && isTruckInGroup(droid, oilGrabbersGroup)) {
			const hasEnemyOilContest = enumRange(droid.x, droid.y, 8, ENEMIES, false).some(obj =>
				(obj.type === STRUCTURE && obj.stattype === RESOURCE_EXTRACTOR) ||
				(obj.type === DROID && obj.droidType === DROID_CONSTRUCT)
			);
			const shouldDefendLowOilDerrick = !isHighOilMap &&
				distBetweenTwoPoints(BASE.x, BASE.y, structure.x, structure.y) > 8;
			if (hasEnemyOilContest || shouldDefendLowOilDerrick) {
				scanAndDefendPosition(structure, droid);
			}
		}
	}
}

// Contest oil wells stolen by enemy trucks right in front of Group B or chain nearby oil (Cobra & SemperFi strategy)
function eventDroidIdle(droid) {
	if (!droid || droid.player !== me || droid.droidType !== DROID_CONSTRUCT) {
		return;
	}
	if (typeof pendingBuildOrders !== "undefined") {
		pendingBuildOrders.delete(droid.id);
	}

	const inOilGroup = isTruckInGroup(droid, oilGrabbersGroup) ||
		(countDroid(DROID_CONSTRUCT, me) === 1 && countStruct(DERRICK_STAT) === 0);
	if (!inOilGroup) {
		return;
	}

	// 1. Check if there is another free safe oil resource within 8 tiles of this Group B truck
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
			setPendingOrder(droid, DERRICK_STAT, nextOil.x, nextOil.y);
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

	if (hasEnemyDerrickOrTruck && !isNearBase(droid.x, droid.y) && isTruckInGroup(droid, oilGrabbersGroup)) {
		scanAndDefendPosition(undefined, droid);
	}
}
