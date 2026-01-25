//Contains the campaign transition technology definitions.

//This array should give a player all the research from Alpha.
const mis_alphaResearchNew = [
	// 1
	"R-Wpn-MG1Mk1", "R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels",
	"R-Sys-MobileRepairTurret01", "R-Sys-Engineering01", "R-Wpn-Flamer01Mk1", "R-Wpn-Flamer-Damage01",
	"R-Wpn-MG-Damage01", "R-Wpn-MG-Damage02", "R-Wpn-MG-ROF01", "R-Defense-Tower01",
	"R-Defense-TankTrap01", "R-Defense-Tower04",

	// 2
	"R-Wpn-MG2Mk1", "R-Wpn-MG-Damage03", "R-Sys-Sensor-Turret01", "R-Struc-PowerModuleMk1",
	"R-Sys-Sensor-Tower01", "R-Wpn-Flamer-Damage02", "R-Wpn-Flamer-ROF01",
	"R-Wpn-Flamer-Range01",	"R-Defense-Tower02",

	// 3
	"R-Wpn-MG3Mk1", "R-Defense-Tower03",

	// 4
	"R-Wpn-Mortar01Lt", "R-Vehicle-Prop-Halftracks",
	"R-Wpn-Cannon1Mk1", "R-Wpn-Mortar-Damage01", "R-Wpn-Mortar-Damage02",
	"R-Wpn-Mortar-ROF01","R-Wpn-Cannon-Damage01", "R-Wpn-Cannon-Damage02",

	// 5
	"R-Struc-Factory-Module", "R-Wpn-Flamer-Damage03", "R-Sys-Sensor-Tower02",
	"R-Struc-Factory-Upgrade01", "R-Struc-Factory-Upgrade02", "R-Defense-HardcreteWall",
	"R-Struc-CommandRelay", "R-Comp-CommandTurret01", "R-Struc-RepairFacility",
	"R-Struc-RprFac-Upgrade01", "R-Struc-RprFac-Upgrade02", "R-Defense-MortarPit",
	"R-Defense-Pillbox01", "R-Defense-Pillbox04", "R-Defense-Pillbox05",
	"R-Defense-WallTower01", "R-Defense-WallTower02", "R-Defense-WallUpgrade01",
	"R-Struc-Materials03", "R-Vehicle-Body05", "R-Vehicle-Metals01", "R-Vehicle-Body04",

	// 6
	"R-Wpn-Cannon2Mk1", "R-Defense-WallTower03", "R-Wpn-Rocket05-MiniPod", "R-Struc-Research-Module",
	"R-Vehicle-Prop-Tracks", "R-Vehicle-Engine01", "R-Defense-Tower06", "R-Defense-Pillbox06",
	"R-Wpn-Rocket-Damage01", "R-Wpn-Rocket-Damage02", "R-Wpn-Rocket-ROF01",
	"R-Wpn-Rocket-ROF02", "R-Wpn-Rocket-ROF03", "R-Defense-WallTower06",
	"R-Wpn-Rocket01-LtAT", "R-Wpn-Rocket03-HvAT", "R-Wpn-RocketSlow-Damage01",
	"R-Defense-WallUpgrade02",

	// 7

	// 8
	"R-Vehicle-Metals02", "R-Wpn-MG-Damage04", "R-Wpn-Rocket02-MRL",
	"R-Wpn-Rocket-Damage02", "R-Wpn-Mortar02Hvy", "R-Wpn-Mortar-Damage03", "R-Wpn-Cannon-Damage03",
	"R-Wpn-RocketSlow-Damage02", "R-Vehicle-Body11", "R-Defense-MRL", "R-Defense-HvyMor",

	// 9
	"R-Struc-Research-Upgrade01", "R-Struc-Research-Upgrade02", "R-Struc-Research-Upgrade03",
	"R-Wpn-Mortar-Acc01", "R-Wpn-Rocket-Accuracy01", "R-Wpn-Rocket-Accuracy02",
	"R-Wpn-RocketSlow-Accuracy01", "R-Wpn-Cannon-Accuracy01", "R-Struc-RprFac-Upgrade03",
	"R-Comp-SynapticLink", "R-Vehicle-Body08", "R-Vehicle-Engine02", "R-Struc-Factory-Upgrade03",
	"R-Struc-Factory-Cyborg", "R-Cyborg-Wpn-MG", "R-Cyborg-Metals01", "R-Cyborg-Metals02",
	"R-Cyborg-Metals03", "R-Cyborg-Wpn-Flamer", "R-Cyborg-Wpn-Rocket",
	"R-Cyborg-Legs01", "R-Defense-WallUpgrade03",

	// 10

	// 11
	"R-Wpn-Cannon3Mk1", "R-Defense-WallTower04", "R-Wpn-RocketSlow-Damage03",
	"R-Cyborg-Wpn-Cannon", "R-Wpn-Rocket-Damage03",

	//12
	"R-Vehicle-Prop-Hover", "R-Vehicle-Metals03", "R-Vehicle-Body12", "R-Vehicle-Engine03",
];

