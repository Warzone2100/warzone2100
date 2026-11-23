// Maps structure.id -> assigned constructor droid.id (or null) for structures scheduled for demolition
const structuresBeingDemolished = new Map();
// Tracks whether capturable safe oil wells currently remain on the map so oilGrabbersGroup helps at base once all safe oils are taken
let hasFreeSafeOils = true;

// Oil the army should help secure: a free oil well on our side of the map that armed enemies are sitting on ({x, y, time})
var contestedOil;
// Game time of the last pending-order cleanup, so the cleanup runs at most once per game tick
let lastPendingCleanupTime = -1;
// "stat@sector" -> game time until which we stop searching a build spot there after a failed search
const placementBackoff = new Map();
// "x_y" -> {t, ok}: whether any of our trucks can reach that oil well (refreshed every minute)
const oilReachCache = new Map();

// Remembers that `truck` is on its way to build/demolish something at (x, y), so other trucks do not duplicate the job.
// The entry lives longer for far targets, so a long walk to a distant oil well is not forgotten half-way.
function setPendingOrder(truck, stat, x, y) {
	if (!truck || typeof pendingBuildOrders === "undefined" || !(pendingBuildOrders instanceof Map)) {
		return;
	}
	const dist = distBetweenTwoPoints(truck.x, truck.y, x, y);
	pendingBuildOrders.set(truck.id, {
		stat: stat,
		x: x,
		y: y,
		time: gameTime,
		ttl: Math.max(25000, Math.min(150000, 25000 + dist * 1500))
	});
}

// All of our structures that are not finished yet (shared by every build helper within the same half second)
function getUnfinishedStructures() {
	return cached("unfinishedStructs", 500, () => enumStruct(me).filter(s => s.status !== BUILT));
}

// Number of our defensive structures around the main base
function countBaseDefenses() {
	return cached("baseDefenses", 2000, () => enumRange(BASE.x, BASE.y, BASE_THREAT_RANGE, me, false)
		.filter(obj => obj.type === STRUCTURE && obj.stattype === DEFENSE).length);
}

// Upgrade mappings where old defenses become obsolete once the replacement is unlocked (evaluated lazily after all includes load)
function getObsoleteDefenseUpgrades() {
	return [
		{ new: "Emplacement-Rocket06-IDF", old: WEAK_DEFENSES },
		{ new: "Sys-SensoTowerWS", old: ["Sys-SensoTower02"] },
		{ new: "Emplacement-HvART-pit", old: ["Emplacement-Rocket06-IDF"] },
		{ new: "X-Super-MassDriver", old: ["X-Super-Cannon"] } // only relevant when an ally shares those technologies
	];
}

// Returns a Set of owned structure IDs that are obsolete and eligible for demolition
function getObsoleteStructureIds() {
	return cached("obsoleteStructIds", 3000, computeObsoleteStructureIds);
}

function computeObsoleteStructureIds() {
	const obsoleteIds = new Set();
	if (isSeaMap) {
		for (const cybFac of enumStruct(me, CYBORG_FACTORY_STAT)) {
			obsoleteIds.add(cybFac.id);
		}
	}
	for (const { new: newStruct, old: oldStructs } of getObsoleteDefenseUpgrades()) {
		if (isStructureAvailable(newStruct)) {
			for (const oldStat of oldStructs) {
				for (const s of enumStruct(me, oldStat)) {
					obsoleteIds.add(s.id);
				}
			}
		}
	}
	return obsoleteIds;
}

// Returns true if a structure is being demolished, scheduled for demolition, or obsolete
function isStructureBeingDemolished(structure, obsoleteIds) {
	if (!structure) {
		return false;
	}
	if (structuresBeingDemolished.has(structure.id)) {
		return true;
	}
	if (isSeaMap && structure.stattype === CYBORG_FACTORY) {
		return true;
	}
	const resolvedObsolete = obsoleteIds !== undefined ? obsoleteIds : getObsoleteStructureIds();
	if (resolvedObsolete.has(structure.id)) {
		return true;
	}
	const demolishers = cached("activeDemolishers", 0, () => enumDroid(me, DROID_CONSTRUCT).filter(d => d.order === DORDER_DEMOLISH));
	const hasActiveDemolisher = demolishers.some(
		d => distBetweenTwoPoints(d.x, d.y, structure.x, structure.y) <= 4
	);
	if (hasActiveDemolisher) {
		return true;
	}
	if (typeof pendingBuildOrders !== "undefined" && pendingBuildOrders instanceof Map) {
		for (const info of pendingBuildOrders.values()) {
			if (info.stat === "__DEMOLISH__" && distBetweenTwoPoints(info.x, info.y, structure.x, structure.y) <= 2) {
				return true;
			}
		}
	}
	return false;
}

// Returns the number of completed structures of a given type
function countCompletedStruct(stat, player = me) {
	return enumStruct(player, stat).filter(s => s.status === BUILT).length;
}

// Returns the number of derricks that are actually backed by completed Power Generators
function getPoweredDerrickCount() {
	return Math.min(countStruct(DERRICK_STAT, me), countCompletedStruct(POW_GEN_STAT, me) * 4);
}

// Cleans up expired, completed, or already-placed pending build orders and finished demolitions
function cleanupPendingBuildOrders() {
	// This is called from almost every build helper: do the real work only once per game tick
	if (lastPendingCleanupTime === gameTime) {
		return;
	}
	lastPendingCleanupTime = gameTime;

	for (const structId of structuresBeingDemolished.keys()) {
		if (getObject(STRUCTURE, me, structId) === null) {
			structuresBeingDemolished.delete(structId);
		}
	}

	if (typeof pendingBuildOrders === "undefined" || !(pendingBuildOrders instanceof Map)) {
		return;
	}

	const activeTrucks = new Map(enumDroid(me, DROID_CONSTRUCT).map(t => [t.id, t]));
	for (const [truckId, info] of pendingBuildOrders.entries()) {
		const truck = activeTrucks.get(truckId);
		if (!truck || gameTime - info.time > (info.ttl || 25000)) {
			pendingBuildOrders.delete(truckId);
			continue;
		}

		if (info.stat === "__HELPBUILD__") {
			const stillUnfinished = enumStruct(me).some(
				s => s.status !== BUILT && distBetweenTwoPoints(s.x, s.y, info.x, info.y) <= 2
			);
			if (!stillUnfinished || truck.order === DORDER_HELPBUILD || gameTime - info.time >= 2500) {
				pendingBuildOrders.delete(truckId);
			}
			continue;
		}

		if (info.stat === "__DEMOLISH__" || info.stat === "__MODULE__" || info.stat === "__RECOVER__") {
			if (truck.order === DORDER_DEMOLISH || truck.order === DORDER_BUILD || truck.order === DORDER_RECOVER || gameTime - info.time >= 2500) {
				pendingBuildOrders.delete(truckId);
			}
			continue;
		}

		const matchingStruct = enumStruct(me, info.stat).find(
			s => distBetweenTwoPoints(s.x, s.y, info.x, info.y) <= 2
		);

		// Once the foundation is on the map and the engine reflects DORDER_BUILD (or it's already BUILT), remove from pending
		if (matchingStruct) {
			if (matchingStruct.status === BUILT || truck.order === DORDER_BUILD || gameTime - info.time >= 2500) {
				pendingBuildOrders.delete(truckId);
			}
			continue;
		}

		// Allow 2500ms for Warzone 2100's queuedOrders (sendDroidInfo) to update truck.order to DORDER_BUILD in C++
		if (gameTime - info.time >= 2500 && truck.order !== DORDER_BUILD) {
			pendingBuildOrders.delete(truckId);
		}
	}
}

// Returns true if the constructor droid is currently busy or has a pending order queued in this tick
function isTruckBusy(mydroid) {
	if (!mydroid) {
		return true;
	}
	if (mydroid.order === DORDER_BUILD ||
		mydroid.order === DORDER_HELPBUILD ||
		mydroid.order === DORDER_LINEBUILD ||
		mydroid.order === DORDER_RECYCLE ||
		mydroid.order === DORDER_DEMOLISH ||
		mydroid.order === DORDER_RECOVER ||
		mydroid.order === DORDER_RTB) {
		return true;
	}
	cleanupPendingBuildOrders();
	if (typeof pendingBuildOrders !== "undefined" && pendingBuildOrders instanceof Map && pendingBuildOrders.has(mydroid.id)) {
		return true;
	}
	return false;
}

// Returns true when power generation or the core base is in a critical bootstrap state
function isPowerOrCoreBaseCritical() {
	return cached("powerOrCoreCritical", 0, computePowerOrCoreBaseCritical);
}

function computePowerOrCoreBaseCritical() {
	const completedHQ = countCompletedStruct(PLAYER_HQ_STAT);
	const completedGens = countCompletedStruct(POW_GEN_STAT);
	const completedLabs = countCompletedStruct(RES_LAB_STAT);
	const completedFacs = countCompletedStruct(FACTORY_STAT);
	const neededGens = Math.max(1, Math.ceil(countStruct(DERRICK_STAT) / 4));
	const hasUnfinishedGen = enumStruct(me, POW_GEN_STAT).some(s => s.status !== BUILT);

	// Critical until our core base (HQ, 1st Power Generator, 1st Research Lab, and 1st Factory) is completed
	if (completedHQ === 0 || completedGens === 0 || (!researchDone && completedLabs === 0) || completedFacs === 0) {
		return true;
	}
	// Critical if a Power Generator is currently being built and power is below 350
	if (hasUnfinishedGen && getRealPower() < 350) {
		return true;
	}
	// Critical if we have unpowered derricks and either haven't queued enough generators or power is low
	if (numUnusedDerricks() > 0 && (countStructWithPending(POW_GEN_STAT) < neededGens || getRealPower() < 350)) {
		return true;
	}
	return false;
}

// Returns the number of existing structures of a given type PLUS any truck currently walking to place its foundation
function countStructWithPending(stat) {
	cleanupPendingBuildOrders();
	let count = countStruct(stat, me);
	if (typeof pendingBuildOrders === "undefined" || !(pendingBuildOrders instanceof Map)) {
		return count;
	}

	for (const [, info] of pendingBuildOrders.entries()) {
		if (info.stat === stat) {
			const placed = enumStruct(me, info.stat).some(
				s => distBetweenTwoPoints(s.x, s.y, info.x, info.y) <= 2
			);
			if (!placed) {
				count++;
			}
		}
	}
	return count;
}

