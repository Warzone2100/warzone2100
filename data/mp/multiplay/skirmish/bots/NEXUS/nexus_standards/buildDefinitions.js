
const BASE_STRUCTURES = {
	factories: [ "A0LightFactory", ],
	templateFactories: [ "A0CyborgFactory", ],
	vtolFactories: [ "A0VTolFactory1", ],
	labs: [ "A0ResearchFacility", ],
	gens: [ "A0PowerGenerator", ],
	hqs: [ "A0CommandCentre", ],
	vtolPads: [ "A0VtolPad", ],
	derricks: [ "A0ResourceExtractor", ],
	uplinks: [ "A0Sat-linkCentre", ],
	repairBays: [ "A0RepairCentre3", ],
	lassats: [ "A0LasSatCommand", ],
	sensors: [ "Sys-SensoTower02", ],
};

const FEATURE_STATS = {
	oils: [ "OilResource", ],
};

const STANDARD_TARGET_WEIGHTS = [
	{stat: FACTORY, value: 10,},
	{stat: CYBORG_FACTORY, value: 10,},
	{stat: VTOL_FACTORY, value: 10,},
	{stat: RESOURCE_EXTRACTOR, value: 9,},
	{stat: HQ, value: 8,},
	{stat: RESEARCH_LAB, value: 7,},
	{stat: POWER_GEN, value: 7,},
	{stat: LASSAT, value: 6,},
	{stat: SAT_UPLINK, value: 6,},
	{stat: REARM_PAD, value: 5,},
	{stat: REPAIR_FACILITY, value: 5,},
];

const STANDARD_MORTAR_DEFENSES = [
	"Emplacement-MortarPit01",
	"Emplacement-MortarPit02",
	"Emplacement-RotMor",
	"Emplacement-MortarPit-Incendiary",
];

const STANDARD_HOWITZER_DEFENSES = [
	"Emplacement-Howitzer105",
	"Emplacement-Howitzer150",
	"Emplacement-RotHow",
	"Emplacement-Howitzer-Incendiary",
];

const STANDARD_INCENDIARIES = [
	"Emplacement-MortarPit01",
	"Emplacement-MortarPit02",
	"Emplacement-MortarPit-Incendiary",
	"Emplacement-RotMor",
	"Emplacement-Howitzer105",
	"Emplacement-Howitzer150",
	"Emplacement-Howitzer-Incendiary",
	"Emplacement-RotHow",
	"Emplacement-HeavyPlasmaLauncher",
];

const STANDARD_ANTI_AIR_DEFENSES = [
	"AASite-QuadMg1",
	"AASite-QuadBof",
	"AASite-QuadRotMg",
	"P0-AASite-SAM1",
	"P0-AASite-Laser",
	"P0-AASite-SAM2",
];

//Always builds one of these structures in base.
const STANDARD_BUILD_FUNDAMENTALS = [
	"A0LightFactory",
	"A0ResearchFacility",
	"A0PowerGenerator",
	"A0CommandCentre",
	"A0CyborgFactory",
	"A0RepairCentre3",
	"Sys-CB-Tower01",
	"Sys-SpyTower",
	"A0LasSatCommand",
	"A0Sat-linkCentre",
	"X-Super-Rocket",
	"X-Super-Cannon",
	"X-Super-MassDriver",
	"X-Super-Missile",
];

//NOTE: Nexus estimates the "bounds" of an ally base by enumerating all of these. Kind of like "territory".
const STANDARD_BASE_STRUCTURES = [
	"A0PowerGenerator",
	"A0LightFactory",
	"A0CommandCentre",
	"A0ResearchFacility",
	"A0CyborgFactory",
	"A0LasSatCommand",
	"A0Sat-linkCentre",
	"A0VTolFactory1",
];

// NOTE: Apparently, these are randomly selected to an extend.
const STANDARD_BASIC_DEFENSES = [
	"PillBox1",
	"Pillbox-RotMG",
	"GuardTower6",
	"PillBox4",
	"GuardTower5",
	"Sys-SensoTower02",
	"WallTower03",
	"WallTower04",
	"GuardTower5",
	"WallTower04",
	"WallTower-HPVcannon",
	"Emplacement-Howitzer105",
	"Emplacement-HvyATrocket",
	"Emplacement-MortarPit02",
	"WallTower06",
	"Emplacement-PulseLaser",
	"Emplacement-Rail2",
	"WallTower-HvATrocket",
	"WallTower-PulseLas",
	"WallTower-Rail2",
	"WallTower-Atmiss",
	"Emplacement-HeavyLaser",
	"WallTower-Rail3"
];

// NOTE: build on the ourskirt of the base "territory"
const STANDARD_FORTIFY_DEFENSES = [
	"WallTower01",
	"WallTower02",
	"WallTower03",
	"WallTower04",
	"WallTower06",
	"WallTower-HPVcannon",
	"WallTower-PulseLas",
	"WallTower-Atmiss",
	"WallTower-Rail2",
];

//NOTE: if any of these are destroyed, requeue them for a rebuilt at that spot.
const STANDARD_REBUILD_STRUCTURES = [
	{type: WALL, stat: "A0HardcreteMk1Wall", name: "Hardcrete Wall"},
	{type: WALL, stat: "A0HardcreteMk1CWall", name: "Hardcrete Corner Wall"},
];