//Basic base structures.
const mis_structsAlpha = [
	"A0CommandCentre",
	"A0PowerGenerator",
	"A0ResourceExtractor",
	"A0ResearchFacility",
	"A0LightFactory",
];

//BETA 2-A bonus research
const mis_playerResBeta = [
	"R-Wpn-AAGun03",
	"R-Defense-AASite-QuadMg1",
];

//This array should give a player all the research from Beta.
const mis_betaResearchNew = [
	// 1
	"R-Sys-Engineering02", "R-Sys-Sensor-Upgrade01", "R-Wpn-AAGun-Damage01",
	"R-Wpn-Cannon-Damage04", "R-Wpn-MG-ROF02", "R-Wpn-Rocket-Damage04",
	"R-Defense-WallUpgrade04", "R-Sys-CBSensor-Tower01", "R-Wpn-AAGun-ROF02",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-MG-Damage05", "R-Wpn-RocketSlow-Damage04",
	"R-Struc-Materials06", "R-Sys-CBSensor-Turret01", "R-Wpn-RocketSlow-Accuracy02",

	// 2

	// 3
	"R-Vehicle-Body02", "R-Wpn-Cannon-ROF01", "R-Wpn-Flame2", "R-Wpn-MG-ROF03",
	"R-Wpn-RocketSlow-ROF01", "R-Defense-HvyFlamer", "R-Vehicle-Metals04",
	"R-Wpn-Cannon-Damage05", "R-Wpn-MG-Damage06", "R-Wpn-Rocket-Damage05",
	"R-Wpn-RocketSlow-Damage05", "R-Cyborg-Metals04", "R-Wpn-Flamer-Damage06",
	"R-Wpn-Mortar-Damage04", "R-Defense-WallUpgrade05", "R-Cyborg-Wpn-Flamer02",

	// 4
	"R-Wpn-RocketSlow-Accuracy03", "R-Wpn-AAGun-Accuracy01", "R-Wpn-Mortar-Acc02",

	// 5
	"R-Vehicle-Body06", "R-Vehicle-Prop-VTOL", "R-Wpn-AAGun02", "R-Wpn-AAGun-Damage02", "R-Wpn-HowitzerMk1",
	"R-Wpn-Rocket06-IDF", "R-Defense-AASite-QuadBof", "R-Defense-Howitzer", "R-Defense-IDFRocket",
	"R-Struc-VTOLFactory", "R-Vehicle-Engine04", "R-Vehicle-Metals05", "R-Wpn-Bomb01",
	"R-Wpn-Flamer-ROF03", "R-Wpn-Howitzer-Damage01", "R-Wpn-Howitzer-ROF01", "R-Wpn-Mortar-ROF02",
	"R-Cyborg-Metals05", "R-Struc-Factory-Upgrade06", "R-Struc-VTOLPad", "R-Vehicle-Armor-Heat01",
	"R-Wpn-Bomb-Damage01", "R-Wpn-Bomb03", "R-Cyborg-Armor-Heat01", "R-Struc-RprFac-Upgrade06",
	"R-Sys-VTOLStrike-Tower01", "R-Sys-VTOLStrike-Turret01", "R-Wpn-Mortar-Damage05",

	// 6
	"R-Struc-Power-Upgrade01", "R-Wpn-Cannon-ROF02", "R-Wpn-MG4", "R-Wpn-RocketSlow-ROF02",
	"R-Cyborg-Wpn-RotMG", "R-Defense-RotMG", "R-Wpn-MG-Damage07", "R-Wpn-Rocket-Damage06",
	"R-Defense-Wall-RotMg", "R-Wpn-AAGun-ROF03", "R-Defense-Pillbox-RotMG",

	// 7
	"R-Wpn-Bomb02", "R-Wpn-Howitzer-Accuracy01", "R-Wpn-Howitzer-Damage02",
	"R-Wpn-AAGun-Accuracy02", "R-Wpn-Howitzer-ROF02", "R-Struc-Research-Upgrade06",
	"R-Struc-VTOLPad-Upgrade03", "R-Wpn-Bomb-Damage02", "R-Wpn-Bomb04",
	"R-Defense-WallUpgrade06", "R-Vehicle-Engine05",

	// 8
	"R-Sys-VTOLCBS-Tower01", "R-Wpn-Cannon4AMk1", "R-Wpn-Mortar3", "R-Wpn-Rocket07-Tank-Killer",
	"R-Defense-Emplacement-HPVcannon", "R-Defense-HvyA-Trocket", "R-Defense-RotMor",
	"R-Defense-WallTower-HPVcannon", "R-Defense-WallTower-HvyA-Trocket", "R-Sys-VTOLCBS-Turret01",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Mortar-Damage06", "R-Wpn-AAGun04", "R-Wpn-AAGun-Damage03", "R-Defense-AASite-QuadRotMg",
	"R-Cyborg-Wpn-Cannon02", "R-Cyborg-Wpn-Rocket02",

	// 9
	"R-Wpn-Cannon5", "R-Wpn-RocketSlow-Damage06", "R-Defense-Wall-VulcanCan",
	"R-Wpn-Cannon-Damage06", "R-Wpn-RocketSlow-ROF03", "R-Wpn-Cannon-ROF03",
	"R-Wpn-MG-Damage08",

	// 10
	"R-Vehicle-Body09", "R-Wpn-HvyHowitzer", "R-Defense-HvyHowitzer", "R-Vehicle-Metals06",
	"R-Wpn-Howitzer-Damage03", "R-Cyborg-Metals06", "R-Vehicle-Engine06", "R-Wpn-Howitzer-Accuracy02",
	"R-Wpn-Howitzer-ROF03", "R-Vehicle-Armor-Heat03", "R-Cyborg-Armor-Heat03",

	// 11
];

