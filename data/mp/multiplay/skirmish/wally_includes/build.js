// Maps structure.id -> assigned constructor droid.id (or null) for structures scheduled for demolition
const structuresBeingDemolished = new Map();
// Tracks whether capturable safe oil wells currently remain on the map so oilGrabbersGroup helps at base once all safe oils are taken
let hasFreeSafeOils = true;

// Upgrade mappings where old defenses become obsolete once the replacement is unlocked (evaluated lazily after all includes load)
function getObsoleteDefenseUpgrades() {
	return [
		{ new: "Emplacement-Rocket06-IDF", old: WEAK_DEFENSES },
		{ new: "Sys-SensoTowerWS", old: ["Sys-SensoTower02"] },
		{ new: "Emplacement-HvART-pit", old: ["Emplacement-Rocket06-IDF"] },
		{ new: "X-Super-MassDriver", old: ["X-Super-Cannon"] }
	];
}

// Returns a Set of owned structure IDs that are obsolete and eligible for demolition
function getObsoleteStructureIds() {
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
	const hasActiveDemolisher = enumDroid(me, DROID_CONSTRUCT).some(
		d => d.order === DORDER_DEMOLISH && distBetweenTwoPoints(d.x, d.y, structure.x, structure.y) <= 4
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
		if (!truck || gameTime - info.time > 25000) {
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

// If a distant truck is walking from far away to place a core base structure that hasn't been placed yet,
// and another idle truck is now much closer to BASE, free the distant truck so the closer truck builds it immediately
function reassignPendingBaseBuildIfCloserTruck(stat) {
	cleanupPendingBuildOrders();
	if (countStruct(stat, me) > 0 || typeof pendingBuildOrders === "undefined" || !(pendingBuildOrders instanceof Map)) {
		return;
	}

	const idleTrucks = findIdleTrucks(BASE);
	if (idleTrucks.length === 0) {
		return;
	}
	const closestIdle = idleTrucks[0];
	const closestIdleDist = distBetweenTwoPoints(closestIdle.x, closestIdle.y, BASE.x, BASE.y);

	const activeTrucks = new Map(enumDroid(me, DROID_CONSTRUCT).map(t => [t.id, t]));
	for (const [truckId, info] of pendingBuildOrders.entries()) {
		if (info.stat === stat) {
			const pendingTruck = activeTrucks.get(truckId);
			if (pendingTruck) {
				const pendingDist = distBetweenTwoPoints(pendingTruck.x, pendingTruck.y, info.x, info.y);
				if (closestIdleDist + 8 < pendingDist) {
					pendingBuildOrders.delete(truckId);
					orderDroid(pendingTruck, DORDER_STOP);
				}
			}
		}
	}
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
			countAndBuild(FACTORY_STAT, 2) ||
			countAndBuild(POW_GEN_STAT, 2) ||
			(numUnusedDerricks() > 0 && countAndBuild(POW_GEN_STAT, countStruct(POW_GEN_STAT) + 1)) ||
			baseMaintenance() ||
			(getMultiTechLevel() < 4 && !researchDone && countAndBuild(RES_LAB_STAT, 2))) {
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
			countAndBuild(FACTORY_STAT, 5)) {
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

// Handle Build following Cobra's buildOrders() progression
function build() {
	if (!countDroid(DROID_CONSTRUCT)) {
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

	// 5. Dedicated defense builders finish half-built defenses first, then build base/forward or Anti-VTOL defenses
	const defensesBuilders = enumGroup(defensesBuildersGroup);
	if (defensesBuilders.length >= 1 &&
		!shouldPrioritizeFirstArmyOverDefenses() &&
		gameTime >= lastAttemptTime &&
		numUnusedDerricks() <= 0 &&
		getRealPower() > MIN_BUILD_POWER) {
		if (getUnfinishedSafeDefenses().length > 0) {
			checkUncompletedStructures(true);
		} else if (!(enemiesHaveVTOLs && shouldBuildAntiAirNow() && buildAntiAir(defensesBuilders))) {
			const def = returnBestDefense();
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
			(getMultiTechLevel() < 4 && !researchDone && countAndBuild(RES_LAB_STAT, 2)) ||
			(numUnusedDerricks() > 0 && countAndBuild(POW_GEN_STAT, countStruct(POW_GEN_STAT) + 1)) ||
			countAndBuild(FACTORY_STAT, 2) ||
			countAndBuild(POW_GEN_STAT, 2) ||
			(getMultiTechLevel() < 4 && !researchDone && countAndBuild(RES_LAB_STAT, 3)) ||
			// Build power modules on low oil after initial combat factories and labs are running
			(countStruct(POW_GEN_STAT) <= 3 && gameTime < 900000 && baseMaintenance(true)) ||
			countAndBuild(REPAIR_FACILITY_STAT, 1) ||
			(countStruct(DERRICK_STAT) >= 6 && countAndBuild(VTOL_FACTORY_STAT, 1))) {
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
			(goodPowerLevel && countAndBuild(REPAIR_FACILITY_STAT, 3))) {
			return true;
		}
	}

	if (getMultiTechLevel() > 1 && countStruct(VTOL_FACTORY_STAT) > 0 && countAndBuild(VTOL_PAD_STAT, 3)) {
		return true;
	}

	return false;
}

// Returns unfinished defense structures in safe or base-adjacent areas that need completion
function getUnfinishedSafeDefenses() {
	const obsoleteIds = getObsoleteStructureIds();
	return enumStruct(me).filter((s) =>
		s.status !== BUILT &&
		s.stattype === DEFENSE &&
		!isStructureBeingDemolished(s, obsoleteIds) &&
		(isNearBase(s.x, s.y) || isAreaSafe(s.x, s.y, 12))
	);
}

// Ensures any structure marked for demolition or half-demolished obsolete structure finishes being demolished
function continuePendingDemolitions(obsoleteIds) {
	cleanupPendingBuildOrders();
	const structs = enumStruct(me);
	for (let i = 0; i < structs.length; ++i) {
		const s = structs[i];
		const isHalfDemolishedObsolete = s.status !== BUILT && (obsoleteIds.has(s.id) || (isSeaMap && s.stattype === CYBORG_FACTORY));
		if (structuresBeingDemolished.has(s.id) || isHalfDemolishedObsolete) {
			demolishThis(s);
		}
	}
}

// Send remaining idle trucks to finish uncompleted structures near the base, safe zones, or orphaned oil derricks
function checkUncompletedStructures(includeDefenseBuilders = false) {
	let hasAssignedTask = false;
	const obsoleteIds = getObsoleteStructureIds();
	continuePendingDemolitions(obsoleteIds);

	const activeBuilders = enumDroid(me, DROID_CONSTRUCT).filter(
		t => t.order === DORDER_BUILD || t.order === DORDER_HELPBUILD
	);

	// Include uncompleted base/defense structures in safe areas and orphaned oil derricks with no truck working on them
	const structlist = enumStruct(me).filter((structure) => {
		if (structure.status === BUILT || isStructureBeingDemolished(structure, obsoleteIds)) {
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
				if (typeof pendingBuildOrders !== "undefined") {
					pendingBuildOrders.set(idleTrucks[j].id, {
						stat: "__HELPBUILD__",
						x: targetStruct.x,
						y: targetStruct.y,
						time: gameTime
					});
				}
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
					if (typeof pendingBuildOrders !== "undefined") {
						pendingBuildOrders.set(truck.id, {
							stat: "__MODULE__",
							x: struct.x,
							y: struct.y,
							time: gameTime
						});
					}
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
	let needVtolPads = 2 * countStructWithPending(VTOL_PAD_STAT) < vtols.length;

	// Prioritize Research Labs first if we have fewer than 3 so research never stalls
	if (!researchDone && countStructWithPending(RES_LAB_STAT) < 3 && buildResearchLabs()) {
		return true;
	}

	if (factoryBuildOrder()) {
		return true;
	}

	if (buildResearchLabs()) {
		return true;
	}

	// Build VTOL pads if needed
	if (needVtolPads && grabTrucksAndBuild(VTOL_PAD_STAT)) {
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
	const maxLimit = typeof getStructureLimit === "function" ? getStructureLimit(REPAIR_FACILITY_STAT, me) : 5;
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

	const baseDefensesCount = enumRange(BASE.x, BASE.y, BASE_THREAT_RANGE, me, false)
		.filter(obj => obj.type === STRUCTURE && obj.stattype === DEFENSE).length;

	// Priority 0: If enemies have VTOLs, keep a balanced forward AA shield in front of essential structures
	if (enemiesHaveVTOLs && shouldBuildAntiAirNow() && buildAntiAir()) {
		return true;
	}

	// Only throttle defense construction if our base already has at least 12 defenses and Ripple is not yet researched
	if (!isStructureAvailable("Emplacement-Rocket06-IDF") && baseDefensesCount >= 12 && !isMyBaseInTrouble) {
		lastAttemptTime = gameTime + RETRY_DELAY;
	}

	// Priority 1: Ensure our main base has a quick initial defense perimeter (6 defenses) or is defended when under attack
	if (isMyBaseInTrouble || baseDefensesCount < 6) {
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
	const derricks = enumStruct(me, DERRICK_STAT).filter(
		derrick => !isNearBase(derrick.x, derrick.y) && isAreaSafe(derrick.x, derrick.y, 16)
	);

	let allProtected = true;
	const requiredDerrickDefs = (isHighOilMap || gameTime < 480000 || getRealPower() < 350) ? 1 : 2;

	for (const derrick of derricks) {
		const hasDerrickEnoughDefenses = enumRange(derrick.x, derrick.y, 7, me, true)
			.filter(obj => obj.stattype === DEFENSE).length >= requiredDerrickDefs;

		// build defenses if derrick has not enough (1 defense at a time so we don't drain power)
		if (!hasDerrickEnoughDefenses) {
			let def = returnBestDefense(true);
			if (def !== null) {
				return grabTrucksAndBuild(def, derrick, null, false);
			}
			allProtected = false;
			break;
		}
	}

	// If all safe derricks are protected, build a defense near the base or push forward along our territory
	if (allProtected) {
		let def = returnBestDefense(false);
		if (def !== null) {
			const buildPos = getDefenseBuildPosition();
			return grabTrucksAndBuild(def, buildPos, null, true);
		}
	}

	return false; // No defenses built
}

// Determines whether to fortify the main base or push defenses forward as we gain territory
function getDefenseBuildPosition(builders) {
	// Always prioritize the main base if it is under attack
	if (isMyBaseInTrouble) {
		return BASE;
	}

	const baseDefensesCount = enumRange(BASE.x, BASE.y, BASE_THREAT_RANGE, me, false)
		.filter(obj => obj.type === STRUCTURE && obj.stattype === DEFENSE).length;

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

		for (const derrick of forwardDerricks) {
			if (!isAreaSafe(derrick.x, derrick.y, 16)) continue;

			const localDefenses = enumRange(derrick.x, derrick.y, 12, me, false)
				.filter(obj => obj.type === STRUCTURE && obj.stattype === DEFENSE).length;

			if (localDefenses < 6) {
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

// Counts active VTOL units belonging to living enemy players
function countEnemyVTOLs() {
	let total = 0;
	for (let i = 0; i < maxPlayers; ++i) {
		if (i !== me && !allianceExistsBetween(me, i) && isPlayerAlive(i)) {
			total += enumDroid(i, DROID_ANY, false).filter(d => d.isVTOL === true).length;
		}
	}
	return total;
}

// Determines if we should prioritize building an Anti-Air defense right now without starving Artillery/Ripple batteries
function shouldBuildAntiAirNow() {
	if (!enemiesHaveVTOLs) {
		return false;
	}

	const aaCount = AA_DEFENSES.reduce((sum, stat) => sum + countStructWithPending(stat), 0);
	const artilleryCount = ARTILLERY_DEFENSES.reduce((sum, stat) => sum + countStructWithPending(stat), 0);
	const hasArtilleryUnlocked = ARTILLERY_DEFENSES.some(stat => isStructureAvailable(stat));

	// Once we have a baseline of 2 AA sites and long-range artillery is unlocked, avoid building more AA until artillery outnumbers AA
	if (aaCount >= 2 && hasArtilleryUnlocked && artilleryCount <= aaCount) {
		return false;
	}

	const enemyVtolCount = countEnemyVTOLs();
	const desiredAA = Math.max(2, Math.min(6, Math.ceil(enemyVtolCount / 2) + 1));

	if (aaCount < desiredAA) {
		return random(100) < 40;
	}
	if (aaCount < 6) {
		return random(100) < 20;
	}
	return random(100) < 8;
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
	if (!enemiesHaveVTOLs) {
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
	const uncompletedStructs = enumStruct(me).filter(s => s.status !== BUILT);

	allBuilders.forEach(truck => {
		// 1. Immediate danger check outside base (including Scavengers)
		if (!isNearBase(truck.x, truck.y)) {
			let nearbyThreats = enumRange(truck.x, truck.y, 12, ENEMIES, false);
			if (scavengerIndex !== undefined) {
				nearbyThreats = nearbyThreats.concat(enumRange(truck.x, truck.y, 12, scavengerIndex, false));
			}
			const isUnsafe = nearbyThreats.some(isUnsafeEnemyObject) || isInDangerZone(truck.x, truck.y, 12);

			if (isUnsafe && truck.order !== DORDER_RTB) {
				markDangerZone(truck.x, truck.y, 120000);
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

// Build factories following Cobra's factoryBuildOrder()
function factoryBuildOrder() {
	const FAC_ORDER = [FACTORY_STAT, CYBORG_FACTORY_STAT, VTOL_FACTORY_STAT];
	const derrNum = countStruct(DERRICK_STAT);
	let num = 1;

	if (derrNum >= 20) {
		num = 5;
	} else if (derrNum >= 16) {
		num = 4;
	} else if (derrNum >= 9) {
		num = 3;
	} else if (derrNum >= 6) {
		num = 2;
	}

	for (const fac of FAC_ORDER) {
		if (!isStructureAvailable(fac)) {
			continue;
		}
		if (fac === CYBORG_FACTORY_STAT && isSeaMap) {
			continue;
		}
		if (fac === VTOL_FACTORY_STAT && !getResearch("R-Struc-VTOLPad").done) {
			continue;
		}

		if (countAndBuild(fac, num)) {
			return true;
		}
	}

	return false;
}

// Build the research labs following Cobra's researchBuildOrder()
function buildResearchLabs() {
	if (researchDone || getMultiTechLevel() === 4 || !isStructureAvailable(RES_LAB_STAT)) {
		return false;
	}

	const currentLabs = countStructWithPending(RES_LAB_STAT);
	if (currentLabs < 5) {
		let amount = 3;
		const derrCount = countStruct(DERRICK_STAT);

		if (derrCount >= 10) {
			amount = 5;
		} else if (derrCount >= 6 || isSeaMap) {
			amount = 4;
		}

		if (currentLabs < amount && countAndBuild(RES_LAB_STAT, amount)) {
			return true;
		}
	}

	return false;
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
	return countStruct(DERRICK_STAT) - countStruct(POW_GEN_STAT) * 4;
}

// Function to check if more derricks are needed due to unused power generators
function needMoreDerricks() {
	var powGenCount = countStruct(POW_GEN_STAT, me);
	var derrickCount = countStruct(DERRICK_STAT, me);

	var minDerricksNeeded = powGenCount * 5; // A power generator can support 4 derricks but we set 5 to be safe

	if (derrickCount >= minDerricksNeeded) {
		return false;
	}

	return true;
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
			if (typeof pendingBuildOrders !== "undefined" && pendingBuildOrders instanceof Map) {
				pendingBuildOrders.set(nearbyRepairing[i].id, {
					stat: "__DEMOLISH__",
					x: structure.x,
					y: structure.y,
					time: gameTime
				});
			}
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
				if (typeof pendingBuildOrders !== "undefined" && pendingBuildOrders instanceof Map) {
					pendingBuildOrders.set(droidList[i].id, {
						stat: "__DEMOLISH__",
						x: structure.x,
						y: structure.y,
						time: gameTime
					});
				}
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
		if (!info.stat.startsWith("__") && gameTime - info.time <= 25000 && distBetweenTwoPoints(loc.x, loc.y, info.x, info.y) <= 3) {
			return true;
		}
	}
	return false;
}

// Picks a build location near (baseX, baseY) that does not overlap with any pending build order from another truck
function pickNonOverlappingLocation(truck, structure, baseX, baseY) {
	const originOffsets = [
		[0, 0],
		[4, 0], [-4, 0], [0, 4], [0, -4],
		[4, 4], [-4, -4], [4, -4], [-4, 4],
		[8, 0], [-8, 0], [0, 8], [0, -8]
	];

	for (const [ox, oy] of originOffsets) {
		const sx = Math.max(0, Math.min(mapWidth - 1, baseX + ox));
		const sy = Math.max(0, Math.min(mapHeight - 1, baseY + oy));
		let loc = pickStructLocation(truck, structure, sx, sy, 0);
		if (!loc) {
			loc = pickStructLocation(truck, structure, sx, sy, 1);
		}
		if (loc && !isLocationNearPendingBuild(loc)) {
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

	if (!structure || !isStructureAvailable(structure)) {
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
					if (typeof pendingBuildOrders !== "undefined") {
						pendingBuildOrders.set(truck.id, { stat: structure, x: loc.x, y: loc.y, time: gameTime });
					}
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
				if (typeof pendingBuildOrders !== "undefined") {
					pendingBuildOrders.set(truck.id, {
						stat: structure,
						x: buildLocation.x,
						y: buildLocation.y,
						time: gameTime
					});
				}
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
		if (typeof pendingBuildOrders !== "undefined") {
			pendingBuildOrders.set(truck.id, {
				stat: def,
				x: loc.x,
				y: loc.y,
				time: gameTime
			});
		}
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
					if (typeof pendingBuildOrders !== "undefined") {
						pendingBuildOrders.set(droid.id, { stat: sen, x: loc.x, y: loc.y, time: gameTime });
					}
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

// Returns all available, safe, reachable oil wells (Cobra & SemperFi 7-tile safety criteria) sorted closest to BASE first
function getUnassignedSafeOils() {
	cleanupPendingBuildOrders();
	const allyBasePositions = alliesList.map(a => a.base);
	const allTrucks = enumDroid(me, DROID_CONSTRUCT);
	if (allTrucks.length === 0) {
		return [];
	}

	const busyTrucks = allTrucks.filter(truck =>
		truck.order === DORDER_BUILD
	);

	return enumFeature(ALL_PLAYERS, OIL_RES_STAT)
		.filter(oil => {
			const tooCloseToAlly = allyBasePositions.some(base =>
				base && distBetweenTwoPoints(base.x, base.y, oil.x, oil.y) < 12
			);
			if (tooCloseToAlly) return false;

			const alreadyTargetedByBusy = busyTrucks.some(truck =>
				distBetweenTwoPoints(truck.x, truck.y, oil.x, oil.y) < 4
			);
			if (alreadyTargetedByBusy) return false;

			if (isOilTargetedByPendingBuild(oil.x, oil.y)) return false;

			if (!isOilAreaSafe(oil.x, oil.y)) return false;

			// Verify at least one of our trucks can reach this oil well
			return allTrucks.some(droid => droidCanReach(droid, oil.x, oil.y));
		})
		.sort(sortByDistToBase);
}

function lookForOil() {
	// Also try to grab nearby free Oil Drums and Artifacts (Crates)
	grabFreeObjects();

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

	const oils = getUnassignedSafeOils();
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
				if (typeof pendingBuildOrders !== "undefined") {
					pendingBuildOrders.set(bestDroid.id, {
						stat: DERRICK_STAT,
						x: oil.x,
						y: oil.y,
						time: gameTime
					});
				}
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