// Returns the number of trucks currently walking to place a new Oil Derrick foundation
function countPendingDerricks(maxDistFromBase) {
	cleanupPendingBuildOrders();
	if (typeof pendingBuildOrders === "undefined" || !(pendingBuildOrders instanceof Map)) {
		return 0;
	}

	let count = 0;
	for (const [, info] of pendingBuildOrders.entries()) {
		if (info.stat === DERRICK_STAT) {
			if (maxDistFromBase !== undefined && distBetweenTwoPoints(BASE.x, BASE.y, info.x, info.y) > maxDistFromBase) {
				continue;
			}
			const placed = enumStruct(me, DERRICK_STAT).some(
				s => distBetweenTwoPoints(s.x, s.y, info.x, info.y) <= 2
			);
			if (!placed) {
				count++;
			}
		}
	}
	return count;
}

// Build a certain number of a structure (Cobra's countAndBuild helper)
function countAndBuild(stat, count) {
	if (countStructWithPending(stat) < count) {
		const buildPos = (stat === REPAIR_FACILITY_STAT) ? getOuterBaseRepairPosition() : BASE;
		if (isStructureAvailable(stat) && grabTrucksAndBuild(stat, buildPos)) {
			return true;
		}
	}
	return false;
}

// Cobra's T2/T3/T4 No Bases build order (strangeBuildOrder)
function strangeBuildOrder() {
	const isNoBasesHighTech = getMultiTechLevel() > 1 && baseType === CAMP_CLEAN;
	if (!isNoBasesHighTech) {
		return false;
	}

	if (!isHighOilMap) {
		if (countAndBuild(FACTORY_STAT, 1) ||
			countAndBuild(POW_GEN_STAT, 1) ||
			(!isSeaMap && countAndBuild(CYBORG_FACTORY_STAT, 1)) ||
			countAndBuild(PLAYER_HQ_STAT, 1) ||
			(getMultiTechLevel() < 4 && !researchDone && countAndBuild(RES_LAB_STAT, 1)) ||
			(numUnusedDerricks() > 0 && countAndBuild(POW_GEN_STAT, countStruct(POW_GEN_STAT) + 1)) ||
			(getDesiredFactoryCount() >= 2 && countAndBuild(FACTORY_STAT, 2)) ||
			baseMaintenance() ||
			countAndBuild(RES_LAB_STAT, Math.min(2, getDesiredLabCount()))) {
			return true;
		}
	} else {
		if (countAndBuild(FACTORY_STAT, 2) ||
			countAndBuild(POW_GEN_STAT, 3) ||
			(getMultiTechLevel() < 4 && !researchDone && countAndBuild(RES_LAB_STAT, 2)) ||
			(numUnusedDerricks() > 0 && countAndBuild(POW_GEN_STAT, 7)) ||
			(!isSeaMap && countAndBuild(CYBORG_FACTORY_STAT, 1)) ||
			countAndBuild(PLAYER_HQ_STAT, 1) ||
			(getMultiTechLevel() < 4 && !researchDone && countAndBuild(RES_LAB_STAT, 3)) ||
			baseMaintenance() ||
			countAndBuild(FACTORY_STAT, 4) ||
			(!isSeaMap && countAndBuild(CYBORG_FACTORY_STAT, 5)) ||
			(numUnusedDerricks() > 0 && countAndBuild(POW_GEN_STAT, 9)) ||
			(getMultiTechLevel() < 4 && !researchDone && countAndBuild(RES_LAB_STAT, 5)) ||
			countAndBuild(FACTORY_STAT, 5) ||
			(numUnusedDerricks() > 0 && countAndBuild(POW_GEN_STAT, countStruct(POW_GEN_STAT) + 1))) {
			return true;
		}
	}

	return false;
}

// Returns true only in early T1 low-oil before we have our initial 4 combat units (or >6 derricks),
// so scarce early power goes to factories and the first army before non-emergency static towers
function shouldPrioritizeFirstArmyOverDefenses() {
	if (isMyBaseInTrouble || isHighOilMap || getMultiTechLevel() !== 1 || gameTime >= 480000) {
		return false;
	}
	return countStruct(DERRICK_STAT) <= 6 && enumGroup(attackerGroup).length < 4;
}

// ---- Low-oil economy scaling ----
// On low-oil maps every structure competes with the army for the same trickle of power, so the number of
// labs, factories and repair bays follows the number of derricks instead of fixed counts.

// How many Research Labs the current economy can keep busy
function getDesiredLabCount() {
	if (researchDone || getMultiTechLevel() === 4) {
		return 0;
	}
	if (isHighOilMap) {
		return 5;
	}
	const derricks = countStruct(DERRICK_STAT);
	let labs = 2;
	if (derricks >= 10) {
		labs = 5;
	} else if (derricks >= 8) {
		labs = 4;
	} else if (derricks >= 5) {
		labs = 3;
	}
	if (isSeaMap) {
		labs = Math.max(labs, 3); // hover maps are won by tech, not by early ground pushes
	}
	// A large unspent bank means the economy can afford more research than the derrick count suggests
	if (getRealPower() > 700) {
		labs = Math.min(5, labs + 1);
	}
	return labs;
}

// How many tank factories the current economy can feed
function getDesiredFactoryCount() {
	const derricks = countStruct(DERRICK_STAT);
	if (isHighOilMap || derricks >= 20) {
		return 5;
	}
	if (derricks >= 16) {
		return 4;
	}
	if (derricks >= 9) {
		return 3;
	}
	if (derricks >= 6 || getRealPower() > 500) {
		return 2;
	}
	return 1;
}

// Army size a low-oil economy should reach before research and static defenses get first call on power
function getLowOilArmyTarget() {
	return Math.max(6, Math.min(24, 4 + countStruct(DERRICK_STAT) * 2));
}

// True on low-oil maps while the ground army is still smaller than the economy can support
function isArmyThin() {
	if (isHighOilMap) {
		return false;
	}
	return cached("armyThin", 1000, () =>
		enumGroup(attackerGroup).length + enumGroup(rallyGroup).length < getLowOilArmyTarget());
}

// Handle Build following Cobra's buildOrders() progression
function build() {
	return profiled("build", buildTick);
}

function buildTick() {
	const trucks = enumDroid(me, DROID_CONSTRUCT);
	if (trucks.length === 0) {
		return;
	}

	// Nothing below can be assigned while every truck is already working: skip the whole (expensive) decision tree
	cleanupPendingBuildOrders();
	if (!trucks.some(t => !isTruckBusy(t))) {
		return;
	}

	// 1. Check unfinished structures first so base builders finish the current building together (Cobra's checkUnfinishedStructures)
	if (checkUncompletedStructures()) {
		return;
	}

	// 2. T2+ No Bases build order (Cobra's strangeBuildOrder)
	if (strangeBuildOrder()) {
		return;
	}

	// 3. Build basic base structures in Cobra's exact priority order (Cobra's buildBaseStructures)
	if (buildBasicBase()) {
		return;
	}

	// 4. Upgrade completed generators, factories, and research labs with modules (Cobra's maintenance)
	if (baseMaintenance()) {
		return;
	}

	// 5. Dedicated defense builders finish half-built defenses first, then build base/forward or Anti-VTOL defenses.
	// Defenses are never urgent to the millisecond: this lane runs about once per second to keep the script light.
	const defensesBuilders = enumGroup(defensesBuildersGroup);
	if (defensesBuilders.length >= 1 &&
		!throttled("defenseBuildersLane", 1200) &&
		!shouldPrioritizeFirstArmyOverDefenses() &&
		gameTime >= lastAttemptTime &&
		numUnusedDerricks() <= 0 &&
		getRealPower() > MIN_BUILD_POWER) {
		if (getUnfinishedSafeDefenses().length > 0) {
			checkUncompletedStructures(true);
		} else if (!(enemiesHaveVTOLs && shouldBuildAntiAirNow() && buildAntiAir(defensesBuilders))) {
			// While the low-oil army is still thin, a small base perimeter is enough: the power goes to units
			const holdBackForArmy = isArmyThin() && !isMyBaseInTrouble && countBaseDefenses() >= 3;
			const def = holdBackForArmy ? null : returnBestDefense();
			if (def) {
				const buildPos = getDefenseBuildPosition(defensesBuilders);
				grabTrucksAndBuild(def, buildPos, defensesBuilders, true);
			}
		}
	}

	// 6. Build remaining fundamental structures and defenses (Cobra's buildBaseStructures2 / buildExtras / buildSpecialStructures / buildDefenses)
	buildFundamentals();
}

// Cobra's exact base build order (buildBaseStructures in cobra_includes/build.js)
function buildBasicBase() {
	const goodPowerLevel = getRealPower() > 250;

	if (getMultiTechLevel() < 4 && !researchDone && isSeaMap && goodPowerLevel && countAndBuild(RES_LAB_STAT, 1)) {
		return true;
	}

	if (!isHighOilMap) {
		if ((goodPowerLevel && countAndBuild(FACTORY_STAT, 1)) ||
			((!goodPowerLevel || getMultiTechLevel() > 1) && countAndBuild(POW_GEN_STAT, 1)) ||
			countAndBuild(FACTORY_STAT, 1) ||
			(getMultiTechLevel() < 4 && !researchDone && countAndBuild(RES_LAB_STAT, 1)) ||
			countAndBuild(POW_GEN_STAT, 1) ||
			countAndBuild(PLAYER_HQ_STAT, 1) ||
			(!isSeaMap && countAndBuild(CYBORG_FACTORY_STAT, 1)) ||
			// A generator only pays off once it has derricks to burn: build one per 4 derricks, never ahead of them
			(numUnusedDerricks() > 0 && countAndBuild(POW_GEN_STAT, countStruct(POW_GEN_STAT) + 1)) ||
			// The power module is the cheapest income boost on low oil: take it before any second lab or factory
			(countStruct(POW_GEN_STAT) <= 3 && gameTime < 900000 && baseMaintenance(true)) ||
			countAndBuild(RES_LAB_STAT, Math.min(2, getDesiredLabCount())) ||
			(getDesiredFactoryCount() >= 2 && countAndBuild(FACTORY_STAT, 2)) ||
			countAndBuild(RES_LAB_STAT, Math.min(3, getDesiredLabCount())) ||
			countAndBuild(REPAIR_FACILITY_STAT, 1) ||
			(vtolsEnabled && countStruct(DERRICK_STAT) >= 8 && countAndBuild(VTOL_FACTORY_STAT, 1))) {
			return true;
		}
	} else {
		const haveAllies = alliesList.length > 0;

		if (((!goodPowerLevel || getMultiTechLevel() > 1) && countAndBuild(POW_GEN_STAT, 1)) ||
			(getRealPower() < 550 && countAndBuild(POW_GEN_STAT, 4)) ||
			countAndBuild(FACTORY_STAT, 2) ||
			(getMultiTechLevel() < 4 && !researchDone && countAndBuild(RES_LAB_STAT, haveAllies ? 2 : 4)) ||
			countAndBuild(POW_GEN_STAT, 1) ||
			countAndBuild(PLAYER_HQ_STAT, 1) ||
			countAndBuild(FACTORY_STAT, 3) ||
			(getMultiTechLevel() < 4 && !researchDone && countAndBuild(RES_LAB_STAT, 5)) ||
			(numUnusedDerricks() > 0 && countAndBuild(POW_GEN_STAT, 7)) ||
			(!isSeaMap && countAndBuild(CYBORG_FACTORY_STAT, 3)) ||
			countAndBuild(FACTORY_STAT, 5) ||
			(numUnusedDerricks() > 0 && countAndBuild(POW_GEN_STAT, 8)) ||
			(!isSeaMap && countAndBuild(CYBORG_FACTORY_STAT, 5)) ||
			// Keep adding generators while derricks are unpowered (up to the game limit), instead of stopping at 8
			(numUnusedDerricks() > 0 && countAndBuild(POW_GEN_STAT, countStruct(POW_GEN_STAT) + 1)) ||
			(goodPowerLevel && countAndBuild(REPAIR_FACILITY_STAT, 3))) {
			return true;
		}
	}

	if (vtolsEnabled && getMultiTechLevel() > 1 && countStruct(VTOL_FACTORY_STAT) > 0 && countAndBuild(VTOL_PAD_STAT, 3)) {
		return true;
	}

	return false;
}

