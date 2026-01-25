// Naming convention notes.
// Distinguish local const arrays|objects with a single underline _ and camelCase.
// Use 2 underlines for local consts that are of immutable types, such as: numbers, booleans, strings. Use camelCase.
// Use _ for global consts that are objects/arrays, and __ for other globals. CAPITALIZE these ones as well with _ seperating words.

//globals/constants/definitions and whatever. Includes at the bottom.
const __COBRA_INCLUDES = "/multiplay/skirmish/cobra_includes/";
const __COBRA_RULESETS = "/multiplay/skirmish/cobra_rulesets/";

//Rulesets here.
include(__COBRA_RULESETS + "CobraStandard.js");

const __LOG_RESEARCH_PATH = false;

const __MIN_ATTACK_DROIDS = 4;
const _MY_BASE = startPositions[me];
const __OIL_RES = "OilResource";
const __MIN_POWER = 150;
const __SUPER_LOW_POWER = 70;
const __MIN_BUILD_POWER = 220;
const __PRODUCTION_POWER = __SUPER_LOW_POWER;
const _ELECTRONIC_DEFENSES = [
	"Sys-SpyTower",
	"WallTower-EMP",
	"Emplacement-MortarEMP",
];
const __BEACON_VTOL_ALARM = "vtolSpotted";

//Research constants
const _TANK_ARMOR = [
	"R-Vehicle-Metals09",
];
const _CYBORG_ARMOR = [
	"R-Cyborg-Metals09",
];
const _TANK_ARMOR_THERMAL = [
	"R-Vehicle-Armor-Heat09",
];
const _CYBORG_ARMOR_THERMAL = [
	"R-Cyborg-Armor-Heat09",
];
const _MODULE_RESEARCH = [
	"R-Struc-Research-Module",
	"R-Struc-Factory-Module",
	"R-Struc-PowerModuleMk1",
];
const _MOST_ESSENTIAL = [
	"R-Wpn-MG-Damage01",
	"R-Sys-Engineering01",
	"R-Defense-Tower01",
];
const _ESSENTIALS = [
	"R-Wpn-MG2Mk1",
	"R-Wpn-MG-Damage02",
	"R-Struc-PowerModuleMk1",
	"R-Struc-Research-Upgrade01",
	"R-Vehicle-Prop-Halftracks",
	"R-Vehicle-Body05",
	"R-Wpn-MG-Damage03",
];
const _ESSENTIALS_2 = [
	"R-Vehicle-Metals02",
	"R-Cyborg-Metals02",
	"R-Wpn-MG3Mk1",
	"R-Struc-RprFac-Upgrade01",
	"R-Struc-Power-Upgrade03a",
	"R-Sys-Autorepair-General",
	"R-Struc-Research-Upgrade09",
];
const _ESSENTIALS_3 = [
	"R-Vehicle-Body04",
	"R-Vehicle-Prop-Hover",
	"R-Struc-Factory-Upgrade09",
];
const _SYSTEM_UPGRADES = [
	"R-Vehicle-Prop-Tracks",
	"R-Struc-RprFac-Upgrade06",
	"R-Sys-Sensor-Upgrade03",
];
const _FLAMER = [
	"R-Wpn-Flame2",
	"R-Wpn-Flamer-ROF03",
	"R-Wpn-Flamer-Damage09",
];
const _SENSOR_TECH = [
	"R-Sys-CBSensor-Tower01",
	"R-Sys-Sensor-WSTower",
	"R-Sys-Sensor-UpLink",
	"R-Wpn-LasSat",
	"R-Sys-Resistance-Circuits",
];
const _DEFENSE_UPGRADES = [
	"R-Struc-Materials01",
	"R-Struc-Materials02",
	"R-Struc-Materials03",
	"R-Defense-WallUpgrade01",
	"R-Defense-WallUpgrade02",
	"R-Defense-WallUpgrade03",
	"R-Defense-WallUpgrade04",
	"R-Defense-WallUpgrade05",
	"R-Defense-WallUpgrade06",
	"R-Defense-WallUpgrade07",
	"R-Defense-WallUpgrade08",
	"R-Defense-WallUpgrade09",
	"R-Defense-WallUpgrade10",
	"R-Defense-WallUpgrade11",
	"R-Defense-WallUpgrade12",
];
const _BODY_RESEARCH = [
	"R-Vehicle-Body08",
	"R-Vehicle-Body12",
	"R-Vehicle-Body09",
	"R-Vehicle-Body10",
	"R-Vehicle-Engine09",
	"R-Vehicle-Body14",
];
const _VTOL_RES = [
	"R-Struc-VTOLPad-Upgrade01",
	"R-Wpn-Bomb02",
	"R-Struc-VTOLPad-Upgrade03",
	"R-Wpn-Bomb-Damage03",
	"R-Struc-VTOLPad-Upgrade06",
	"R-Wpn-Bomb04",
	"R-Wpn-Bomb05",
	"R-Wpn-Bomb06",
];

