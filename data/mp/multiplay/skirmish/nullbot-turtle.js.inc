
	/********************************/
	/* Generic branch specific code */
	/********************************/

function constructPersonality() {
		
	this.MIN_BUILDERS = 2;		// the usual number of trucks that construct base structures
	this.MAX_BUILDERS = 3;		// the maximum number of trucks that construct base structures
	this.MIN_OILERS = 4;		// the usual number of trucks used for oil hunting
	this.MAX_OILERS = 15;		// the maximum number of trucks used for oil hunting
	this.MIN_DEFENDERS = 0;		// the minimum number of tanks before producing more trucks than necessary
	this.DEFENSIVENESS = 0;		// regulates chance of spending money on defensive structures
	this.PEACE_TIME = 0;		// the amount of minutes for free scouting (regrouping disabled, no defenses built)

	this.THIS_AI_MAKES_TANKS = false;
	this.THIS_AI_MAKES_CYBORGS = false;
	this.THIS_AI_MAKES_VTOLS = true;

	this.researchPathPrimary = [
		"R-Defense-Tower01",
		"R-Wpn-MG-Damage03",
		"R-Struc-PowerModuleMk1",
		"R-Defense-Tower06",
		"R-Defense-Pillbox01",
		"R-Defense-WallTower01",
		"R-Defense-WallUpgrade02",
		"R-Defense-MRL",
		"R-Wpn-Rocket-ROF01",
		"R-Defense-MortarPit",
		"R-Sys-Sensor-Tower02",
		"R-Defense-Pillbox06",
		"R-Defense-WallTower06",
		"R-Defense-Sunburst",
		"R-Defense-HvyMor",
		"R-Defense-WallUpgrade03",
		"R-Struc-Materials03",
		"R-Vehicle-Body05",
		"R-Vehicle-Metals02",
		"R-Sys-CBSensor-Tower01",
		"R-Defense-IDFRocket",
		"R-Struc-Power-Upgrade01c",
		"R-Struc-VTOLPad-Upgrade01",
		"R-Wpn-Bomb02",
		"R-Defense-Howitzer",
		"R-Wpn-Rocket-ROF03",
		"R-Defense-HvyA-Trocket",
		"R-Defense-WallTower-HvyA-Trocket",
		"R-Defense-Super-Rocket",
		"R-Vehicle-Metals05",
		"R-Wpn-Bomb-Accuracy03",
		"R-Struc-VTOLPad-Upgrade03",
		"R-Defense-WallUpgrade06",
		"R-Struc-Materials06",
		"R-Struc-Power-Upgrade03a",
		"R-Sys-Autorepair-General",
		"R-Sys-SpyTower",
		"R-Wpn-Bomb05",
		"R-Defense-HvyHowitzer",
		"R-Defense-GuardTower-ATMiss",
		"R-Defense-WallTower-A-Tmiss",
		"R-Defense-SamSite1",
		"R-Defense-HvyArtMissile",
		"R-Vehicle-Metals09",
		"R-Wpn-Missile-Damage03",
		"R-Wpn-Missile-ROF03",
		"R-Wpn-Missile-Accuracy02",
		"R-Defense-SamSite2",
		"R-Vehicle-Body07",
		"R-Struc-Materials06",
		"R-Wpn-LasSat",
		"R-Defense-WallUpgrade09",
		"R-Defense-Super-Missile",
		"R-Defense-EMPMortar",
		"R-Defense-EMPCannon",
		"R-Struc-Materials09",
		"R-Defense-WallUpgrade12",
		"R-Struc-VTOLPad-Upgrade06",
	];
	
	this.researchPathFundamental = [
	];
	
	this.researchPathAP = [
	];
	
	this.researchPathAT = [
	];
	
	this.researchPathAA = [
	];
	
	this.researchPathAB = [
	];

	this.tankBodies = [
	[
	],
	];

	this.tankPropulsions = [
	[
	],
	];

	this.vtolBodies = [
		"Body7ABT", // retribution
		"Body6SUPP", // panther
		"Body8MBT", // scorpion
		"Body5REC", // cobra
	];

	this.apWeapons = [
	[
	],
	];

	this.atWeapons = [
	[
	],
	];
	
	this.abWeapons = [
	[
	],
	];
	
	this.aaWeapons = [
	[
	],
	];

	this.vtolWeapons = [
		"Bomb5-VTOL-Plasmite", // plasmite bombs
		"RailGun2-VTOL", // rail gun
		"Bomb4-VTOL-HvyINC", // thermite bombs
		"Bomb2-VTOL-HvHE", // heap bombs
		"RailGun1-VTOL", // needle
		"Missile-VTOL-AT", // scourge
		"Laser2PULSE-VTOL", // pulse laser
		"Cannon5Vulcan-VTOL", // ac
		"Rocket-VTOL-HvyA-T", // tk
		"Bomb3-VTOL-LtINC", // phosphor bombs
		"MG4ROTARY-VTOL", // ag
		"Cannon4AUTO-VTOL", // hpv
		"Rocket-VTOL-LtA-T", // lancer
		"Bomb1-VTOL-LtHE", // cluster bombs
		"MG3-VTOL", // hmg
		"Rocket-VTOL-Pod", // minipod
	];

	this.apCyborgStats = [
	];

	this.atCyborgStats = [
	];

	this.defenses = [ 
	[
	],
	];

	this.artillery = [
	[
		"Emplacement-Howitzer150", // ground shaker
		"Emplacement-Howitzer-Incenediary", // incendiary howitzer
		"Emplacement-RotHow", // hellstorm
		"Emplacement-Howitzer105", // howitzer
		"Emplacement-MortarPit-Incenediary", // incendiary mortar
		"Emplacement-MortarPit02", // bombard
		"Emplacement-RotMor", // pepperpot
		"Emplacement-MortarPit01", // mortar
	],
	[
		"Emplacement-HvART-pit", // archangel
		"Emplacement-Rocket06-IDF", // ripples
		"Emplacement-MortarPit02", // bombard
		"Emplacement-MortarPit-Incenediary", // incendiary mortar
		"Emplacement-RotMor", // pepperpot
		"Emplacement-MortarPit01", // mortar
	],
	];

	this.antiair = [
		"P0-AASite-Laser", // stormbringer
		"P0-AASite-SAM2", // vindicator
		"P0-AASite-SAM1", // avenger
		"AASite-QuadRotMg", // whirlwind
		"P0-AASite-Sunburst", // sunburst
		"AASite-QuadBof", // flak cannon
		"AASite-QuadMg1", // hurricane
	];

	this.hardpoints = [
	[
		"GuardTower-ATMiss", // scourge tower
		"GuardTower-ATMiss", // scourge tower
		"Emplacement-HvyATrocket", // tk emplacement
		"GuardTower5", // lancer tower
		"GuardTower6", // minipod tower
		"GuardTower1", // mg tower
	],
	[
		"Emplacement-MdART-pit", // seraph
		"Emplacement-MdART-pit", // seraph
		"Emplacement-MdART-pit", // seraph
		"Emplacement-MRL-pit", // mra emplacement
		"WallTower01", // hmg
		"PillBox1", // mg bunker
		"GuardTower1", // mg tower
	],
	[
		"X-Super-Missile", // super missile fort
		"X-Super-Rocket", // super rocket fort
		"GuardTower5", // lancer tower
		"GuardTower6", // minipod tower
		"PillBox1", // mg bunker
		"GuardTower1", // mg tower
	],
	[
		"Emplacement-MortarEMP", // emp mortar
		"Sys-SpyTower", // nexus link tower
		"Emplacement-MdART-pit", // seraph
		"GuardTower-ATMiss", // scourge tower
		"Emplacement-HvyATrocket", // tk emplacement
		"Emplacement-MRL-pit", // mra emplacement
		"GuardTower6", // minipod tower
		"PillBox1", // mg bunker
		"GuardTower1", // mg tower
	],
	[
		"WallTower-Atmiss", // scourge
		"WallTower-HvATrocket", // tk
		"WallTower06", // lancer
		"WallTower01", // hmg
		"PillBox1", // mg bunker
		"GuardTower1", // mg tower
	],
	[
		"WallTower-EMP", // emp
		"Emplacement-MdART-pit", // seraph
		"Emplacement-MRL-pit", // mra emplacement
		"WallTower01", // hmg
		"PillBox1", // mg bunker
		"GuardTower1", // mg tower
	],
	];

	this.hardpoints = this.hardpoints.concat(this.artillery);
	this.hardpoints[this.hardpoints.length]=this.antiair;
	this.hardpoints[this.hardpoints.length]=this.antiair;
	this.hardpoints[this.hardpoints.length]=this.antiair;
	
}