// Returns unfinished defense structures in safe or base-adjacent areas that need completion
function getUnfinishedSafeDefenses() {
	const obsoleteIds = getObsoleteStructureIds();
	return getUnfinishedStructures().filter((s) =>
		s.stattype === DEFENSE &&
		!isStructureBeingDemolished(s, obsoleteIds) &&
		(isNearBase(s.x, s.y) || isAreaSafe(s.x, s.y, 12))
	);
}

// Ensures any structure marked for demolition or half-demolished obsolete structure finishes being demolished
function continuePendingDemolitions(obsoleteIds) {
	cleanupPendingBuildOrders();

	// Structures we already decided to demolish (looked up by id instead of scanning the whole base every tick)
	for (const structId of Array.from(structuresBeingDemolished.keys())) {
		const scheduled = getObject(STRUCTURE, me, structId);
		if (scheduled) {
			demolishThis(scheduled);
		}
	}

	// Half-demolished obsolete structures that lost their demolisher
	const unfinished = getUnfinishedStructures();
	for (let i = 0; i < unfinished.length; ++i) {
		const s = unfinished[i];
		if (!structuresBeingDemolished.has(s.id) && (obsoleteIds.has(s.id) || (isSeaMap && s.stattype === CYBORG_FACTORY))) {
			demolishThis(s);
		}
	}
}

// Send remaining idle trucks to finish uncompleted structures near the base, safe zones, or orphaned oil derricks
function checkUncompletedStructures(includeDefenseBuilders = false) {
	let hasAssignedTask = false;
	const obsoleteIds = getObsoleteStructureIds();
	continuePendingDemolitions(obsoleteIds);

	const unfinishedStructs = getUnfinishedStructures();
	if (unfinishedStructs.length === 0) {
		return false;
	}

	const activeBuilders = enumDroid(me, DROID_CONSTRUCT).filter(
		t => t.order === DORDER_BUILD || t.order === DORDER_HELPBUILD
	);

	// Include uncompleted base/defense structures in safe areas and orphaned oil derricks with no truck working on them
	const structlist = unfinishedStructs.filter((structure) => {
		if (isStructureBeingDemolished(structure, obsoleteIds)) {
			return false;
		}
		if (structure.stattype === RESOURCE_EXTRACTOR) {
			if (!isOilAreaSafe(structure.x, structure.y)) {
				return false;
			}
			return !activeBuilders.some(t => distBetweenTwoPoints(t.x, t.y, structure.x, structure.y) <= 3);
		}
		return distBetweenTwoPoints(BASE.x, BASE.y, structure.x, structure.y) < 40 || isAreaSafe(structure.x, structure.y, 12);
	}).sort((a, b) => {
		const getPriority = (s) => (isBaseStructure(s.stattype) ? 0 : (s.stattype === RESOURCE_EXTRACTOR ? 2 : 1));
		const diff = getPriority(a) - getPriority(b);
		return diff !== 0 ? diff : sortByDistToBase(a, b);
	});

	if (structlist.length === 0) {
		return false;
	}

	const oilIds = hasFreeSafeOils
		? new Set((typeof oilGrabbersGroup !== "undefined" ? enumGroup(oilGrabbersGroup) : []).map(d => d.id))
		: new Set();

	for (let s = 0; s < structlist.length; ++s) {
		const targetStruct = structlist[s];
		const useAllBuilders = includeDefenseBuilders || targetStruct.stattype === DEFENSE;
		let idleTrucks;

		if (targetStruct.stattype === RESOURCE_EXTRACTOR) {
			idleTrucks = findIdleTrucks(targetStruct, oilGrabbersGroup);
			if (idleTrucks.length === 0) {
				idleTrucks = findIdleTrucks(targetStruct);
			}
		} else if (useAllBuilders) {
			idleTrucks = enumDroid(me, DROID_CONSTRUCT)
				.filter(d => !oilIds.has(d.id) && canDroidHelp(d, targetStruct.x, targetStruct.y))
				.sort((a, b) => distBetweenTwoPoints(a.x, a.y, targetStruct.x, targetStruct.y) - distBetweenTwoPoints(b.x, b.y, targetStruct.x, targetStruct.y));
			if (idleTrucks.length === 0 && isPowerOrCoreBaseCritical()) {
				idleTrucks = enumDroid(me, DROID_CONSTRUCT)
					.filter(d => canDroidHelp(d, targetStruct.x, targetStruct.y))
					.sort((a, b) => distBetweenTwoPoints(a.x, a.y, targetStruct.x, targetStruct.y) - distBetweenTwoPoints(b.x, b.y, targetStruct.x, targetStruct.y));
			}
		} else {
			idleTrucks = findIdleTrucks(targetStruct, baseBuildersGroup);
			if (idleTrucks.length === 0) {
				idleTrucks = findIdleTrucks(targetStruct);
			}
		}

		if (idleTrucks.length === 0) {
			continue;
		}

		const maxTrucks = targetStruct.stattype === RESOURCE_EXTRACTOR ? Math.min(2, idleTrucks.length) : idleTrucks.length;
		for (let j = 0; j < maxTrucks; ++j) {
			if (orderDroidObj(idleTrucks[j], DORDER_HELPBUILD, targetStruct)) {
				setPendingOrder(idleTrucks[j], "__HELPBUILD__", targetStruct.x, targetStruct.y);
				hasAssignedTask = true;
			}
		}
	}

	return hasAssignedTask;
}

// Build modules on generators, factories, and research labs (Cobra's maintenance function)
function baseMaintenance(earlyGen = false) {
	// Check if there are no power generators.
	if (countStruct(POW_GEN_STAT) === 0) {
		return false;
	}

	const MIN_POWER_FOR_MODULE = -70;
	let struct = null;
	let module = "";
	let success = false;

	// Cobra's module priority order (on low oil: Power Module -> 1st Factory Module -> Research Module -> 2nd Factory Module)
	const moduleList = earlyGen
		? [
			{ "mod": "A0PowMod1", "amount": 1, "structure": POW_GEN_STAT }
		]
		: (!isHighOilMap
			? [
				{ "mod": "A0PowMod1", "amount": 1, "structure": POW_GEN_STAT },
				{ "mod": "A0FacMod1", "amount": 1, "structure": FACTORY_STAT },
				{ "mod": "A0ResearchModule1", "amount": 1, "structure": RES_LAB_STAT },
				{ "mod": "A0FacMod1", "amount": 2, "structure": FACTORY_STAT },
				{ "mod": "A0FacMod1", "amount": 2, "structure": VTOL_FACTORY_STAT }
			]
			: [
				{ "mod": "A0PowMod1", "amount": 1, "structure": POW_GEN_STAT },
				{ "mod": "A0ResearchModule1", "amount": 1, "structure": RES_LAB_STAT },
				{ "mod": "A0FacMod1", "amount": 2, "structure": FACTORY_STAT },
				{ "mod": "A0FacMod1", "amount": 2, "structure": VTOL_FACTORY_STAT }
			]);

	for (let i = 0; i < moduleList.length; ++i) {
		if (isStructureAvailable(moduleList[i].mod)) {
			if (moduleList[i].structure === VTOL_FACTORY_STAT && !componentAvailable("V-Tol")) {
				continue;
			}
			const structList = enumStruct(me, moduleList[i].structure).sort(sortByDistToBase);

			for (let c = 0; c < structList.length; c++) {
				if (structList[c].status === BUILT && structList[c].modules < moduleList[i].amount) {
					struct = structList[c];
					module = moduleList[i].mod;
					break;
				}
			}
			if (struct !== null) {
				break;
			}
		}
	}

	if (struct && (getRealPower() > MIN_POWER_FOR_MODULE || module === "A0PowMod1" || countStruct(DERRICK_STAT) >= 12)) {
		const idleTrucks = findIdleTrucks(struct, baseBuildersGroup);

		for (const truck of idleTrucks) {
			if (canDroidHelp(truck, struct.x, struct.y)) {
				if (orderDroidBuild(truck, DORDER_BUILD, module, struct.x, struct.y)) {
					setPendingOrder(truck, "__MODULE__", struct.x, struct.y);
					success = true;
					break;
				}
			}
		}
	}

	if (demolishCyborgFactories()) {
		success = true;
	}

	return success;
}

function buildFundamentals() {
	let vtols = enumGroup(vtolGroup);
	// One rearming pad per VTOL (like Cobra): with one pad for two, half of the air force was always queueing
	let needVtolPads = countStructWithPending(VTOL_PAD_STAT) < vtols.length;

	// Prioritize Research Labs first while we have fewer than the economy can keep busy (at most 3 here) so research never stalls
	if (!researchDone && countStructWithPending(RES_LAB_STAT) < Math.min(3, getDesiredLabCount()) && buildResearchLabs()) {
		return true;
	}

	if (factoryBuildOrder()) {
		return true;
	}

	if (buildResearchLabs()) {
		return true;
	}

	// Build VTOL pads if needed (pads are allowed between other structures, see getPlacementBlockingSteps)
	if (vtolsEnabled && needVtolPads && grabTrucksAndBuild(VTOL_PAD_STAT)) {
		return true;
	}

	if (isStructureAvailable(UPLINK_STAT) && countStructWithPending(UPLINK_STAT) === 0 && grabTrucksAndBuild(UPLINK_STAT)) {
		return true;
	}

	if (isStructureAvailable(LASERLINK_STAT) && countStructWithPending(LASERLINK_STAT) === 0 && grabTrucksAndBuild(LASERLINK_STAT)) {
		return true;
	}

	if (isStructureAvailable(REPAIR_FACILITY_STAT)) {
		if (buildRepairFacilities()) {
			return true;
		}
	}

	// Demolition and extra defenses are never urgent: evaluate this lane about once per second to keep the script light
	if (throttled("defenseFundamentalsLane", 1000)) {
		return false;
	}

	// Check if there are old defenses to demolish
	if (demolishOldDefenses()) {
		return true;
	}

	// "All fundamental buildings built -- proceed to build defenses"
	if (buildDefenses()) {
		return true;
	}

	return false;
}