//This is used for giving allies in Gamma technology (3-b/3-2/3-c)
const mis_gammaAllyRes = mis_alphaResearchNew.concat(mis_playerResBeta).concat(mis_betaResearchNew);

const mis_gammaResearchNew = [
	// 1
	"R-Wpn-Howitzer03-Rot", "R-Wpn-MG-Damage09", "R-Struc-Power-Upgrade02", "R-Sys-Engineering03",
	"R-Wpn-Cannon-Damage07", "R-Wpn-AAGun-Damage04", "R-Defense-WallUpgrade07", "R-Struc-Materials09",
	"R-Defense-RotHow", "R-Wpn-Howitzer-Damage04", "R-Wpn-Flamer-ROF04", "R-Wpn-RocketSlow-ROFRR01",

	// 2
	"R-Wpn-Cannon-Damage08", "R-Wpn-AAGun-Damage05", "R-Wpn-Howitzer-Damage05",
	"R-Defense-WallUpgrade08",

	// 3
	"R-Struc-Research-Upgrade07", "R-Wpn-Laser01", "R-Wpn-Mortar-Acc03", "R-Vehicle-Body03",
	"R-Wpn-Cannon-ROF04", "R-Struc-Research-Upgrade08", "R-Struc-Research-Upgrade09",
	"R-Cyborg-Wpn-Laser1", "R-Defense-PrisLas", "R-Defense-WallTower-PulseLas",
	"R-Wpn-Energy-Accuracy01", "R-Wpn-Bomb-Damage03", "R-Vehicle-Metals07",
	"R-Vehicle-Engine07", "R-Wpn-AAGun-ROF04", "R-Wpn-Mortar-ROF04", "R-Wpn-Energy-Damage01",
	"R-Wpn-Energy-Damage02", "R-Wpn-Energy-ROF01", "R-Cyborg-Metals07", "R-Vehicle-Armor-Heat04",
	"R-Wpn-Howitzer-ROF04", "R-Cyborg-Armor-Heat04", "R-Wpn-RocketSlow-ROF04",
	"R-Defense-WallUpgrade09",

	// 4
	"R-Wpn-Cannon-ROF05", "R-Wpn-Cannon-ROF06", "R-Wpn-Cannon-Damage09", "R-Wpn-AAGun-Damage06",
	"R-Wpn-Howitzer-Damage06", "R-Wpn-AAGun-ROF05", "R-Wpn-AAGun-ROF06", "R-Wpn-RocketSlow-ROF05",
	"R-Wpn-RocketSlow-ROF06", "R-Wpn-MG-Damage10",

	// 5
	"R-Sys-Resistance-Upgrade01", "R-Sys-Resistance-Upgrade02", "R-Sys-Resistance-Upgrade03",
	"R-Sys-Resistance-Upgrade04",

	// 6
	"R-Vehicle-Body07", "R-Wpn-RailGun01", "R-Struc-VTOLPad-Upgrade04", "R-Wpn-Missile-LtSAM",
	"R-Vehicle-Metals08", "R-Vehicle-Engine08", "R-Cyborg-Wpn-Rail1", "R-Defense-GuardTower-Rail1",
	"R-Wpn-Rail-Damage01", "R-Struc-VTOLPad-Upgrade05", "R-Struc-VTOLPad-Upgrade06", "R-Defense-SamSite1",
	"R-Wpn-Missile-Damage01", "R-Vehicle-Armor-Heat05", "R-Wpn-Rail-Accuracy01", "R-Wpn-Missile-Accuracy01",
	"R-Wpn-AAGun-Accuracy03", "R-Wpn-Howitzer-Accuracy03", "R-Wpn-Rail-ROF01", "R-Wpn-Missile2A-T",
	"R-Cyborg-Wpn-ATMiss", "R-Defense-GuardTower-ATMiss", "R-Defense-WallTower-A-Tmiss", "R-Wpn-Missile-Damage02",
	"R-Wpn-Missile-ROF01", "R-Cyborg-Metals08", "R-Cyborg-Armor-Heat05", "R-Defense-WallUpgrade10",

	// 7
	"R-Wpn-MdArtMissile", "R-Wpn-Laser02", "R-Wpn-RailGun02",
	"R-Wpn-Missile-HvSAM", "R-Defense-SamSite2", "R-Wpn-Missile-Accuracy02",
	"R-Defense-PulseLas", "R-Wpn-Energy-ROF02", "R-Wpn-Energy-Damage03", "R-Wpn-Energy-ROF03",
	"R-Defense-MdArtMissile", "R-Wpn-Missile-ROF02", "R-Defense-Rail2", "R-Defense-WallTower-Rail2",
	"R-Wpn-Rail-Damage02", "R-Wpn-Rail-ROF02", "R-Defense-WallUpgrade11", "R-Defense-WallUpgrade12",

	// 8
	"R-Sys-Resistance", "R-Comp-MissileCodes01", "R-Comp-MissileCodes02", "R-Comp-MissileCodes03",

	// 9
	"R-Wpn-RailGun03", "R-Vehicle-Body10", "R-Wpn-HvArtMissile",
	"R-Wpn-Rail-Damage03", "R-Defense-Rail3", "R-Defense-WallTower-Rail3", "R-Wpn-Rail-ROF03",
	"R-Vehicle-Metals09", "R-Vehicle-Engine09", "R-Defense-HvyArtMissile", "R-Wpn-Missile-Damage03",
	"R-Wpn-Missile-ROF03", "R-Cyborg-Metals09", "R-Vehicle-Armor-Heat06", "R-Cyborg-Armor-Heat06",

];

