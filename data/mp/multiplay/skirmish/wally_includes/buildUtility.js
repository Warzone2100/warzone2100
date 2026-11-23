// Every list is ordered best first; the first AVAILABLE entry is built.
// Entries marked (shared tech only) belong to weapon lines Wally does not research (cannons, rail guns, flamers,
// mortars, EMP): they are used only if an ally shares the technology or the game starts with it.

const EMP_DEFENSES = [
	"Emplacement-MortarEMP", // EMP Mortar Pit (shared tech only)
	"WallTower-EMP", // EMP Cannon Tower (shared tech only)
];

const ARTILLERY_DEFENSES = [
	"Emplacement-HvART-pit", // Archangel Missile Battery
	"Emplacement-HeavyPlasmaLauncher", // Heavy Plasma Launcher (shared tech only)
	"Emplacement-Rocket06-IDF", // Ripple Rocket Battery
];

const FORTRESS_DEFENSES = [
	"X-Super-Missile", // Missile Fortress
	"X-Super-Rocket", // Rocket Bastion
	"X-Super-MassDriver", // Mass Driver Fortress (shared tech only)
	"X-Super-Cannon", // Cannon Fortress (shared tech only)
];

const SENSORS_TOWERS = [
	"Sys-SensoTowerWS", // Wide Spectrum Sensor Tower
	"Sys-SensoTower02", // Hardened Sensor Tower
];

const WEAK_DEFENSES = [
	"GuardTower6", // Mini-Rocket Tower
	"PillBox1", // Heavy Machinegun Bunker
	"GuardTower1", // Machinegun Guard Tower (early T1 fallback)
];

// Mid-game mix of the two weapon lines: anti-tank rockets, machineguns against cyborgs, rocket splash
const MEDIUM_DEFENSES = [
	"WallTower06", // Lancer Hardpoint
	"Pillbox-RotMG", // Assault Gun Bunker
	"WallTower01", // Heavy Machinegun Hardpoint
	"Emplacement-MRL-pit", // Mini-Rocket Battery
];

const STRONG_DEFENSES = [
	"Emplacement-Rocket06-IDF", // Ripple Rocket Battery
	"Emplacement-HeavyPlasmaLauncher", // Heavy Plasma Launcher (shared tech only)
	"WallTower-Atmiss", // Scourge Missile Hardpoint
	"WallTower-HvATrocket", // Tank-Killer Hardpoint
	"WallTower-TwinAssaultGun", // Twin Assault Gun Hardpoint
	"WallTower06", // Lancer Hardpoint (fallback)
];

// Best first. Wally researches the Machinegun AA line (Hurricane -> Whirlwind) and the Rocket/Missile AA line
// (Sunburst -> Avenger -> Vindicator); the others are only used when an ally shares the technology.
const AA_DEFENSES = [
	"P0-AASite-SAM2", // Vindicator SAM Site
	"P0-AASite-SAM1", // Avenger SAM Site
	"P0-AASite-Laser", // Stormbringer Emplacement
	"AASite-QuadRotMg", // Whirlwind AA Site
	"P0-AASite-Sunburst", // Sunburst AA Site
	"AASite-QuadBof", // Cyclone AA Flak Site
	"AASite-QuadMg1" // Hurricane AA Site
];

// STRUCTURES.stattype
const structureTypes = [
	"HQ",
	"FACTORY",
	"POWER_GEN",
	"RESOURCE_EXTRACTOR",
	"LASSAT",
	"DEFENSE",
	"WALL",
	"RESEARCH_LAB",
	"REPAIR_FACILITY",
	"CYBORG_FACTORY",
	"VTOL_FACTORY",
	"REARM_PAD",
	"SAT_UPLINK",
	"GATE",
	"STRUCT_GENERIC",
	"COMMAND_CONTROL"
];
