
	/******************************/
	/* Hover branch specific code */
	/******************************/

function constructPersonality() {

	this.MIN_BUILDERS = 3;		// the usual number of trucks that construct base structures
	this.MAX_BUILDERS = 3;		// the maximum number of trucks that construct base structures
	this.MIN_OILERS = 2;		// the usual number of trucks used for oil hunting
	this.MAX_OILERS = 6;		// the maximum number of trucks used for oil hunting
	// special trick for Hover AI: need to make sure hover trucks are built
	this.MIN_DEFENDERS = 1;		// the minimum number of tanks before producing more trucks than necessary
	this.DEFENSIVENESS = 3;		// regulates chance of spending money on defensive structures
	this.PEACE_TIME = 12;		// the amount of minutes for free scouting (regrouping disabled, no defenses built)

	this.THIS_AI_MAKES_TANKS = true;
	this.THIS_AI_MAKES_CYBORGS = false;
	this.THIS_AI_MAKES_VTOLS = true;

	this.researchPathPrimary = [
		"R-Sys-Engineering01",
		"R-Struc-Research-Module",
		"R-Wpn-Rocket05-MiniPod",
		"R-Vehicle-Prop-Hover",
		"R-Wpn-Rocket02-MRL",
		"R-Vehicle-Body05",
		"R-Wpn-Rocket-ROF01",
		"R-Struc-RepairFacility",
		"R-Defense-MRL",
		"R-Defense-Pillbox06",
		"R-Vehicle-Body11",
		"R-Wpn-Rocket-ROF02",
		"R-Struc-RprFac-Upgrade01",
		"R-Defense-WallTower06",
	];

	this.researchPathFundamental = [
		"R-Struc-Power-Upgrade01c",
		"R-Sys-Sensor-Tower02",
		"R-Vehicle-Body12",
		"R-Struc-Power-Upgrade03a",
		"R-Struc-Materials03",
		"R-Struc-VTOLPad-Upgrade01",
		"R-Struc-RprFac-Upgrade04",
		"R-Vehicle-Body09",
		"R-Vehicle-Body10",
		"R-Struc-Research-Upgrade09",
		"R-Struc-VTOLPad-Upgrade06",
		"R-Vehicle-Body14",
		"R-Wpn-LasSat",
		"R-Struc-Materials09",
		"R-Struc-RprFac-Upgrade06",
	];

	this.researchPathAT = [
		"R-Wpn-Rocket-ROF03",
		"R-Defense-Sunburst",
		"R-Defense-Super-Rocket",
		"R-Defense-HvyA-Trocket",
		"R-Defense-WallTower-HvyA-Trocket",
		"R-Defense-GuardTower-ATMiss",
		"R-Defense-WallTower-A-Tmiss",
		"R-Wpn-Missile-ROF03",
		"R-Wpn-Missile-Damage03",
		"R-Wpn-Missile-Accuracy02",
		"R-Defense-Super-Missile",
	];

	this.researchPathAP = this.researchPathAT;

	this.researchPathAA = [
		"R-Defense-Sunburst",
		"R-Defense-SamSite1",
		"R-Defense-SamSite2",
	];

	this.researchPathAB = [
		"R-Wpn-Rocket03-HvAT",
		"R-Defense-IDFRocket",
		"R-Defense-HvyArtMissile",
	];

	this.tankBodies = [
	[
		"Body14SUP", // dragon
		"Body13SUP", // wyvern
		"Body10MBT", // vengeance
		"Body9REC", // tiger
		"Body7ABT", // retribution
		"Body12SUP", // mantis
		"Body11ABT", // python
		"Body8MBT", // scorpion
		"Body5REC", // cobra
		"Body1REC", // viper
	],
	];

	this.tankPropulsions = [
	[
		"hover01", // hover
	],
	];

	this.vtolBodies = [
		"Body7ABT", // retribution
		"Body6SUPP", // panther
		"Body8MBT", // scorpion
		"Body5REC", // cobra
	];

	// this particular AI doesn't research 
	// most of the AP weapons anyway, but it 
	// will utilize AP weapons researched 
	// by his allies or available at start
	this.apWeapons = [
	[
		"Missile-MdArt", // seraph
		"Laser2PULSEMk1", // pulse laser
		"MG5TWINROTARY", // tag
		"PlasmiteFlamer", // plasmite
		"MG4ROTARYMk1", // ag
		"Flame2", // inferno
		"Rocket-MRL", // mini-rocket arty
	],
	];

	this.atWeapons = [
	[
		"Missile-A-T", // scourge
		"Rocket-HvyA-T", // tk
		"Rocket-LtA-T", // lancer
	],
	];

	this.abWeapons = [
	[
		"Missile-MdArt", // seraph
		"Rocket-BB", // bunker buster
		"Rocket-MRL", // mra
	],
	];
	
	this.aaWeapons = [
	[
		"Missile-HvySAM", // vindicator
		"Missile-LtSAM", // avenger
		"Rocket-Sunburst", // sunburst
	],
	];

	this.vtolWeapons = [
		"Missile-VTOL-AT", // scourge
		"Rocket-VTOL-HvyA-T", // tk
		"Rocket-VTOL-LtA-T", // lancer
		"Rocket-VTOL-Pod", // minipod
	];

	this.apCyborgStats = [
	[
	],
	];

	this.atCyborgStats = [
	[
	],
	];

	this.defenses = [
	[
		"GuardTower-ATMiss", // scourge tower
		"Emplacement-HvyATrocket", // tk emplacement
		"GuardTower5", // lancer tower
		"GuardTower6", // minipod tower
	],
	[
		"Emplacement-MdART-pit", // seraph emplacement
		"Emplacement-MRL-pit", // mra emplacement
	],
	[
		"X-Super-Missile", // super missile fort
		"X-Super-Rocket", // super rocket fort
	],
	];

	this.artillery = [
	[
		"Emplacement-HvART-pit", // archangel
		"Emplacement-Rocket06-IDF", // ripples
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
		"WallTower-Atmiss", // scourge
		"WallTower-HvATrocket", // tk
		"WallTower06", // lancer
	],
	];

}

