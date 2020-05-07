
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
		weaponPaths: [
			weaponStats.rockets_AT,
			weaponStats.machineguns,
			weaponStats.rockets_AS,
			weaponStats.rockets_AA,
			weaponStats.rockets_Arty,
		],
		earlyResearch: [
			"R-Defense-Tower01",
			"R-Defense-Pillbox01",
			"R-Defense-Tower06",
			"R-Defense-MRL",
		],
		minTanks: 3, becomeHarder: 3, maxTanks: 21,
		minTrucks: 5, minHoverTrucks: 4, maxSensors: 6,
		minMiscTanks: 1, maxMiscTanks: 6,
		vtolness: 100,
		defensiveness: 100, // this enables turtle AI specific code
		maxPower: 300,
		repairAt: 50,
	},
	MC: {
		chatalias: "mc",
		weaponPaths: [
			weaponStats.cannons,
			weaponStats.machineguns,
			weaponStats.mortars,
			weaponStats.fireMortars,
			weaponStats.cannons_AA,
		],
		earlyResearch: [
			"R-Defense-Tower01",
			"R-Defense-Pillbox04",
			"R-Defense-WallTower02",
			"R-Defense-MortarPit",
		],
		minTanks: 3, becomeHarder: 3, maxTanks: 21,
		minTrucks: 5, minHoverTrucks: 4, maxSensors: 6,
		minMiscTanks: 1, maxMiscTanks: 6,
		vtolness: 100, defensiveness: 100,
		maxPower: 300,
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
	if (buildMinimum(structures.factories, 1)) return true;
	if (buildMinimum(structures.labs, 1)) return true;
	if (buildMinimum(structures.hqs, 1)) return true;
	if (buildMinimum(structures.labs, 3)) return true;
	if (buildMinimum(structures.gens, 1)) return true;
	if (buildMinimumDerricks(3)) return true;
	return withChance(25) ? captureSomeOil() : buildDefenses();
}

////////////////////////////////////////////////////////////////////////////////////////////
// Proceed with the main code

include(NB_INCLUDES + "_main.js");
