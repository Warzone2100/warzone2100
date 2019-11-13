//globals/constants/definitions and whatever. Includes at the bottom.
const COBRA_INCLUDES = "/multiplay/skirmish/cobra_includes/";
const COBRA_RULESETS = "/multiplay/skirmish/cobra_rulesets/";

//Rulesets here.
include(COBRA_RULESETS + "CobraStandard.js");

const MAX_GRUDGE = 50000;
const MIN_ATTACK_DROIDS = 4;
const FACTORY = "A0LightFactory";
const CYBORG_FACTORY = "A0CyborgFactory";
const VTOL_FACTORY = "A0VTolFactory1";
const MY_BASE = startPositions[me];
const OIL_RES = "OilResource";
const MIN_POWER = 150;
const SUPER_LOW_POWER = 70;
const MIN_BUILD_POWER = 220;
const PRODUCTION_POWER = SUPER_LOW_POWER;
const ELECTRONIC_DEFENSES = [
	"Sys-SpyTower",
	"WallTower-EMP",
	"Emplacement-MortarEMP",
];

//Research constants
const TANK_ARMOR = [
	"R-Vehicle-Metals09",
	"R-Vehicle-Armor-Heat09",
];
const CYBORG_ARMOR = [
	"R-Cyborg-Metals09",
	"R-Cyborg-Armor-Heat09",
];
const ESSENTIALS = [
	"R-Wpn-MG-Damage01",
	"R-Sys-Engineering01",
	"R-Defense-Tower01",
	"R-Wpn-MG2Mk1",
	"R-Wpn-MG-Damage02",
	"R-Struc-PowerModuleMk1",
	"R-Struc-Research-Upgrade01",
	"R-Vehicle-Metals01",
	"R-Cyborg-Metals01",
];
const ESSENTIALS_2 = [
	"R-Vehicle-Prop-Halftracks",
	"R-Struc-RprFac-Upgrade01",
	"R-Vehicle-Body11",
	"R-Sys-Sensor-Upgrade01",
	"R-Vehicle-Prop-Hover",
	"R-Struc-Factory-Upgrade09",
];
const ESSENTIALS_3 = [
	"R-Struc-Power-Upgrade03a",
	"R-Struc-Research-Upgrade09",
	"R-Sys-Autorepair-General",
];
const SYSTEM_UPGRADES = [
	"R-Sys-MobileRepairTurretHvy",
	"R-Vehicle-Prop-Tracks",
	"R-Struc-RprFac-Upgrade06",
	"R-Sys-Sensor-Upgrade03",
];
const FLAMER = [
	"R-Wpn-Flame2",
	"R-Wpn-Flamer-ROF03",
	"R-Wpn-Flamer-Damage09",
];
const SENSOR_TECH = [
	"R-Sys-CBSensor-Tower01",
	"R-Sys-Sensor-WSTower",
	"R-Sys-Sensor-UpLink",
];
const DEFENSE_UPGRADES = [
	"R-Vehicle-Metals05",
	"R-Cyborg-Metals05",
	"R-Sys-Resistance-Circuits",
	"R-Wpn-LasSat",
	"R-Struc-Materials09",
	"R-Defense-WallUpgrade12",
];
const BODY_RESEARCH_1 = [
	"R-Vehicle-Body12",
	"R-Vehicle-Body09",
];
const BODY_RESEARCH_2 = [
	"R-Vehicle-Body10",
	"R-Vehicle-Engine09",
	"R-Vehicle-Body14",
];
const VTOL_ESSENTIALS = [
	"R-Struc-VTOLPad-Upgrade01",
	"R-Wpn-Bomb01",
];
const VTOL_RES = [
	"R-Struc-VTOLPad-Upgrade06",
	"R-Wpn-Bomb02",
	"R-Wpn-Bomb-Accuracy03",
	"R-Wpn-Bomb05",
	"R-Wpn-Bomb06",
];

//Production constants
const TANK_BODY = [
	"Body14SUP", // Dragon
	"Body13SUP", // Wyvern
	"Body10MBT", // Vengeance
	"Body7ABT",  // Retribution
	"Body9REC",  // Tiger
	"Body12SUP", // Mantis
	"Body6SUPP", // Panther
	"Body11ABT", // Python
	"Body8MBT",  // Scorpion
	"Body5REC",  // Cobra
	"Body1REC",  // Viper
];
const SYSTEM_BODY = [
	"Body3MBT",  // Retaliation
	"Body4ABT",  // Bug
	"Body1REC",  // Viper
];
const SYSTEM_PROPULSION = [
	"hover01", // hover
	"wheeled01", // wheels
];
const VTOL_BODY = [
	"Body7ABT",  // Retribution
	"Body6SUPP", // Panther
	"Body8MBT",  // Scorpion
	"Body5REC",  // Cobra
	"Body1REC",  // Viper
];
const REPAIR_TURRETS = [
	"HeavyRepair",
	"LightRepair1",
];