// Below lies all the "Classic" campaign lists. Some are just for documentation purposes
// given there are many research tree bugs that failed to give the player research.
// these are commented out and are historical archives.

//This array should give a player all the research from Alpha.
const mis_alphaResearchNewClassic = [
	// 1
	"R-Wpn-MG1Mk1", "R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels",
	"R-Wpn-MG-Damage01", "R-Wpn-MG-Damage02", "R-Wpn-MG-Damage03", "R-Wpn-Flamer01Mk1",
	"R-Defense-Tower01", "R-Sys-Engineering01", "R-Sys-MobileRepairTurret01",
	"R-Defense-TankTrap01",
	// 2
	"R-Wpn-Flamer01Mk1", "R-Wpn-Flamer-Damage01", "R-Wpn-Flamer-Damage02", "R-Wpn-Flamer-Damage03",
	"R-Wpn-Flamer-ROF01", "R-Wpn-MG2Mk1", "R-Sys-Sensor-Tower01", "R-Sys-Sensor-Turret01",
	"R-Struc-PowerModuleMk1",
	// 3
	"R-Wpn-MG3Mk1",
	// 4
	"R-Wpn-Mortar01Lt", "R-Wpn-Mortar-Damage01", "R-Wpn-Mortar-Damage02",
	"R-Wpn-Mortar-Damage03", "R-Vehicle-Prop-Halftracks",
	// 5
	"R-Wpn-Cannon1Mk1", "R-Wpn-Cannon-Damage01", "R-Wpn-Cannon-Damage02", "R-Wpn-Cannon-Damage03",
	"R-Struc-CommandRelay", "R-Comp-CommandTurret01", "R-Struc-Factory-Module",
	"R-Vehicle-Body05", "R-Wpn-Cannon2Mk1", "R-Struc-RepairFacility",
	"R-Struc-Factory-Upgrade01", "R-Struc-Factory-Upgrade02", "R-Wpn-MG-ROF01",
	"R-Defense-HardcreteWall", "R-Defense-WallUpgrade01", "R-Defense-WallUpgrade02",
	"R-Defense-WallUpgrade03", "R-Struc-Materials01", "R-Struc-Materials02",
	"R-Struc-Materials03", "R-Defense-WallTower02", "R-Defense-WallTower03",
	"R-Defense-Pillbox01", "R-Defense-WallTower01", "R-Defense-Pillbox04",
	"R-Defense-Pillbox05", "R-Defense-MortarPit", "R-Struc-RprFac-Upgrade01",
	"R-Struc-RprFac-Upgrade02",

	// 6 ("R-Comp-CommandTurret01" used to be here due to built Command Relay requirement)
	"R-Wpn-Rocket05-MiniPod", "R-Defense-Tower06", "R-Wpn-Rocket-Damage01", "R-Wpn-Rocket-Damage02",
	"R-Wpn-Rocket-Damage03", "R-Wpn-Rocket01-LtAT",  "R-Wpn-Rocket-ROF01", "R-Wpn-Rocket-ROF02",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Damage01", "R-Defense-Pillbox06", "R-Defense-WallTower06",
	"R-Wpn-RocketSlow-Damage02", "R-Wpn-RocketSlow-Damage03", "R-Wpn-Rocket03-HvAT",
	"R-Vehicle-Prop-Tracks", "R-Vehicle-Engine01", "R-Struc-Research-Module",
	// 7

	// 8
	"R-Wpn-Rocket02-MRL", "R-Defense-MRL", "R-Vehicle-Metals01", "R-Vehicle-Metals02",
	"R-Wpn-Mortar02Hvy", "R-Vehicle-Body04", "R-Vehicle-Body11",
	// 9
	//Original game needed a cyborg factory built for these but we don't do this anymore
	//Cyborg production upgrades actually do nothing so we don't care about them anymore
	"R-Struc-Factory-Upgrade03", "R-Struc-RprFac-Upgrade03", "R-Vehicle-Body08",
	"R-Vehicle-Engine02", "R-Comp-SynapticLink", "R-Struc-Factory-Cyborg",
	"R-Cyborg-Wpn-MG", "R-Cyborg-Legs01", "R-Cyborg-Wpn-Cannon", "R-Cyborg-Wpn-Flamer", "R-Cyborg-Wpn-Rocket",
	"R-Cyborg-Metals01", "R-Cyborg-Metals02", "R-Cyborg-Metals03",
	// 10

	// 11
	"R-Wpn-Cannon3Mk1", "R-Defense-WallTower04",
	// 12
	"R-Vehicle-Prop-Hover", "R-Vehicle-Metals03", "R-Vehicle-Body12",
];

