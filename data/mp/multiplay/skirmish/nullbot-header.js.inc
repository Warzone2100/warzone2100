
const	NUM_GROUPS = 6; // the maximum number of combat groups

const	NO_WARRIORS_AT_ALL = 1;		// the number of droids that doesn't make a cluster yet

const	MIN_VTOL_GROUPS = 2;		// the maximum number of VTOL groups
const	MAX_VTOL_GROUPS = 6;		// the maximum number of VTOL groups

const	ABS_MAX_HOVER_TRUCKS = 13;			// make sure we build at least some hover trucks

const	MAX_DISPERSE = 3; 		// the maximum dispersion of the attack group
const	RETREAT_AT = 50;		// send the unit for repair if it has this much hit point percent left

const	NUM_VTOL_PADS = 1;		// this many vtols per vtol pad are allowed
const	BASE_SIZE = 20;		// range of alertness

const	LOW_POWER = 700;		//decrease ambitions when power is below this level

const	MIN_SENSOR_DISTANCE = 10;		// minimum distance between sensor towers
const	MAX_SENSOR_TANKS = 4;

var	EXTREME_LOW_POWER = 250;

// some definitions useful for both personality code and tail code

const	lab = "A0ResearchFacility";
const	factory = "A0LightFactory";
const	command = "A0CommandCentre";
const	generator = "A0PowerGenerator";
const	derrick = "A0ResourceExtractor";
const	borgfac = "A0CyborgFactory";
const	vtolfac = "A0VTolFactory1";
const	vtolpad = "A0VtolPad";
const	satlink = "A0Sat-linkCentre";
const	lassat = "A0LasSatCommand";
const	oilres = "OilResource";
const	cbtower = "Sys-CB-Tower01";
const	oildrum = "OilDrum";
const	artifact = "Crate";

const	pmod = "A0PowMod1";
const	fmod = "A0FacMod1";
const	rmod = "A0ResearchModule1";

// the following were declared as variables in order to 
// make it possible to override them in personality 
// definitions. but please don't override them unless
// the personality is to work in a modded environment.

var	sensors = [
	"Sys-SensoTowerWS", // wide spectrum sensor
	"Sys-SensoTower02", // hardened sensor
	"Sys-SensoTower01", // sensor
];

var	repair = [
	"A0RepairCentre3", // repair facility
];

var sensorTurrets = [
	"Sensor-WideSpec", // wide spectrum sensor
	"SensorTurret1Mk1", // sensor
];

var	vtolPropulsions = [
	"V-Tol", // guess what ...
];

var	truckBodies = [
	"Body3MBT", // retaliation
	"Body2SUP", // leopard
	"Body4ABT", // bug
	"Body1REC", // viper
];

var	truckPropulsions = [
	"hover01", // hover
	"wheeled01", // wheels
];


// attack this; bottom items first
var	attackTargets = [
	borgfac,
	factory,
	vtolfac,
	derrick,
	satlink,
	lassat,
];

// attack this when going all-in
var allInAttackTargets = [
	borgfac,
	factory,
	vtolfac,
	satlink,
	lassat,
];

function on31() {
	// HACK: see if we are on 3.2 or 3.1.
	return typeof(chat) == "undefined";
}

// get cyborg body depending on the game version
function bB(stat) {
	if (on31())
		return stat;
	if (stat.indexOf("Hvy") > -1)
		return "CyborgHeavyBody";
	else
		return "CyborgLightBody";
}

// some trivial functions useful in personality definitions

// random integer between 0 and max-1 (for convenience)
function random(max) {
	if (max<=0)
		return 0;
	return Math.floor(Math.random() * max);
}

// returns true iff gameTime is less than n minutes
function earlyGame(n) {
	return (gameTime < n*60000);
}

// returns minutes of the hour; can be used as a pseudo-random number
function getTime() {
	var today = new Date();
	return today.getMinutes();
}

// returns a number that is guaranteed to be different for every team
function teamIdentifier() {
	var j=1,s=0;
	for (var i=0; i<maxPlayers; ++i) {
		if (allianceExistsBetween(me,i))
			s+=j;
		j*=2;
	}
	return s;
}

// returns the AI version, based on the file name
function nbVersionString() {
	var str=scriptName;
	switch(difficulty) {
		case EASY: str=str+" "+_("EASY"); break;
		case MEDIUM: str=str+" "+_(" MEDIUM"); break;
		case HARD: str=str+" "+_("HARD"); break;
		case INSANE: str=str+" "+_(" INSANE"); break;
	}
	return str;
}

// returns the AI's power, as accurately as possible in the current API
function myPower() {
	var pwr = playerPower(me);
	if (typeof(queuedPower) != "undefined")
		return pwr - queuedPower(me);
	return pwr;
}


// here come definitions of standard branches used by adaptive research AIs