//List of Cobra personalities:
var subPersonalities =
{
	AC:
	{
		"primaryWeapon": weaponStats.cannons,
		"secondaryWeapon": weaponStats.gauss,
		"artillery": weaponStats.howitzers,
		"antiAir": weaponStats.AA,
		"factoryOrder": [FACTORY, CYBORG_FACTORY, VTOL_FACTORY],
		"defensePriority": 30,
		"vtolPriority": 20,
		"alloyPriority": 20,
		"useLasers": true,
		"cyborgThreatPercentage": 0.20,
		"resPath": "generic",
		"res": [
			"R-Wpn-Cannon-Damage02",
			"R-Wpn-Cannon-ROF02",
		],
	},
	AR:
	{
		"primaryWeapon": weaponStats.flamers,
		"secondaryWeapon": weaponStats.gauss,
		"artillery": weaponStats.mortars,
		"antiAir": weaponStats.AA,
		"factoryOrder": [FACTORY, CYBORG_FACTORY, VTOL_FACTORY],
		"defensePriority": 20,
		"vtolPriority": 40,
		"alloyPriority": 25,
		"useLasers": true,
		"cyborgThreatPercentage": 0.40,
		"resPath": "generic",
		"res": [
			"R-Wpn-Flamer-Damage03",
			"R-Wpn-Flamer-ROF01",
		],
	},
	AB:
	{
		"primaryWeapon": weaponStats.rockets_AT,
		"secondaryWeapon": weaponStats.gauss,
		"artillery": weaponStats.rockets_Arty,
		"antiAir": weaponStats.rockets_AA,
		"factoryOrder": [VTOL_FACTORY, FACTORY, CYBORG_FACTORY],
		"defensePriority": 10,
		"vtolPriority": 50,
		"alloyPriority": 10,
		"useLasers": true,
		"resPath": "offensive",
		"cyborgThreatPercentage": 0.10,
		"res": [
			"R-Wpn-Rocket02-MRL",
		],
	},
	AM:
	{
		"primaryWeapon": weaponStats.machineguns,
		"secondaryWeapon": weaponStats.machineguns,
		"artillery": weaponStats.mortars,
		"antiAir": weaponStats.AA,
		"factoryOrder": [FACTORY, CYBORG_FACTORY, VTOL_FACTORY],
		"defensePriority": 50,
		"vtolPriority": 40,
		"alloyPriority": 15,
		"useLasers": true,
		"resPath": "generic",
		"cyborgThreatPercentage": 100,
		"res": [
			"R-Wpn-MG2Mk1",
		],
	},
	AA:
	{
		"primaryWeapon": weaponStats.mortars,
		"secondaryWeapon": weaponStats.AS,
		"artillery": weaponStats.fireMortars,
		"antiAir": weaponStats.AA,
		"factoryOrder": [FACTORY, CYBORG_FACTORY, VTOL_FACTORY],
		"defensePriority": 70,
		"vtolPriority": 15,
		"alloyPriority": 15,
		"useLasers": true,
		"resPath": "offensive",
		"cyborgThreatPercentage": 0.30,
		"res": [
			"R-Wpn-Mortar02Hvy",
			"R-Wpn-Mortar-ROF02",
			"R-Wpn-Mortar-Acc01",
			"R-Wpn-Mortar-Damage03",
		],
	},
	AL:
	{
		"primaryWeapon": weaponStats.lasers,
		"secondaryWeapon": weaponStats.gauss,
		"artillery": weaponStats.fireMortars,
		"antiAir": weaponStats.AA,
		"factoryOrder": [VTOL_FACTORY, FACTORY, CYBORG_FACTORY],
		"defensePriority": 10,
		"vtolPriority": 100,
		"alloyPriority": 10,
		"useLasers": true,
		"cyborgThreatPercentage": 0.10,
		"resPath": "air",
		"res": [
			"R-Wpn-Laser01",
			"R-Wpn-Mortar-Incenediary",
		],
	},
};

// Groups
var attackGroup;
var vtolGroup;
var sensorGroup;
var repairGroup;
var artilleryGroup;
var constructGroup;
var oilGrabberGroup;
var retreatGroup;

var grudgeCount; //See who bullies this bot the most and act on it. DO NOT let this use the scavenger player number.
var personality; //What personality is this instance of Cobra using.
var lastMsg; //The last Cobra chat message.
var forceHover; //Use hover propulsion only.
var seaMapWithLandEnemy; //Hover map with an enemy sharing land with Cobra.
var turnOffCyborgs; //Turn of cyborgs (hover maps/chat).
var researchComplete; //Check if done with research.
var turnOffMG; //This is only used for when the personalities don't have their weapons researched.
var useArti;
var useVtol;
var lastAttackedByScavs;
var prevResPath; // Previous personality research path. Volatile.
var currently_dead; // Used to detect if Cobra is, basically, dead. If true, the script is put in a very low perf impact state.

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
var secondaryWeaponTech;
var secondaryWeaponExtra;
var defenseTech;
var standardDefenseTech;
var machinegunWeaponTech;
var machinegunWeaponExtra;
var empWeapons;


//Now include everthing else.
include(COBRA_INCLUDES + "performance.js");
include(COBRA_INCLUDES + "miscFunctions.js");
include(COBRA_INCLUDES + "build.js");
include(COBRA_INCLUDES + "production.js");
include(COBRA_INCLUDES + "tactics.js");
include(COBRA_INCLUDES + "mapDynamics.js");
include(COBRA_INCLUDES + "research.js");
include(COBRA_INCLUDES + "events.js");
include(COBRA_INCLUDES + "chat.js");
include(COBRA_INCLUDES + "adaption.js");

const MIN_TRUCKS = mapOilLevel() !== "NTW" ? 6 : 10;