//BETA 2-A additional research
const mis_playerResBetaClassic = [
	"R-Wpn-AAGun03",
	"R-Defense-AASite-QuadMg1",
	"R-Sys-Sensor-Tower02",
];

// These are not available to the player during Alpha because of classic balance research tree bugs
/*
const mis_betaBonusResClassic = [
	"R-Sys-Sensor-Tower02",
	"R-Defense-HvyMor",
	"R-Sys-Sensor-Tower02",
	"R-Wpn-Mortar-ROF01",
	"R-Struc-Research-Upgrade01",
	"R-Struc-Research-Upgrade02",
	"R-Struc-Research-Upgrade03",
	"R-Wpn-Mortar-Acc01",
	"R-Wpn-Rocket-Accuracy01",
	"R-Wpn-Rocket-Accuracy02",
	"R-Wpn-RocketSlow-Accuracy01",
	"R-Wpn-Cannon-Accuracy01",
	"R-Vehicle-Engine03",
];
*/

//Tech missing from the original Beta campaign research tree file.
/*
const mis_MissingAlphaTechClassic = [
	"R-Wpn-MG-Damage01", "R-Wpn-MG-Damage02", "R-Wpn-MG-Damage03",
	"R-Wpn-Flamer-Damage01", "R-Wpn-Mortar-Damage01", "R-Wpn-Mortar-Damage02",
	"R-Wpn-Cannon-Damage01", "R-Wpn-Cannon-Damage02", "R-Struc-Factory-Upgrade01",
	"R-Struc-Factory-Upgrade02", "R-Defense-WallUpgrade01", "R-Struc-Materials01",
	"R-Struc-Materials02", "R-Wpn-Rocket-Damage01", "R-Wpn-Rocket-Damage02",
	"R-Wpn-Rocket-ROF01", "R-Wpn-RocketSlow-Damage01", "R-Wpn-RocketSlow-Damage02",
	"R-Vehicle-Engine01", "R-Vehicle-Metals01", "R-Vehicle-Metals02",
	"R-Vehicle-Engine02", "R-Cyborg-Metals01", "R-Cyborg-Metals02",
];
*/

// This is all the research the original game gave you upon going into Beta 1.
const mis_betaStartingResearchClassic = [
	"R-Cyborg-Legs01",
	"R-Cyborg-Metals03",
	"R-Defense-WallUpgrade03",
	"R-Struc-Factory-Upgrade03",
	"R-Struc-Materials03",
	"R-Struc-Research-Upgrade03",
	"R-Struc-RprFac-Upgrade03",
	"R-Sys-Engineering01",
	"R-Sys-MobileRepairTurret01",
	"R-Wpn-Cannon-Damage03",
	"R-Wpn-Flamer-Damage03",
	"R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage04",
	"R-Wpn-MG-ROF01",
	"R-Wpn-Mortar-Damage03",
	"R-Wpn-Rocket-Accuracy02",
	"R-Wpn-Rocket-ROF03",
	"R-Wpn-RocketSlow-Accuracy02",
	"R-Wpn-RocketSlow-Damage03",
	"R-Vehicle-Engine03",
	"R-Vehicle-Metals03",
	//"R-Wpn-AAGun03",
	//"R-Defense-AASite-QuadMg1",
	//"R-Sys-Sensor-Tower02",
	"R-Defense-MRL",
];