// Builds initial structures in the following order:
//     factory, lab, cc, lab, lab
// Then build extra structures if necessary.
// Also make sure we have at least one generator before our initial money runs out
function buildOrder() {
	var labCount = enumStruct(me, lab).length;
	var factoryCount = enumStruct(me, factory).length;
	var commandCount = enumStruct(me, command).length;
	var genCount = enumStruct(me, generator).length;
	var derrickCount = enumStruct(me, derrick).length;
	var borgCount = enumStruct(me, borgfac).length;
	var vtolCount = enumStruct(me, vtolfac).length;
	if (genCount < 2 && playerPower(me) < EXTREME_LOW_POWER && isStructureAvailable(generator,me))
		return buildBasicStructure(generator);
	if (factoryCount < 1 && isStructureAvailable(factory,me))
		return buildBasicStructure(factory);
	if (labCount < 1 && isStructureAvailable(lab,me))
		return buildBasicStructure(lab);
	if (commandCount < 1 && isStructureAvailable(command,me))
		return buildBasicStructure(command);
	if (labCount < 2 && isStructureAvailable(lab,me))
		return buildBasicStructure(lab,0);
	if ((genCount < 2) && isStructureAvailable(generator,me))
		return buildBasicStructure(generator);
	if (labCount < 3 && isStructureAvailable(lab,me))
		return buildBasicStructure(lab,0);
	if (vtolCount < 1 && isStructureAvailable(vtolfac,me))
		return buildBasicStructure(vtolfac,0);
	return false;
}
