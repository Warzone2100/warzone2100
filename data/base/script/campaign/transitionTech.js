//Contains the camapign transition technology definitions.

//This array should give a player all the research from Alpha.
const ALPHA_RESEARCH = [
	"R-Wpn-MG1Mk1", "R-Vehicle-Body01",
	"R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels", "R-Wpn-Flamer-Damage03",
	"R-Sys-Engineering01", "R-Sys-MobileRepairTurret01",
	"R-Struc-PowerModuleMk1", "R-Wpn-MG2Mk1",
	"R-Wpn-MG3Mk1", "R-Wpn-Cannon1Mk1", "R-Defense-WallUpgrade03",
	"R-Struc-Factory-Upgrade03","R-Vehicle-Metals03", "R-Cyborg-Wpn-MG",
	"R-Cyborg-Metals03", "R-Struc-Factory-Cyborg-Upgrade03",
	"R-Struc-Materials03", "R-Struc-Research-Upgrade03",
	"R-Struc-RprFac-Upgrade03", "R-Wpn-MG-ROF01",
	"R-Wpn-Cannon-Damage03", "R-Wpn-Rocket05-MiniPod", "R-Wpn-Rocket-Damage03",
	"R-Wpn-Flamer-ROF01", "R-Wpn-MG-Damage04",
	"R-Wpn-Mortar-Damage03", "R-Wpn-Rocket-Accuracy02",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Damage03",
	"R-Wpn-RocketSlow-Accuracy01", "R-Vehicle-Engine03",
	"R-Defense-MRL", "R-Comp-CommandTurret01",
	"R-Cyborg-Wpn-Cannon", "R-Cyborg-Wpn-Flamer",
	"R-Cyborg-Wpn-Rocket", "R-Defense-MortarPit", "R-Defense-Pillbox01",
	"R-Defense-Pillbox04", "R-Defense-Pillbox05", "R-Defense-Pillbox06",
	"R-Defense-TankTrap01", "R-Defense-Tower01", "R-Defense-Tower06",
	"R-Defense-WallTower01", "R-Defense-WallTower02", "R-Defense-WallTower03",
	"R-Defense-WallTower04", "R-Defense-WallTower06",
	"R-Vehicle-Body11", "R-Vehicle-Body12", "R-Vehicle-Engine03",
	"R-Vehicle-Prop-Tracks", "R-Vehicle-Prop-Hover", "R-Vehicle-Prop-Wheels",
	"R-Wpn-Cannon-Accuracy01", "R-Wpn-Cannon3Mk1", "R-Wpn-Mortar-Acc01",
	"R-Wpn-Mortar-ROF01", "R-Defense-HvyMor", "R-Wpn-Rocket03-HvAT",
];

//BETA 2-A
const ALPHA_TECH = [
	"MG1Mk1", "Cannon1Mk1", "Cannon2A-TMk1", "Cannon375mmMk1", "Flame1Mk1",
	"MG2Mk1", "MG3Mk1", "Mortar1Mk1", "Mortar2Mk1", "Rocket-BB", "Rocket-LtA-T",
	"Rocket-MRL", "Rocket-Pod", "Cannon1-VTOL", "MG1-VTOL", "MG2-VTOL",
	"MG3-VTOL", "Rocket-VTOL-LtA-T", "Rocket-VTOL-Pod", "Rocket-VTOL-BB",
	"CyborgFlamer01", "CyborgCannon", "CyborgChaingun", "Cyb-Wpn-Atmiss",
	"Cyb-Wpn-Laser", "Cyb-Wpn-Rail1", "CyborgRocket", "CyborgRotMG",
	"CommandTurret1", "Body1REC", "Body5REC", "Body11ABT", "Body4ABT",
	"Body8MBT", "Body12SUP", "CyborgCannonGrd", "CyborgFlamerGrd",
	"CyborgChain1Ground", "CyborgRkt1Ground", "HalfTrack", "hover01",
	"tracked01", "wheeled01", "Spade1Mk1", "SensorTurret1Mk1",
	"CommandBrain01",
];

const STRUCTS_ALPHA = [
	"A0CommandCentre", "A0PowerGenerator", "A0ResourceExtractor",
	"A0ResearchFacility", "A0LightFactory", "A0ComDroidControl",
	"A0CyborgFactory", "A0FacMod1", "A0HardcreteMk1CWall",
	"A0HardcreteMk1Wall", "A0PowMod1", "A0RepairCentre3",
	"A0ResearchModule1", "A0TankTrap", "PillBox1", "PillBox4",
	"PillBox5", "PillBox6", "TankTrapC", "WallTower01",
	"WallTower03", "WallTower04", "WallTower06", "AASite-QuadMg1",
	"Emplacement-MortarPit01", "Emplacement-MRL-pit",
];