//This array should give a player all the research from Beta.
const mis_betaResearchNewClassic = [
	// 1
	"R-Wpn-AAGun-Damage01", "R-Wpn-AAGun-Damage02", "R-Wpn-AAGun-Damage03",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-AAGun-ROF01", "R-Wpn-AAGun-ROF02",
	"R-Wpn-Cannon-Damage04", "R-Wpn-Cannon-Damage05", "R-Wpn-Cannon-ROF01",
	"R-Wpn-Rocket-Damage04", "R-Wpn-Rocket-Damage05", "R-Wpn-Rocket-Damage06",

	"R-Sys-Engineering02", "R-Defense-WallUpgrade04", "R-Defense-WallUpgrade05",
	"R-Defense-WallUpgrade06", "R-Struc-Materials04", "R-Struc-Materials05",
	"R-Struc-Materials06", "R-Sys-Sensor-Upgrade01", "R-Sys-CBSensor-Tower01",
	"R-Sys-CBSensor-Turret01", "R-Wpn-MG-ROF02", "R-Wpn-MG-Damage05",
	"R-Wpn-MG-Damage06", "R-Wpn-MG-Damage07",
	// 2
	// 3
	"R-Wpn-RocketSlow-ROF01", "R-Vehicle-Body06", "R-Vehicle-Metals04",
	"R-Cyborg-Metals04", "R-Wpn-Flame2", "R-Defense-HvyFlamer",
	"R-Wpn-Flamer-Damage04", "R-Wpn-MG-ROF03",
	// 4
	"R-Wpn-RocketSlow-Accuracy03", "R-Wpn-AAGun-Accuracy01", "R-Wpn-Mortar-Acc02",
	// 5
	// VTOL production upgrades actually do nothing so we don't care about them anymore
	"R-Wpn-Rocket06-IDF", "R-Defense-IDFRocket", "R-Struc-Factory-Upgrade04",
	"R-Struc-RprFac-Upgrade04", "R-Wpn-Cannon-ROF02",
	"R-Wpn-AAGun02", "R-Defense-AASite-QuadBof", "R-Vehicle-Body02", "R-Vehicle-Metals05",
	"R-Vehicle-Armor-Heat01", "R-Cyborg-Metals05", "R-Cyborg-Armor-Heat01",
	"R-Vehicle-Prop-VTOL", "R-Vehicle-Engine04", "R-Wpn-Bomb01", "R-Wpn-Bomb03",
	"R-Wpn-Bomb-Damage01", "R-Struc-VTOLFactory", "R-Struc-VTOLPad", "R-Sys-VTOLStrike-Tower01",
	"R-Sys-VTOLStrike-Turret01", "R-Wpn-HowitzerMk1", "R-Defense-Howitzer", "R-Wpn-Howitzer-Damage01",
	"R-Wpn-Howitzer-Damage02",
	// 6
	"R-Wpn-Cannon4AMk1", "R-Wpn-Cannon-ROF03", "R-Defense-Emplacement-HPVcannon",
	"R-Defense-WallTower-HPVcannon", "R-Wpn-MG4", "R-Wpn-AAGun04", "R-Defense-AASite-QuadRotMg",
	"R-Wpn-AAGun-ROF03", "R-Cyborg-Wpn-RotMG", "R-Defense-RotMG", "R-Defense-Wall-RotMg",
	"R-Struc-Power-Upgrade01",
	// 7
	"R-Struc-VTOLPad-Upgrade01", "R-Struc-VTOLPad-Upgrade02",
	"R-Struc-Research-Upgrade04", "R-Struc-Research-Upgrade05", "R-Struc-Research-Upgrade06",
	"R-Wpn-Bomb02", "R-Wpn-Howitzer-Accuracy01", "R-Wpn-Howitzer-Accuracy02",
	"R-Wpn-AAGun-Accuracy02",
	// 8
	"R-Sys-VTOLCBS-Tower01", "R-Sys-VTOLCBS-Turret01", "R-Wpn-Mortar3", "R-Defense-RotMor",
	"R-Wpn-Rocket07-Tank-Killer",
	// 9
	"R-Wpn-Cannon5", "R-Defense-Wall-VulcanCan", "R-Wpn-Cannon-Damage06",
	// 10
	"R-Vehicle-Body09", "R-Vehicle-Metals06", "R-Cyborg-Metals06", "R-Vehicle-Armor-Heat02",
	"R-Cyborg-Armor-Heat02", "R-Vehicle-Armor-Heat03", "R-Cyborg-Armor-Heat03",
	"R-Vehicle-Engine05", "R-Vehicle-Engine06", "R-Wpn-HvyHowitzer",
	"R-Defense-HvyHowitzer", "R-Wpn-Howitzer-Damage03",
	// 11
];

//GAMMA 3-A additional research
const mis_playerResGammaClassic = [
	"R-Wpn-Bomb04", "R-Defense-Pillbox-RotMG", "R-Defense-HvyA-Trocket",
	"R-Defense-WallTower-HvyA-Trocket", "R-Defense-HvyMor",
];

//This is used for giving allies in Gamma technology (3-b/3-2/3-c)
const mis_gammaAllyResClassic = mis_alphaResearchNewClassic.concat(mis_playerResBetaClassic).concat(mis_betaResearchNewClassic);