// Builds initial structures in the following order:
//     lab, lab, factory, lab, generator, generator, lab, CC, factory
// Doesn't build the 2nd factory until 10th minute of the game.
// Demolishes the 4th research lab when hovers are available
// Then build extra structures if necessary.
// Also make sure we have at least one generator before our initial money runs out
function buildOrder() {
	var labCount = enumStruct(me, lab).length;
	var factoryCount = enumStruct(me, factory).length;
	var commandCount = enumStruct(me, command).length;
	var genCount = enumStruct(me, generator).length;
	var derrickCount = enumStruct(me, derrick).length;
	var vtolCount = enumStruct(me, vtolfac).length;
	var labsNeeded=4;
	if (countAllies()>3)
		--labsNeeded;
	if (countAllies()>1 || iHaveHovers())
		--labsNeeded;
	if (genCount < 2 && playerPower(me) < EXTREME_LOW_POWER && isStructureAvailable(generator,me))
		return buildBasicStructure(generator);
	if (labCount < 2 && isStructureAvailable(lab,me))
		return buildBasicStructure(lab);
	if (factoryCount < 1 && isStructureAvailable(factory,me))
		return buildBasicStructure(factory);
	if (labCount < 3 && isStructureAvailable(lab,me))
		return buildBasicStructure(lab,0);
	if ((genCount < 2) && isStructureAvailable(generator,me))
		return buildBasicStructure(generator);
	if (labCount < labsNeeded && isStructureAvailable(lab,me))
		return buildBasicStructure(lab,0);
	if (commandCount < 1 && isStructureAvailable(command,me))
		return buildBasicStructure(command);
	if (factoryCount < 2 && isStructureAvailable(factory,me) && (!earlyGame(10)&&!UNUSUAL_SITUATION))
		return buildBasicStructure(factory,0);
	if (vtolCount < 1 && isStructureAvailable(vtolfac,me))
		return buildBasicStructure(vtolfac,0);
	return false;
}
