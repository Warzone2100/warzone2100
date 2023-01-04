
////////////////////////////////////////////////////////////////////////////////
// Nexus personality definitions.
// A personality is a highly moddable set of information that can create new
// and interesting game-play.
////////////////////////////////////////////////////////////////////////////////

var branch; // what branch personality is being used. A string.
var nexusBranch = {
	generic1: {
		"type": "land",
		"name": "generic1",
		"minimums": {
			scouts: 3,
			defenders: 4,
			attackers: 8,
			cyborgs: 15,
		},
		"maximums" : {
			scouts: 3,
			defenders: 5,
			attackers: MAX_DROID_LIMIT,
			cyborgs: 35,
		},
		"numVtolGroups": 1,
		"numVtolsPerGroup": 10,
		"numVtolDefenders": 5,
		"maxVTOLs": 5,
		//.stattype
		"vtolTargets": [
			{structure: HQ, weight: 10},
			{structure: FACTORY, weight: 80},
			{structure: CYBORG_FACTORY, weight: 60},
			{structure: VTOL_FACTORY, weight: 100},
			{structure: LASSAT, weight: 90},
			{structure: SAT_UPLINK, weight: 10},
			{structure: RESEARCH_LAB, weight: 10},
			{structure: POWER_GEN, weight: 15},
			{structure: RESOURCE_EXTRACTOR, weight: 20},
			{structure: REPAIR_FACILITY, weight: 10},
		],
		// How many tiles away from scout base to expand ~every minute, if safe
		"tileExpandRate": 12,
		// Research to prioritize first
		"earlyResearch": [
			"R-Wpn-MG3Mk1",
			"R-Vehicle-Prop-Halftracks",
			"R-Wpn-Rocket02-MRL",
			"R-Wpn-MG-Damage03",
			"R-Sys-MobileRepairTurret01",
			"R-Defense-Pillbox06",
			"R-Wpn-Rocket-ROF03",
		],
		"factoryPreference": [
			FACTORY,
			CYBORG_FACTORY,
			VTOL_FACTORY,
		],
		//The minimum amount of certain base structures this personality will build, in order.
		"buildOrder": [
			{stat: "F", count: 2},
			{stat: "R", count: 1},
			{stat: "C", count: 1},
			{stat: "R", count: 2},
		],
		"moduleOrder": [
			{mod: "A0PowMod1", amount: 1, structure: BASE_STRUCTURES.gens, maxBuilders: 2,},
			{mod: "A0ResearchModule1", amount: 1, structure: BASE_STRUCTURES.labs, maxBuilders: 1,},
			{mod: "A0FacMod1", amount: 2, structure: BASE_STRUCTURES.factories, maxBuilders: 3,},
			{mod: "A0FacMod1", amount: 2, structure: BASE_STRUCTURES.vtolFactories, maxBuilders: 1,}
		],
	},
	vtol1: {
		"type": "air",
		"name": "vtol1",
		"minimums": {
			scouts: 3,
			defenders: 12,
			attackers: 0,
			cyborgs: 25,
		},
		"maximums" : {
			scouts: 3,
			defenders: 20,
			attackers: 0,
			cyborgs: 50,
		},
		"numVtolGroups": 10,
		"numVtolsPerGroup": 10,
		"numVtolDefenders": 5,
		"maxVTOLs": 70,
		"vtolTargets": [
			{structure: HQ, weight: 10},
			{structure: FACTORY, weight: 80},
			{structure: CYBORG_FACTORY, weight: 60},
			{structure: VTOL_FACTORY, weight: 100},
			{structure: LASSAT, weight: 90},
			{structure: SAT_UPLINK, weight: 10},
			{structure: RESEARCH_LAB, weight: 10},
			{structure: POWER_GEN, weight: 15},
			{structure: RESOURCE_EXTRACTOR, weight: 20},
			{structure: REPAIR_FACILITY, weight: 10},
		],
		"tileExpandRate": 12,
		"earlyResearch": [
			"R-Wpn-MG3Mk1",
			"R-Wpn-Rocket02-MRL",
			"R-Wpn-MG-Damage02",
			"R-Defense-Pillbox06",
			"R-Wpn-Rocket-ROF03",
			"R-Struc-VTOLFactory",
			"R-Struc-VTOLPad",
			"R-Vehicle-Body08",
		],
		"factoryPreference": [
			VTOL_FACTORY,
			CYBORG_FACTORY,
			FACTORY,
		],
		"buildOrder": [
			{stat: "V", count: 2},
			{stat: "R", count: 1},
			{stat: "C", count: 2},
			{stat: "R", count: 2},
		],
		"moduleOrder": [
			{mod: "A0PowMod1", amount: 1, structure: BASE_STRUCTURES.gens, maxBuilders: 2,},
			{mod: "A0FacMod1", amount: 2, structure: BASE_STRUCTURES.vtolFactories, maxBuilders: 3,},
			{mod: "A0FacMod1", amount: 2, structure: BASE_STRUCTURES.factories, maxBuilders: 2,},
			{mod: "A0ResearchModule1", amount: 1, structure: BASE_STRUCTURES.labs, maxBuilders: 2,},
		],
	},
};