//Production constants
const _TANK_BODY = [
	"Body14SUP", // Dragon
	"Body13SUP", // Wyvern
	"Body10MBT", // Vengeance
	"Body9REC",  // Tiger
	"Body11ABT", // Python
	"Body5REC",  // Cobra
	"Body1REC",  // Viper
];
const _SYSTEM_BODY = [
	"Body3MBT",  // Retaliation
	"Body2SUP",  // Leopard
	"Body4ABT",  // Bug
	"Body1REC",  // Viper
];
const _SYSTEM_PROPULSION = [
	"hover01", // hover
	"wheeled01", // wheels
];
const _VTOL_BODY = [
	"Body7ABT",  // Retribution
	"Body6SUPP", // Panther
	"Body12SUP", // Mantis
	"Body8MBT",  // Scorpion
	"Body5REC",  // Cobra
	"Body1REC",  // Viper
];
const _ARTILLERY_SENSORS = [
	"Sensor-WideSpec",
	"SensorTurret1Mk1",
];


//List of Cobra personalities:
var subPersonalities =
{
	AC:
	{
		"primaryWeapon": _WEAPON_STATS.cannons,
		"secondaryWeapon": _WEAPON_STATS.gauss,
		"artillery": _WEAPON_STATS.mortars,
		"antiAir": _WEAPON_STATS.cannons_AA,
		"factoryOrder": [_STRUCTURES.factory, _STRUCTURES.cyborgFactory, _STRUCTURES.vtolFactory],
		"defensePriority": 15,
		"vtolPriority": 40,
		"alloyPriority": 33,
		"useLasers": true,
		"cyborgThreatPercentage": 0.08,
		"retreatScanRange": 12,
		"resPath": "generic",
		"res": [
			"R-Wpn-Cannon-Damage02",
			"R-Wpn-Cannon-ROF02",
		],
		"canPlayBySelf": true,
		"allowAutomaticPersonalityOverride": true,
		"beaconArmyPercentage": 60,
		"beaconArtilleryPercentage": 40,
		"beaconVtolPercentage": 60,
	},
	AR:
	{
		"primaryWeapon": _WEAPON_STATS.flamers,
		"secondaryWeapon": _WEAPON_STATS.gauss,
		"artillery": _WEAPON_STATS.mortars,
		"antiAir": _WEAPON_STATS.AA,
		"factoryOrder": [_STRUCTURES.factory, _STRUCTURES.cyborgFactory, _STRUCTURES.vtolFactory],
		"defensePriority": 15,
		"vtolPriority": 50,
		"alloyPriority": 35,
		"useLasers": true,
		"cyborgThreatPercentage": 0.40,
		"retreatScanRange": 5,
		"resPath": "offensive",
		"res": [
			"R-Wpn-Flamer-Damage03",
			"R-Wpn-Flamer-ROF01",
		],
		"canPlayBySelf": false,
		"allowAutomaticPersonalityOverride": true,
		"beaconArmyPercentage": 60,
		"beaconArtilleryPercentage": 40,
		"beaconVtolPercentage": 60,
	},
	AB:
	{
		"primaryWeapon": _WEAPON_STATS.rockets_AT,
		"secondaryWeapon": _WEAPON_STATS.gauss,
		"artillery": _WEAPON_STATS.rockets_Arty,
		"antiAir": _WEAPON_STATS.rockets_AA,
		"factoryOrder": [_STRUCTURES.factory, _STRUCTURES.cyborgFactory, _STRUCTURES.vtolFactory, ],
		"defensePriority": 20,
		"vtolPriority": 10,
		"alloyPriority": 25,
		"useLasers": true,
		"resPath": "offensive",
		"retreatScanRange": 12,
		"cyborgThreatPercentage": 0.10,
		"res": [
			"R-Wpn-Rocket02-MRL",
			"R-Wpn-Rocket-ROF01",
			"R-Wpn-Rocket-Damage02",
		],
		"canPlayBySelf": true,
		"allowAutomaticPersonalityOverride": true,
		"beaconArmyPercentage": 50,
		"beaconArtilleryPercentage": 40,
		"beaconVtolPercentage": 50,
	},
	AM:
	{
		"primaryWeapon": _WEAPON_STATS.machineguns,
		"secondaryWeapon": _WEAPON_STATS.lasers,
		"artillery": _WEAPON_STATS.mortars,
		"antiAir": _WEAPON_STATS.AA,
		"factoryOrder": [_STRUCTURES.factory, _STRUCTURES.cyborgFactory, _STRUCTURES.vtolFactory],
		"defensePriority": 15,
		"vtolPriority": 80,
		"alloyPriority": 35,
		"useLasers": true,
		"resPath": "generic",
		"retreatScanRange": 12,
		"cyborgThreatPercentage": 100,
		"res": [
			"R-Wpn-MG2Mk1",
		],
		"canPlayBySelf": true,
		"allowAutomaticPersonalityOverride": true,
		"beaconArmyPercentage": 50,
		"beaconArtilleryPercentage": 50,
		"beaconVtolPercentage": 40,
	},
	AA:
	{
		"primaryWeapon": _WEAPON_STATS.mortars,
		"secondaryWeapon": _WEAPON_STATS.AS,
		"artillery": _WEAPON_STATS.fireMortars,
		"antiAir": _WEAPON_STATS.cannons_AA,
		"factoryOrder": [_STRUCTURES.factory, _STRUCTURES.cyborgFactory, _STRUCTURES.vtolFactory],
		"defensePriority": 45,
		"vtolPriority": 66,
		"alloyPriority": 10,
		"useLasers": true,
		"resPath": "offensive",
		"retreatScanRange": 12,
		"cyborgThreatPercentage": 0.15,
		"res": [
			"R-Wpn-Mortar02Hvy",
			"R-Wpn-Mortar-ROF02",
			"R-Wpn-Mortar-Acc01",
			"R-Wpn-Mortar-Damage03",
		],
		"canPlayBySelf": (baseType >= CAMP_WALLS),
		"allowAutomaticPersonalityOverride": true,
		"beaconArmyPercentage": 40,
		"beaconArtilleryPercentage": 40,
		"beaconVtolPercentage": 70,
	},
	AV:
	{
		"primaryWeapon":_WEAPON_STATS.cannons,
		"secondaryWeapon": _WEAPON_STATS.gauss,
		"artillery": _WEAPON_STATS.fireMortars,
		"antiAir": _WEAPON_STATS.cannons_AA,
		"factoryOrder": [_STRUCTURES.vtolFactory, _STRUCTURES.factory, _STRUCTURES.cyborgFactory],
		"defensePriority": 100,
		"vtolPriority": 100,
		"alloyPriority": 10,
		"useLasers": true,
		"resPath": "air",
		"retreatScanRange": 9,
		"cyborgThreatPercentage": 0.1,
		"res": [
			"R-Wpn-MG2Mk1",
			"R-Wpn-Bomb02",
			"R-Struc-VTOLPad-Upgrade01",
		],
		"canPlayBySelf": (baseType >= CAMP_BASE),
		"allowAutomaticPersonalityOverride": false,
		"beaconArmyPercentage": 80,
		"beaconArtilleryPercentage": 80,
		"beaconVtolPercentage": 60,
	},
};