const PLAYER_RES_BETA = [
	"R-Wpn-MG1Mk1", "R-Sys-Engineering01", "R-Cyborg-Legs01",
	"R-Vehicle-Metals03", "R-Cyborg-Metals03", "R-Defense-WallUpgrade03",
	"R-Struc-Factory-Upgrade03", "R-Struc-Factory-Cyborg-Upgrade03",
	"R-Struc-Materials03", "R-Struc-Research-Upgrade03",
	"R-Struc-RprFac-Upgrade03", "R-Wpn-MG-ROF01",
	"R-Sys-MobileRepairTurret01", "R-Wpn-Cannon-Damage03",
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01", "R-Wpn-MG-Damage04",
	"R-Wpn-Mortar-Damage03", "R-Wpn-Rocket-Accuracy02",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy01",
	"R-Wpn-RocketSlow-Damage03", "R-Vehicle-Engine03",
	"R-Wpn-AAGun03", "R-Defense-AASite-QuadMg1",
	"R-Sys-Sensor-Tower02", "R-Defense-MRL",
];

//GAMMA 3-A
const BETA_TECH = [
	"MG1Mk1", "Cannon1Mk1", "Cannon2A-TMk1", "Cannon375mmMk1", "Flame1Mk1",
	"MG2Mk1", "MG3Mk1", "Mortar1Mk1", "Mortar2Mk1", "Rocket-BB", "Rocket-LtA-T",
	"Rocket-MRL", "Rocket-Pod", "Cannon1-VTOL", "MG1-VTOL", "MG2-VTOL",
	"MG3-VTOL", "Rocket-VTOL-LtA-T", "Rocket-VTOL-Pod", "Rocket-VTOL-BB",
	"CyborgFlamer01", "CyborgCannon", "CyborgChaingun", "Cyb-Wpn-Atmiss",
	"Cyb-Wpn-Laser", "Cyb-Wpn-Rail1", "CyborgRocket", "CyborgRotMG",
	"CommandTurret1", "Body1REC", "Body5REC", "Body11ABT", "Body4ABT",
	"Body8MBT", "Body12SUP", "CyborgCannonGrd", "CyborgFlamerGrd",
	"CyborgChain1Ground", "CyborgRkt1Ground", "HalfTrack", "hover01",
	"tracked01", "wheeled01", "Spade1Mk1", "SensorTurret1Mk1",
	"CommandBrain01", "V-Tol", "Sys-CBTurret01", "Sys-VstrikeTurret01",
	"Sys-VTOLCBTurret01", "Body2SUP", "Body6SUPP", "Body9REC", "AAGun2Mk1",
	"Bomb1-VTOL-LtHE", "Bomb2-VTOL-HvHE", "Bomb3-VTOL-LtINC",
	"Cannon4AUTO-VTOL", "Cannon4AUTOMk1", "Cannon5Vulcan-VTOL", "Cannon5VulcanMk1",
	"Flame2", "Howitzer105Mk1", "Howitzer150Mk1", "MG4ROTARY-VTOL", "MG4ROTARYMk1",
	"Mortar3ROTARYMk1", "Rocket-HvyA-T", "Rocket-IDF", "Rocket-VTOL-HvyA-T",
	"QuadRotAAGun",
];

const STRUCTS_GAMMA = [
	"A0CommandCentre", "A0PowerGenerator", "A0ResourceExtractor",
	"A0ResearchFacility", "A0LightFactory", "A0ComDroidControl",
	"A0CyborgFactory", "A0FacMod1", "A0HardcreteMk1CWall",
	"A0HardcreteMk1Wall", "A0PowMod1", "A0RepairCentre3",
	"A0ResearchModule1", "A0TankTrap", "PillBox1", "PillBox4",
	"PillBox5", "PillBox6", "TankTrapC", "WallTower01",
	"WallTower03", "WallTower04", "WallTower06", "AASite-QuadMg1",
	"Emplacement-MortarPit02", "Emplacement-MRL-pit", "A0VTolFactory1",
	"A0VtolPad", "Sys-CB-Tower01", "Sys-SensoTower02", "Sys-VTOL-CB-Tower01",
	"Sys-VTOL-RadarTower01", "AASite-QuadBof", "AASite-QuadRotMg",
	"Emplacement-HPVcannon", "Emplacement-Howitzer105", "Emplacement-Howitzer150",
	"Emplacement-Rocket06-IDF", "Emplacement-RotMor", "Emplacement-Rocket06-IDF",
	"Emplacement-HvyATrocket", "Wall-RotMg", "Wall-VulcanCan", "WallTower-HvATrocket",
	"WallTower-HPVcannon", "Tower-Projector", "Pillbox-RotMG",
];