// vocabulary to recognize common research paths from chat
// first column is the name of the path defined below, will be prefixed with "standardResearchPath" automatically
// other columns are words that name it, should be lowercase
var standardResearchPathVocabulary = [
	[ "Nothing", "0", "null", "zero", "nothing", "empty" ],
	[ "Unblock", "clear", "free", "anything", "unblock" ],
	[ "Fundamental", "fn", "fun", "fnd", "fund", "fundamental", "fundamentals" ],
	[ "Cannons", "c", "cn", "can", "cannon", "cannons" ],
	[ "Rockets", "r", "rx", "rox", "rock", "rocket", "rockets", "msl", ",missile", "missiles" ],
	[ "Flamers", "f", "fl", "flame", "flamer", "flamers", "burn" ],
	[ "Machineguns", "m", "mg", "gun", "guns", "machinegun", "machineguns"],
	[ "Lasers", "l", "ls", "las", "laser", "lasers" ],
	[ "Hurricane", "h", "hu", "hur", "hurr", "hurricane", "hurricanes" ],
	[ "Whirlwind", "w", "wh", "whirl", "whirlwind", "whirlwinds" ],
	[ "Vindicator", "v", "vi", "vin", "vind", "vindicator", "vindicators" ],
	[ "FlakCannon", "flak", "flakcannon", "flakcannons"],
	[ "Stormbringer", "s", "st", "storm", "stormie", "stormbringer", "stormbringers" ],
	[ "Ripples", "ri", "rip", "ripple", "ripples" ],
	[ "Howitzers", "ho", "how", "howy", "howies", "howiez", "howitzer", "howitzers" ],
	[ "Incendiary", "in", "inc", "plasmitebomb", "plasmitebombs", "incend", "incendiary", "incendiaries" ],
	[ "BunkerBuster", "b", "bb", "bunkerbuster", "bunkerbusters" ],
	[ "Electronic", "e", "el", "electronic", "n", "nexus", "emp" ],
	[ "Noob", "noob" ],
];

function standardSwitchResearchPath(msg) {
	if (difficulty==EASY)
		return "Dynamic research paths are not supported on Easy difficulty.";
	var path="";
	var list="";
	for (var i=0; i<standardResearchPathVocabulary.length; ++i) {
		for (var j=1; j<standardResearchPathVocabulary[i].length; ++j) {
			if (msg==standardResearchPathVocabulary[i][j]) {
				path=standardResearchPathVocabulary[i][0];
			}
		}
		list=list+standardResearchPathVocabulary[i][1]+" ";
	}
	if (path!="") {
		eval("forcedResearchPath=standardResearchPath"+path+";");
		return "Switched to a new research path: "+path;
	} else {
		return "Try one of these: "+list;
	}
}

// find out whether the AI is told to stop researching
function researchBlocked() {
	if (typeof(forcedResearchPath)!="undefined")
		return forcedResearchPath[0]=="BLOCKER";
	else
		return false;
}

const standardResearchPathNothing = [
	"BLOCKER",
];

const standardResearchPathUnblock = [
];

const standardResearchPathNoob = [
	"R-Wpn-MG3Mk1",
	"R-Vehicle-Prop-Halftracks",
	"R-Wpn-Rocket02-MRL",
	"R-Wpn-Flamer-ROF01",
	"R-Wpn-Cannon-Damage02",
	"R-Wpn-Rocket-ROF03",
	"R-Wpn-MG-Damage03",
	"R-Defense-Pillbox06",
	"R-Defense-WallTower06",
	"R-Struc-RepairFacility",
	"R-Defense-AASite-QuadMg1",
	"R-Wpn-Rocket03-HvAT",
	"R-Defense-WallTower01",
	"R-Vehicle-Body08",
	"R-Defense-WallTower02",
	"R-Vehicle-Body11",
	"R-Vehicle-Prop-Hover",
	"R-Defense-WallTower03",
	"R-Wpn-Cannon-Damage05",
	"R-Cyborg-Armor-Heat01",
	"R-Wpn-AAGun-Damage01",
	"R-Struc-VTOLPad-Upgrade01",
	"R-Defense-MortarPit-Incenediary",
	"R-Defense-WallTower-HPVcannon",
	"R-Sys-CBSensor-Tower01",
	"R-Wpn-Bomb03",
	"R-Wpn-AAGun-ROF02",
	"R-Struc-Materials03",
	"R-Defense-RotMG",
	"R-Defense-Wall-RotMg",
	"R-Defense-AASite-QuadBof",
	"R-Struc-Power-Upgrade01c",
	"R-Cyborg-Hvywpn-TK",
	"R-Cyborg-Hvywpn-Mcannon",
	"R-Wpn-Bomb02",
	"R-Defense-Sunburst",
	"R-Vehicle-Body12",
	"R-Defense-HvyMor",
	"R-Vehicle-Prop-Tracks",
	"R-Struc-RprFac-Upgrade04",
	"R-Defense-Wall-VulcanCan",
	"R-Defense-Howitzer",
	"R-Defense-WallTower04",
	"R-Defense-Cannon6",
	"R-Defense-IDFRocket",
	"R-Vehicle-Body09",
	"R-Struc-Power-Upgrade03a",
	"R-Wpn-Bomb04",
	"R-Defense-Howitzer-Incenediary",
	"R-Defense-WallTower-HvyA-Trocket",
	"R-Defense-Super-Rocket",
	"R-Defense-Super-Cannon",
	"R-Defense-RotHow",
	"R-Defense-PlasmiteFlamer",
	"R-Defense-WallTower-TwinAGun",
	"R-Struc-Materials06",
	"R-Wpn-Bomb05",
	"R-Cyborg-Hvywpn-PulseLsr",
	"R-Defense-WallTower-PulseLas",
	"R-Cyborg-Hvywpn-A-T",
	"R-Cyborg-Hvywpn-RailGunner",
	"R-Defense-WallTower-Rail2",
	"R-Defense-HvyHowitzer",
	"R-Vehicle-Body10",
	"R-Struc-Research-Upgrade09",
	"R-Defense-SamSite2",
	"R-Sys-SpyTower",
	"R-Wpn-Energy-ROF03",
	"R-Wpn-Energy-Damage03",
	"R-Wpn-Missile-Damage03",
	"R-Wpn-Missile-ROF03",
	"R-Wpn-Missile-Accuracy02",
	"R-Defense-HvyArtMissile",
	"R-Defense-AA-Laser",
	"R-Defense-WallTower-A-Tmiss",
	"R-Defense-WallTower-Rail3",
	"R-Defense-Super-Missile",
	"R-Cyborg-Metals09",
	"R-Struc-VTOLPad-Upgrade06",
	"R-Cyborg-Armor-Heat09",
	"R-Vehicle-Body14",
	"R-Defense-MassDriver",
	"R-Wpn-LasSat",
	"R-Struc-Materials09",
	"R-Struc-RprFac-Upgrade06",
	"R-Defense-EMPMortar",
	"R-Defense-EMPCannon",
];
	
