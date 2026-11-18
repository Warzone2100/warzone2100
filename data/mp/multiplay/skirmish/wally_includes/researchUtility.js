// CHOICES ARE MADE FROM TOP TO BOTTOM

// ------------------ START ------------------


const START_COMPONENTS = [
	"R-Wpn-MG1Mk1", // Machinegun (unlocks immediate first tank turret & MG tower)
	"R-Sys-Engineering01", // Engineering (unlocks modules, repair facility & engine path)
	"R-Wpn-MG2Mk1", // Twin Machinegun
	"R-Vehicle-Engine01", // Fuel Injection Engine Mk1
	"R-Vehicle-Prop-Halftracks", // Half-tracked Propulsion (core tank propulsion)
	"R-Wpn-Rocket05-MiniPod", // Mini-Rocket Pod (unlocks rocket tanks & rocket towers)
	"R-Struc-Research-Module", // Research Module (accelerates all subsequent research)
	"R-Struc-PowerModuleMk1", // Power Module (boosts power generation)
	"R-Vehicle-Metals01", // Composite Alloys Mk1 (T1 armor, required for Cobra)
	"R-Vehicle-Body04", // Bug light body
	"R-Vehicle-Body05", // Cobra medium body (core T1/T2 tank body)
	"R-Struc-RepairFacility", // Repair Facility
	"R-Defense-Tower01", // Machinegun Guard Tower (earliest base defense)
	"R-Defense-Tower06", // Mini-Rocket Guard Tower
	"R-Defense-Pillbox01", // Heavy Machinegun Bunker
	"R-Struc-Factory-Cyborg", // Cyborg Factory
	"R-Struc-Factory-Module", // Factory Module
	"R-Vehicle-Prop-Hover", // Hover Propulsion
]

// ------------------ BODY ------------------

const FUNDAMENTALS_BODY_T1 = [
	"R-Vehicle-Body05", // Cobra
	"R-Vehicle-Body04", // Bug
	"R-Vehicle-Metals01", // T1 armor
	"R-Cyborg-Metals01", // T1 cyborg armor
	"R-Vehicle-Armor-Heat01", // T1 heat armor 
]

const FUNDAMENTALS_BODY_T2 = [
	"R-Vehicle-Body11", // Python
	"R-Vehicle-Body08", // Scorpion
	"R-Vehicle-Metals03", // T2 armor
	"R-Cyborg-Metals03", // T2 cyborg armor
	"R-Vehicle-Armor-Heat03", // T2 heat armor
]

const FUNDAMENTALS_BODY_T3 = [
	"R-Vehicle-Body09", // Tiger
	"R-Vehicle-Body12", // Mantis
	"R-Vehicle-Body03", // Retaliation
	"R-Vehicle-Body07", // Retribution
	"R-Vehicle-Metals06", // T3 armor
	"R-Cyborg-Metals06", // T3 cyborg armor
	"R-Vehicle-Armor-Heat06", // T3 heat armor 
]

const FUNDAMENTALS_BODY_T4 = [
	"R-Vehicle-Body10", // Vengeance
	"R-Vehicle-Metals09", // T4 armor
	"R-Cyborg-Metals09", // T4 cyborg armor
	"R-Vehicle-Armor-Heat09", // T4 heat armor 
	"R-Vehicle-Body13", // Wyvern
	"R-Vehicle-Body14", // Dragon
]

// ------------------ PROPULSION ------------------

const FUNDAMENTALS_PROPULSION_T1 = [
	"R-Vehicle-Prop-Halftracks", // Half Tracks
	"R-Vehicle-Engine01",
	"R-Vehicle-Engine02",
]

const FUNDAMENTALS_PROPULSION_T2 = [
	"R-Vehicle-Prop-Tracks", // Tracks
	"R-Vehicle-Prop-VTOL", // VTOL Propulsion
	"R-Vehicle-Engine03",
	"R-Vehicle-Engine04",
]

const FUNDAMENTALS_PROPULSION_T3 = [
	"R-Vehicle-Engine05",
	"R-Vehicle-Engine06",
]