// These are not available to the player during Beta because of classic balance research tree bugs.
/*
const mis_gammaBonusResClassic = [
	"R-Wpn-RocketSlow-Damage04",
	"R-Wpn-RocketSlow-Damage05",
	"R-Wpn-RocketSlow-Damage06",
	"R-Wpn-RocketSlow-Accuracy02",
	"R-Wpn-RocketSlow-ROF02",
	"R-Wpn-RocketSlow-ROF03",
	"R-Wpn-Flamer-Damage05",
	"R-Wpn-Flamer-Damage06",
	"R-Wpn-Mortar-Damage04",
	"R-Wpn-Mortar-Damage05",
	"R-Wpn-Mortar-Damage06",
	"R-Wpn-Flamer-ROF02",
	"R-Wpn-Flamer-ROF03",
	"R-Wpn-Howitzer-ROF01",
	"R-Struc-Factory-Upgrade05",
	"R-Struc-Factory-Upgrade06",
	"R-Struc-RprFac-Upgrade05",
	"R-Struc-RprFac-Upgrade06",
	"R-Wpn-Mortar-ROF02",
	"R-Wpn-Mortar-ROF03",
	"R-Defense-Pillbox-RotMG",
	"R-Wpn-Howitzer-Damage02",
	"R-Wpn-Howitzer-Damage03",
	"R-Wpn-Howitzer-ROF02",
	"R-Struc-VTOLPad-Upgrade03",
	"R-Wpn-Bomb-Damage02",
	//"R-Wpn-Bomb04", --not ever available
	"R-Defense-HvyA-Trocket",
	"R-Defense-WallTower-HvyA-Trocket",
	"R-Wpn-Howitzer-ROF03",
];
*/

//Tech missing from the original Gamma campaign research tree file compared to Beta.
/*
const mis_MissingBetaTechClassic = [
	"R-Wpn-AAGun-Damage01", "R-Wpn-AAGun-Damage02", "R-Wpn-AAGun-ROF01",
	"R-Wpn-AAGun-ROF02", "R-Wpn-Cannon-Damage04", "R-Wpn-Cannon-Damage05",
	"R-Wpn-Cannon-ROF01", "R-Wpn-Rocket-Damage04", "R-Wpn-Rocket-Damage05",
	"R-Defense-WallUpgrade04", "R-Defense-WallUpgrade05", "R-Struc-Materials04",
	"R-Struc-Materials05", "R-Wpn-MG-ROF02", "R-Wpn-MG-Damage05",
	"R-Wpn-MG-Damage06", "R-Wpn-RocketSlow-ROF01", "R-Vehicle-Metals04",
	"R-Cyborg-Metals04", "R-Wpn-Flamer-Damage04", "R-Wpn-AAGun-Accuracy01",
	"R-Struc-Factory-Upgrade04", "R-Struc-RprFac-Upgrade04", "R-Vehicle-Metals05",
	"R-Vehicle-Armor-Heat01", "R-Cyborg-Metals05", "R-Cyborg-Armor-Heat01",
	"R-Vehicle-Engine04", "R-Wpn-Bomb-Damage01", "R-Wpn-Howitzer-Damage01",
	"R-Wpn-Howitzer-Damage02", "R-Struc-VTOLPad-Upgrade01", "R-Struc-VTOLPad-Upgrade02",
	"R-Struc-Research-Upgrade04", "R-Struc-Research-Upgrade05", "R-Wpn-Howitzer-Accuracy01",
	"R-Vehicle-Engine05"
];
*/

// This is all the research the original game gave you upon going into Gamma 1.
const mis_gammaStartingResearchClassic = [
	"R-Wpn-Cannon-Accuracy02",
	"R-Wpn-Cannon-Damage06",
	"R-Wpn-Cannon-ROF03",
	"R-Wpn-Flamer-Damage06",
	"R-Wpn-Flamer-ROF03",
	"R-Wpn-Howitzer-Accuracy02",
	"R-Wpn-Howitzer-Damage03",
	"R-Wpn-MG-Damage07",
	"R-Wpn-MG-ROF03",
	"R-Wpn-Mortar-Acc02",
	"R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03",
	"R-Wpn-Rocket-Accuracy02",
	"R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03",
	"R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage06",
	"R-Wpn-RocketSlow-ROF03",
	"R-Vehicle-Armor-Heat03", //was 2
	"R-Vehicle-Engine06",
	"R-Vehicle-Metals06",
	"R-Cyborg-Metals06",
	"R-Cyborg-Armor-Heat03", //was 2
	"R-Defense-WallUpgrade06",
	"R-Struc-Factory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade03",
	"R-Struc-Materials06",
	"R-Struc-Power-Upgrade01",
	"R-Struc-Research-Upgrade06",
	"R-Struc-RprFac-Upgrade06",
	"R-Sys-Engineering02",
	"R-Sys-MobileRepairTurret01",
	"R-Sys-Sensor-Upgrade01",
	"R-Wpn-AAGun-Accuracy02",
	"R-Wpn-AAGun-Damage03",
	"R-Wpn-AAGun-ROF03",
	"R-Wpn-Bomb-Damage02",
];