const standardResearchPathFundamental = [
	"R-Vehicle-Prop-Halftracks",
	"R-Struc-PowerModuleMk1",
	"R-Vehicle-Body05",
	"R-Struc-RprFac-Upgrade01",
	"R-Struc-Materials01",
	"R-Sys-Sensor-Tower02",
	"R-Vehicle-Body08",
	"R-Vehicle-Body11",
	"R-Cyborg-Armor-Heat03",
	"R-Struc-Power-Upgrade01c",
	"R-Sys-CBSensor-Tower01",
	"R-Vehicle-Prop-Tracks",
	"R-Struc-Materials04",
	"R-Struc-VTOLPad-Upgrade01",
	"R-Vehicle-Body12",
	"R-Struc-Power-Upgrade03a",
	"R-Vehicle-Body09",
	"R-Struc-VTOLPad-Upgrade03",
	"R-Sys-Autorepair-General",
	"R-Vehicle-Body10",
	"R-Wpn-LasSat",
	"R-Struc-RprFac-Upgrade04",
	"R-Cyborg-Metals09",
	"R-Cyborg-Armor-Heat09",
	"R-Vehicle-Body14",
	"R-Struc-VTOLPad-Upgrade06",
	"R-Struc-Materials09",
	"R-Struc-RprFac-Upgrade06",
];

const standardResearchPathCannons = [
	"R-Wpn-Cannon-Damage03",
	"R-Defense-WallTower02",
	"R-Defense-Pillbox04",
	"R-Defense-Emplacement-HPVcannon",
	"R-Defense-WallTower-HPVcannon",
	"R-Wpn-Cannon-Damage05",
	"R-Defense-AASite-QuadBof",
	"R-Wpn-Cannon-Damage07",
	"R-Defense-WallTower04",
	"R-Cyborg-Hvywpn-Mcannon",
	"R-Cyborg-Hvywpn-HPV",
	"R-Defense-Wall-VulcanCan",
	"R-Wpn-Cannon-ROF06",
	"R-Cyborg-Hvywpn-Acannon",
	"R-Defense-Cannon6",
	"R-Defense-Super-Cannon",
	"R-Wpn-Cannon-Damage09",
	"R-Cyborg-Hvywpn-RailGunner",
	"R-Defense-WallTower-Rail2",
	"R-Defense-WallTower-Rail3",
	"R-Defense-MassDriver",
];

const standardResearchPathRockets = [
	"R-Wpn-Rocket-Damage02",
	"R-Defense-Pillbox06",
	"R-Wpn-Rocket-ROF01",
	"R-Defense-MRL",
	"R-Defense-WallTower06",
	"R-Defense-Sunburst",
	"R-Wpn-Rocket-ROF03",
	"R-Defense-HvyA-Trocket",
	"R-Cyborg-Hvywpn-TK",
	"R-Defense-WallTower-HvyA-Trocket",
	"R-Defense-Super-Rocket",
	"R-Defense-GuardTower-ATMiss",
	"R-Cyborg-Hvywpn-A-T",
	"R-Defense-WallTower-A-Tmiss",
	"R-Defense-MdArtMissile",
	"R-Wpn-Missile-Damage03",
	"R-Wpn-Missile-ROF03",
	"R-Wpn-Missile-Accuracy02",
	"R-Defense-Super-Missile",
];

const standardResearchPathMachineguns = [
	"R-Wpn-MG-Damage02",
	"R-Defense-Tower01",
	"R-Defense-Pillbox01",
	"R-Wpn-MG3Mk1",
	"R-Wpn-MG-ROF02",
	"R-Defense-AASite-QuadMg1",
	"R-Wpn-MG-ROF03",
	"R-Wpn-AAGun-Damage01",
	"R-Defense-Wall-RotMg",
	"R-Wpn-AAGun-ROF02",
	"R-Defense-RotMG",
	"R-Wpn-MG-Damage04",
	"R-Defense-WallTower-TwinAGun",
	"R-Wpn-MG-Damage05",
];

