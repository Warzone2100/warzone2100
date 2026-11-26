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

// Game time at which research first had to wait for a missing Power Generator (0 = not waiting)
var researchHoldSince = 0;

// True while research should wait because the power is needed for something that comes first.
// These are short bootstrap situations only; none of them can keep the labs idle for long.
function shouldHoldResearch() {
	const power = getRealPower();

	// No truck left: keep enough power to build one before anything else
	if (countDroid(DROID_CONSTRUCT) === 0 && power < 100) {
		return true;
	}

	// The very first generator is not placed / not finished yet
	const totalGens = countStruct(POW_GEN_STAT, me);
	const completedGens = countCompletedStruct(POW_GEN_STAT, me);
	if ((totalGens === 0 && power < 150) || (completedGens === 0 && totalGens > 0 && power < 60)) {
		return true;
	}

	// Derricks are waiting for a generator that has not been placed yet: hold the labs so the generator is paid first,
	// but for 30 seconds at most. (If the generator cannot be placed for any reason, research must not stall with it.)
	const neededGens = Math.ceil(countStruct(DERRICK_STAT, me) / 4);
	if (numUnusedDerricks() > 0 && totalGens < neededGens && power < 120) {
		if (researchHoldSince === 0 || gameTime < researchHoldSince) {
			researchHoldSince = gameTime;
		}
		return gameTime - researchHoldSince < 30000;
	}

	researchHoldSince = 0;
	return false;
}

// Last resort for a lab when every branch of the plan is finished or waiting for a topic another lab is working on:
// the cheapest available topic outside the weapon lines Wally does not use. An off-plan upgrade beats an idle lab.
function pursueFallbackResearch(lab) {
	const skipVtol = shouldSkipVtolResearch();
	const available = enumResearch();
	const usable = available.filter(r =>
		!r.name.includes("Cannon") &&
		!r.name.includes("Acannon") &&
		!r.name.includes("Mcannon") &&
		!r.name.includes("HPV") &&
		!r.name.includes("Mortar") &&
		!r.name.includes("Howitzer") &&
		!r.name.includes("Flame") &&
		!r.name.includes("Laser") &&
		!r.name.includes("RailGun") &&
		!r.name.includes("Rail-") &&
		!(skipVtol && isVtolResearch(r.name)) &&
		!(!vtolsEnabled && (r.name.includes("AAGun") || r.name.includes("SAM") || r.name.includes("Sunburst")))
	);
	const pool = (usable.length > 0 ? usable : available).slice().sort((a, b) => a.points - b.points);

	for (let i = 0; i < pool.length; ++i) {
		if (pursueResearch(lab, pool[i].name)) {
			return true;
		}
	}
	return false;
}

// Gives a research topic to every idle Research Lab.
//
// There is deliberately NO power threshold here any more. A lab only pays once, when its topic starts, and then works
// for one to three minutes: research costs about half a power point per second and lab (roughly what one derrick with
// its generator produces). How much of the economy goes into research is therefore decided by how many labs get BUILT
// (see getDesiredLabCount in build.js: 2 labs up to 4 derricks, 3 up to 7, 4 up to 9, then 5), and every lab that
// exists is always kept working. A topic started without enough power simply waits its turn in the game's power queue.
function research() {
	if (researchDone) {
		return;
	}

	const builtLabs = enumStruct(me, RES_LAB_STAT).filter(lab => lab.status === BUILT);
	const labList = builtLabs.filter(lab => structureIdle(lab));

	if (labList.length === 0) {
		return;
	}

	if (shouldHoldResearch()) {
		return;
	}

	const totalLabs = builtLabs.length;
	const busyLabs = totalLabs - labList.length;

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
	let started = 0;

	for (const lab of labList) {
		let found = false;

		// Try each research priority
		for (const researchCategory of researchPriorities) {
			if (found) break;
			found = evalResearch(lab, researchCategory);
		}

		if (!found) {
			found = pursueFallbackResearch(lab);
		}

		if (found) {
			started++;
		}
	}

	if (started < labList.length) {
		wallyLog("research: " + (labList.length - started) + " lab(s) found no topic (" + allResearchList.length + " topics available)");
	}
}

// A topic just finished: give that lab its next one right away instead of waiting for the research timer.
// (Only at full reaction speed: on Easy and Medium the announced reaction delay still applies.)
function eventResearched(finishedResearch, structure, player) {
	if (player !== undefined && player !== me) {
		return;
	}
	if (reactionSpeedDelay === 0 && structure && !throttled("researchOnEvent", 200)) {
		research();
	}
}
