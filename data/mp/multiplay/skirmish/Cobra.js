//globals/constants/definitions and whatever. Includes at the bottom.
const COBRA_INCLUDES = "/multiplay/skirmish/cobra_includes/";
const COBRA_RULESETS = "/multiplay/skirmish/cobra_rulesets/";

//Rulesets here.
include(COBRA_RULESETS + "CobraStandard.js");

const LOG_RESEARCH_PATH = false;

const MIN_ATTACK_DROIDS = 4;
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
const BEACON_VTOL_ALARM = "vtolSpotted";

//Research constants
const TANK_ARMOR = [
	"R-Vehicle-Metals09",
	"R-Vehicle-Armor-Heat09",
];
const CYBORG_ARMOR = [
	"R-Cyborg-Metals09",
	"R-Cyborg-Armor-Heat09",
];
const MODULE_RESEARCH = [
	"R-Struc-Research-Module",
	"R-Struc-Factory-Module",
	"R-Struc-PowerModuleMk1",
];
const MOST_ESSENTIAL = [
	"R-Wpn-MG-Damage01",
	"R-Sys-Engineering01",
	"R-Defense-Tower01",
];
const ESSENTIALS = [
	"R-Wpn-MG2Mk1",
	"R-Wpn-MG-Damage02",
	"R-Struc-PowerModuleMk1",
	"R-Struc-Research-Upgrade01",
	"R-Vehicle-Prop-Halftracks",
	"R-Vehicle-Body05",
	"R-Wpn-MG-Damage03",
];
const ESSENTIALS_2 = [
	"R-Vehicle-Metals02",
	"R-Cyborg-Metals02",
	"R-Wpn-MG3Mk1",
	"R-Struc-RprFac-Upgrade01",
	"R-Sys-Autorepair-General",
	"R-Struc-Power-Upgrade03a",
	"R-Struc-Research-Upgrade09",
];
const ESSENTIALS_3 = [
	"R-Vehicle-Body04",
	"R-Vehicle-Prop-Hover",
	"R-Struc-Factory-Upgrade09",
];
const SYSTEM_UPGRADES = [
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
	"R-Wpn-LasSat",
	"R-Sys-Resistance-Circuits",
];
const DEFENSE_UPGRADES = [
	"R-Struc-Materials09",
	"R-Defense-WallUpgrade12",
];
const BODY_RESEARCH = [
	"R-Vehicle-Body08",
	"R-Vehicle-Body12",
	"R-Vehicle-Body09",
	"R-Vehicle-Body10",
	"R-Vehicle-Engine09",
	"R-Vehicle-Body14",
];
const VTOL_RES = [
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
const TANK_BODY = [
	"Body14SUP", // Dragon
	"Body13SUP", // Wyvern
	"Body10MBT", // Vengeance
	"Body9REC",  // Tiger
	"Body11ABT", // Python
	"Body5REC",  // Cobra
	"Body1REC",  // Viper
];
const SYSTEM_BODY = [
	"Body3MBT",  // Retaliation
	"Body2SUP",  // Leopard
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
	"Body12SUP", // Mantis
	"Body8MBT",  // Scorpion
	"Body5REC",  // Cobra
	"Body1REC",  // Viper
];
const ARTILLERY_SENSORS = [
	"Sensor-WideSpec",
	"SensorTurret1Mk1",
];


//List of Cobra personalities:
var subPersonalities =
{
	AC:
	{
		"primaryWeapon": weaponStats.cannons,
		"secondaryWeapon": weaponStats.gauss,
		"artillery": weaponStats.mortars,
		"antiAir": weaponStats.cannons_AA,
		"factoryOrder": [structures.factory, structures.cyborgFactory, structures.vtolFactory],
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
		"beaconArmyPercentage": 60,
		"beaconArtilleryPercentage": 40,
		"beaconVtolPercentage": 60,
	},
	AR:
	{
		"primaryWeapon": weaponStats.flamers,
		"secondaryWeapon": weaponStats.gauss,
		"artillery": weaponStats.mortars,
		"antiAir": weaponStats.AA,
		"factoryOrder": [structures.factory, structures.cyborgFactory, structures.vtolFactory],
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
		"beaconArmyPercentage": 60,
		"beaconArtilleryPercentage": 40,
		"beaconVtolPercentage": 60,
	},
	AB:
	{
		"primaryWeapon": weaponStats.rockets_AT,
		"secondaryWeapon": weaponStats.gauss,
		"artillery": weaponStats.rockets_Arty,
		"antiAir": weaponStats.rockets_AA,
		"factoryOrder": [structures.vtolFactory, structures.factory, structures.cyborgFactory],
		"defensePriority": 20,
		"vtolPriority": 50,
		"alloyPriority": 25,
		"useLasers": true,
		"resPath": "offensive",
		"retreatScanRange": 12,
		"cyborgThreatPercentage": 0.10,
		"res": [
			"R-Wpn-Rocket02-MRL",
		],
		"canPlayBySelf": true,
		"beaconArmyPercentage": 50,
		"beaconArtilleryPercentage": 40,
		"beaconVtolPercentage": 50,
	},
	AM:
	{
		"primaryWeapon": weaponStats.machineguns,
		"secondaryWeapon": weaponStats.lasers,
		"artillery": weaponStats.mortars,
		"antiAir": weaponStats.AA,
		"factoryOrder": [structures.factory, structures.cyborgFactory, structures.vtolFactory],
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
		"beaconArmyPercentage": 50,
		"beaconArtilleryPercentage": 50,
		"beaconVtolPercentage": 40,
	},
	AA:
	{
		"primaryWeapon": weaponStats.mortars,
		"secondaryWeapon": weaponStats.AS,
		"artillery": weaponStats.fireMortars,
		"antiAir": weaponStats.cannons_AA,
		"factoryOrder": [structures.factory, structures.cyborgFactory, structures.vtolFactory],
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
		"canPlayBySelf": false,
		"beaconArmyPercentage": 40,
		"beaconArtilleryPercentage": 40,
		"beaconVtolPercentage": 70,
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

const MIN_TRUCKS_PER_GROUP = !highOilMap() ? 3 : 5;