const standardResearchPathFlamers = [
	"R-Wpn-Flamer-ROF01",
	"R-Wpn-Flamer-Damage03",
	"R-Defense-HvyFlamer",
	"R-Vehicle-Prop-Hover",
	"R-Wpn-Flamer-ROF03",
	"R-Defense-PlasmiteFlamer",
	"R-Wpn-Flamer-Damage09",
];

const standardResearchPathLasers = [
	"R-Cyborg-Hvywpn-PulseLsr",
	"R-Defense-WallTower-PulseLas",
	"R-Wpn-Energy-ROF03",
	"R-Wpn-Energy-Damage03",
];

const standardResearchPathHurricane = [
	"R-Defense-AASite-QuadMg1",
	"R-Wpn-AAGun-ROF02",
	"R-Wpn-AAGun-Damage03",
];

const standardResearchPathWhirlwind = [
	"R-Defense-AASite-QuadRotMg",
	"R-Wpn-AAGun-ROF03",
];

const standardResearchPathVindicator = [
	"R-Defense-Sunburst",
	"R-Defense-SamSite1",
	"R-Defense-SamSite2",
];

const standardResearchPathFlakCannon = [
	"R-Defense-AASite-QuadBof",
];

const standardResearchPathStormbringer = [
	"R-Defense-AA-Laser",
];

const standardResearchPathRipples = [
	"R-Defense-IDFRocket",
	"R-Defense-HvyArtMissile",
];

const standardResearchPathHowitzers = [
	"R-Defense-MortarPit",
	"R-Defense-HvyMor",
	"R-Wpn-Bomb01",
	"R-Struc-VTOLPad-Upgrade01",
	"R-Defense-Howitzer",
	"R-Wpn-Bomb02",
	"R-Defense-HvyHowitzer",
	"R-Wpn-Howitzer-Damage06",
	"R-Wpn-Howitzer-ROF04",
	"R-Wpn-Howitzer-Accuracy03",
];

const standardResearchPathIncendiary = [
	"R-Defense-MortarPit-Incenediary",
	"R-Wpn-Bomb03",
	"R-Struc-VTOLPad-Upgrade01",
	"R-Defense-Howitzer-Incenediary",
	"R-Wpn-Bomb04",
	"R-Wpn-Howitzer-Damage03",
	"R-Wpn-Howitzer-ROF04",
	"R-Wpn-Howitzer-Accuracy03",
	"R-Wpn-Bomb05",
];
	
const standardResearchPathBunkerBuster = [
	"R-Defense-MRL",
	"R-Wpn-Rocket03-HvAT",
];

const standardResearchPathElectronic = [
	"R-Sys-SpyTower",
	"R-Defense-EMPMortar",
	"R-Defense-EMPCannon",
];

const standardTankBodies = [
	[
		"Body10MBT", // vengeance
		"Body7ABT", // retribution
		"Body9REC", // tiger
		"Body12SUP", // mantis
		"Body6SUPP", // panther
		"Body8MBT", // scorpion
		"Body11ABT", // python
		"Body3MBT", // retaliation
		"Body5REC", // cobra
		"Body2SUP", // leopard
		"Body4ABT", // bug
		"Body1REC", // viper
	],
	[
		"Body14SUP", // dragon
		"Body13SUP", // wyvern
		"Body10MBT", // vengeance
		"Body9REC", // tiger
		"Body7ABT", // retribution
		"Body6SUPP", // panther
		"Body12SUP", // mantis
		"Body11ABT", // python
		"Body8MBT", // scorpion
		"Body3MBT", // retaliation
		"Body5REC", // cobra
		"Body2SUP", // leopard
		"Body4ABT", // bug
		"Body1REC", // viper
	],
];

const standardTankBodiesLight = [
	"Body10MBT", // vengeance
	"Body7ABT", // retribution
	"Body12SUP", // mantis
	"Body8MBT", // scorpion
	"Body11ABT", // python
	"Body9REC", // tiger
	"Body3MBT", // retaliation
	"Body5REC", // cobra
	"Body6SUPP", // panther
	"Body4ABT", // bug
	"Body1REC", // viper
	"Body2SUP", // leopard
];

const standardTankBodiesHeavy = [
	"Body14SUP", // dragon
	"Body13SUP", // wyvern
	"Body10MBT", // vengeance
	"Body7ABT", // retribution
	"Body9REC", // tiger
	"Body11ABT", // python
	"Body12SUP", // mantis
	"Body3MBT", // retaliation
	"Body6SUPP", // panther
	"Body5REC", // cobra
	"Body8MBT", // scorpion
	"Body2SUP", // leopard
	"Body1REC", // viper
	"Body4ABT", // bug
];

const standardTankPropulsions = [
	[
		"tracked01", // tracks
		"HalfTrack", // half-track
		"wheeled01", // wheels
	],
	[
		"hover01", // hover
		"HalfTrack", // half-track
		"wheeled01", // wheels
	],
];

const standardTankPropulsionsNoob = [
	[
		"tracked01", // tracks
		"HalfTrack", // half-track
		"wheeled01", // wheels
	],
	[
		"HalfTrack", // half-track
		"wheeled01", // wheels
	],
	[
		"hover01", // hover
		"wheeled01", // wheels
	],
];

const standardTankPropulsionsHover = [
	[
		"hover01", // hover
	],
];