const PLAYER_RES_GAMMA = [
	"R-Wpn-MG1Mk1", "R-Sys-Engineering02", "R-Cyborg-Legs01",
	"R-Vehicle-Metals06", "R-Cyborg-Metals06", "R-Defense-WallUpgrade06",
	"R-Struc-Factory-Upgrade06", "R-Struc-Factory-Cyborg-Upgrade06",
	"R-Struc-Materials06", "R-Struc-Research-Upgrade06",
	"R-Struc-RprFac-Upgrade06", "R-Wpn-MG-ROF03",
	"R-Sys-MobileRepairTurret01", "R-Wpn-Cannon-Damage06",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage07",
	"R-Wpn-Mortar-Damage06", "R-Wpn-Rocket-Accuracy02",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage06", "R-Vehicle-Engine06",
	"R-Wpn-AAGun03", "R-Defense-AASite-QuadMg1",
	"R-Sys-Sensor-Tower02", "R-Defense-MRL", "R-Wpn-Cannon-Accuracy02",
	"R-Wpn-Cannon-ROF03", "R-Wpn-Howitzer-Accuracy02", "R-Wpn-Howitzer-Damage03",
	"R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Damage06",
	"R-Wpn-RocketSlow-ROF03", "R-Vehicle-Armor-Heat03", "R-Cyborg-Armor-Heat03",
	"R-Struc-VTOLFactory-Upgrade03", "R-Struc-VTOLPad-Upgrade03",
	"R-Struc-Power-Upgrade01", "R-Sys-Sensor-Upgrade01", "R-Wpn-AAGun-Accuracy02",
	"R-Wpn-AAGun-Damage03", "R-Wpn-AAGun-ROF03", "R-Wpn-Bomb-Accuracy02", "R-Wpn-Bomb03",
	"R-Wpn-Howitzer-ROF03", "R-Sys-CBSensor-Turret01", "R-Sys-VTOLStrike-Turret01",
	"R-Cyborg-Wpn-RotMG-Grd", "R-Defense-AASite-QuadRotMg", "R-Defense-Emplacement-HPVcannon",
	"R-Defense-Howitzer", "R-Defense-HvyFlamer", "R-Defense-RotMG", "R-Defense-Wall-VulcanCan",
	"R-Defense-WallTower-HPVcannon", "R-Defense-HvyHowitzer", "R-Defense-Wall-RotMg",
];

//This is used for giving allies in Gamma technology (3-b/3-2/3-c)
const GAMMA_ALLY_RES = [
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage06", "R-Wpn-Cannon-ROF03",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-Howitzer-Accuracy02",
	"R-Wpn-Howitzer-Damage03", "R-Wpn-MG-Damage07", "R-Wpn-MG-ROF03",
	"R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06", "R-Wpn-Mortar-ROF03",
	"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06", "R-Wpn-Rocket-ROF03",
	"R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage06", "R-Wpn-RocketSlow-ROF03",
	"R-Vehicle-Armor-Heat02", "R-Vehicle-Engine06", "R-Vehicle-Metals06", "R-Cyborg-Metals06",
	"R-Cyborg-Armor-Heat02", "R-Defense-WallUpgrade06", "R-Struc-Factory-Upgrade06",
	"R-Struc-Factory-Cyborg-Upgrade06", "R-Struc-VTOLFactory-Upgrade03",
	"R-Struc-VTOLPad-Upgrade03", "R-Struc-Materials06", "R-Struc-Power-Upgrade01",
	"R-Struc-Research-Upgrade06", "R-Struc-RprFac-Upgrade06", "R-Sys-Engineering02",
	"R-Sys-MobileRepairTurret01", "R-Sys-Sensor-Upgrade01", "R-Wpn-AAGun-Accuracy02",
	"R-Wpn-AAGun-Damage03", "R-Wpn-AAGun-ROF03", "R-Wpn-Bomb-Accuracy02",
];

//...