const FUNDAMENTALS_PROPULSION_T4 = [
	"R-Vehicle-Engine07",
	"R-Vehicle-Engine08",
	"R-Vehicle-Engine09" // Engine upgrades
]

// ------------------ CANNONS ------------------

const FUNDAMENTALS_CANNON_WEAPON = [
	"R-Wpn-Cannon-Damage02", // T2 Damage upgrades
	"R-Wpn-Cannon2Mk1", // Medium Cannon
	"R-Wpn-Cannon-Accuracy01", // T1 Accuracy upgrades
	"R-Wpn-Cannon-Damage03", // T3 Damage upgrades
	"R-Wpn-Cannon4AMk1", // Hyper Velocity Cannon
	"R-Wpn-Cannon-Damage04", // T4 Damage upgrades
	"R-Wpn-Cannon-ROF01", // T1 ReloadTime upgrades
	"R-Wpn-Cannon-Accuracy02", // T2 Accuracy upgrades
	"R-Wpn-Cannon-Damage05", // T5 Damage upgrades
	"R-Wpn-Cannon5", // Assault Cannon
	"R-Cyborg-Hvywpn-Acannon", // Cyborg Assault Cannon
	"R-Wpn-Cannon-ROF02", // T2 ReloadTime upgrades
	"R-Wpn-Cannon-Damage06", // T6 Damage upgrades
	"R-Wpn-Cannon-ROF03", // T3 ReloadTime upgrades
	"R-Wpn-Cannon6TwinAslt", // Twin Assault Cannon
	"R-Wpn-Cannon-Damage07", // T7 Damage upgrades
	"R-Wpn-PlasmaCannon", // Plasma Cannon
	"R-Wpn-Cannon-ROF04", // T4 ReloadTime upgrades
	"R-Wpn-Cannon-Damage08", // T8 Damage upgrades
	"R-Wpn-Cannon-ROF05", // T5 ReloadTime upgrades
	"R-Wpn-Cannon-Damage09", // T9 Damage upgrades
	"R-Wpn-MortarEMP", // EMP Cannon
	"R-Wpn-Cannon-ROF06", // T6 ReloadTime upgrades
	"R-Cyborg-Hvywpn-RailGunner", // Cyborg Rail Gun
	"R-Wpn-RailGun03" // Gauss Cannon
]

// ------------------ MACHINEGUN ------------------

const FUNDAMENTALS_MACHINEGUN_WEAPON_T1 = [
	"R-Wpn-MG3Mk1", // Heavy Machine Gun
	"R-Wpn-MG-Damage01", // T1 Damage upgrades
	"R-Wpn-MG-ROF01", // T1 ReloadTime upgrades
]

const FUNDAMENTALS_MACHINEGUN_WEAPON_T2 = [
	"R-Wpn-MG4", // Cyborg Assault Gun
	"R-Wpn-MG-Damage05", // T2 Damage upgrades
	"R-Wpn-MG-ROF02", // T2 ReloadTime upgrades
]

const FUNDAMENTALS_MACHINEGUN_WEAPON_T3 = [
	"R-Wpn-MG5", // Twin Assault Gun
	"R-Wpn-Laser01", // Cyborg Flashlight
	"R-Wpn-MG-Damage08", // T3 Damage upgrades
	"R-Wpn-MG-ROF03", // T3 ReloadTime upgrades
]

const FUNDAMENTALS_MACHINEGUN_WEAPON_T4 = [
	"R-Wpn-MG-Damage10", // T4 Damage upgrades
	"R-Wpn-Laser02", // Pulse Laser
	"R-Cyborg-Hvywpn-PulseLsr", // Super Cyborg Pulse Laser
	"R-Wpn-Energy-Damage01", // T4 Laser Damage upgrades
	"R-Wpn-Energy-ROF01", // T4 Laser ReloadTime upgrades
	"R-Wpn-HvyLaser", // Heavy Laser
	"R-Wpn-Energy-Damage03", // T4+ Laser Damage upgrades
	"R-Wpn-Energy-ROF03", // T4+ Laser ReloadTime upgrades
]