function buildDefensesForAllies() {
	if (!isHighOilMap) {
		return false; // Only build defenses for allies on high oil maps
	}

	// Count all defenses near my base only once
	const myDefensesCount = enumRange(BASE.x, BASE.y, BASE_THREAT_RANGE, me, true)
		.filter((obj) => obj.stattype === DEFENSE).length;

	// Determine the maximum number of defenses to build for allies
	const maxDefensesForAllies = myDefensesCount >= 100 ? 100 : 30;

	// If I have fewer than 30 defenses, don't build any
	if (myDefensesCount < 30) {
		return false;
	}

	// Process each ally
	for (const ally of alliesList) {
		// Get the ally's base position
		const allyHQ = enumStruct(ally.position, PLAYER_HQ_STAT)[0];
		const allyBasePosition = allyHQ || startPositions[ally.position];

		// If an ally is too far or if my base is in trouble, skip this ally
		if (distBetweenTwoPoints(BASE.x, BASE.y, allyBasePosition.x, allyBasePosition.y) > 60 || isMyBaseInTrouble) {
			continue; // Skip ally and check next one
		}

		// Combine defenses from both my base and the ally's base in a single `enumRange` call
		const allAlliedDefenses = enumRange(
			allyBasePosition.x, allyBasePosition.y, BASE_THREAT_RANGE, ally.position, true
		)
			.concat(
				enumRange(allyBasePosition.x, allyBasePosition.y, BASE_THREAT_RANGE, me, true)
			)
			.filter((obj) => obj.stattype === DEFENSE).length;

		// Skip if the ally already has enough defenses
		if (allAlliedDefenses >= maxDefensesForAllies) {
			continue;
		}

		// If there's enough power, build a defense
		if (getRealPower() > MIN_BUILD_POWER) {
			const def = returnBestDefense();
			if (def !== null) {
				grabTrucksAndBuild(def, allyBasePosition);
				return true; // Exit once a defense is built
			}
		} else {
			return false; // Not enough power to build defenses
		}
	}

	return false; // Return false if no defenses were built for allies
}

// Builds Repair Facilities up to the maximum structure limit (5 in standard skirmish),
// placing them on the outer perimeter of the base (away from essential core buildings) and at safe forward positions.
function buildRepairFacilities() {
	const engineLimit = typeof getStructureLimit === "function" ? getStructureLimit(REPAIR_FACILITY_STAT, me) : 5;
	// Low-oil economies cannot afford a full set of repair bays: scale them with the derrick count
	const derrickCount = countStruct(DERRICK_STAT);
	const economyLimit = isHighOilMap ? 5 : (derrickCount >= 12 ? 3 : (derrickCount >= 8 ? 2 : 1));
	const maxLimit = Math.min(engineLimit, economyLimit);
	const currentCount = countStructWithPending(REPAIR_FACILITY_STAT);

	if (currentCount >= maxLimit) {
		return false;
	}

	// Build the first 1-2 repair facilities as soon as we have minimal build power; build up to the max limit when economy is healthy
	if (currentCount >= 2 && getRealPower() < MIN_BUILD_POWER) {
		return false;
	}

	// Count how many repair facilities we have around our base perimeter
	const basePerimeterRepairs = enumStruct(me, REPAIR_FACILITY_STAT).filter(
		r => distBetweenTwoPoints(BASE.x, BASE.y, r.x, r.y) <= BASE_THREAT_RANGE + 15
	);

	// Keep at least 3 Repair Facilities on the outer perimeter of our base (or up to maxLimit if no forward positions exist)
	if (basePerimeterRepairs.length < Math.min(3, maxLimit)) {
		const perimeterPos = getOuterBaseRepairPosition();
		if (grabTrucksAndBuild(REPAIR_FACILITY_STAT, perimeterPos)) {
			return true;
		}
	}

	// Try to build remaining Repair Facilities at forward protected derricks
	if (buildForwardRepairFacilities(maxLimit)) {
		return true;
	}

	// If no forward derrick is suitable (e.g. flat map), build the remaining Repair Facilities up to maxLimit on the base perimeter
	if (currentCount < maxLimit) {
		const perimeterPos = getOuterBaseRepairPosition();
		if (grabTrucksAndBuild(REPAIR_FACILITY_STAT, perimeterPos)) {
			return true;
		}
	}

	return false;
}

// Calculates a position outside the core base buildings (at ~14 tiles from BASE toward the front or map center)
// so Repair Facilities don't clutter factories, labs, and generators, and damaged units reach them faster.
function getOuterBaseRepairPosition() {
	const targetPos = (currentEnemy && currentEnemy.base)
		? currentEnemy.base
		: { x: Math.floor(mapWidth / 2), y: Math.floor(mapHeight / 2) };

	const totalDist = distBetweenTwoPoints(BASE.x, BASE.y, targetPos.x, targetPos.y);
	if (totalDist < 15) {
		return BASE;
	}

	const dx = (targetPos.x - BASE.x) / totalDist;
	const dy = (targetPos.y - BASE.y) / totalDist;
	// Perpendicular vector to spread multiple repair facilities along an arc outside the base
	const px = -dy;
	const py = dx;

	const existingCount = countStruct(REPAIR_FACILITY_STAT);
	const offsets = [0, 6, -6, 12, -12];
	const sideOffset = offsets[existingCount % offsets.length];
	const forwardOffset = 14; // 14 tiles away from core BASE

	const rx = Math.max(4, Math.min(mapWidth - 4, Math.round(BASE.x + dx * forwardOffset + px * sideOffset)));
	const ry = Math.max(4, Math.min(mapHeight - 4, Math.round(BASE.y + dy * forwardOffset + py * sideOffset)));

	return { x: rx, y: ry };
}

function buildForwardRepairFacilities(maxLimit = 5) {
	if (getRealPower() < MIN_BUILD_POWER || countStructWithPending(REPAIR_FACILITY_STAT) >= maxLimit) {
		return false;
	}

	// Get all remote derricks
	let remoteDerricks = enumStruct(me, DERRICK_STAT).filter(
		derrick => !isNearBase(derrick.x, derrick.y)
	);

	if (currentEnemy && currentEnemy.base) {
		const baseToEnemy = distBetweenTwoPoints(BASE.x, BASE.y, currentEnemy.base.x, currentEnemy.base.y);

		// Filter out derricks that are further from the enemy than we are (must be between us and them)
		remoteDerricks = remoteDerricks.filter(d =>
			distBetweenTwoPoints(d.x, d.y, currentEnemy.base.x, currentEnemy.base.y) < baseToEnemy
		);

		// Sort by distance to enemy base (closest to enemy first)
		remoteDerricks.sort((a, b) => {
			return distBetweenTwoPoints(a.x, a.y, currentEnemy.base.x, currentEnemy.base.y) -
				   distBetweenTwoPoints(b.x, b.y, currentEnemy.base.x, currentEnemy.base.y);
		});
	} else {
		// Fallback: sort by furthest from our base
		remoteDerricks.sort(sortByDistToBase).reverse();
	}

	for (const derrick of remoteDerricks) {
		// Only build forward base if derrick is protected (has at least 1 defense nearby) and the area is safe
		const hasDefenses = enumRange(derrick.x, derrick.y, 15, me, false)
			.filter(obj => obj.type === STRUCTURE && obj.stattype === DEFENSE).length >= 1;

		if (hasDefenses && isAreaSafe(derrick.x, derrick.y, 18)) {
			// Check if we already have a repair facility nearby
			const nearbyRepairFacilities = enumRange(derrick.x, derrick.y, 18, me, false)
				.filter(obj => obj.type === STRUCTURE && obj.stattype === REPAIR_FACILITY).length;

			if (nearbyRepairFacilities === 0 && countStruct(REPAIR_FACILITY_STAT) < maxLimit) {
				if (grabTrucksAndBuild(REPAIR_FACILITY_STAT, derrick)) {
					return true;
				}
			}
		}
	}

	return false;
}


var lastAttemptTime = 0;
const RETRY_DELAY = 20000; // 20-second delay in milliseconds

function buildDefenses() {
	// Never spend power on defenses while power generation or the core base is critical, while derricks are unpowered,
	// or before the initial low-oil army (4 units) is formed
	if (isPowerOrCoreBaseCritical() || numUnusedDerricks() > 0 || shouldPrioritizeFirstArmyOverDefenses()) {
		return false;
	}

	// If the game time is less than the retry time, exit early
	if (gameTime < lastAttemptTime) {
		return false;
	}

	// Require a healthy power buffer early on before building defenses so generators/factories aren't starved
	const minDefensePower = isMyBaseInTrouble
		? MIN_BUILD_POWER
		: (countCompletedStruct(POW_GEN_STAT) < 2 || gameTime < 180000 ? 220 : 110);
	if (getRealPower() < minDefensePower) {
		return false;
	}

	// Finish any half-built safe defenses before laying down new foundations
	if (getUnfinishedSafeDefenses().length > 0) {
		return checkUncompletedStructures(true);
	}

	const baseDefensesCount = countBaseDefenses();
	// While the low-oil army is still thin, keep only a minimal perimeter and one tower per remote derrick
	const armyFirst = isArmyThin() && !isMyBaseInTrouble;

	// Priority 0: If enemies have VTOLs, keep a balanced forward AA shield in front of essential structures
	if (enemiesHaveVTOLs && shouldBuildAntiAirNow() && buildAntiAir()) {
		return true;
	}

	// Only throttle defense construction if our base already has at least 12 defenses and Ripple is not yet researched
	if (!isStructureAvailable("Emplacement-Rocket06-IDF") && baseDefensesCount >= 12 && !isMyBaseInTrouble) {
		lastAttemptTime = gameTime + RETRY_DELAY;
	}

	// Priority 1: Ensure our main base has a quick initial defense perimeter (6 defenses) or is defended when under attack
	if (isMyBaseInTrouble || baseDefensesCount < (armyFirst ? 3 : 6)) {
		let def = returnBestDefense(false);
		if (def !== null && grabTrucksAndBuild(def, BASE, null, true)) {
			return true;
		}
	}

	// Priority 2: Build defenses for allies on high oil maps
	if (buildDefensesForAllies()) {
		return true;
	}

	// Priority 3: Protect remote conquered derricks ONLY if the area around them is safe
	let allProtected = true;
	const requiredDerrickDefs = (isHighOilMap || armyFirst || gameTime < 480000 || getRealPower() < 350) ? 1 : 2;

	// build defenses if a derrick has not enough (1 defense at a time so we don't drain power)
	const exposedDerrick = findDerrickNeedingDefense(requiredDerrickDefs);
	if (exposedDerrick) {
		let def = returnBestDefense(true);
		if (def !== null) {
			return grabTrucksAndBuild(def, exposedDerrick, null, false);
		}
		allProtected = false;
	}

	// If all safe derricks are protected, build a defense near the base or push forward along our territory
	if (allProtected && !armyFirst) {
		let def = returnBestDefense(false);
		if (def !== null) {
			const buildPos = getDefenseBuildPosition();
			return grabTrucksAndBuild(def, buildPos, null, true);
		}
	}

	return false; // No defenses built
}