const standardTankATNoobOne = [
	"RailGun3Mk1", // gauss cannon
	"RailGun2Mk1", // rail gun
	"Missile-A-T", // scourge
	"Cannon6TwinAslt", // twin assault cannon
	"Cannon375mmMk1", // heavy cannon
	"Rocket-HvyA-T", // tk
	"Cannon4AUTOMk1", // hpv
	"Cannon2A-TMk1", // medium cannon
	"Rocket-LtA-T", // lancer
	"Cannon1Mk1", // light cannon
	"Rocket-Pod", // minipod
];

const standardTankATNoobTwo = [
	"Missile-A-T", // scourge
	"RailGun3Mk1", // gauss cannon
	"RailGun2Mk1", // rail gun
	"Rocket-HvyA-T", // tk
	"Cannon375mmMk1", // heavy cannon
	"Cannon6TwinAslt", // twin assault cannon
	"Rocket-LtA-T", // lancer
	"Cannon2A-TMk1", // medium cannon
	"Cannon4AUTOMk1", // hpv
	"Rocket-Pod", // minipod
	"Cannon1Mk1", // light cannon
];

const standardTankAPNoobOne = [
	"Laser2PULSEMk1", // pulse laser
	"Laser3BEAMMk1", // flashlight
	"MG5TWINROTARY", // tag
	"MG4ROTARYMk1", // ag
	"MG3Mk1", // hmg
	"MG2Mk1", // tmg
	"MG1Mk1", // mg
];

const standardTankAPNoobTwo = [
	"Missile-MdArt", // seraph
	"Laser2PULSEMk1", // pulse laser
	"Laser3BEAMMk1", // flashlight
	"PlasmiteFlamer", // plasmite
	"Flame2", // inferno
	"MG5TWINROTARY", // tag
	"MG4ROTARYMk1", // ag
	"Rocket-MRL", // mini-rocket arty
	"Flame1Mk1", // flamer
	"MG3Mk1", // hmg
	"MG2Mk1", // tmg
	"MG1Mk1", // mg
];

const standardTankCannonOne = [
	"RailGun3Mk1", // gauss cannon
	"RailGun2Mk1", // rail gun
	"Cannon6TwinAslt", // twin assault cannon
	"Cannon375mmMk1", // heavy cannon
	"Cannon4AUTOMk1", // hpv
	"Cannon2A-TMk1", // medium cannon
	"Cannon1Mk1", // light cannon
];

const standardTankCannonTwo = [
	"RailGun3Mk1", // gauss cannon
	"RailGun2Mk1", // rail gun
	"Cannon375mmMk1", // heavy cannon
	"Cannon6TwinAslt", // twin assault cannon
	"Cannon2A-TMk1", // medium cannon
	"Cannon4AUTOMk1", // hpv
	"Cannon1Mk1", // light cannon
];

const standardTankRocket = [
	"Missile-A-T", // scourge
	"Rocket-HvyA-T", // tk
	"Rocket-LtA-T", // lancer
	"Rocket-MRL", // mini-rocket arty, not really AT but still better than minipod in most cases
	"Rocket-Pod", // minipod
];

const standardTankMachinegun = [
	"MG5TWINROTARY", // tag
	"MG4ROTARYMk1", // ag
	"MG3Mk1", // hmg
	"MG2Mk1", // tmg
	"MG1Mk1", // mg
];

const standardTankFlamer = [
	"PlasmiteFlamer", // plasmite
	"Flame2", // inferno
	"Flame1Mk1", // flamer
	"MG2Mk1", // tmg
	"MG1Mk1", // mg
];

const standardTankLaser = [
	"Laser2PULSEMk1", // pulse laser
];

const standardTankAPRocket = [
	"Missile-MdArt", // seraph
	"Rocket-MRL", // mini-rocket arty
];

const standardTankAPMachinegunRocket = [
	"Missile-MdArt", // seraph
	"MG5TWINROTARY", // tag
	"MG4ROTARYMk1", // ag
	"Rocket-MRL", // mini-rocket arty
	"MG3Mk1", // hmg
	"MG2Mk1", // tmg
	"MG1Mk1", // mg
];

const standardTankAB = [
	[
		"Missile-MdArt", // seraph
		"Howitzer-Incenediary", // incendiary howitzer
		"Howitzer03-Rot", // hellstorm
		"Howitzer105Mk1", // howitzer
		"Mortar-Incenediary", // incendiary mortar
		"Mortar2Mk1", // bombard
		"Mortar3ROTARYMk1", // pepperpot
		"Rocket-BB", // bunker buster
		"Rocket-MRL", // mra
		"Mortar1Mk1", // mortar
	],
	[
		"Missile-MdArt", // seraph
		"Howitzer03-Rot", // hellstorm
		"Howitzer-Incenediary", // incendiary howitzer
		"Howitzer105Mk1", // howitzer
		"Mortar2Mk1", // bombard
		"Mortar3ROTARYMk1", // pepperpot
		"Mortar-Incenediary", // incendiary mortar
		"Rocket-BB", // bunker buster
		"Rocket-MRL", // mra
		"Mortar1Mk1", // mortar
	],
	[
		"Howitzer03-Rot", // hellstorm
		"Howitzer-Incenediary", // incendiary howitzer
		"Missile-MdArt", // seraph
		"Mortar3ROTARYMk1", // pepperpot
		"Mortar-Incenediary", // incendiary mortar
		"Mortar2Mk1", // bombard
		"Rocket-BB", // bunker buster
		"Rocket-MRL", // mra
		"Mortar1Mk1", // mortar
	],
	[
		"Missile-MdArt", // seraph
		"Rocket-BB", // bunker buster
		"Rocket-MRL", // mra
	],
];