// ------------------ ROCKETS ------------------

const FUNDAMENTALS_ROCKET_WEAPON_T1 = [
	"R-Wpn-Rocket-Accuracy01", // T1 Accuracy upgrades
	"R-Wpn-Rocket-Damage01", // T1 Damage upgrades
	"R-Wpn-Rocket-ROF01", // T1 ReloadTime upgrades
]

const FUNDAMENTALS_ROCKET_WEAPON_T2 = [
	"R-Wpn-Rocket01-LtAT", // Lancer
	"R-Wpn-Rocket01-LtAT", // Cyborg Lancer
	"R-Wpn-Rocket-Accuracy02", // T2 Accuracy upgrades
	"R-Wpn-Rocket-Damage02", // T2 Damage upgrades
	"R-Wpn-Rocket-ROF02", // T2 ReloadTime upgrades
]

const FUNDAMENTALS_ROCKET_WEAPON_T3 = [
	"R-Wpn-Rocket07-Tank-Killer", // Tank Killer
	"R-Cyborg-Hvywpn-TK", // Super Cyborg Tank Killer
	"R-Wpn-RocketSlow-Accuracy01", // T3 Slow Rocket Accuracy upgrades
	"R-Wpn-Rocket-Damage03", // T3 Damage upgrades
	"R-Wpn-Rocket-ROF03", // T3 ReloadTime upgrades
	"R-Wpn-Rocket03-HvAT", // Bunker Buster (low priority)
]

const FUNDAMENTALS_ROCKET_WEAPON_T4 = [
	"R-Wpn-Missile2A-T", // Scourge Missile
	"R-Cyborg-Hvywpn-A-T", // Super Cyborg Scourge
	"R-Wpn-RocketSlow-Accuracy02", // T4 Slow Rocket Accuracy upgrades
	"R-Wpn-Rocket-Damage09", // T4 Damage upgrades
]

// ------------------ FIRE ------------------

const FUNDAMENTALS_FIRE_WEAPON = [
	"R-Wpn-Flamer-Damage03", // T2 upgrades
	"R-Wpn-Flame2", // Inferno
	"R-Wpn-Plasmite-Flamer", // Plasmite Flamer
	"R-Wpn-HeavyPlasmaLauncher", // Heavy Plasma Launcher
]

const FUNDAMENTALS_HOWITZER_WEAPON = [
	"R-Wpn-HvyHowitzer", // Ground shaker
	"R-Wpn-Howitzer03-Rot", // Hellstorm
	"R-Wpn-HowitzerMk1", // Howitzer
]

// ------------------ VTOL BOMBS ------------------

const FUNDAMENTALS_VTOL_BOMBS_T2 = [
	"R-Wpn-Bomb01", // Cluster Bombs Bay
	"R-Wpn-Bomb-Damage01", // HE Bomb Shells
	"R-Wpn-Bomb02", // HEAP Bomb Bay
	"R-Wpn-Bomb03", // Phosphor Bomb Bay
]

const FUNDAMENTALS_VTOL_BOMBS_T3 = [
	"R-Wpn-Bomb-Damage02", // Improved Bomb Warhead
	"R-Wpn-Bomb04", // Thermite Bomb Bay
	"R-Struc-VTOLPad-Upgrade01", // Automated VTOL Rearming
	"R-Struc-VTOLPad-Upgrade03", // Automated VTOL Rearming Mk3
	"R-Struc-VTOLPad-Upgrade06", // Robotic VTOL Rearming Mk3
]

// ------------------ DEFENCES ------------------

const AA_RESEARCH = [
	"R-Wpn-AAGun03", // Hurricane AA Turret
	"R-Defense-AASite-QuadMg1", // Hurricane AA Site
	"R-Wpn-AAGun02", // Cyclone Flak Turret
	"R-Defense-AASite-QuadBof", // Cyclone AA Flak Site
	"R-Wpn-AAGun04", // Whirlwind AA Turret
	"R-Defense-AASite-QuadRotMg", // Whirlwind AA Site
	"R-Wpn-Missile-LtSAM", // Avenger SAM
	"R-Defense-SamSite1", // Avenger SAM Site
	"R-Wpn-Missile-HvSAM", // Vindicator SAM
	"R-Defense-SamSite2" // Vindicator SAM Site
]