// Game time until which we already know that every safe remote derrick has its defenses (see findDerrickNeedingDefense)
let derricksProtectedUntil = 0;
let derricksProtectedFor = 0;

// Returns the first remote derrick in a safe area with fewer than `requiredDefs` defenses next to it, or null.
// The cheap "count the towers" test runs before the expensive area-safety test, the search stops at the first hit,
// and a "nothing to do" answer is remembered for 5 seconds: with 40+ derricks this used to cost hundreds of scans per second.
function findDerrickNeedingDefense(requiredDefs) {
	if (derricksProtectedFor === requiredDefs && gameTime < derricksProtectedUntil && gameTime > derricksProtectedUntil - 6000) {
		return null;
	}

	const derricks = enumStruct(me, DERRICK_STAT);
	for (let i = 0; i < derricks.length; ++i) {
		const derrick = derricks[i];
		if (isNearBase(derrick.x, derrick.y)) {
			continue;
		}
		const localDefenses = enumRange(derrick.x, derrick.y, 7, me, false)
			.filter(obj => obj.type === STRUCTURE && obj.stattype === DEFENSE).length;
		if (localDefenses >= requiredDefs) {
			continue;
		}
		if (isAreaSafe(derrick.x, derrick.y, 16)) {
			return derrick;
		}
	}

	derricksProtectedUntil = gameTime + 5000;
	derricksProtectedFor = requiredDefs;
	return null;
}

// Determines whether to fortify the main base or push defenses forward as we gain territory
function getDefenseBuildPosition(builders) {
	// Always prioritize the main base if it is under attack
	if (isMyBaseInTrouble) {
		return BASE;
	}

	const baseDefensesCount = countBaseDefenses();

	// Once the home base has at least 8 core defenses, send 70% of new defenses forward to expand territory
	if (baseDefensesCount < 8 || random(100) < 30) {
		return BASE;
	}

	// 1. First, check if there are forward derricks in a SAFE area that need to be turned into strong outposts
	let forwardDerricks = enumStruct(me, DERRICK_STAT).filter(d => !isNearBase(d.x, d.y));

	if (forwardDerricks.length > 0) {
		if (currentEnemy && currentEnemy.base) {
			const baseToEnemy = distBetweenTwoPoints(BASE.x, BASE.y, currentEnemy.base.x, currentEnemy.base.y);
			const advancingDerricks = forwardDerricks.filter(d =>
				distBetweenTwoPoints(d.x, d.y, currentEnemy.base.x, currentEnemy.base.y) < baseToEnemy
			);
			if (advancingDerricks.length > 0) {
				forwardDerricks = advancingDerricks;
			}
		}
		forwardDerricks.sort(sortByDistToBase);

		// (Cheap tower count first; the expensive safety test only for derricks that still need towers)
		for (const derrick of forwardDerricks) {
			const localDefenses = enumRange(derrick.x, derrick.y, 12, me, false)
				.filter(obj => obj.type === STRUCTURE && obj.stattype === DEFENSE).length;

			if (localDefenses < 6 && isAreaSafe(derrick.x, derrick.y, 16)) {
				return { x: derrick.x, y: derrick.y };
			}
		}
	}

	// 2. For flat maps without middle derricks (or once all derricks are fortified):
	// Advance progressively along the line from our BASE toward the current enemy base
	if (currentEnemy && currentEnemy.base && isTeamInStableAdvantage()) {
		const forwardPos = getProgressiveForwardPosition(currentEnemy.base);
		if (forwardPos) {
			return forwardPos;
		}
	}

	return BASE;
}

// Checks if our army + allied armies have a stable military advantage and sufficient base army size
function isTeamInStableAdvantage() {
	const myArmySize = enumGroup(attackerGroup).length;
	const minRequiredArmy = isHighOilMap ? 12 : 3;

	if (myArmySize < minRequiredArmy || isHelpingAlly) {
		return false;
	}

	let alliedPowerCount = countDroid(DROID_ANY, me) - countDroid(DROID_CONSTRUCT, me);
	for (const ally of alliesList) {
		if (isPlayerAlive(ally.position)) {
			alliedPowerCount += countDroid(DROID_ANY, ally.position) - countDroid(DROID_CONSTRUCT, ally.position);
		}
	}

	let visibleEnemyCombatDroids = 0;
	for (const enemy of enemiesList) {
		if (enemy.position !== undefined && isPlayerAlive(enemy.position)) {
			visibleEnemyCombatDroids += enumDroid(enemy.position, DROID_WEAPON, true).length
				+ enumDroid(enemy.position, DROID_CYBORG, true).length
				+ enumDroid(enemy.position, DROID_PERSON, true).length;
		}
	}

	return alliedPowerCount >= visibleEnemyCombatDroids * 0.8;
}

// Calculates progressive outpost positions along the vector from BASE to the enemy base
function getProgressiveForwardPosition(enemyBase) {
	const totalDist = distBetweenTwoPoints(BASE.x, BASE.y, enemyBase.x, enemyBase.y);
	if (totalDist <= BASE_THREAT_RANGE + 18) {
		return null; // Enemy base is already right next to us
	}

	const dx = (enemyBase.x - BASE.x) / totalDist;
	const dy = (enemyBase.y - BASE.y) / totalDist;

	// Step forward every 12 tiles starting from the front edge of our base, stopping 22 tiles before the enemy base
	const startDist = Math.max(16, BASE_THREAT_RANGE);
	const maxDist = totalDist - 22;
	const STEP = 12;

	for (let dist = startDist; dist <= maxDist; dist += STEP) {
		const px = Math.round(BASE.x + dx * dist);
		const py = Math.round(BASE.y + dy * dist);

		// Ensure coordinates are inside map boundaries
		if (px < 4 || py < 4 || px >= mapWidth - 4 || py >= mapHeight - 4) {
			continue;
		}

		// Stop advancing if this sector is not safe for our construction trucks
		if (!isAreaSafe(px, py, 16)) {
			break; // Frontline reached — do not send trucks into unsafe territory
		}

		// Count how many defenses we already built at this step
		const stepDefenses = enumRange(px, py, 10, me, false)
			.filter(obj => obj.type === STRUCTURE && obj.stattype === DEFENSE).length;

		// Fortify each step with 3 defenses before advancing to the next step closer to the enemy
		if (stepDefenses < 3) {
			return { x: px, y: py };
		}
	}

	return null;
}


function returnBestDefense(isDerrickDefense = false) {
	const ARTILLERY_CHANCE = 85;
	const BASTION_CHANCE = 15;
	const EMP_CHANCE = 10;
	const SENSOR_CHANCE = 5;

	let defenses;
	let bestDefense = null;

	// If it is a derrick, build the best available defense tier
	if (isDerrickDefense) {
		let defenseList;
		if (isStructureAvailable("Emplacement-Rocket06-IDF") || isStructureAvailable("WallTower-HvATrocket")) {
			defenseList = STRONG_DEFENSES;
		} else if (isStructureAvailable("WallTower06") || isStructureAvailable("Pillbox-RotMG")) {
			defenseList = MEDIUM_DEFENSES;
		} else {
			defenseList = WEAK_DEFENSES;
		}
		for (let i = 0; i < defenseList.length; i++) {
			if (isStructureAvailable(defenseList[i])) {
				return defenseList[i];
			}
		}
		return null;
	}

	// If the ripple is unavailable, build weak/medium defenses that are actually unlocked
	if (!isStructureAvailable("Emplacement-Rocket06-IDF")) {
		const availableMedium = MEDIUM_DEFENSES.filter(d => isStructureAvailable(d));
		if (availableMedium.length > 0) {
			return availableMedium[random(availableMedium.length)];
		}
		const availableWeak = WEAK_DEFENSES.filter(d => isStructureAvailable(d));
		if (availableWeak.length > 0) {
			return availableWeak[random(availableWeak.length)];
		}
		return null;
	}

	if (random(100) < SENSOR_CHANCE) {
		defenses = SENSORS_TOWERS;
	} else if (random(100) < EMP_CHANCE) {
		defenses = EMP_DEFENSES;
	} else if (random(100) < ARTILLERY_CHANCE) {
		defenses = ARTILLERY_DEFENSES;
	} else if (random(100) < BASTION_CHANCE) {
		defenses = FORTRESS_DEFENSES;
	} else {
		defenses = ARTILLERY_DEFENSES;
	}

	for (let i = 0; i < defenses.length; i++) {
		if (isStructureAvailable(defenses[i])) {
			bestDefense = defenses[i];
			break;
		}
	}

	// Fall back through Artillery, Medium, then Weak defenses so unresearched special categories never suppress Ripples
	if (!bestDefense) {
		const fallbackLists = [ARTILLERY_DEFENSES, MEDIUM_DEFENSES, WEAK_DEFENSES];
		for (const list of fallbackLists) {
			for (let i = 0; i < list.length; i++) {
				if (isStructureAvailable(list[i])) {
					return list[i];
				}
			}
		}
	}

	return bestDefense;
}

// Counts active VTOL units belonging to living enemy players (refreshed every 8 seconds)
function countEnemyVTOLs() {
	return cached("enemyVtolCount", 8000, () => {
		let total = 0;
		for (let i = 0; i < maxPlayers; ++i) {
			if (i !== me && !allianceExistsBetween(me, i) && isPlayerAlive(i)) {
				total += enumDroid(i, DROID_ANY, false).filter(d => d.isVTOL === true).length;
			}
		}
		return total;
	});
}

function countAASites() {
	return AA_DEFENSES.reduce((sum, stat) => sum + countStructWithPending(stat), 0);
}

// Number of static AA sites we want: one per enemy VTOL on low oil (two on high oil), like Cobra, within sane limits
function getDesiredAASites() {
	const enemyVtolCount = countEnemyVTOLs();
	const cap = isHighOilMap ? 20 : 10;
	return Math.max(2, Math.min(cap, isHighOilMap ? enemyVtolCount * 2 : enemyVtolCount));
}

// Determines if the next defense should be an Anti-Air site
function shouldBuildAntiAirNow() {
	if (!enemiesHaveVTOLs || !vtolsEnabled) {
		return false;
	}

	const aaCount = countAASites();
	const desiredAA = getDesiredAASites();
	if (aaCount >= desiredAA) {
		return false;
	}

	// The first half of the AA shield is urgent; the rest alternates with ground defenses so Ripples are not starved
	return aaCount < Math.ceil(desiredAA / 2) || random(100) < 50;
}

