// Returns the number of existing structures of a given type PLUS any truck currently walking to place its foundation
function countStructWithPending(stat) {
	let count = countStruct(stat, me);
	if (typeof pendingBuildOrders === "undefined" || !(pendingBuildOrders instanceof Map)) {
		return count;
	}

	const activeTrucks = new Map(enumDroid(me, DROID_CONSTRUCT).map(t => [t.id, t]));
	for (const [truckId, info] of pendingBuildOrders.entries()) {
		const truck = activeTrucks.get(truckId);
		// Remove expired or completed pending build orders
		if (!truck || truck.order !== DORDER_BUILD || gameTime - info.time > 25000) {
			pendingBuildOrders.delete(truckId);
			continue;
		}
		if (info.stat === stat) {
			// Check if the foundation has already been placed at (info.x, info.y)
			const placed = enumRange(info.x, info.y, 3, me, false).some(
				obj => obj.type === STRUCTURE && obj.id !== undefined
			);
			if (placed) {
				pendingBuildOrders.delete(truckId);
			} else {
				count++;
			}
		}
	}
	return count;
}

// Handle Build in a linear, non-blocking progression so idle trucks never sit around doing nothing
function build() {
	if (!countDroid(DROID_CONSTRUCT)) {
		return;
	}

	// 1. Start any missing core base structures (assigns 1 idle truck per missing structure in parallel)
	buildBasicBase();

	// 2. Dedicated defense builders build base/forward defenses and forward Anti-VTOL defenses when power allows
	const defensesBuilders = enumGroup(defensesBuildersGroup);
	if (defensesBuilders.length >= 1 && getRealPower() > MIN_BUILD_POWER) {
		if (enemiesHaveVTOLs && shouldBuildAntiAirNow()) {
			buildAntiAir(defensesBuilders);
		}
		const def = returnBestDefense();
		if (def !== null) {
			const buildPos = getDefenseBuildPosition(defensesBuilders);
			grabTrucksAndBuild(def, buildPos, defensesBuilders, true);
		}
	}

	// 3. Upgrade completed generators, research labs, and factories with modules
	baseMaintenance();

	// 4. Build fundamental structures (Research Labs, Factories, Repair Facilities, VTOL Pads, Uplink/LasSat, Defenses)
	buildFundamentals();

	// 5. Any remaining idle base builders help finish uncompleted structures near the base
	checkUncompletedStructures();
}