const mis_gammaResearchNewClassic = [
	// 1
	"R-Wpn-Howitzer03-Rot", "R-Defense-RotHow", "R-Wpn-Howitzer-Damage04",
	"R-Wpn-Howitzer-Damage05", "R-Wpn-Howitzer-Damage06", "R-Wpn-MG-Damage08", "R-Wpn-MG-Damage09",
	"R-Wpn-Cannon-Damage07", "R-Wpn-Cannon-Damage08", "R-Wpn-AAGun-Damage04", "R-Wpn-AAGun-Damage05",
	"R-Wpn-AAGun-Damage06", "R-Wpn-Cannon-Damage09", "R-Struc-Power-Upgrade02", "R-Sys-Engineering03",
	"R-Defense-WallUpgrade07", "R-Defense-WallUpgrade08", "R-Defense-WallUpgrade09",
	"R-Struc-Materials07", "R-Struc-Materials08", "R-Struc-Materials09",
	// 2
	// 3
	"R-Struc-Research-Upgrade07", "R-Struc-Research-Upgrade08", "R-Struc-Research-Upgrade09",
	"R-Wpn-Laser01", "R-Cyborg-Wpn-Laser1", "R-Defense-PrisLas", "R-Defense-WallTower-PulseLas",
	"R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage01", "R-Wpn-Energy-ROF01", "R-Wpn-Energy-Damage02",
	"R-Vehicle-Body03", "R-Vehicle-Metals07", "R-Cyborg-Metals07", "R-Vehicle-Armor-Heat04",
	"R-Cyborg-Armor-Heat04", "R-Vehicle-Engine07", "R-Wpn-Mortar-Acc03", "R-Wpn-Bomb-Damage03",
	"R-Wpn-Cannon-ROF04", "R-Wpn-Cannon-ROF05", "R-Wpn-Cannon-ROF06", "R-Wpn-AAGun-ROF04",
	"R-Wpn-AAGun-ROF05", "R-Wpn-AAGun-ROF06", "R-Wpn-Mortar-ROF04", "R-Wpn-Howitzer-ROF04",
	// 4
	// 5
	"R-Sys-Resistance-Upgrade01", "R-Sys-Resistance-Upgrade02", "R-Sys-Resistance-Upgrade03",
	// 6
	"R-Wpn-RailGun01", "R-Wpn-Rail-Damage01", "R-Cyborg-Wpn-Rail1", "R-Defense-GuardTower-Rail1",
	"R-Wpn-Rail-Accuracy01", "R-Wpn-Rail-ROF01", "R-Wpn-Howitzer-Accuracy03", "R-Wpn-AAGun-Accuracy03",
	"R-Struc-VTOLPad-Upgrade04", "R-Struc-VTOLPad-Upgrade05", "R-Struc-VTOLPad-Upgrade06",
	"R-Vehicle-Body07", "R-Vehicle-Metals08", "R-Cyborg-Metals08", "R-Vehicle-Armor-Heat05",
	"R-Cyborg-Armor-Heat05", "R-Vehicle-Engine08", "R-Wpn-Missile-LtSAM", "R-Defense-SamSite1",
	"R-Wpn-Missile-Damage01", "R-Wpn-Missile-Accuracy01", "R-Wpn-Missile2A-T",
	"R-Cyborg-Wpn-ATMiss", "R-Defense-GuardTower-ATMiss", "R-Defense-WallTower-A-Tmiss",
	"R-Wpn-Missile-Damage02", "R-Wpn-Missile-ROF01", "R-Wpn-Missile-HvSAM", "R-Defense-SamSite2",
	// 7
	"R-Wpn-RailGun02", "R-Defense-Rail2", "R-Defense-WallTower-Rail2", "R-Wpn-Rail-Damage02",
	"R-Wpn-Rail-ROF02", "R-Wpn-Rail-Damage03", "R-Wpn-Rail-ROF03", "R-Wpn-MdArtMissile", "R-Wpn-Missile-Accuracy02",
	"R-Wpn-Missile-ROF02", "R-Defense-MdArtMissile", "R-Wpn-Laser02", "R-Defense-PulseLas",
	"R-Wpn-Energy-ROF02", "R-Wpn-Energy-Damage03", "R-Wpn-Energy-ROF03",
	// 8
	"R-Sys-Resistance", "R-Comp-MissileCodes01", "R-Comp-MissileCodes02", "R-Comp-MissileCodes03",
	// 9
	"R-Vehicle-Body10", "R-Vehicle-Metals09", "R-Cyborg-Metals09", "R-Vehicle-Armor-Heat06",
	"R-Cyborg-Armor-Heat06", "R-Vehicle-Engine09",  "R-Wpn-HvArtMissile", "R-Defense-HvyArtMissile",
	"R-Wpn-Missile-Damage03", "R-Wpn-Missile-ROF03", "R-Wpn-RailGun03",
	"R-Defense-Rail3", "R-Defense-WallTower-Rail3",
];

// ...
