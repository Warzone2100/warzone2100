
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
	R: {
		chatalias: "r",
		weaponPaths: [ // weapons to use; put late-game paths below!
			weaponStats.rockets_AT,
			weaponStats.rockets_AS,
			weaponStats.rockets_AA,
			weaponStats.rockets_Arty,
		],
		earlyResearch: [ // fixed research path for the early game
			"R-Sys-Engineering01",
			"R-Struc-Research-Module",
			"R-Wpn-Rocket05-MiniPod",
			"R-Vehicle-Prop-Hover",
			"R-Wpn-Rocket02-MRL",
			"R-Vehicle-Body05",
			"R-Vehicle-Prop-Halftracks",
			"R-Wpn-Rocket-ROF01",
			"R-Struc-RepairFacility",
			"R-Defense-MRL",
			"R-Defense-Pillbox06",
			"R-Vehicle-Body11",
			"R-Wpn-Rocket-ROF02",
			"R-Struc-RprFac-Upgrade01",
			"R-Defense-WallTower06",
		],
		minTanks: 5, // minimal attack force at game start
		becomeHarder: 2, // how much to increase attack force every 5 minutes
		maxTanks: 16, // maximum for the minTanks value (since it grows at becomeHarder rate)
		minTrucks: 1, // minimal number of trucks around
		minHoverTrucks: 4, // minimal number of hover trucks around
		maxSensors: 1, // number of mobile sensor cars to produce
		minMiscTanks: 1, // number of tanks to start harassing enemy
		maxMiscTanks: 3, // number of tanks used for defense and harass
		vtolness: 75, // the chance % of not making droids when adaptation mechanism chooses vtols
		defensiveness: 75, // same thing for defenses; set this to 100 to enable turtle AI specific code
		maxPower: 2500, // build expensive things if we have more than that
		repairAt: 60, // how much % healthy should droid be to join the attack group instead of repairing
	},
};

// this function describes the early build order
// you can rely on personality.chatalias for choosing different build orders for
// different subpersonalities
function buildOrder() {
	// HACK: Tweak the rocket path a bit.
	personality.weaponPaths[0].weapons = [
		{ res: "R-Wpn-Rocket02-MRL", stat: "Rocket-MRL", weight: WEIGHT.MEDIUM }, // mra
		{ res: "R-Wpn-Rocket01-LtAT", stat: "Rocket-LtA-T", weight: WEIGHT.MEDIUM }, // lancer
		{ res: "R-Wpn-Rocket07-Tank-Killer", stat: "Rocket-HvyA-T", weight: WEIGHT.MEDIUM }, // tk
		{ res: "R-Wpn-Missile2A-T", stat: "Missile-A-T", weight: WEIGHT.MEDIUM }, // scourge
		{ res: "R-Wpn-MdArtMissile", stat: "Missile-MdArt", weight: WEIGHT.HEAVY }, // seraph
	];
	// Only use this build order in early game, on standard difficulty, in T1 no bases.
	// Otherwise, fall back to the safe build order.
	if (gameTime > 720000 || difficulty === INSANE
	                      || isStructureAvailable("A0ComDroidControl") || baseType !== CAMP_CLEAN)
		return buildOrder_StandardFallback();
	if (buildMinimum(structures.labs, 2)) return true;
	if (buildMinimum(structures.factories, 1)) return true;
	if (buildMinimum(structures.labs, 3)) return true;
	if (buildMinimum(structures.gens, 1)) return true;
	if (buildMinimumDerricks(2)) return true;
	if (buildMinimum(structures.labs, 4)) return true;
	if (buildMinimum(structures.hqs, 1)) return true;
	return captureSomeOil();
}



////////////////////////////////////////////////////////////////////////////////////////////
// Proceed with the main code

include(NB_INCLUDES + "_main.js");