// Linear base bootstrap so that in a "No Bases / No Research" match every idle truck immediately gets a distinct job
function buildBasicBase() {
	let assignedAny = false;
	const trucks = enumDroid(me, DROID_CONSTRUCT);

	// Priority 1: 1st Tank Factory (immediate truck & unit production)
	if (countStructWithPending(FACTORY_STAT) === 0 && grabTrucksAndBuild(FACTORY_STAT, BASE, trucks)) {
		assignedAny = true;
	}

	// Priority 2: 1st Research Lab (start unlocking weapons & engineering immediately)
	if (!researchDone && countStructWithPending(RES_LAB_STAT) === 0 && grabTrucksAndBuild(RES_LAB_STAT, BASE, trucks)) {
		assignedAny = true;
	}

	// Priority 3: 1st Power Generator (start generating power from derricks & oil drums)
	if (countStructWithPending(POW_GEN_STAT) === 0 && grabTrucksAndBuild(POW_GEN_STAT, BASE, trucks)) {
		assignedAny = true;
	}

	// Priority 4: Command Center / HQ (unlocks radar & tank design)
	if (countStructWithPending(PLAYER_HQ_STAT) === 0 && grabTrucksAndBuild(PLAYER_HQ_STAT, BASE, trucks)) {
		assignedAny = true;
	}

	// Stop here if any of the Big 4 core buildings haven't even been started yet
	if (countStructWithPending(FACTORY_STAT) === 0 ||
		(!researchDone && countStructWithPending(RES_LAB_STAT) === 0) ||
		countStructWithPending(POW_GEN_STAT) === 0 ||
		countStructWithPending(PLAYER_HQ_STAT) === 0) {
		return assignedAny;
	}

	// Priority 5: Additional Power Generator whenever we have more derricks than generators can support
	const neededGens = Math.max(1, Math.ceil(countStruct(DERRICK_STAT) / 4));
	if (countStructWithPending(POW_GEN_STAT) < neededGens && grabTrucksAndBuild(POW_GEN_STAT, BASE, trucks)) {
		assignedAny = true;
	}

	// Priority 6: 2nd Research Lab (crucial in "No Research" starts to research weapons + propulsion in parallel)
	if (!researchDone && countStructWithPending(RES_LAB_STAT) < 2 && grabTrucksAndBuild(RES_LAB_STAT, BASE, trucks)) {
		assignedAny = true;
	}

	// Priority 7: 1st Cyborg Factory (as soon as researched on land maps)
	if (!isSeaMap && isStructureAvailable(CYBORG_FACTORY_STAT) && countStructWithPending(CYBORG_FACTORY_STAT) === 0 && grabTrucksAndBuild(CYBORG_FACTORY_STAT, BASE, trucks)) {
		assignedAny = true;
	}

	// Priority 8: 1st Repair Facility on outer base perimeter (as soon as researched)
	if (isStructureAvailable(REPAIR_FACILITY_STAT) && countStructWithPending(REPAIR_FACILITY_STAT) === 0) {
		const perimeterPos = getOuterBaseRepairPosition();
		if (grabTrucksAndBuild(REPAIR_FACILITY_STAT, perimeterPos, trucks)) {
			assignedAny = true;
		}
	}

	// Priority 9: 3rd Research Lab once we have at least 4 derricks or healthy power
	if (!researchDone && (countStruct(DERRICK_STAT) >= 4 || getRealPower() >= 200) && countStructWithPending(RES_LAB_STAT) < 3 && grabTrucksAndBuild(RES_LAB_STAT, BASE, trucks)) {
		assignedAny = true;
	}

	// Priority 10: 2nd Tank Factory once we have at least 4 derricks or healthy power
	if ((countStruct(DERRICK_STAT) >= 4 || getRealPower() >= 250) && countStructWithPending(FACTORY_STAT) < 2 && grabTrucksAndBuild(FACTORY_STAT, BASE, trucks)) {
		assignedAny = true;
	}

	return assignedAny;
}

// Send any remaining idle trucks to help finish uncompleted structures near the base
function checkUncompletedStructures() {
	let hasAssignedTask = false;

	// Find uncompleted structures near the base, sorted closest to base first
	let structlist = enumStruct(me).filter((structure) => (
		structure.status !== BUILT &&
		distBetweenTwoPoints(BASE.x, BASE.y, structure.x, structure.y) < 40
	)).sort(sortByDistToBase);

	if (structlist.length === 0) {
		return false;
	}

	// Gather idle trucks from baseBuildersGroup first so defense builders stay focused on building new defenses
	const idleTrucks = findIdleTrucks(structlist[0], baseBuildersGroup);
	if (idleTrucks.length === 0) {
		return false;
	}

	let structIdx = 0;
	for (let j = 0; j < idleTrucks.length; ++j) {
		const targetStruct = structlist[structIdx % structlist.length];
		if (canDroidHelp(idleTrucks[j], targetStruct.x, targetStruct.y)) {
			if (orderDroidObj(idleTrucks[j], DORDER_HELPBUILD, targetStruct)) {
				hasAssignedTask = true;
				structIdx++;
			}
		}
	}

	return hasAssignedTask;
}