// Calculates a position in front of our furthest essential base structures (Factories, Labs, Power Gens, HQ)
// towards the enemy direction so AA defenses intercept enemy VTOLs before they reach our buildings.
function getForwardAntiAirPosition() {
	const targetPos = (currentEnemy && currentEnemy.base)
		? currentEnemy.base
		: { x: Math.floor(mapWidth / 2), y: Math.floor(mapHeight / 2) };

	const totalDist = distBetweenTwoPoints(BASE.x, BASE.y, targetPos.x, targetPos.y);
	if (totalDist < 12) {
		return BASE;
	}

	// Every second site goes where enemy VTOLs actually dropped their bombs in the last 5 minutes (base or remote derricks),
	// as long as that spot has fewer than 2 AA sites covering it
	if (lastVtolStrike && gameTime - lastVtolStrike.time < 300000 && countAASites() % 2 === 1) {
		const coveringSites = enumRange(lastVtolStrike.x, lastVtolStrike.y, 7, me, false)
			.filter(obj => obj.type === STRUCTURE && obj.stattype === DEFENSE && obj.canHitAir === true).length;
		if (coveringSites < 2) {
			return { x: lastVtolStrike.x, y: lastVtolStrike.y };
		}
	}

	// Unit vector from BASE toward enemy
	const dx = (targetPos.x - BASE.x) / totalDist;
	const dy = (targetPos.y - BASE.y) / totalDist;
	// Perpendicular vector to spread AA sites across the front line of the base
	const px = -dy;
	const py = dx;

	// Find how far forward our essential base structures extend along (dx, dy)
	const essentialStats = [FACTORY_STAT, CYBORG_FACTORY_STAT, VTOL_FACTORY_STAT, RES_LAB_STAT, POW_GEN_STAT, PLAYER_HQ_STAT];
	const essentialStructs = essentialStats
		.flatMap(stat => enumStruct(me, stat))
		.filter(s => distBetweenTwoPoints(BASE.x, BASE.y, s.x, s.y) <= BASE_THREAT_RANGE + 10);

	let maxForwardProj = 8; // Minimum 8 tiles in front of BASE
	for (const s of essentialStructs) {
		const proj = (s.x - BASE.x) * dx + (s.y - BASE.y) * dy;
		if (proj > maxForwardProj) {
			maxForwardProj = proj;
		}
	}

	// Place AA defenses 6 tiles ahead of the furthest essential structure
	const forwardDist = Math.min(BASE_THREAT_RANGE + 6, maxForwardProj + 6);
	const aaCount = AA_DEFENSES.reduce((sum, stat) => sum + countStruct(stat, me), 0);
	const sideOffsets = [0, 5, -5, 10, -10, 15, -15];
	const sideOffset = sideOffsets[aaCount % sideOffsets.length];

	const ax = Math.max(4, Math.min(mapWidth - 4, Math.round(BASE.x + dx * forwardDist + px * sideOffset)));
	const ay = Math.max(4, Math.min(mapHeight - 4, Math.round(BASE.y + dy * forwardDist + py * sideOffset)));

	return { x: ax, y: ay };
}

function buildAntiAir(trucks) {
	if (!enemiesHaveVTOLs || !vtolsEnabled) {
		return false;
	}

	let bestAADefense = null;

	// Find the strongest available AA defense
	for (let i = 0; i < AA_DEFENSES.length; i++) {
		if (isStructureAvailable(AA_DEFENSES[i])) {
			bestAADefense = AA_DEFENSES[i];
			break;
		}
	}

	if (!bestAADefense) {
		return false; // No AA researched yet
	}

	const forwardAAPos = getForwardAntiAirPosition();
	return grabTrucksAndBuild(bestAADefense, forwardAAPos, trucks, true);
}

// Order all builders in danger OR stationary/idle for > 60 seconds to return to base immediately!
function callIdleBuildersToBase() {
	const allBuilders = enumDroid(me, DROID_CONSTRUCT);
	const uncompletedStructs = getUnfinishedStructures();

	allBuilders.forEach(truck => {
		// 1. Immediate danger check outside base (including Scavengers).
		// Only armed enemies that outnumber our local guards make a truck run: an enemy truck or derrick nearby does not,
		// and an old danger zone alone does not either (that used to spread the blacklist across the whole map).
		if (!isNearBase(truck.x, truck.y) && truck.order !== DORDER_RTB) {
			const balance = localThreatBalance(truck.x, truck.y, 8);

			if (balance.threats > balance.guards) {
				markDangerZone(truck.x, truck.y, 60000);
				if (typeof pendingBuildOrders !== "undefined") pendingBuildOrders.delete(truck.id);
				orderDroid(truck, DORDER_RTB);
				return;
			}
		}

		// 2. Check if the truck has been stationary/idle for more than 60 seconds (60000 ms)
		const prev = truckIdleTracker.get(truck.id);
		const moved = prev ? distBetweenTwoPoints(truck.x, truck.y, prev.x, prev.y) > 1 : true;
		const isActivelyBuildingNearby = ((truck.order === DORDER_BUILD || truck.order === DORDER_HELPBUILD || truck.order === DORDER_LINEBUILD) &&
			uncompletedStructs.some(s => distBetweenTwoPoints(truck.x, truck.y, s.x, s.y) <= 4)) ||
			(truck.order === DORDER_DEMOLISH && enumStruct(me).some(s => distBetweenTwoPoints(truck.x, truck.y, s.x, s.y) <= 4));
		const isExplicitlyIdle = truck.order === DORDER_NONE || truck.order === DORDER_STOP || truck.order === DORDER_HOLD;

		if ((!moved && !isActivelyBuildingNearby) || isExplicitlyIdle) {
			if (!prev) {
				truckIdleTracker.set(truck.id, { x: truck.x, y: truck.y, idleSince: gameTime });
			} else if (gameTime - prev.idleSince >= 60000) {
				// Truck has been idle or stuck in the same spot for > 60 seconds: reset and send back to base!
				truckIdleTracker.set(truck.id, { x: truck.x, y: truck.y, idleSince: gameTime });
				if (typeof pendingBuildOrders !== "undefined") pendingBuildOrders.delete(truck.id);

				if (!isNearBase(truck.x, truck.y)) {
					orderDroid(truck, DORDER_RTB);
				} else {
					orderDroidLoc(truck, DORDER_MOVE, BASE.x, BASE.y);
				}
			}
		} else {
			truckIdleTracker.set(truck.id, { x: truck.x, y: truck.y, idleSince: gameTime });
		}
	});
}

// Build factories following Cobra's factoryBuildOrder(), scaled to what the economy can feed
function factoryBuildOrder() {
	const FAC_ORDER = [FACTORY_STAT, CYBORG_FACTORY_STAT, VTOL_FACTORY_STAT];
	const derrNum = countStruct(DERRICK_STAT);
	const num = getDesiredFactoryCount();

	for (const fac of FAC_ORDER) {
		if (!isStructureAvailable(fac)) {
			continue;
		}
		if (fac === CYBORG_FACTORY_STAT && isSeaMap) {
			continue;
		}

		let wanted = num;
		if (fac === VTOL_FACTORY_STAT) {
			if (!vtolsEnabled || !getResearch("R-Struc-VTOLPad").done) {
				continue;
			}
			// VTOLs are a luxury on low oil: none below 8 derricks, a single factory below 12
			if (!isHighOilMap) {
				if (derrNum < 8) {
					continue;
				}
				if (derrNum < 12) {
					wanted = 1;
				}
			}
		}

		if (countAndBuild(fac, wanted)) {
			return true;
		}
	}

	return false;
}

// Build the research labs the economy can keep busy (see getDesiredLabCount)
function buildResearchLabs() {
	if (researchDone || getMultiTechLevel() === 4 || !isStructureAvailable(RES_LAB_STAT)) {
		return false;
	}

	const amount = getDesiredLabCount();
	return countStructWithPending(RES_LAB_STAT) < amount && countAndBuild(RES_LAB_STAT, amount);
}


// Demolish old defenses if an upgrade is available
function demolishOldDefenses() {
	for (const { new: newStruct, old: oldStructs } of getObsoleteDefenseUpgrades()) {
		if (isStructureAvailable(newStruct)) {
			const targets = oldStructs
				.flatMap(type => enumStruct(me, type))
				.sort(sortByDistToBase);

			for (const struct of targets) {
				if (demolishThis(struct)) {
					return true;
				}
			}
		}
	}

	return false;
}

// Demolish useless cyborg factories
function demolishCyborgFactories() {
	if (isSeaMap) {
		const cybFacs = enumStruct(me, CYBORG_FACTORY_STAT);
		if (cybFacs.length > 0) {
			return demolishThis(cybFacs[0]);
		}
	}
	return false;
}

// If positive, there are oil derricks that unused due to lack of power generators.
function numUnusedDerricks() {
	const gens = countStruct(POW_GEN_STAT);
	// Once the generator limit is reached no further generator can be built: extra derricks must not
	// keep the base in "save power for a generator" mode forever (that used to stop all defenses on huge oil maps)
	const genLimit = typeof getStructureLimit === "function" ? getStructureLimit(POW_GEN_STAT, me) : 10;
	if (gens >= genLimit) {
		return 0;
	}
	return countStruct(DERRICK_STAT) - gens * 4;
}

// Return all the trucks that are currently free to build, sorted closest to `obj` first.
// Keeps dedicated oilGrabbersGroup trucks focused on oil capture unless no non-oil builders exist in an emergency.
function findIdleTrucks(obj, builderGroupName) {
	if (obj === null || obj === undefined) {
		obj = BASE;
	}

	const oilIds = hasFreeSafeOils
		? new Set((typeof oilGrabbersGroup !== "undefined" ? enumGroup(oilGrabbersGroup) : []).map(d => d.id))
		: new Set();
	let builders;
	if (builderGroupName) {
		builders = enumGroup(builderGroupName);
	} else if (isPowerOrCoreBaseCritical()) {
		builders = enumDroid(me, DROID_CONSTRUCT).filter(d => !oilIds.has(d.id));
		if (builders.length === 0) {
			builders = enumDroid(me, DROID_CONSTRUCT);
		}
	} else {
		builders = enumGroup(baseBuildersGroup);
	}

	let droidlist = builders.filter(d => canDroidHelp(d, obj.x, obj.y));

	// If the requested group has no free trucks, fall back to other non-oil constructor droids first
	if (droidlist.length === 0 && !builderGroupName) {
		droidlist = enumDroid(me, DROID_CONSTRUCT).filter(d => !oilIds.has(d.id) && canDroidHelp(d, obj.x, obj.y));
	}

	droidlist.sort((a, b) => distBetweenTwoPoints(a.x, a.y, obj.x, obj.y) - distBetweenTwoPoints(b.x, b.y, obj.x, obj.y));
	return droidlist;
}

