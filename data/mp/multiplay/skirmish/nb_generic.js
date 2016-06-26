
/*
 * This file defines a standard AI personality for the base game. 
 * 
 * It relies on ruleset definition in /rulesets/ to provide
 * standard strategy descriptions and necessary game stat information.
 * 
 * Then it passes control to the main code.
 * 
 */

// You can redefine these paths when you make a customized AI
// for a map or a challenge.
NB_PATH = "/multiplay/skirmish/";
NB_INCLUDES = NB_PATH + "nb_includes/";
NB_RULESETS = NB_PATH + "nb_rulesets/";
NB_COMMON = NB_PATH + "nb_common/";

// please don't touch this line
include(NB_INCLUDES + "_head.js");

////////////////////////////////////////////////////////////////////////////////////////////
// Start the actual personality definition

// the rules in which this personality plays
include(NB_RULESETS + "standard.js");
include(NB_COMMON + "standard_build_order.js");

// variables defining the personality
var subpersonalities = {
	MR: {
		chatalias: "mr",
		weaponPaths: [ // weapons to use; put late-game paths below!
			weaponStats.rockets_AT, 
			weaponStats.machineguns, 
			weaponStats.rockets_AS, 
			weaponStats.rockets_AA, 
			weaponStats.rockets_Arty,
		],
		earlyResearch: [ // fixed research path for the early game
			"R-Wpn-MG-Damage01",
			"R-Defense-Tower01",
			"R-Vehicle-Prop-Halftracks",
			"R-Struc-PowerModuleMk1",
			"R-Wpn-MG-Damage03",
		],
		minTanks: 1, // minimal attack force at game start
		becomeHarder: 3, // how much to increase attack force every 5 minutes
		maxTanks: 16, // maximum for the minTanks value (since it grows at becomeHarder rate)
		minTrucks: 2, // minimal number of trucks around
		minHoverTrucks: 3, // minimal number of hover trucks around
		maxSensors: 1, // number of mobile sensor cars to produce
		minMiscTanks: 1, // number of tanks to start harassing enemy
		maxMiscTanks: 2, // number of tanks used for defense and harass
		vtolness: 65, // the chance % of not making droids when adaptation mechanism chooses vtols
		defensiveness: 65, // same thing for defenses; set this to 100 to enable turtle AI specific code
		maxPower: 700, // build expensive things if we have more than that
		repairAt: 50, // how much % healthy should droid be to join the attack group instead of repairing
	},
	MC: {
		chatalias: "mc",
		weaponPaths: [
			weaponStats.cannons, 
			weaponStats.machineguns, 
			weaponStats.mortars,
			weaponStats.cannons_AA,
		],
		earlyResearch: [
			"R-Wpn-MG-Damage01",
			"R-Vehicle-Prop-Halftracks",
			"R-Wpn-Cannon1Mk1",
			"R-Struc-PowerModuleMk1",
			"R-Wpn-MG-Damage03",
		],
		minTanks: 1, becomeHarder: 3, maxTanks: 16,
		minTrucks: 3, minHoverTrucks: 4, maxSensors: 1,
		minMiscTanks: 1, maxMiscTanks: 2,
		vtolness: 65, defensiveness: 65,
		maxPower: 700,
		repairAt: 50,
	},
	FR: {
		chatalias: "fr",
		weaponPaths: [
			weaponStats.rockets_AT, 
			weaponStats.flamers,
			weaponStats.fireMortars,
			weaponStats.rockets_AA,
			weaponStats.lasers,
		],
		earlyResearch: [
			"R-Wpn-Flamer-ROF01",
			"R-Vehicle-Prop-Halftracks",
			"R-Struc-PowerModuleMk1",
		],
		minTanks: 1, becomeHarder: 3, maxTanks: 16,
		minTrucks: 3, minHoverTrucks: 4, maxSensors: 1,
		minMiscTanks: 1, maxMiscTanks: 2,
		vtolness: 65, defensiveness: 65,
		maxPower: 700,
		repairAt: 50,
	},
	FC: {
		chatalias: "fc",
		weaponPaths: [
			weaponStats.cannons, 
			weaponStats.flamers, 
			weaponStats.fireMortars,
			weaponStats.cannons_AA,
			weaponStats.lasers,
		],
		earlyResearch: [
			"R-Wpn-Flamer-ROF01",
			"R-Vehicle-Prop-Halftracks",
			"R-Wpn-Cannon1Mk1",
			"R-Struc-PowerModuleMk1",
		],
		minTanks: 1, becomeHarder: 3, maxTanks: 16,
		minTrucks: 3, minHoverTrucks: 4, maxSensors: 1,
		minMiscTanks: 1, maxMiscTanks: 2,
		vtolness: 65, defensiveness: 65,
		maxPower: 700,
		repairAt: 50,
	},
};

// this function describes the early build order
// you can rely on personality.chatalias for choosing different build orders for
// different subpersonalities
function buildOrder() {
	// Only use this build order in early game, on standard difficulty, in T1 no bases.
	// Otherwise, fall back to the safe build order.
	if (gameTime > 300000 || difficulty === INSANE
	                      || isStructureAvailable("A0ComDroidControl") || baseType !== CAMP_CLEAN)
		return buildOrder_StandardFallback();
	if (personality.chatalias === "fc" || personality.chatalias == "fr") {
		if (buildMinimum(structures.labs, 1)) return true;
		if (buildMinimum(structures.factories, 1)) return true;
		if (buildMinimum(structures.labs, 2)) return true;
		if (buildMinimum(structures.factories, 2)) return true;
		if (buildMinimumDerricks(2)) return true;
		if (buildMinimum(structures.hqs, 1)) return true;
		if (buildMinimum(structures.gens, 2)) return true;
	} else {
		if (buildMinimum(structures.factories, 2)) return true;
		if (buildMinimumDerricks(1)) return true;
		if (buildMinimum(structures.labs, 1)) return true;
		if (buildMinimum(structures.hqs, 1)) return true;
		if (buildMinimum(structures.factories, 3)) return true;
		if (buildMinimum(structures.gens, 2)) return true;
	}
	return captureSomeOil();
}



////////////////////////////////////////////////////////////////////////////////////////////
// Proceed with the main code

include(NB_INCLUDES + "_main.js");