// Build modules and check research completion.
function baseMaintenance() {
	// Check if there are no power generators.
	if (countStruct(POW_GEN_STAT) === 0) {
		return false;
	}

	// Minimum power required for a module to be considered for upgrade.
	const MIN_POWER_FOR_MODULE = -50;

	// letiables for storing the selected structure and module for upgrade.
	let struct = null;
	let module = "";

	// letiable to track the success of the maintenance process.
	let success = false;

	// List of modules with their required amount and corresponding structure type.
	const moduleList = [
		{ "mod": "A0PowMod1", "amount": 1, "structure": POW_GEN_STAT },
		{ "mod": "A0ResearchModule1", "amount": 1, "structure": RES_LAB_STAT },
		{ "mod": "A0FacMod1", "amount": 2, "structure": FACTORY_STAT },
		{ "mod": "A0FacMod1", "amount": 2, "structure": VTOL_FACTORY_STAT }
	];

	// Iterate through the module list to find a suitable structure for upgrade.
	for (let i = 0; i < moduleList.length; ++i) {
		if (isStructureAvailable(moduleList[i].mod)) {
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

	// Check if there is a selected structure and either the power is sufficient or it's a power module,
	// or if the count of DERRICK_STAT structures is greater than or equal to 12.
	if (struct && (getRealPower() > MIN_POWER_FOR_MODULE || module === "A0PowMod1" || countStruct(DERRICK_STAT) >= 12)) {

		// Find all idle trucks
		const idleTrucks = findIdleTrucks();

		// Assign up to 2 idle trucks to build the module
		let assigned = 0;
		for (const truck of idleTrucks) {
			// Check if the truck can help with building at the selected location
			if (canDroidHelp(truck, struct.x, struct.y)) {
				if (orderDroidBuild(truck, DORDER_BUILD, module, struct.x, struct.y)) {
					success = true;
					assigned++;
					if (assigned >= 2) break;
				}
			}
		}
	}

	// Check if it's a sea map, then demolish cyborg factories
	if (demolishCyborgFactories()) {
		success = true;
	}

	// Return the final success status.
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

	// Build Anti-VTOL defenses in front of essential base structures if enemies have VTOLs
	if (enemiesHaveVTOLs && shouldBuildAntiAirNow() && buildAntiAir()) {
		return true;
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
	// If the game time is less than the retry time, exit early
	if (gameTime < lastAttemptTime) {
		return false;
	}

	// If not enough power is available, exit
	if (getRealPower() < MIN_BUILD_POWER) {
		return false;
	}

	const baseDefensesCount = enumRange(BASE.x, BASE.y, BASE_THREAT_RANGE, me, false)
		.filter(obj => obj.type === STRUCTURE && obj.stattype === DEFENSE).length;

	// Priority 0: If enemies have VTOLs, ensure our forward AA shield in front of essential structures is built
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

	for (const derrick of derricks) {
		const hasDerrickEnoughDefenses = enumRange(derrick.x, derrick.y, 7, me, true)
			.filter(obj => obj.stattype === DEFENSE).length >= 3;

		// build defenses if derrick has not enough
		if (!hasDerrickEnoughDefenses) {
			let def = returnBestDefense(true);
			if (def !== null) {
				return grabTrucksAndBuild(def, derrick, null, true);
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
	const minRequiredArmy = isHighOilMap ? 12 : 6;

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
				+ enumDroid(enemy.position, DROID_CYBORG, true).length;
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
	const ARTILLERY_CHANCE = 75;
	const BASTION_CHANCE = 15;
	const EMP_CHANCE = 25;
	const SENSOR_CHANCE = 5;

	let defenses;
	let bestDefense;

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

	return bestDefense;
}

// Determines if we should prioritize building an Anti-Air defense right now
function shouldBuildAntiAirNow() {
	if (!enemiesHaveVTOLs) {
		return false;
	}
	const aaCount = AA_DEFENSES.reduce((sum, stat) => sum + countStructWithPending(stat), 0);
	// Always build until we have at least 6 AA defenses in front of our essential structures; afterwards, 35% chance
	if (aaCount < 6) {
		return true;
	}
	return random(100) < 35;
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
		// 1. Immediate danger check outside base
		if (!isNearBase(truck.x, truck.y)) {
			const isUnsafe = enumRange(truck.x, truck.y, 12, ENEMIES, false)
				.some(isUnsafeEnemyObject) || isInDangerZone(truck.x, truck.y, 12);

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
		const isActivelyBuildingNearby = (truck.order === DORDER_BUILD || truck.order === DORDER_HELPBUILD || truck.order === DORDER_LINEBUILD) &&
			uncompletedStructs.some(s => distBetweenTwoPoints(truck.x, truck.y, s.x, s.y) <= 4);
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

// Build factories. Attempts to build at least 1 of each factory.
function factoryBuildOrder() {
	const FAC_ORDER = [FACTORY_STAT, CYBORG_FACTORY_STAT, VTOL_FACTORY_STAT];
	const derrNum = countStruct(DERRICK_STAT);
	let num = 1;

	if (derrNum >= 20) {
		num = 5;
	} else if (derrNum >= 16) {
		num = 4;
	} else if (derrNum >= 12) {
		num = 3;
	} else if (derrNum >= 6) {
		num = 2;
	}

	for (const fac of FAC_ORDER) {
		if (!isStructureAvailable(fac)) {
			continue;
		}
		// If the fac is a CYBORG_FACTORY and the map is marine, go to the next iteration
		if (fac === CYBORG_FACTORY_STAT && isSeaMap) {
			continue;
		}

		// Check if there are enough such factories already built or pending
		if (countStructWithPending(fac) < num) {
			if (grabTrucksAndBuild(fac)) {
				return true;
			}
		}
	}

	return false;
}

// Build the research labs. Attempts to build up to a certain number based on derricks.
function buildResearchLabs() {
	if (researchDone || !isStructureAvailable(RES_LAB_STAT)) {
		return false;
	}

	const derrCount = countStruct(DERRICK_STAT);
	let amount = 2;

	// Scale research labs linearly with derricks or available power
	if (derrCount >= 18 || getRealPower() >= 1000) {
		amount = 5;
	} else if (derrCount >= 10 || getRealPower() >= 500) {
		amount = 4;
	} else if (derrCount >= 4 || getRealPower() >= 200) {
		amount = 3;
	}

	const currentLabs = countStructWithPending(RES_LAB_STAT);

	if (currentLabs < amount) {
		if (grabTrucksAndBuild(RES_LAB_STAT)) {
			return true;
		}
	}

	return false;
}


// Demolish old defenses if an upgrade is available
function demolishOldDefenses() {
	const upgradeMap = [
		{ new: "Emplacement-Rocket06-IDF", old: WEAK_DEFENSES },
		{ new: "Sys-SensoTowerWS", old: ["Sys-SensoTower02"] },
		{ new: "Emplacement-HvART-pit", old: ["Emplacement-Rocket06-IDF"] },
		{ new: "X-Super-MassDriver", old: ["X-Super-Cannon"] }
	];

	for (const { new: newStruct, old: oldStructs } of upgradeMap) {
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
// Falls back to all constructor droids if the primary group has no free trucks so no truck ever sits idle.
function findIdleTrucks(obj, builderGroupName) {
	if (obj === null || obj === undefined) {
		obj = BASE;
	}

	let builders = builderGroupName ? enumGroup(builderGroupName) : enumGroup(baseBuildersGroup);
	let droidlist = builders.filter(d => canDroidHelp(d, obj.x, obj.y));

	// If the requested group has no free trucks, fall back to all constructor droids so idle trucks in the other group help out
	if (droidlist.length === 0 && !builderGroupName) {
		droidlist = enumDroid(me, DROID_CONSTRUCT).filter(d => canDroidHelp(d, obj.x, obj.y));
	}

	droidlist.sort((a, b) => distBetweenTwoPoints(a.x, a.y, obj.x, obj.y) - distBetweenTwoPoints(b.x, b.y, obj.x, obj.y));
	return droidlist;
}

// Demolish object.
function demolishThis(structure) {
	if (!structure) return false;
	let success = false;
	let droidList = findIdleTrucks(structure, defensesBuildersGroup);
	if (droidList.length === 0) {
		droidList = findIdleTrucks(structure, baseBuildersGroup);
	}

	for (let i = 0, d = droidList.length; i < d; ++i) {
		if (canDroidHelp(droidList[i], structure.x, structure.y)) {
			if (orderDroidObj(droidList[i], DORDER_DEMOLISH, structure)) {
				success = true;
				break; // One truck is enough to demolish a structure
			}
		}
	}

	return success;
}

// Function to grab available trucks and start building a structure.
// When `allowMultipleLocations` is true (used for defenses), each idle truck picks its own build spot so multiple defenses go up simultaneously!
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

	// For defenses (allowMultipleLocations === true), let every available truck in the list start its own defense!
	if (allowMultipleLocations) {
		let buildSuccessful = false;
		for (const truck of idleTrucks) {
			let loc = pickStructLocation(truck, structure, base.x, base.y, 0);
			if (!loc) {
				loc = pickStructLocation(truck, structure, base.x, base.y, 1);
			}
			if (loc && canDroidHelp(truck, loc.x, loc.y)) {
				if (orderDroidBuild(truck, DORDER_BUILD, structure, loc.x, loc.y)) {
					if (typeof pendingBuildOrders !== "undefined") {
						pendingBuildOrders.set(truck.id, { stat: structure, x: loc.x, y: loc.y, time: gameTime });
					}
					buildSuccessful = true;
				}
			}
		}
		return buildSuccessful;
	}

	const firstTruck = idleTrucks[0];
	// Try maxBlockingTiles = 0 first; if the base is tight/crowded, fall back to maxBlockingTiles = 1
	let buildLocation = pickStructLocation(firstTruck, structure, base.x, base.y, 0);
	if (!buildLocation) {
		buildLocation = pickStructLocation(firstTruck, structure, base.x, base.y, 1);
	}

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
	if (mydroid.order === DORDER_BUILD ||
		mydroid.order === DORDER_HELPBUILD ||
		mydroid.order === DORDER_LINEBUILD ||
		mydroid.order === DORDER_RECYCLE ||
		mydroid.order === DORDER_DEMOLISH ||
		mydroid.order === DORDER_RECOVER ||
		mydroid.order === DORDER_RTB) {
		return false;
	}

	// Check reachability to (bx, by) or its immediate adjacent tiles (in case (bx, by) is occupied by HQ or an existing structure)
	return droidCanReach(mydroid, bx, by) ||
		droidCanReach(mydroid, bx + 2, by) ||
		droidCanReach(mydroid, bx - 2, by) ||
		droidCanReach(mydroid, bx, by + 2) ||
		droidCanReach(mydroid, bx, by - 2);
}

function lookForOil() {
	// Also try to grab nearby free Oil Drums and Artifacts (Crates)
	grabFreeObjects();

	// Limit the number of "extra" derricks. Each power generator can only handle 4 derricks.
	// If we have 10 more derricks than our power generators can process, stop capturing more until we build another generator.
	if (countStruct(DERRICK_STAT) - (countStruct(POW_GEN_STAT) * 4) >= 10) {
		return false;
	}

	// Get all available idle constructor droids from baseBuildersGroup (keep defensesBuildersGroup dedicated to defenses!)
	let droids = enumGroup(baseBuildersGroup).filter(droid =>
		droid.order !== DORDER_BUILD &&
		droid.order !== DORDER_HELPBUILD &&
		droid.order !== DORDER_LINEBUILD &&
		droid.order !== DORDER_RECYCLE &&
		droid.order !== DORDER_DEMOLISH &&
		droid.order !== DORDER_RECOVER &&
		droid.order !== DORDER_RTB
	);

	// In a "No Bases" start where core structures aren't even started yet, reserve trucks near BASE to start the base first
	const missingCoreBuildings =
		(countStructWithPending(FACTORY_STAT) === 0 ? 1 : 0) +
		(!researchDone && countStructWithPending(RES_LAB_STAT) === 0 ? 1 : 0) +
		(countStructWithPending(POW_GEN_STAT) === 0 ? 1 : 0) +
		(countStructWithPending(PLAYER_HQ_STAT) === 0 ? 1 : 0);

	if (missingCoreBuildings > 0) {
		droids.sort((a, b) => distBetweenTwoPoints(a.x, a.y, BASE.x, BASE.y) - distBetweenTwoPoints(b.x, b.y, BASE.x, BASE.y));
		droids = droids.slice(missingCoreBuildings); // Reserve closest trucks for missing core buildings
	}

	if (droids.length === 0) return false;

	const UNSAFE_AREA_RANGE = 18;
	let success = false;

	// Allied base positions
	let allyBasePositions = alliesList.map((a) => a.base);

	// Get all constructor droids that are currently building something
	let busyTrucks = enumDroid(me, DROID_CONSTRUCT).filter(truck =>
		truck.order === DORDER_BUILD || truck.order === DORDER_HELPBUILD
	);

	// Pre-filter oils with cheap distance checks and sort by distance to base
	let oils = enumFeature(ALL_PLAYERS, OIL_RES_STAT)
		.filter((oil) => {
			let tooCloseToAlly = allyBasePositions.some(base =>
				base && distBetweenTwoPoints(base.x, base.y, oil.x, oil.y) < 15
			);
			if (tooCloseToAlly) return false;

			// Check if any of our trucks are already building near this oil
			let alreadyTargeted = busyTrucks.some(truck =>
				distBetweenTwoPoints(truck.x, truck.y, oil.x, oil.y) < 5
			);
			return !alreadyTargeted;
		})
		.sort(sortByDistToBase);

	if (oils.length === 0) return false;

	// Assign one truck per oil — closest idle truck to each oil
	let assignedDroids = new Set();

	for (const oil of oils) {
		if (assignedDroids.size >= droids.length) break; // All idle trucks assigned!

		// Check if the oil area AND the midpoint along the route are safe from all enemies and ambushes
		if (!isAreaSafe(oil.x, oil.y, UNSAFE_AREA_RANGE)) continue;

		let bestDroid = null;
		let bestDist = 99999;

		for (const droid of droids) {
			if (assignedDroids.has(droid.id)) continue;

			let dist = distBetweenTwoPoints(droid.x, droid.y, oil.x, oil.y);
			if (dist < bestDist && droidCanReach(droid, oil.x, oil.y)) {
				// Also verify that the midpoint along the truck's travel path isn't blocked by enemies
				const midX = Math.round((droid.x + oil.x) / 2);
				const midY = Math.round((droid.y + oil.y) / 2);
				if (dist > 20 && !isAreaSafe(midX, midY, 12)) {
					continue;
				}
				bestDist = dist;
				bestDroid = droid;
			}
		}

		if (bestDroid) {
			assignedDroids.add(bestDroid.id);
			if (orderDroidBuild(bestDroid, DORDER_BUILD, DERRICK_STAT, oil.x, oil.y)) {
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

	// Use idle combat units or defense builders so we don't slow down base oil trucks
	const collectors = enumGroup(attackerGroup)
		.concat(enumGroup(defensesBuildersGroup))
		.filter(d =>
			d.order !== DORDER_RECOVER &&
			d.order !== DORDER_BUILD &&
			d.order !== DORDER_HELPBUILD &&
			d.order !== DORDER_RTR
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
			orderDroidObj(bestDroid, DORDER_RECOVER, item);
			count++;
		}
	}

	return count > 0;
}
