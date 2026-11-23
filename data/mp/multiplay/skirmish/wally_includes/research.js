function isPowerResearch(research) {
	const POWERS = [
		"R-Struc-Power-Upgrade01",
		"R-Struc-Power-Upgrade01b",
		"R-Struc-Power-Upgrade01c",
		"R-Struc-Power-Upgrade02",
		"R-Struc-Power-Upgrade03",
		"R-Struc-Power-Upgrade03a",
	];

	for (let i = 0, len = POWERS.length; i < len; ++i) {
		if (research === POWERS[i]) {
			return true;
		}
	}

	return false;
}

// VTOL technology (propulsion, factory, pads, bombs)
function isVtolResearch(research) {
	return research.includes("VTOL") || research.startsWith("R-Wpn-Bomb");
}

// VTOL technology is skipped when VTOLs are disabled in the game, and postponed on low oil
// until the economy can pay for an air force (same 8-derrick rule as the VTOL factory)
function shouldSkipVtolResearch() {
	return !vtolsEnabled || (!isHighOilMap && countStruct(DERRICK_STAT) < 8);
}

function isResearchDone(research) {
	const info = getResearch(research);
	return info !== null && info !== undefined && info.done === true;
}

// Research that must not wait for its turn in the tier lists.
// The Super Scourge Cyborg is promoted the moment the Scourge Missile is finished: it is the only Scourge unit
// that can also shoot VTOLs. Before that, the Super Tank-Killer Cyborg gets the same treatment after the Tank Killer.
function getHotResearch() {
	const hot = [];
	if (!isSeaMap) {
		if (isResearchDone("R-Wpn-Missile2A-T")) {
			hot.push("R-Cyborg-Hvywpn-A-T");
		} else if (isResearchDone("R-Wpn-Rocket07-Tank-Killer")) {
			hot.push("R-Cyborg-Hvywpn-TK");
		}
	}
	return hot;
}

function evalResearch(lab, list) {
	const sufficientPower = getRealPower() > 15000;
	const skipVtol = shouldSkipVtolResearch();

	for (let i = 0; i < list.length; ++i) {
		if (sufficientPower && isPowerResearch(list[i])) {
			// Skip power research if we have enough power
			continue;
		}
		if (isSeaMap && list[i] === "R-Struc-Factory-Cyborg") {
			continue;
		}
		if (skipVtol && isVtolResearch(list[i])) {
			continue;
		}

		if (pursueResearch(lab, list[i])) {
			return true; // Research found and pursued
		}
	}

	return false; // No research pursued
}

// Builds the ordered list of research branches for the current game situation
function getResearchPriorities() {
	const lowOil = !isHighOilMap;
	const antiAirNeeded = vtolsEnabled && enemiesHaveVTOLs;

	return [
		isSeaMap ? ["R-Vehicle-Prop-Hover"] : null,
		START_COMPONENTS,
		getHotResearch(),

		// Enemy VTOLs are flying: get a working AA weapon before anything else
		antiAirNeeded ? AA_RESEARCH_EARLY : null,

		// T1 (unified Rocket/Missile tree + Machinegun line + economy)
		FUNDAMENTALS_ROCKET_WEAPON_T1,
		FUNDAMENTALS_MACHINEGUN_WEAPON_T1,
		lowOil ? LOW_OIL_ECONOMY_T1 : null,
		FUNDAMENTALS_BODY_T1,
		FUNDAMENTALS_PROPULSION_T1,
		FUNDAMENTALS_UPGRADES_T1,
		FUNDAMENTALS_DEFENCE_T1,

		// T2
		FUNDAMENTALS_ROCKET_WEAPON_T2,
		FUNDAMENTALS_MACHINEGUN_WEAPON_T2,
		vtolsEnabled ? AA_RESEARCH_PROACTIVE : null,
		lowOil ? LOW_OIL_ECONOMY_T2 : null,
		antiAirNeeded ? AA_RESEARCH_MID : null,
		FUNDAMENTALS_BODY_T2,
		FUNDAMENTALS_PROPULSION_T2,
		FUNDAMENTALS_UPGRADES_T2,
		FUNDAMENTALS_DEFENCE_T2,

		// T3 (Rocket/Missile + Twin Assault Gun, HEAP/Thermite VTOL bombs alongside Rocket VTOLs)
		FUNDAMENTALS_ROCKET_WEAPON_T3,
		FUNDAMENTALS_MACHINEGUN_WEAPON_T3,
		FUNDAMENTALS_BODY_T3,
		FUNDAMENTALS_VTOL_BOMBS_T2,
		FUNDAMENTALS_VTOL_BOMBS_T3,
		FUNDAMENTALS_PROPULSION_T3,
		FUNDAMENTALS_UPGRADES_T3,
		FUNDAMENTALS_DEFENCE_T3,

		// T4
		FUNDAMENTALS_ROCKET_WEAPON_T4,
		antiAirNeeded ? AA_RESEARCH_LATE : null,
		FUNDAMENTALS_MACHINEGUN_WEAPON_T4,
		FUNDAMENTALS_BODY_T4,
		FUNDAMENTALS_DEFENCE_T4,
		FUNDAMENTALS_PROPULSION_T4,
		FUNDAMENTALS_UPGRADES_T4
	].filter(list => list && list.length > 0);
}