// Groups
var attackGroup;
var vtolGroup;
var sensorGroup;
var artilleryGroup;
var constructGroup;
var constructGroupNTWExtra;
var oilGrabberGroup;
var retreatGroup;

var grudgeCount; //See who bullies this bot the most and act on it. DO NOT let this use the scavenger player number.
var personality; //What personality is this instance of Cobra using.
var lastMsg; //The last Cobra chat message.
var lastMsgThrottle; //Last game time a chat messge was sent - throttles Cobra AIs from talking to eachother too much.
var forceHover; //Use hover propulsion only.
var seaMapWithLandEnemy; //Hover map with an enemy sharing land with Cobra.
var turnOffCyborgs; //Turn of cyborgs (hover maps/chat).
var researchComplete; //Check if done with research.
var turnOffMG; //This is only used for when the personalities don't have their weapons researched.
var useArti;
var useVtol;
var lastAttackedByScavs;
var currently_dead; // Used to detect if Cobra is, basically, dead. If true, the script is put in a very low perf impact state.
var beacon; //latest friendly beacon location
var enemyUsedElectronicWarfare; //Detect if an enemy used a Nexus Link against us... if so, research the resistance upgrades
var startAttacking;
var lastShuffleTime;
var forceDerrickBuildDefense;
var randomResearchLabStart;
var cyborgOnlyGame;
var resObj; //Object that holds some game objects and data releated to research choices.
var noBasesHighTechStart; //For those T2+ no bases matches.
var weirdMapBaseDesign; //Attempt to find if we are on maps like 4p-cockpit which have almost no base structures in the base.

// -- Weapon research list (initializeResearchLists).
var techlist;
var weaponTech;
var laserTech;
var artilleryTech;
var artillExtra;
var laserExtra;
var extraTech;
var cyborgWeaps;
var antiAirTech;
var antiAirExtras;
var extremeLaserTech;
var extremeLaserExtra;
var secondaryWeaponTech;
var secondaryWeaponExtra;
var defenseTech;
var standardDefenseTech;
var machinegunWeaponTech;
var machinegunWeaponExtra;
var empWeapons;


//Now include everthing else.
include(__COBRA_INCLUDES + "performance.js");
include(__COBRA_INCLUDES + "miscFunctions.js");
include(__COBRA_INCLUDES + "build.js");
include(__COBRA_INCLUDES + "production.js");
include(__COBRA_INCLUDES + "tactics.js");
include(__COBRA_INCLUDES + "mapDynamics.js");
include(__COBRA_INCLUDES + "research.js");
include(__COBRA_INCLUDES + "events.js");
include(__COBRA_INCLUDES + "chat.js");
include(__COBRA_INCLUDES + "adaption.js");

const __MIN_TRUCKS_PER_GROUP = !highOilMap() ? 3 : 5;