const standardTankAA = [
	[
		"AAGunLaser", // stormbringer
		"Missile-HvySAM", // vindicator
		"Missile-LtSAM", // avenger
		"QuadRotAAGun", // whirlwind
		"AAGun2Mk1", // flak cannon
		"Rocket-Sunburst", // sunburst
		"QuadMg1AAGun", // hurricane
	],
];


const standardCyborgAPNoobOne = [
	[bB("Cyb-Hvybod-PulseLsr"),"Cyb-Hvywpn-PulseLsr"], // super pulse
	[bB("Cyb-Bod-Las1"),"Cyb-Wpn-Laser"], // flashlight
	[bB("CybRotMgGrd"),"CyborgRotMG"], // assault gun
	[bB("Cyb-Bod-Grenade"), "Cyb-Wpn-Grenade"], // mortar
	[bB("CyborgChain1Ground"), "CyborgChaingun"], // machinegun
];

const standardCyborgAPNoobTwo = [
	[bB("Cyb-Hvybod-PulseLsr"),"Cyb-Hvywpn-PulseLsr"], // super pulse
	[bB("Cyb-Bod-Las1"),"Cyb-Wpn-Laser"], // flashlight
	[bB("CybRotMgGrd"),"CyborgRotMG"], // assault gun
	[bB("Cyb-Bod-Thermite"), "Cyb-Wpn-Thermite"], // thermite
	[bB("CyborgFlamerGrd"), "CyborgFlamer01"], // flamer
	[bB("CyborgChain1Ground"), "CyborgChaingun"], // machinegun
];

const standardCyborgATNoobOne = [
	[bB("Cyb-Hvybod-A-T"),"Cyb-Hvywpn-A-T"], // super scourge
	[bB("Cyb-Hvybod-RailGunner"),"Cyb-Hvywpn-RailGunner"], // super rail
	[bB("Cyb-Bod-Rail1"),"Cyb-Wpn-Rail1"], // needle
	[bB("Cyb-Bod-Atmiss"),"Cyb-Wpn-Atmiss"], // scourge
	[bB("Cyb-Hvybod-Acannon"),"Cyb-Hvywpn-Acannon"], // super auto cannon
	[bB("CyborgRkt1Ground"), "CyborgRocket"], // lancer
	[bB("CyborgCannonGrd"), "CyborgCannon"], // cannon
];

const standardCyborgATNoobTwo = [
	[bB("Cyb-Hvybod-RailGunner"),"Cyb-Hvywpn-RailGunner"], // super rail
	[bB("Cyb-Hvybod-A-T"),"Cyb-Hvywpn-A-T"], // super scourge
	[bB("Cyb-Bod-Atmiss"),"Cyb-Wpn-Atmiss"], // scourge
	[bB("Cyb-Bod-Rail1"),"Cyb-Wpn-Rail1"], // needle
	[bB("Cyb-Hvybod-HPV"),"Cyb-Hvywpn-HPV"], // super hpv cannon
	[bB("CyborgRkt1Ground"), "CyborgRocket"], // lancer
	[bB("CyborgCannonGrd"), "CyborgCannon"], // cannon
];

const standardCyborgATNoobThree = [
	[bB("Cyb-Hvybod-A-T"),"Cyb-Hvywpn-A-T"], // super scourge
	[bB("Cyb-Hvybod-RailGunner"),"Cyb-Hvywpn-RailGunner"], // super rail
	[bB("Cyb-Bod-Rail1"),"Cyb-Wpn-Rail1"], // needle
	[bB("Cyb-Bod-Atmiss"),"Cyb-Wpn-Atmiss"], // scourge
	[bB("Cyb-Hvybod-TK"),"Cyb-Hvywpn-TK"], // super tk
	[bB("Cyb-Hvybod-Mcannon"),"Cyb-Hvywpn-Mcannon"], // super cannon
	[bB("CyborgRkt1Ground"), "CyborgRocket"], // lancer
	[bB("CyborgCannonGrd"), "CyborgCannon"], // cannon
];

const standardCyborgCannonOne = [
	[bB("Cyb-Hvybod-RailGunner"),"Cyb-Hvywpn-RailGunner"], // super rail
	[bB("Cyb-Bod-Rail1"),"Cyb-Wpn-Rail1"], // needle
	[bB("Cyb-Hvybod-Acannon"),"Cyb-Hvywpn-Acannon"], // super auto cannon
	[bB("CyborgCannonGrd"), "CyborgCannon"], // cannon
];

