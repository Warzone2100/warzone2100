// CHOICES ARE MADE FROM TOP TO BOTTOM !!!

const BUILDER_BODY_LIST = [
	"Body3MBT", // Retaliation
	"Body2SUP", // Leopard
	"Body4ABT", // Bug
	"Body1REC", // viper
];

const BUILDER_PROP_LIST = [
	"hover01", // Hover
	"wheeled01", // Wheels
];

const SCANNER_BODY_LIST = [
	"Body7ABT", // Retribution
	"Body3MBT",  // Retaliation
	"Body2SUP", // Leopard
	"Body4ABT", // Bug
	"Body1REC", // viper
];

// ========================
// TANK BODY SECTION
// ========================
const TANK_BODY_LIST = [
	"Body14SUP", // dragon
	"Body10MBT", // vengeance
	"Body9REC", // Tiger
	"Body11ABT", // python
	"Body5REC", // cobra
	"Body1REC", // viper
];

// ========================
// TANK MEDIUM BODY SECTION
// ========================
const TANK_MEDIUM_BODY_LIST = [
	"Body7ABT", // Retribution
	"Body6SUPP", // Panther
	"Body8MBT", // Scorpion
	"Body5REC", // Cobra
];

const TANK_BODY_LIST_THERMIC = [
	"Body14SUP", // Dragon
	"Body13SUP", // Wyvern
	"Body7ABT", // Retribution
	"Body9REC", // Tiger
	"Body12SUP", // Mantis
	"Body8MBT", // Scorpion
	"Body4ABT", // Bug
	"Body1REC", // Viper
];

// ========================
// TANK PROP SECTION
// ========================
const TANK_PROP_LIST = [
	"tracked01", // track
	"HalfTrack", // half-track
	"wheeled01" // wheels (early T1 fallback before Half-Tracks are researched)
];

// ========================
// TANK HOWITZERS SECTION
// ========================
const HOWITZERS_WEAPON_LIST = [
	"Howitzer150Mk1", // Ground Shaker
	"Howitzer03-Rot", // Hellstorm
	"Howitzer-Incendiary", // Howitzer Incendiary
	"Howitzer105Mk1" // Howitzer
];

// ========================
// TANK ROCKET SECTION
// ========================
const ROCKET_WEAPON_LIST = [
	"Missile-A-T", // Scourge
	"Rocket-HvyA-T", // Tank-killer
	"Rocket-LtA-T", // Lancer
	"Rocket-Pod" // Rocket Pod
];

// ========================
// TANK ROCKET ARRAY SECTION (Anti-Personnel / Splash Support sharing Rocket upgrades)
// ========================
const ROCKET_ARRAY_WEAPON_LIST = [
	"Missile-MdArt", // Seraph Missile Array
	"Rocket-MRL-Hvy", // Heavy Rocket Array
	"Rocket-MRL" // Mini-Rocket Array
];

// ========================
// TANK BUNKER BUSTER
// ========================
const BUNKER_BUSTER_WEAPON = [
	"Rocket-BB",
];

// ========================
// TANK ANTI-AIR SECTION
// ========================
const AA_WEAPON_LIST = [
	"AAGunLaser",      // Stormbringer AA Laser
	"Missile-HvySAM",  // Vindicator SAM
	"Missile-LtSAM",   // Avenger SAM
	"QuadRotAAGun",    // Whirlwind AA Turret
	"AAGun2Mk1Quad",   // Quad AA Flak Cannon
	"Rocket-Sunburst", // Sunburst AA Rocket Array
	"AAGun2Mk1",       // AA Flak Cannon
	"QuadMg1AAGun",    // Hurricane AA Turret
];

// True for a ground tank carrying a dedicated anti-air turret
function isAntiAirDroid(droid) {
	if (!droid || droid.droidType !== DROID_WEAPON || droid.isVTOL === true) {
		return false;
	}
	const weaponName = (droid.weapons && droid.weapons[0]) ? droid.weapons[0].name : "";
	return AA_WEAPON_LIST.includes(weaponName);
}

// Share (%) of anti-personnel tanks that carry Machineguns once Rocket Arrays are also available
const MACHINEGUN_SHARE_OF_AP = 50;

// ========================
// TANK MACHINE GUN SECTION
// ========================
const MACHINEGUN_WEAPON_LIST = [
	"HeavyLaser", // Heavy laser
	"Laser2PULSEMk1", // Pulse laser
	"MG5TWINROTARY", // Twin assault gun
	"MG4ROTARYMk1", // Single assault gun
	"MG3Mk1", // Heavy machine gun
	"MG2Mk1", // Twin machine gun
	"MG1Mk1" // Machine gun (early T1 fallback)
];

// ========================
// TANK FLAME SECTION
// ========================
const FLAME_WEAPON_LIST = [
	"PlasmaHeavy", // Heavy plasma launcher
	"PlasmiteFlamer", // Plasmite
	"Flame2", // Inferno
	"Flame1Mk1", // Flamer
];

// ========================
// CYBORG ROCKETS SECTION
// ========================
const CYBORG_HEAVY_ROCKETS = [
	"Cyb-Hvywpn-A-T", // Super Cyborg scourge
	"Cyb-Hvywpn-TK" // Super Cyborg tank killer
];

const CYBORG_LIGHT_ROCKETS = [
	"Cyb-Wpn-Atmiss", // Cyborg scourge
	"CyborgRocket" // Cyborg lancer
];

// ========================
// CYBORG ROCKETS SECTION
// ========================
const CYBORG_HEAVY_LASER = [
	"Cyb-Hvywpn-PulseLsr" // Pulse laser
];

const CYBORG__LIGHT_MACHINEGUN = [
	"CyborgRotMG", // Assault Gunner (Heavy machinegun)
	"CyborgChaingun" // Machinegunner
];

// ========================
// CYBORG FLAMERS SECTION
// ========================
const CYBORG_FLAMERS = [
	"Cyb-Wpn-Thermite", // Cyborg thermite
	"CyborgFlamer01", // Cyborg flamer
];

// ========================
// VTOL BOMBS SECTION
// ========================
const VTOL_BODY_LIST = [
	"Body7ABT", // Retribution
	"Body10MBT", // Vengeance
	"Body9REC", // Tiger
	"Body12SUP", // Mantis
	"Body8MBT", // Scorpion
	"Body11ABT", // Python
	"Body3MBT", // Retaliation
	"Body5REC", // Cobra
	"Body4ABT", // Bug
	"Body1REC", // Viper
];

const VTOL_ROCKET_WEAPONS = [
	"Missile-VTOL-AT", // Scourge VTOL
	"Rocket-VTOL-HvyA-T", // Tank-Killer VTOL
	"Rocket-VTOL-LtA-T", // Lancer VTOL
	"Rocket-VTOL-Pod", // Mini-Rocket Pod VTOL
];

const VTOL_BOMB_WEAPONS = [
	"Bomb5-VTOL-Plasmite", // Plasmite Bomb Bay
	"Bomb4-VTOL-HvyINC", // Thermite Bomb Bay
	"Bomb2-VTOL-HvHE", // HEAP Bomb Bay
];

const VTOL_WEAPONS = VTOL_ROCKET_WEAPONS.concat(VTOL_BOMB_WEAPONS);