function research() {
	// Don't research if:
	// We don't have builders
	// We finished all the researches
	if (!countDroid(DROID_CONSTRUCT) || researchDone) {
		return;
	}

	const builtLabs = enumStruct(me, RES_LAB_STAT).filter(lab => lab.status === BUILT);
	let labList = builtLabs.filter(lab => structureIdle(lab));

	if (labList.length === 0) {
		return;
	}

	// Preserve power only before our first generator is placed/built or when an additional unplaced generator is needed;
	// avoid holding labs idle once Gen #1 exists and needed generators are already queued
	const totalGens = countStruct(POW_GEN_STAT, me);
	const completedGens = enumStruct(me, POW_GEN_STAT).filter(s => s.status === BUILT).length;
	const neededGens = Math.ceil(countStruct(DERRICK_STAT, me) / 4);
	if ((totalGens === 0 && getRealPower() < 150) ||
		(completedGens === 0 && totalGens > 0 && getRealPower() < 60) ||
		(numUnusedDerricks() > 0 && totalGens < neededGens && getRealPower() < 120)) {
		return;
	}

	const totalLabs = builtLabs.length;
	let busyLabs = totalLabs - labList.length;

	// Power rules. High oil: a busy lab only needs the usual minimum.
	// Low oil: research and the army share the same trickle of power, so
	//  - the number of labs working at once follows the derrick count (unless power is piling up), and
	//  - while the army is still below what the economy can support, the factories get first call on power.
	let maxBusyLabs = totalLabs;
	let firstLabPowerFloor = -Infinity;
	let extraLabPowerFloor = MIN_RESEARCH_POWER;
	if (!isHighOilMap) {
		if (getRealPower() <= 300) {
			maxBusyLabs = Math.max(1, Math.min(totalLabs, Math.ceil(countStruct(DERRICK_STAT, me) / 2)));
		}
		if (isArmyThin() && !isMyBaseInTrouble) {
			firstLabPowerFloor = 20;
			extraLabPowerFloor = 80;
		} else {
			extraLabPowerFloor = 20;
		}
	}

	if (busyLabs >= maxBusyLabs || getRealPower() < (busyLabs === 0 ? firstLabPowerFloor : extraLabPowerFloor)) {
		return;
	}

	// List of all currently available research items
	const allResearchList = enumResearch();
	if (allResearchList.length === 0) {
		// IMPORTANT: In a "No Research" start, there are only a few root tech nodes at the beginning.
		// If busyLabs > 0, other labs are currently researching prerequisites that will unlock the rest of the tree!
		// Only mark researchDone if ALL labs are idle (busyLabs === 0) AND end-game Dragon body ("Body14SUP") is already unlocked.
		if (busyLabs === 0 && totalLabs > 0 && componentAvailable("Body14SUP")) {
			researchDone = true;
		}
		return;
	}

	const researchPriorities = getResearchPriorities();

	for (let lab of labList) {
		// Re-check the power rules for every lab we start in this tick
		if (busyLabs >= maxBusyLabs || getRealPower() < (busyLabs === 0 ? firstLabPowerFloor : extraLabPowerFloor)) {
			break;
		}

		let found = false;

		// Try each research priority
		for (let researchCategory of researchPriorities) {
			if (found) break;
			found = evalResearch(lab, researchCategory);
		}

		// Last step: if all priority branches are currently locked by another lab's in-progress prerequisite,
		// avoid drifting into off-branch Cannon/Mortar/Howitzer/Flame/Laser trees while the Rocket and Machinegun branches are active.
		// On low oil an idle lab is cheaper than an off-plan topic, unless power is piling up.
		if (!found && (isHighOilMap || getRealPower() >= 150)) {
			const skipVtol = shouldSkipVtolResearch();
			const currentAvailable = enumResearch();
			const filteredAvailable = currentAvailable.filter(r =>
				!r.name.includes("Cannon") &&
				!r.name.includes("Mortar") &&
				!r.name.includes("Howitzer") &&
				!r.name.includes("Flame") &&
				!r.name.includes("Laser") &&
				!r.name.includes("RailGun") &&
				!(skipVtol && isVtolResearch(r.name)) &&
				!(!vtolsEnabled && (r.name.includes("AAGun") || r.name.includes("SAM") || r.name.includes("Sunburst")))
			);
			const pool = filteredAvailable.length > 0 ? filteredAvailable : currentAvailable;
			if (pool.length > 0) {
				const idx = Math.floor(Math.random() * pool.length);
				found = pursueResearch(lab, pool[idx].name);
			}
		}

		if (found) {
			busyLabs++;
		}
	}
}