const standardCyborgCannonTwo = [
	[bB("Cyb-Hvybod-RailGunner"),"Cyb-Hvywpn-RailGunner"], // super rail
	[bB("Cyb-Bod-Rail1"),"Cyb-Wpn-Rail1"], // needle
	[bB("Cyb-Hvybod-HPV"),"Cyb-Hvywpn-HPV"], // super hpv cannon
	[bB("CyborgCannonGrd"), "CyborgCannon"], // cannon
];
const standardCyborgCannonThree = [
	[bB("Cyb-Hvybod-RailGunner"),"Cyb-Hvywpn-RailGunner"], // super rail
	[bB("Cyb-Bod-Rail1"),"Cyb-Wpn-Rail1"], // needle
	[bB("Cyb-Hvybod-Mcannon"),"Cyb-Hvywpn-Mcannon"], // super cannon
	[bB("CyborgCannonGrd"), "CyborgCannon"], // cannon
];

const standardCyborgRocket = [
	[bB("Cyb-Hvybod-A-T"),"Cyb-Hvywpn-A-T"], // super scourge
	[bB("Cyb-Bod-Atmiss"),"Cyb-Wpn-Atmiss"], // scourge
	[bB("Cyb-Hvybod-TK"),"Cyb-Hvywpn-TK"], // super tk
	[bB("CyborgRkt1Ground"), "CyborgRocket"], // lancer
];

const standardCyborgMachinegun = [
	[bB("CybRotMgGrd"),"CyborgRotMG"], // assault gun
	[bB("CyborgChain1Ground"), "CyborgChaingun"], // machinegun
];

const standardCyborgFlamer = [
	[bB("Cyb-Bod-Thermite"), "Cyb-Wpn-Thermite"], // thermite
	[bB("CyborgFlamerGrd"), "CyborgFlamer01"], // flamer
];

const standardCyborgLaser = [
	[bB("Cyb-Hvybod-PulseLsr"),"Cyb-Hvywpn-PulseLsr"], // super pulse
	[bB("Cyb-Bod-Las1"),"Cyb-Wpn-Laser"], // flashlight
];

const standardCyborgMachinegunMortar = [
	[bB("CybRotMgGrd"),"CyborgRotMG"], // assault gun
	[bB("Cyb-Bod-Grenade"), "Cyb-Wpn-Grenade"], // mortar
	[bB("CyborgChain1Ground"), "CyborgChaingun"], // machinegun
];

// standard build order definitions

function standardCheckLowPower() {
	if (typeof(countStruct)!="undefined") { // start conditions changed in 3.2, so more workaround is necessary
		var genCount = countStruct(generator,me);
		var derrickCount = countStruct(derrick,me);
		var factoryCount = countStruct(factory,me)+countStruct(borgfac,me);
		if (myPower()<LOW_POWER) {
			if (genCount==0)
				return buildBasicStructure(generator); 
			if (derrickCount==0)
				if (factoryCount>0) {
					return true; // don't do anything unless a derrick is found
				}
		}
	}
	return false;
}

function standardBuildOrderNoob() {
	// a noob doesn't know what a build order is, so it relies
	// on keepBuildingThings() from the start
	return false;
}

function standardBuildOrder32() {
	// The standard build order for v3.2
	var labCount = enumStruct(me, lab).length;
	var factoryCount = enumStruct(me, factory).length;
	var commandCount = enumStruct(me, command).length;
	var genCount = enumStruct(me, generator).length;
	var derrickCount = enumStruct(me, derrick).length;
	var borgCount = enumStruct(me, borgfac).length;
	var vtolCount = enumStruct(me, vtolfac).length;
	if (derrickCount > 0 && genCount == 0)
		return buildBasicStructure(generator);
	if (labCount < 1)
		return buildBasicStructure(lab);
	if (factoryCount < 1)
		return buildBasicStructure(factory);
	if (derrickCount < 1)
		return true;
	if (genCount < 2)
		return buildBasicStructure(generator);
	if (commandCount < 1)
		return buildBasicStructure(command);
	if (derrickCount < 5)
		return true;
	return false;
}

function standardBuildOrderFRCFR() {
	if (!on31())
		return standardBuildOrder32();
	// Builds initial structures in the following order:
	//     factory, lab, cc, factory, generator, lab, factory, generator, lab.
	//     * Doesn't build 3rd lab until 10th minute of the game.
	// Also make sure we have at least one generator before our initial money runs out
	var labCount = enumStruct(me, lab).length;
	var factoryCount = enumStruct(me, factory).length;
	var commandCount = enumStruct(me, command).length;
	var genCount = enumStruct(me, generator).length;
	var derrickCount = enumStruct(me, derrick).length;
	var borgCount = enumStruct(me, borgfac).length;
	var vtolCount = enumStruct(me, vtolfac).length;
	if (genCount < 2 && myPower() < EXTREME_LOW_POWER && isStructureAvailable(generator,me))
		return buildBasicStructure(generator); 
	if (factoryCount < 1 && isStructureAvailable(factory,me))
		return buildBasicStructure(factory); 
	if (labCount < 1 && isStructureAvailable(lab,me) && !researchBlocked())
		return buildBasicStructure(lab); 
	if (commandCount < 1 && isStructureAvailable(command,me))
		return buildBasicStructure(command); 
	if (factoryCount < 2 && isStructureAvailable(factory,me))
		return buildBasicStructure(factory,0); 
	if ((genCount < 1) && isStructureAvailable(generator,me))
		return buildBasicStructure(generator); 
	if (labCount < 2 && isStructureAvailable(lab,me) && !researchBlocked())
		return buildBasicStructure(lab,0); 
	if (factoryCount < 3 && isStructureAvailable(factory,me))
		return buildBasicStructure(factory,0); 
	if ((genCount < 2) && isStructureAvailable(generator,me))
		return buildBasicStructure(generator,0); 
	if (labCount < 3 && isStructureAvailable(lab,me) && !researchBlocked() && (!earlyGame(10)&&!UNUSUAL_SITUATION))
		return buildBasicStructure(lab,0); 
	if (borgCount < 2 && isStructureAvailable(borgfac,me) && RATE_TANK>0)
		return buildBasicStructure(borgfac,0); 
	if (vtolCount < 1 && isStructureAvailable(vtolfac,me))
		return buildBasicStructure(vtolfac,0); 
	return false;
}