// Demolish object and prevent other trucks from repairing/rebuilding it.
function demolishThis(structure) {
	if (!structure) return false;

	if (!structuresBeingDemolished.has(structure.id)) {
		structuresBeingDemolished.set(structure.id, null);
	}

	// Redirect any nearby builder or pending __HELPBUILD__ truck off construction/repair onto demolition
	const nearbyRepairing = enumDroid(me, DROID_CONSTRUCT).filter((d) => {
		if ((d.order === DORDER_BUILD || d.order === DORDER_HELPBUILD) &&
			distBetweenTwoPoints(d.x, d.y, structure.x, structure.y) <= 3) {
			return true;
		}
		if (typeof pendingBuildOrders !== "undefined" && pendingBuildOrders instanceof Map && pendingBuildOrders.has(d.id)) {
			const info = pendingBuildOrders.get(d.id);
			return info.stat === "__HELPBUILD__" && distBetweenTwoPoints(info.x, info.y, structure.x, structure.y) <= 2;
		}
		return false;
	});

	for (let i = 0; i < nearbyRepairing.length; ++i) {
		if (orderDroidObj(nearbyRepairing[i], DORDER_DEMOLISH, structure)) {
			setPendingOrder(nearbyRepairing[i], "__DEMOLISH__", structure.x, structure.y);
			structuresBeingDemolished.set(structure.id, nearbyRepairing[i].id);
		}
	}

	// Keep the existing demolisher working if one is already assigned or actively demolishing this structure
	const assignedId = structuresBeingDemolished.get(structure.id);
	const assignedDroid = assignedId ? getObject(DROID, me, assignedId) : null;
	const assignedHasOrder = assignedDroid && (
		assignedDroid.order === DORDER_DEMOLISH ||
		(typeof pendingBuildOrders !== "undefined" &&
			pendingBuildOrders instanceof Map &&
			pendingBuildOrders.has(assignedDroid.id) &&
			pendingBuildOrders.get(assignedDroid.id).stat === "__DEMOLISH__")
	);
	const nearbyDemolishing = enumDroid(me, DROID_CONSTRUCT).some(
		d => d.order === DORDER_DEMOLISH && distBetweenTwoPoints(d.x, d.y, structure.x, structure.y) <= 4
	);
	if (assignedHasOrder || nearbyDemolishing) {
		return true;
	}

	let success = false;
	let droidList = findIdleTrucks(structure, defensesBuildersGroup);
	if (droidList.length === 0) {
		droidList = findIdleTrucks(structure, baseBuildersGroup);
	}
	if (droidList.length === 0) {
		droidList = findIdleTrucks(structure);
	}

	for (let i = 0, d = droidList.length; i < d; ++i) {
		if (canDroidHelp(droidList[i], structure.x, structure.y)) {
			if (orderDroidObj(droidList[i], DORDER_DEMOLISH, structure)) {
				setPendingOrder(droidList[i], "__DEMOLISH__", structure.x, structure.y);
				structuresBeingDemolished.set(structure.id, droidList[i].id);
				success = true;
				break; // One truck is enough to demolish a structure
			}
		}
	}

	return success;
}

// Returns true if (loc.x, loc.y) overlaps with an active pending build order from another truck
function isLocationNearPendingBuild(loc) {
	cleanupPendingBuildOrders();
	if (!loc || typeof pendingBuildOrders === "undefined" || !(pendingBuildOrders instanceof Map)) {
		return false;
	}
	for (const [, info] of pendingBuildOrders.entries()) {
		if (!info.stat.startsWith("__") && gameTime - info.time <= (info.ttl || 25000) && distBetweenTwoPoints(loc.x, loc.y, info.x, info.y) <= 3) {
			return true;
		}
	}
	return false;
}

// How many blocked sides a build spot may have, in the order we try them.
// VTOL rearming pads can be driven over, so they are allowed in the gaps between other structures (Cobra does the same);
// everything else keeps a free ring around it so units are not walled in.
function getPlacementBlockingSteps(structure) {
	return structure === VTOL_PAD_STAT ? [8] : [0, 1];
}

// True while a recent search found no room for `structure` around (x, y): there is no point asking again yet
function isPlacementBackedOff(structure, x, y) {
	const backoffUntil = placementBackoff.get(structure + "@" + (x >> 3) + "_" + (y >> 3));
	return backoffUntil !== undefined && gameTime < backoffUntil && gameTime > backoffUntil - 20000;
}

// Picks a build location near (baseX, baseY) that does not overlap with any pending build order from another truck
function pickNonOverlappingLocation(truck, structure, baseX, baseY) {
	// A failed search scans thousands of tiles in the engine. When an area is full, do not repeat it every tick.
	const backoffKey = structure + "@" + (baseX >> 3) + "_" + (baseY >> 3);
	if (isPlacementBackedOff(structure, baseX, baseY)) {
		return null;
	}

	const originOffsets = [
		[0, 0],
		[4, 0], [-4, 0], [0, 4], [0, -4],
		[4, 4], [-4, -4], [4, -4], [-4, 4],
		[8, 0], [-8, 0], [0, 8], [0, -8]
	];
	const blockingSteps = getPlacementBlockingSteps(structure);

	for (const [ox, oy] of originOffsets) {
		const sx = Math.max(0, Math.min(mapWidth - 1, baseX + ox));
		const sy = Math.max(0, Math.min(mapHeight - 1, baseY + oy));
		let loc = null;
		for (let b = 0; b < blockingSteps.length && !loc; ++b) {
			loc = pickStructLocation(truck, structure, sx, sy, blockingSteps[b]);
		}
		if (!loc) {
			if (ox === 0 && oy === 0) {
				// The engine already searched a 30-tile radius around the origin: the shifted origins below
				// cover the same ground, so give up and retry in a few seconds
				placementBackoff.set(backoffKey, gameTime + 6000 + random(4000));
				return null;
			}
			continue;
		}
		if (!isLocationNearPendingBuild(loc)) {
			return loc;
		}
	}
	return null;
}

// Function to grab available trucks and start building a structure.
// When `allowMultipleLocations` is true (used for defenses), idle trucks pick distinct non-overlapping build spots, capped to protect power.
function grabTrucksAndBuild(structure, base, trucks, allowMultipleLocations = false) {
	// If no base position is provided, use BASE (keep original startPositions[me] or offset outside HQ)
	if (!base) {
		base = BASE;
	}

	if (!structure || !isStructureAvailable(structure) || isPlacementBackedOff(structure, base.x, base.y)) {
		return false;
	}

	let idleTrucks;
	if (trucks) {
		idleTrucks = trucks
			.filter(t => canDroidHelp(t, base.x, base.y))
			.sort((a, b) => distBetweenTwoPoints(a.x, a.y, base.x, base.y) - distBetweenTwoPoints(b.x, b.y, base.x, base.y));
	} else {
		idleTrucks = findIdleTrucks(base);
	}

	if (idleTrucks.length === 0) {
		return false; // No trucks available
	}

	// For defenses (allowMultipleLocations === true), start non-overlapping defenses capped by available power & generators
	if (allowMultipleLocations) {
		let buildSuccessful = false;
		const maxParallelDefenses = (getRealPower() >= 600 && countCompletedStruct(POW_GEN_STAT) >= 2) ? 2 : 1;
		let startedCount = 0;
		for (const truck of idleTrucks) {
			if (startedCount >= maxParallelDefenses) {
				break;
			}
			const loc = pickNonOverlappingLocation(truck, structure, base.x, base.y);
			if (loc && canDroidHelp(truck, loc.x, loc.y)) {
				if (orderDroidBuild(truck, DORDER_BUILD, structure, loc.x, loc.y)) {
					setPendingOrder(truck, structure, loc.x, loc.y);
					buildSuccessful = true;
					startedCount++;
				}
			}
		}
		return buildSuccessful;
	}

	const firstTruck = idleTrucks[0];
	// Pick a location that does not collide with another truck's pending build order in the same tick
	const buildLocation = pickNonOverlappingLocation(firstTruck, structure, base.x, base.y);

	if (!buildLocation) {
		return false;
	}

	// Assign 1 closest idle truck to start placing the foundation so other idle trucks can start other buildings in parallel;
	// once the foundation is placed, checkUncompletedStructures() will automatically send any extra idle trucks with DORDER_HELPBUILD.
	for (const truck of idleTrucks) {
		if (canDroidHelp(truck, buildLocation.x, buildLocation.y)) {
			if (orderDroidBuild(truck, DORDER_BUILD, structure, buildLocation.x, buildLocation.y)) {
				setPendingOrder(truck, structure, buildLocation.x, buildLocation.y);
				return true;
			}
		}
	}

	return false;
}

function canDroidHelp(mydroid, bx, by) {
	if (isTruckBusy(mydroid)) {
		return false;
	}

	// Check reachability to (bx, by) or its immediate adjacent tiles (in case (bx, by) is occupied by HQ or an existing structure)
	return droidCanReach(mydroid, bx, by) ||
		droidCanReach(mydroid, bx + 2, by) ||
		droidCanReach(mydroid, bx - 2, by) ||
		droidCanReach(mydroid, bx, by + 2) ||
		droidCanReach(mydroid, bx, by - 2);
}

// Do not snatch trucks away from unfinished structures (DORDER_HELPBUILD) or active construction orders
function canTruckCaptureOil(mydroid) {
	return !isTruckBusy(mydroid);
}

// Immediately build a defense structure right next to `truck` (Cobra & SemperFi strategy)
function buildDefenseNearTruck(truck) {
	if (!truck || isPowerOrCoreBaseCritical() || getRealPower() < MIN_BUILD_POWER) {
		return false;
	}
	const def = returnBestDefense(true);
	if (!def || !isStructureAvailable(def)) {
		return false;
	}
	let loc = pickStructLocation(truck, def, truck.x, truck.y, 1);
	if (!loc) {
		loc = pickStructLocation(truck, def, truck.x, truck.y, 0);
	}
	if (loc && orderDroidBuild(truck, DORDER_BUILD, def, loc.x, loc.y)) {
		setPendingOrder(truck, def, loc.x, loc.y);
		return true;
	}
	return false;
}