const FUNDAMENTALS_DEFENCE_T1 = [
	"R-Defense-Tower01", // Machinegun Guard Tower
	"R-Defense-Tower06", // Mini-Rocket Guard Tower
	"R-Defense-Pillbox01", // Heavy Machinegun Bunker
	"R-Defense-WallUpgrade03", // T1 defences upgrades
]

const FUNDAMENTALS_DEFENCE_T2 = [
	"R-Defense-WallTower06", // Lancer Hardpoint
	"R-Defense-RotMG", // Rotary Machine Gun Bunker
	"R-Defense-IDFRocket", // Ripple Rocket Battery
	"R-Defense-WallUpgrade06", // T2 defences upgrades
]

const FUNDAMENTALS_DEFENCE_T3 = [
	"R-Sys-Sensor-WSTower", // Wide Spectrum Sensor Tower
	"R-Defense-WallUpgrade09", // T3 defences upgrades
]

const FUNDAMENTALS_DEFENCE_T4 = [
	"R-Defense-Super-Cannon", // Cannon Fortress
	"R-Defense-EMPMortar", // EMP Mortar Pit
	"R-Defense-HeavyPlasmaLauncher", // Heavy Plasma Launcher
	"R-Defense-WallUpgrade12", // T4 defences upgrades
	"R-Defense-HvyArtMissile", // Archangel Missile Battery
	"R-Defense-Super-Missile", // Missile Fortress
	"R-Defense-MassDriver", // Mass Driver Fortress	
]

// ------------------ UPGRADES ------------------

const FUNDAMENTALS_UPGRADES_T1 = [
	"R-Struc-Power-Upgrade01", // Power generator efficiency
	"R-Struc-Research-Upgrade01", // Research speed
	"R-Struc-Research-Upgrade02", // Research speed
	"R-Struc-Factory-Upgrade01", // Factory production speed
	"R-Struc-RprFac-Upgrade01", // Repair facility speed
	"R-Sys-Sensor-Upgrade01", // Sensor range
]

const FUNDAMENTALS_UPGRADES_T2 = [
	"R-Struc-Power-Upgrade02", // Power generator efficiency
	"R-Struc-Research-Upgrade05", // Research speed
	"R-Struc-Research-Upgrade06", // Research speed
	"R-Struc-Factory-Upgrade04", // Factory production speed
	"R-Struc-VTOLFactory", // VTOL Factory
	"R-Struc-VTOLPad", // VTOL Rearming Pad
	"R-Sys-Sensor-Upgrade02", // Sensor range
]

const FUNDAMENTALS_UPGRADES_T3 = [
	"R-Struc-Power-Upgrade03", // Power generator efficiency
	"R-Struc-Factory-Upgrade07", // Factory production speed
	"R-Struc-Research-Upgrade06", // Research speed
	"R-Struc-RprFac-Upgrade04", // Repair facility speed
	"R-Sys-Sensor-Upgrade03", // Sensor range
	"R-Sys-Autorepair-General", // Auto-Repair
	"R-Sys-Sensor-UpLink", // Satellite Uplink Center
	"R-Struc-Research-Upgrade07", // Research speed
	"R-Struc-Power-Upgrade01b", // Power generator efficiency
]

const FUNDAMENTALS_UPGRADES_T4 = [
	"R-Struc-Power-Upgrade01c", // Power generator efficiency
	"R-Struc-Research-Upgrade08", // Research speed
	"R-Struc-Factory-Upgrade09", // Factory production speed
	"R-Wpn-LasSat", // Laser Satellite Command Post
	"R-Sys-Resistance-Circuits", // Nexus resistance Circuits
	"R-Struc-Power-Upgrade03a",  // Power generator efficiency
	"R-Struc-Research-Upgrade09", // Research speed	
]