function standardBuildOrderRFFRC() {
	if (!on31())
		return standardBuildOrder32();
	// Builds initial structures in the following order:
	//     lab, factory, factory, lab, command center, generator, generator, factory, lab.
	//     * Doesn't build 3rd factory until 8th minute of the game.
	//     * Doesn't build 3rd lab until 10th minute of the game.
	// Also make sure we have at least one generator before our initial money runs out
	var labCount = enumStruct(me, lab).length;
	var factoryCount = enumStruct(me, factory).length;
	var commandCount = enumStruct(me, command).length;
	var genCount = enumStruct(me, generator).length;
	var derrickCount = enumStruct(me, derrick).length;
	var borgCount = enumStruct(me, borgfac).length;
	var vtolCount = enumStruct(me, vtolfac).length;
	if (genCount < 2 && myPower() < EXTREME_LOW_POWER && isStructureAvailable(generator,me))
		return buildBasicStructure(generator);
	if (labCount < 1 && isStructureAvailable(lab,me) && !researchBlocked())
		return buildBasicStructure(lab);
	if (factoryCount < 1 && isStructureAvailable(factory,me))
		return buildBasicStructure(factory);
	if (factoryCount < 2 && isStructureAvailable(factory,me))
		return buildBasicStructure(factory,0);
	if (labCount < 2 && isStructureAvailable(lab,me) && !researchBlocked())
		return buildBasicStructure(lab,0);
	if (commandCount < 1 && isStructureAvailable(command,me))
		return buildBasicStructure(command,0);
	if ((genCount < 2) && isStructureAvailable(generator,me))
		return buildBasicStructure(generator);
	if (factoryCount < 3 && isStructureAvailable(factory,me) && (!earlyGame(8)&&!UNUSUAL_SITUATION))
		return buildBasicStructure(factory,0);
	if (labCount < 3 && isStructureAvailable(lab,me) && !researchBlocked() && (!earlyGame(10)&&!UNUSUAL_SITUATION))
		return buildBasicStructure(lab,0);
	if (borgCount < 2 && isStructureAvailable(borgfac,me) && RATE_TANK>0)
		return buildBasicStructure(borgfac,0);
	if (vtolCount < 1 && isStructureAvailable(vtolfac,me)) 
		return buildBasicStructure(vtolfac,0);
	return false;
}

function standardBuildOrderRFFCR() {
	if (!on31())
		return standardBuildOrder32();
	// Builds initial structures in the following order:
	//     lab, factory, factory, command center, lab, generator, generator, factory, factory, lab
	//     * Doesn't build 3rd lab until 15th minute of the game.
	// Also make sure we have at least one generator before our initial money runs out
	var labCount = enumStruct(me, lab).length;
	var factoryCount = enumStruct(me, factory).length;
	var commandCount = enumStruct(me, command).length;
	var genCount = enumStruct(me, generator).length;
	var derrickCount = enumStruct(me, derrick).length;
	var borgCount = enumStruct(me, borgfac).length;
	var vtolCount = enumStruct(me, vtolfac).length;
	if (genCount < 2 && myPower() < EXTREME_LOW_POWER && isStructureAvailable(generator,me))
		return buildBasicStructure(generator);
	if (labCount < 1 && isStructureAvailable(lab,me) && !researchBlocked())
		return buildBasicStructure(lab);
	if (factoryCount < 1 && isStructureAvailable(factory,me))
		return buildBasicStructure(factory);
	if (factoryCount < 2 && isStructureAvailable(factory,me))
		return buildBasicStructure(factory,0);
	if (commandCount < 1 && isStructureAvailable(command,me))
		return buildBasicStructure(command,0);
	if (labCount < 2 && isStructureAvailable(lab,me) && !researchBlocked())
		return buildBasicStructure(lab,0);
	if ((genCount < 2) && isStructureAvailable(generator,me))
		return buildBasicStructure(generator);
	if (factoryCount < 4 && isStructureAvailable(factory,me) && (!UNUSUAL_SITUATION))
		return buildBasicStructure(factory,0);
	if (labCount < 3 && isStructureAvailable(lab,me) && !researchBlocked() && (!earlyGame(15)&&!UNUSUAL_SITUATION))
		return buildBasicStructure(lab,0);
	if (borgCount < 3 && isStructureAvailable(borgfac,me) && RATE_TANK>0)
		return buildBasicStructure(borgfac,0);
	if (vtolCount < 1 && isStructureAvailable(vtolfac,me)) 
		return buildBasicStructure(vtolfac,0);
	return false;
}