// Called from eventStructureBuilt / eventDroidIdle to defend remote derricks or contest stolen oil wells (Cobra & SemperFi strategy)
function scanAndDefendPosition(structure, droid) {
	if (!droid || isPowerOrCoreBaseCritical() || getRealPower() < MIN_BUILD_POWER) {
		return false;
	}
	if (structure && isBaseStructure(structure.stattype)) {
		return false;
	}

	let enemyDerricks = enumRange(droid.x, droid.y, 8, ENEMIES, false).filter(isDerrick);
	if (scavengerIndex !== undefined) {
		enemyDerricks = enemyDerricks.concat(enumRange(droid.x, droid.y, 8, scavengerIndex, false).filter(isDerrick));
	}

	const localStructs = enumRange(droid.x, droid.y, 7, me, false).filter(obj => obj.type === STRUCTURE);
	const localDefenses = localStructs.filter(obj => obj.stattype === DEFENSE);

	// Always contest stolen oil wells immediately with a tower
	if (enemyDerricks.length > 0 && localDefenses.length === 0) {
		return buildDefenseNearTruck(droid);
	}

	// For uncontested remote derricks in early T1 low oil, avoid draining low power before initial army/generators exist
	const minRemoteDefensePower = countCompletedStruct(POW_GEN_STAT) < 2 ? 150 : 90;
	if (shouldPrioritizeFirstArmyOverDefenses() || getRealPower() < minRemoteDefensePower) {
		return false;
	}

	const minDefenses = (gameTime < 480000 || isHighOilMap || getRealPower() < 350) ? 1 : 2;
	if (localDefenses.length < minDefenses) {
		return buildDefenseNearTruck(droid);
	}

	// Try a sensor tower later in the game like Cobra & SemperFi
	if (gameTime > 600000 && random(100) < 50 && localStructs.length < 5) {
		for (let i = 0; i < SENSORS_TOWERS.length; ++i) {
			const sen = SENSORS_TOWERS[i];
			if (isStructureAvailable(sen)) {
				const loc = pickStructLocation(droid, sen, droid.x, droid.y, 1);
				if (loc && orderDroidBuild(droid, DORDER_BUILD, sen, loc.x, loc.y)) {
					setPendingOrder(droid, sen, loc.x, loc.y);
					return true;
				}
				break;
			}
		}
	}
	return false;
}

// Returns true if an oil well at (ox, oy) already has a pending derrick build order from one of our walking trucks
function isOilTargetedByPendingBuild(ox, oy) {
	cleanupPendingBuildOrders();
	if (typeof pendingBuildOrders === "undefined" || !(pendingBuildOrders instanceof Map)) {
		return false;
	}
	for (const [, info] of pendingBuildOrders.entries()) {
		if (info.stat === DERRICK_STAT && distBetweenTwoPoints(info.x, info.y, ox, oy) <= 3) {
			return true;
		}
	}
	return false;
}

// True if at least one of our trucks can drive to this oil well (cached for a minute per well)
function isOilReachable(oil, trucks) {
	const key = oil.x + "_" + oil.y;
	const hit = oilReachCache.get(key);
	if (hit !== undefined && gameTime >= hit.t && gameTime - hit.t < 60000) {
		return hit.ok;
	}
	const ok = trucks.some(droid => droidCanReach(droid, oil.x, oil.y));
	oilReachCache.set(key, { t: gameTime, ok: ok });
	return ok;
}

// Returns up to `maxCount` available, safe, reachable oil wells, closest to BASE first.
// Stops scanning as soon as it has enough, so maps with a hundred wells are not fully re-validated every second.
function getUnassignedSafeOils(maxCount) {
	cleanupPendingBuildOrders();
	const allyBasePositions = alliesList.map(a => a.base);
	const allTrucks = enumDroid(me, DROID_CONSTRUCT);
	if (allTrucks.length === 0) {
		return [];
	}
	if (maxCount === undefined) {
		maxCount = Infinity;
	}

	const busyTrucks = allTrucks.filter(truck =>
		truck.order === DORDER_BUILD
	);

	const oils = enumFeature(ALL_PLAYERS, OIL_RES_STAT).sort(sortByDistToBase);
	const result = [];
	let firstContested = null;

	for (let i = 0; i < oils.length && result.length < maxCount; ++i) {
		const oil = oils[i];

		const tooCloseToAlly = allyBasePositions.some(base =>
			base && distBetweenTwoPoints(base.x, base.y, oil.x, oil.y) < 12
		);
		if (tooCloseToAlly) continue;

		const alreadyTargetedByBusy = busyTrucks.some(truck =>
			distBetweenTwoPoints(truck.x, truck.y, oil.x, oil.y) < 4
		);
		if (alreadyTargetedByBusy) continue;

		if (isOilTargetedByPendingBuild(oil.x, oil.y)) continue;

		if (!isOilReachable(oil, allTrucks)) continue;

		if (!isOilAreaSafe(oil.x, oil.y)) {
			if (!firstContested) {
				firstContested = oil;
			}
			continue;
		}

		result.push(oil);
	}

	// Remember the closest free oil that armed enemies are sitting on, if it is on our side of the map:
	// the army uses it as a target so the trucks can come back and claim it (see attack.js)
	if (firstContested) {
		const distToUs = distBetweenTwoPoints(BASE.x, BASE.y, firstContested.x, firstContested.y);
		const distToEnemy = (currentEnemy && currentEnemy.base)
			? distBetweenTwoPoints(currentEnemy.base.x, currentEnemy.base.y, firstContested.x, firstContested.y)
			: Infinity;
		if (distToUs <= distToEnemy * 1.25) {
			contestedOil = { x: firstContested.x, y: firstContested.y, time: gameTime };
		}
	}

	return result;
}

function lookForOil() {
	return profiled("lookForOil", lookForOilTick);
}

function lookForOilTick() {
	// Also try to grab nearby free Oil Drums and Artifacts (Crates)
	if (!throttled("grabFreeObjects", 3000)) {
		grabFreeObjects();
	}

	const allTrucks = enumDroid(me, DROID_CONSTRUCT);
	if (allTrucks.length === 0) {
		return false;
	}

	// Cobra strategy: keep up to 2 non-oil base trucks building the core base (1st Factory / 1st Generator)
	// while dedicated oilGrabbersGroup trucks (and extra idle trucks) immediately capture oil wells
	const oilIds = hasFreeSafeOils
		? new Set((typeof oilGrabbersGroup !== "undefined" ? enumGroup(oilGrabbersGroup) : []).map(d => d.id))
		: new Set();
	const coreBaseIncomplete = countCompletedStruct(FACTORY_STAT) === 0 || countCompletedStruct(POW_GEN_STAT) === 0;
	const busyAtBaseCount = allTrucks.filter(t => !oilIds.has(t.id) && !canTruckCaptureOil(t) && distBetweenTwoPoints(t.x, t.y, BASE.x, BASE.y) < 30).length;

	const freeOilGrabbers = allTrucks.filter(droid => oilIds.has(droid.id) && canTruckCaptureOil(droid));
	let otherFreeTrucks = allTrucks.filter(droid => !oilIds.has(droid.id) && canTruckCaptureOil(droid));

	if (coreBaseIncomplete) {
		const reserveForBase = Math.max(0, 2 - busyAtBaseCount);
		if (reserveForBase > 0) {
			otherFreeTrucks.sort((a, b) => distBetweenTwoPoints(a.x, a.y, BASE.x, BASE.y) - distBetweenTwoPoints(b.x, b.y, BASE.x, BASE.y));
			otherFreeTrucks = otherFreeTrucks.slice(reserveForBase);
		}
	}

	const droids = freeOilGrabbers.concat(otherFreeTrucks);

	if (droids.length === 0) return false;

	// A couple more candidates than free trucks is enough for a good truck/oil matching
	const oils = getUnassignedSafeOils(droids.length + 2);
	hasFreeSafeOils = oils.length > 0;
	if (oils.length === 0) {
		// Cobra strategy: once free oils are exhausted, occasionally fortify unguarded remote derricks
		if (!isPowerOrCoreBaseCritical() && !shouldPrioritizeFirstArmyOverDefenses() && random(100) < 20 && getRealPower() > MIN_BUILD_POWER) {
			buildDefenses();
		}
		return false;
	}

	let success = false;
	let assignedDroids = new Set();

	for (const oil of oils) {
		if (assignedDroids.size >= droids.length) break;

		let bestDroid = null;
		let bestDist = 99999;

		for (const droid of droids) {
			if (assignedDroids.has(droid.id)) continue;

			const dist = distBetweenTwoPoints(droid.x, droid.y, oil.x, oil.y);
			if (dist < bestDist && droidCanReach(droid, oil.x, oil.y)) {
				bestDist = dist;
				bestDroid = droid;
			}
		}

		if (bestDroid) {
			assignedDroids.add(bestDroid.id);
			if (orderDroidBuild(bestDroid, DORDER_BUILD, DERRICK_STAT, oil.x, oil.y)) {
				setPendingOrder(bestDroid, DERRICK_STAT, oil.x, oil.y);
				success = true;
			}
		}
	}

	return success;
}

// Collect free Oil Drums (+100 power) and Artifacts (Crates) on the map
function grabFreeObjects() {
	const collectibles = enumFeature(ALL_PLAYERS, OIL_DRUM_STAT)
		.concat(enumFeature(ALL_PLAYERS, ARTIFACT_STAT))
		.sort(sortByDistToBase);

	if (collectibles.length === 0) {
		return false;
	}

	// Only use combat units during initial base/power bootstrap so construction trucks focus 100% on HQ, safe oil derricks, and generators
	const candidateCollectors = isPowerOrCoreBaseCritical()
		? enumGroup(attackerGroup)
		: enumGroup(attackerGroup).concat(enumGroup(defensesBuildersGroup));

	const collectors = candidateCollectors
		.filter(d =>
			d.order !== DORDER_RECOVER &&
			d.order !== DORDER_BUILD &&
			d.order !== DORDER_HELPBUILD &&
			d.order !== DORDER_RTR &&
			!isTruckBusy(d)
		);

	if (collectors.length === 0) {
		return false;
	}

	let assignedCollectors = new Set();
	const MAX_COLLECT_ITEMS = 3; // Process up to 3 closest items per tick to keep CPU usage minimal
	let count = 0;

	for (const item of collectibles) {
		if (count >= MAX_COLLECT_ITEMS || assignedCollectors.size >= collectors.length) break;

		// Skip items if the area around them is not safe
		if (!isAreaSafe(item.x, item.y, 18)) continue;

		let bestDroid = null;
		let bestDist = 99999;

		for (const droid of collectors) {
			if (assignedCollectors.has(droid.id)) continue;

			const dist = distBetweenTwoPoints(droid.x, droid.y, item.x, item.y);
			if (dist < bestDist && droidCanReach(droid, item.x, item.y)) {
				bestDist = dist;
				bestDroid = droid;
			}
		}

		if (bestDroid) {
			assignedCollectors.add(bestDroid.id);
			if (orderDroidObj(bestDroid, DORDER_RECOVER, item)) {
				if (bestDroid.droidType === DROID_CONSTRUCT && typeof pendingBuildOrders !== "undefined") {
					pendingBuildOrders.set(bestDroid.id, {
						stat: "__RECOVER__",
						x: item.x,
						y: item.y,
						time: gameTime
					});
				}
				count++;
			}
		}
	}

	return count > 0;
}
