
const structures = {
	factory: "A0LightFactory",
	cyborgFactory: "A0CyborgFactory",
	vtolFactory: "A0VTolFactory1",
	lab: "A0ResearchFacility",
	gen: "A0PowerGenerator",
	hq: "A0CommandCentre",
	relay: "A0ComDroidControl",
	vtolPad: "A0VtolPad",
	derrick: "A0ResourceExtractor",
	repair: "A0RepairCentre3",
	uplink: "A0Sat-linkCentre",
	lassat: "A0LasSatCommand",
};

const powerUps = [ "OilDrum", "Crate" ];

const sensorTurrets = [
	"SensorTurret1Mk1", // sensor
	"Sys-CBTurret01", //counter-battery
	"Sys-VTOLCBTurret01", //VTOL counter-battery
	"Sys-VstrikeTurret01", //VTOL Strike turret
	"Sensor-WideSpec", // wide spectrum sensor
];

// body and propulsion arrays don't affect fixed template droids
const bodyStats = [
	{ res: "R-Vehicle-Body01", stat: "Body1REC"  }, // viper
	{ res: "R-Vehicle-Body05", stat: "Body5REC"  }, // cobra
	{ res: "R-Vehicle-Body11", stat: "Body11ABT"  }, // python
	{ res: "R-Vehicle-Body02", stat: "Body2SUP"  }, // leopard
	{ res: "R-Vehicle-Body06", stat: "Body6SUPP" }, // panther
	{ res: "R-Vehicle-Body09", stat: "Body9REC" }, // tiger
	{ res: "R-Vehicle-Body13", stat: "Body13SUP" }, // wyvern
	{ res: "R-Vehicle-Body14", stat: "Body14SUP" }, // dragon
	{ res: "R-Vehicle-Body04", stat: "Body4ABT" }, // bug
	{ res: "R-Vehicle-Body08", stat: "Body8MBT" }, // scorpion
	{ res: "R-Vehicle-Body12", stat: "Body12SUP" }, // mantis
	{ res: "R-Vehicle-Body03", stat: "Body3MBT" }, // retaliation
	{ res: "R-Vehicle-Body07", stat: "Body7ABT" }, // retribution
	{ res: "R-Vehicle-Body10", stat: "Body10MBT" }, // vengeance
];

const propulsionStats = [
	{ res: "R-Vehicle-Prop-Wheels", stat: "wheeled01"  },
	{ res: "R-Vehicle-Prop-Halftracks", stat: "HalfTrack" },
	{ res: "R-Vehicle-Prop-Tracks", stat: "tracked01" },
	{ res: "R-Vehicle-Prop-Hover", stat: "hover01" },
	{ res: "R-Vehicle-Prop-VTOL", stat: "V-Tol" },
];

const weaponStats =
{
	machineguns:
	{
		alias: "mg",
		weapons: [
			{ res: "R-Wpn-MG1Mk1", stat: "MG1Mk1" }, // mg
			{ res: "R-Wpn-MG2Mk1", stat: "MG2Mk1" }, // tmg
			{ res: "R-Wpn-MG3Mk1", stat: "MG3Mk1" }, // hmg
			{ res: "R-Wpn-MG4", stat: "MG4ROTARYMk1" }, // ag
			{ res: "R-Wpn-MG5", stat: "MG5TWINROTARY" }, // tag
		],
		// VTOL weapons of the path, in the same order.
		vtols: [
			{ res: "R-Wpn-MG3Mk1", stat: "MG3-VTOL" }, // vtol hmg
			{ res: "R-Wpn-MG4", stat: "MG4ROTARY-VTOL" }, // vtol ag
		],
		defenses: [
			{ res: "R-Defense-Tower01", stat: "GuardTower1" }, // hmg tower
			{ res: "R-Defense-Pillbox01", stat: "PillBox1" }, // hmg bunker
			{ res: "R-Defense-WallTower01", stat: "WallTower01" }, // hmg hardpoint
			{ res: "R-Defense-RotMG", stat: "Pillbox-RotMG" }, // ag bunker
			{ res: "R-Defense-Wall-RotMg", stat: "Wall-RotMg" }, // ag hardpoint
			{ res: "R-Defense-WallTower-TwinAGun", stat: "WallTower-TwinAssaultGun" }, // tag hardpoint
		],
		// Cyborg templates, better borgs below, as usual.
		templates: [
			{ res: "R-Wpn-MG1Mk1", body: "CyborgLightBody", prop: "CyborgLegs", weapons: [ "CyborgChaingun", ] }, // mg cyborg
			{ res: "R-Wpn-MG4", body: "CyborgLightBody", prop: "CyborgLegs", weapons: [ "CyborgRotMG", ] }, // ag cyborg
		],
		// Extra things to research on this path, even if they don't lead to any new stuff
		extras: [
			"R-Wpn-MG-Damage08",
			"R-Wpn-MG-ROF03",
		],
	},
	flamers:
	{
		alias: "fl",
		weapons: [
			{ res: "R-Wpn-Flamer01Mk1", stat: "Flame1Mk1" }, // flamer
			{ res: "R-Wpn-Flame2", stat: "Flame2" }, // inferno
			{ res: "R-Wpn-Plasmite-Flamer", stat: "PlasmiteFlamer" }, // plasmite
		],
		vtols: [],
		defenses: [
			{ res: "R-Defense-Pillbox05", stat: "PillBox5" }, // flamer bunker
			{ res: "R-Defense-HvyFlamer", stat: "Tower-Projector" }, // inferno bunker
			{ res: "R-Defense-PlasmiteFlamer", stat: "Plasmite-flamer-bunker" }, // plasmite bunker
		],
		templates: [
			{ res: "R-Wpn-Flamer01Mk1", body: "CyborgLightBody", prop: "CyborgLegs", weapons: [ "CyborgFlamer01", ] }, // flamer cyborg
			{ res: "R-Wpn-Flame2", body: "CyborgLightBody", prop: "CyborgLegs", weapons: [ "Cyb-Wpn-Thermite", ] }, // thermite flamer cyborg
		],
		extras: [
			"R-Wpn-Flamer-ROF01",
			"R-Wpn-Flamer-Damage03",
			"R-Wpn-Flamer-ROF03",
			"R-Wpn-Flamer-Damage09",
		],
	},
	cannons:
	{
		alias: "cn",
		weapons: [
			{ res: "R-Wpn-Cannon1Mk1", stat: "Cannon1Mk1" }, // lc
			{ res: "R-Wpn-Cannon2Mk1", stat: "Cannon2A-TMk1" }, // mc
			{ res: "R-Wpn-Cannon4AMk1", stat: "Cannon4AUTOMk1" }, // hpv
			{ res: "R-Wpn-Cannon5", stat: "Cannon5VulcanMk1" }, // ac
			{ res: "R-Wpn-Cannon6TwinAslt", stat: "Cannon6TwinAslt" }, // tac
			{ res: "R-Wpn-Cannon3Mk1", stat: "Cannon375mmMk1" }, // hc
		],
		fastFire: [
			{ res: "R-Wpn-Cannon4AMk1", stat: "Cannon4AUTOMk1" }, // hpv
			{ res: "R-Wpn-Cannon5", stat: "Cannon5VulcanMk1" }, // ac
			{ res: "R-Wpn-Cannon6TwinAslt", stat: "Cannon6TwinAslt" }, // tac
		],
		vtols: [
			{ res: "R-Wpn-Cannon1Mk1", stat: "Cannon1-VTOL" }, // lc
			{ res: "R-Wpn-Cannon4AMk1", stat: "Cannon4AUTO-VTOL" }, // hpv
			{ res: "R-Wpn-Cannon5", stat: "Cannon5Vulcan-VTOL" }, // ac
		],
		defenses: [
			{ res: "R-Defense-Pillbox04", stat: "PillBox4" }, // lc bunker
			{ res: "R-Defense-WallTower02", stat: "WallTower02" }, // lc hard
			{ res: "R-Defense-WallTower03", stat: "WallTower03" }, // mc hard
			{ res: "R-Defense-Emplacement-HPVcannon", stat: "Emplacement-HPVcannon" }, // hpv empl
			{ res: "R-Defense-WallTower-HPVcannon", stat: "WallTower-HPVcannon" }, // hpv hard
			{ res: "R-Defense-Wall-VulcanCan", stat: "Wall-VulcanCan" }, // ac hard
			{ res: "R-Defense-Cannon6", stat: "PillBox-Cannon6" }, // tac bunker
			{ res: "R-Defense-WallTower04", stat: "WallTower04" }, // hc hard
			//{ res: "R-Defense-Super-Cannon", stat: "X-Super-Cannon" }, // cannon fort
		],
		templates: [
			{ res: "R-Wpn-Cannon1Mk1", body: "CyborgLightBody", prop: "CyborgLegs", weapons: [ "CyborgCannon", ] }, // lc borg
			{ res: "R-Cyborg-Hvywpn-Mcannon", body: "CyborgHeavyBody", prop: "CyborgLegs", weapons: [ "Cyb-Hvywpn-Mcannon", ] }, // mc super
			{ res: "R-Cyborg-Hvywpn-HPV", body: "CyborgHeavyBody", prop: "CyborgLegs", weapons: [ "Cyb-Hvywpn-HPV", ] }, // hpv super
			{ res: "R-Cyborg-Hvywpn-Acannon", body: "CyborgHeavyBody", prop: "CyborgLegs", weapons: [ "Cyb-Hvywpn-Acannon", ] }, // ac super
		],
		extras: [
			"R-Wpn-Cannon-Damage03",
			"R-Wpn-Cannon-ROF01",
			"R-Wpn-Cannon-Damage05",
			"R-Wpn-Cannon-Accuracy01",
			"R-Wpn-Cannon-ROF02",
			"R-Wpn-Cannon-Accuracy02",
			"R-Wpn-Cannon-ROF06",
			"R-Wpn-Cannon-Damage09",
		],
	},
	gauss:
	{
		alias: "ga",
		weapons: [
			{ res: "R-Wpn-RailGun01", stat: "RailGun1Mk1" }, // needle
			{ res: "R-Wpn-RailGun02", stat: "RailGun2Mk1" }, // rail
			{ res: "R-Wpn-RailGun03", stat: "RailGun3Mk1" }, // gauss
		],
		vtols: [
			{ res: "R-Wpn-RailGun01", stat: "RailGun1-VTOL" }, // needle
			{ res: "R-Wpn-RailGun02", stat: "RailGun2-VTOL" }, // rail
		],
		defenses: [
			{ res: "R-Defense-GuardTower-Rail1", stat: "GuardTower-Rail1" }, // needle tower
			{ res: "R-Defense-Rail2", stat: "Emplacement-Rail2" }, // rail empl
			{ res: "R-Defense-WallTower-Rail2", stat: "WallTower-Rail2" }, // rail hard
			{ res: "R-Defense-Rail3", stat: "Emplacement-Rail3" }, // gauss empl
			{ res: "R-Defense-WallTower-Rail3", stat: "WallTower-Rail3" }, // gauss hard
			//{ res: "R-Defense-MassDriver", stat: "X-Super-MassDriver" }, // mass driver fort
		],
		templates: [
			{ res: "R-Wpn-RailGun01", body: "CyborgLightBody", prop: "CyborgLegs", weapons: [ "Cyb-Wpn-Rail1", ] }, // needle borg
			{ res: "R-Cyborg-Hvywpn-RailGunner", body: "CyborgHeavyBody", prop: "CyborgLegs", weapons: [ "Cyb-Hvywpn-RailGunner", ] }, // rail super
		],
		extras: [
			"R-Wpn-Rail-Damage03", // sure it's required by gauss, but what if our AI uses only cyborgs and vtols?
			"R-Wpn-Rail-ROF03",
		],
	},
	cannons_AA:
	{
		alias: "cnaa",
		weapons: [
			{ res: "R-Wpn-AAGun01", stat: "AAGun2Mk1" },
			{ res: "R-Wpn-AAGun02", stat: "AAGun2Mk1Quad" },
		],
		vtols: [],
		defenses: [
			{ res: "R-Defense-AASite-QuadBof", stat: "AASite-QuadBof" },
			{ res: "R-Defense-WallTower-DoubleAAgun", stat: "WallTower-DoubleAAGun" },
			{ res: "R-Defense-AASite-QuadBof02", stat: "AASite-QuadBof02" },
			{ res: "R-Defense-WallTower-DoubleAAgun02", stat: "WallTower-DoubleAAGun02" },
		],
		templates: [],
		extras: [
			"R-Wpn-Cannon-Damage03",
			"R-Wpn-Cannon-ROF01",
			"R-Wpn-Cannon-Damage05",
			"R-Wpn-Cannon-Accuracy01",
			"R-Wpn-Cannon-ROF02",
			"R-Wpn-Cannon-Accuracy02",
			"R-Wpn-Cannon-ROF06",
			"R-Wpn-Cannon-Damage09",
		],
	},
	mortars:
	{
		alias: "mor",
		weapons: [
			{ res: "R-Wpn-Mortar01Lt", stat: "Mortar1Mk1" },
			{ res: "R-Wpn-Mortar3", stat: "Mortar3ROTARYMk1" },
			{ res: "R-Wpn-Mortar02Hvy", stat: "Mortar2Mk1" },
			{ res: "R-Wpn-HowitzerMk1", stat: "Howitzer105Mk1" },
			{ res: "R-Wpn-Howitzer03-Rot", stat: "Howitzer03-Rot" },
			{ res: "R-Wpn-HvyHowitzer", stat: "Howitzer150Mk1" },
		],
		fastFire: [
			{ res: "R-Wpn-Mortar3", stat: "Mortar3ROTARYMk1" },
			{ res: "R-Wpn-Howitzer03-Rot", stat: "Howitzer03-Rot" },
		],
		vtols: [
			{ res: "R-Wpn-Bomb01", stat: "Bomb1-VTOL-LtHE" },
			{ res: "R-Wpn-Bomb02", stat: "Bomb2-VTOL-HvHE" },
		],
		defenses: [
			//{ res: "R-Defense-MortarPit", stat: "Emplacement-MortarPit01" },
			{ res: "R-Defense-HvyMor", stat: "Emplacement-MortarPit02" },
			{ res: "R-Defense-RotMor", stat: "Emplacement-RotMor" },
			{ res: "R-Defense-Howitzer", stat: "Emplacement-Howitzer105" },
			{ res: "R-Defense-RotHow", stat: "Emplacement-RotHow" },
			{ res: "R-Defense-HvyHowitzer", stat: "Emplacement-Howitzer150" },
		],
		templates: [
			{ res: "R-Wpn-Mortar01Lt", body: "CyborgLightBody", prop: "CyborgLegs", weapons: [ "Cyb-Wpn-Grenade", ] },
		],
		extras: [
			"R-Wpn-Mortar-Damage01",
			"R-Wpn-Mortar-Damage02",
			"R-Wpn-Mortar-ROF01",
			"R-Wpn-Mortar-Acc01",
			"R-Wpn-Mortar-Acc02",
			"R-Wpn-Mortar-ROF02",
			"R-Wpn-Mortar-Damage03",
			"R-Wpn-Mortar-Acc03",
			"R-Wpn-Mortar-Damage04",
			"R-Wpn-Mortar-Damage05",
			"R-Wpn-Mortar-ROF03",
			"R-Wpn-Mortar-Damage06",
			"R-Wpn-Mortar-ROF04",
			"R-Wpn-Howitzer-Damage01",
			"R-Wpn-Howitzer-ROF01",
			"R-Wpn-Howitzer-ROF02",
			"R-Wpn-Howitzer-Damage02",
			"R-Wpn-Howitzer-Damage03",
			"R-Wpn-Howitzer-Accuracy01",
			"R-Wpn-Howitzer-Accuracy02",
			"R-Wpn-Howitzer-Accuracy03",
			"R-Wpn-Howitzer-ROF03",
			"R-Wpn-Howitzer-ROF04",
			"R-Wpn-Howitzer-Damage04",
			"R-Wpn-Howitzer-Damage05",
			"R-Wpn-Howitzer-Damage06",
		],
	},
	howitzers:
	{
		alias: "how",
		weapons: [
			{ res: "R-Wpn-HowitzerMk1", stat: "Howitzer105Mk1" },
			{ res: "R-Wpn-Howitzer03-Rot", stat: "Howitzer03-Rot" },
			{ res: "R-Wpn-HvyHowitzer", stat: "Howitzer150Mk1" },
		],
		fastFire: [
			{ res: "R-Wpn-Howitzer03-Rot", stat: "Howitzer03-Rot" },
		],
		vtols: [
			{ res: "R-Wpn-Bomb01", stat: "Bomb1-VTOL-LtHE" },
			{ res: "R-Wpn-Bomb02", stat: "Bomb2-VTOL-HvHE" },
		],
		defenses: [
			{ res: "R-Defense-Howitzer", stat: "Emplacement-Howitzer105" },
			{ res: "R-Defense-RotHow", stat: "Emplacement-RotHow" },
			{ res: "R-Defense-HvyHowitzer", stat: "Emplacement-Howitzer150" },
		],
		templates: [],
		extras: [
			"R-Wpn-Howitzer-Damage01",
			"R-Wpn-Howitzer-ROF01",
			"R-Wpn-Howitzer-ROF02",
			"R-Wpn-Howitzer-Damage02",
			"R-Wpn-Howitzer-Damage03",
			"R-Wpn-Howitzer-Accuracy01",
			"R-Wpn-Howitzer-Accuracy02",
			"R-Wpn-Howitzer-Accuracy03",
			"R-Wpn-Howitzer-ROF03",
			"R-Wpn-Howitzer-ROF04",
			"R-Wpn-Howitzer-Damage04",
			"R-Wpn-Howitzer-Damage05",
			"R-Wpn-Howitzer-Damage06",
		],
	},
	fireMortars:
	{
		alias: "fmor",
		weapons: [
			{ res: "R-Wpn-Mortar01Lt", stat: "Mortar1Mk1" },
			{ res: "R-Wpn-Mortar3", stat: "Mortar3ROTARYMk1" },
			{ res: "R-Wpn-Mortar02Hvy", stat: "Mortar2Mk1" },
			{ res: "R-Wpn-Mortar-Incendiary", stat: "Mortar-Incendiary" },
			{ res: "R-Wpn-Howitzer-Incendiary", stat: "Howitzer-Incendiary" },
		],
		fastFire: [
			{ res: "R-Wpn-Mortar3", stat: "Mortar3ROTARYMk1" },
			{ res: "R-Wpn-Howitzer03-Rot", stat: "Howitzer03-Rot" },
		],
		vtols: [
			{ res: "R-Wpn-Bomb03", stat: "Bomb3-VTOL-LtINC" },
			{ res: "R-Wpn-Bomb04", stat: "Bomb4-VTOL-HvyINC" },
			{ res: "R-Wpn-Bomb05", stat: "Bomb5-VTOL-Plasmite" },
		],
		defenses: [
			{ res: "R-Defense-MortarPit-Incendiary", stat: "Emplacement-MortarPit-Incendiary" },
			{ res: "R-Defense-Howitzer-Incendiary", stat: "Emplacement-Howitzer-Incendiary" },
		],
		templates: [],
		extras: [
			"R-Wpn-Mortar-Damage01",
			"R-Wpn-Mortar-Damage02",
			"R-Wpn-Mortar-ROF01",
			"R-Wpn-Mortar-Acc01",
			"R-Wpn-Mortar-Acc02",
			"R-Wpn-Mortar-ROF02",
			"R-Wpn-Mortar-Damage03",
			"R-Wpn-Mortar-Acc03",
			"R-Wpn-Mortar-Damage04",
			"R-Wpn-Mortar-Damage05",
			"R-Wpn-Mortar-ROF03",
			"R-Wpn-Mortar-Damage06",
			"R-Wpn-Mortar-ROF04",
			"R-Wpn-Howitzer-Damage01",
			"R-Wpn-Howitzer-ROF01",
			"R-Wpn-Howitzer-ROF02",
			"R-Wpn-Howitzer-Damage02",
			"R-Wpn-Howitzer-Damage03",
			"R-Wpn-Howitzer-Accuracy01",
			"R-Wpn-Howitzer-Accuracy02",
			"R-Wpn-Howitzer-Accuracy03",
			"R-Wpn-Howitzer-ROF03",
			"R-Wpn-Howitzer-ROF04",
			"R-Wpn-Howitzer-Damage04",
			"R-Wpn-Howitzer-Damage05",
			"R-Wpn-Howitzer-Damage06",
			"R-Wpn-Flamer-Damage09",
		],
	},
	rockets_AT:
	{
		alias: "rkt",
		weapons: [
			{ res: "R-Wpn-Rocket05-MiniPod", stat: "Rocket-Pod" }, // pod
			{ res: "R-Wpn-Rocket01-LtAT", stat: "Rocket-LtA-T" }, // lancer
			{ res: "R-Wpn-Rocket07-Tank-Killer", stat: "Rocket-HvyA-T" }, // tk
			{ res: "R-Wpn-Missile2A-T", stat: "Missile-A-T" }, // scourge
		],
		vtols: [
			{ res: "R-Wpn-Rocket05-MiniPod", stat: "Rocket-VTOL-Pod" }, // pod
			{ res: "R-Wpn-Rocket01-LtAT", stat: "Rocket-VTOL-LtA-T" }, // lancer
			{ res: "R-Wpn-Rocket07-Tank-Killer", stat: "Rocket-VTOL-HvyA-T" }, // tk
			{ res: "R-Wpn-Missile2A-T", stat: "Missile-VTOL-AT" }, // scourge
		],
		defenses: [
			{ res: "R-Defense-Tower06", stat: "GuardTower6" }, // pod tower
			{ res: "R-Defense-Pillbox06", stat: "GuardTower5" }, // lancer tower
			{ res: "R-Defense-WallTower06", stat: "WallTower06" }, // lancer hardpoint
			{ res: "R-Defense-HvyA-Trocket", stat: "Emplacement-HvyATrocket" }, // tk emplacement
			{ res: "R-Defense-WallTower-HvyA-Trocket", stat: "WallTower-HvATrocket" }, // tk hardpoint
			//{ res: "R-Defense-Super-Rocket", stat: "X-Super-Rocket" }, // rocket bastion
			{ res: "R-Defense-GuardTower-ATMiss", stat: "GuardTower-ATMiss" }, // scourge tower
			{ res: "R-Defense-WallTower-A-Tmiss", stat: "WallTower-Atmiss" }, // scourge hardpoint
			//{ res: "R-Defense-Super-Missile", stat: "X-Super-Missile" }, // missile fortress
		],
		templates: [
			{ res: "R-Wpn-Rocket01-LtAT", body: "CyborgLightBody", prop: "CyborgLegs", weapons: [ "CyborgRocket", ] }, // lancer borg
			{ res: "R-Cyborg-Hvywpn-TK", body: "CyborgHeavyBody", prop: "CyborgLegs", weapons: [ "Cyb-Hvywpn-TK", ] }, // tk super
			{ res: "R-Wpn-Missile2A-T", body: "CyborgLightBody", prop: "CyborgLegs", weapons: [ "Cyb-Wpn-Atmiss", ] }, // scourge borg
			{ res: "R-Cyborg-Hvywpn-A-T", body: "CyborgHeavyBody", prop: "CyborgLegs", weapons: [ "Cyb-Hvywpn-A-T", ] }, // scourge super
		],
		extras: [
			"R-Wpn-Rocket-ROF02",
			"R-Wpn-Rocket-Damage03",
			"R-Wpn-Rocket-Accuracy01",
			"R-Wpn-Rocket-ROF03",
			"R-Wpn-Rocket-Damage09",
			"R-Wpn-Rocket-Accuracy02",
			"R-Wpn-RocketSlow-Accuracy02",
			"R-Wpn-Missile-Damage01",
			"R-Wpn-Missile-ROF01",
			"R-Wpn-Missile-Accuracy02",
			"R-Wpn-Missile-Damage03",
			"R-Wpn-Missile-ROF03",
		],
	},
	missile_AT:
	{
		alias: "miss",
		weapons: [
			{ res: "R-Wpn-Missile2A-T", stat: "Missile-A-T" }, // scourge
		],
		vtols: [
			{ res: "R-Wpn-Missile2A-T", stat: "Missile-VTOL-AT" }, // scourge
		],
		defenses: [
			{ res: "R-Defense-GuardTower-ATMiss", stat: "GuardTower-ATMiss" }, // scourge tower
			{ res: "R-Defense-WallTower-A-Tmiss", stat: "WallTower-Atmiss" }, // scourge hardpoint
			//{ res: "R-Defense-Super-Missile", stat: "X-Super-Missile" }, // missile fortress
		],
		templates: [
			{ res: "R-Wpn-Missile2A-T", body: "CyborgLightBody", prop: "CyborgLegs", weapons: [ "Cyb-Wpn-Atmiss", ] }, // scourge borg
			{ res: "R-Cyborg-Hvywpn-A-T", body: "CyborgHeavyBody", prop: "CyborgLegs", weapons: [ "Cyb-Hvywpn-A-T", ] }, // scourge super
		],
		extras: [
			"R-Wpn-Missile-Damage01",
			"R-Wpn-Missile-ROF01",
			"R-Wpn-Missile-Accuracy02",
			"R-Wpn-Missile-Damage03",
			"R-Wpn-Missile-ROF03",
		],
	},
	rockets_Arty:
	{
		alias: "rkta",
		weapons: [
			{ res: "R-Wpn-Rocket02-MRL", stat: "Rocket-MRL" }, // mra
			{ res: "R-Wpn-Rocket02-MRLHvy", stat: "Rocket-MRL-Hvy" }, // hra
			{ res: "R-Wpn-Rocket06-IDF", stat: "Rocket-IDF" }, // ripple
			{ res: "R-Wpn-MdArtMissile", stat: "Missile-MdArt" }, // seraph
			{ res: "R-Wpn-HvArtMissile", stat: "Missile-HvyArt" }, // archie
		],
		fastFire: [
			{ res: "R-Wpn-Rocket02-MRLHvy", stat: "Rocket-MRL-Hvy" },
			{ res: "R-Wpn-MdArtMissile", stat: "Missile-MdArt" },
		],
		vtols: [
			{ res: "R-Wpn-Rocket03-HvAT", stat: "Rocket-VTOL-BB" }, // bb
		],
		defenses: [
			{ res: "R-Defense-MRL", stat: "Emplacement-MRL-pit" }, // mra
			{ res: "R-Defense-MRLHvy", stat: "Emplacement-MRLHvy-pit" }, // hra
			{ res: "R-Defense-IDFRocket", stat: "Emplacement-Rocket06-IDF" }, // ripple
			{ res: "R-Defense-MdArtMissile", stat: "Emplacement-MdART-pit" }, // seraph
			{ res: "R-Defense-HvyArtMissile", stat: "Emplacement-HvART-pit" }, // archie
		],
		templates: [],
		extras: [
			"R-Wpn-Rocket-ROF02",
			"R-Wpn-Rocket-Damage03",
			"R-Wpn-Rocket-Accuracy01",
			"R-Wpn-Rocket-ROF03",
			"R-Wpn-Rocket-Damage09",
			"R-Wpn-Rocket-Accuracy02",
			"R-Wpn-RocketSlow-Accuracy02",
			"R-Wpn-Missile-Damage01",
			"R-Wpn-Missile-ROF01",
			"R-Wpn-Missile-Accuracy02",
			"R-Wpn-Missile-Damage03",
			"R-Wpn-Missile-ROF03",
		],
	},
	missile_Arty:
	{
		alias: "missa",
		weapons: [
			{ res: "R-Wpn-MdArtMissile", stat: "Missile-MdArt" }, // seraph
			{ res: "R-Wpn-HvArtMissile", stat: "Missile-HvyArt" }, // archie
		],
		fastFire: [
			{ res: "R-Wpn-MdArtMissile", stat: "Missile-MdArt" },
		],
		vtols: [],
		defenses: [
			{ res: "R-Defense-MdArtMissile", stat: "Emplacement-MdART-pit" }, // seraph
			{ res: "R-Defense-HvyArtMissile", stat: "Emplacement-HvART-pit" }, // archie
		],
		templates: [],
		extras: [
			"R-Wpn-Missile-Damage01",
			"R-Wpn-Missile-ROF01",
			"R-Wpn-Missile-Accuracy02",
			"R-Wpn-Missile-Damage03",
			"R-Wpn-Missile-ROF03",
		],
	},
	rockets_AS:
	{
		alias: "rktas",
		weapons: [
			{ res: "R-Wpn-Rocket03-HvAT", stat: "Rocket-BB" }, // bb
		],
		vtols: [
			{ res: "R-Wpn-Rocket03-HvAT", stat: "Rocket-VTOL-BB" }, // bb
		],
		defenses: [],
		templates: [],
		extras: [],
	},
	rockets_AA:
	{
		alias: "rktaa",
		weapons: [
			{ res: "R-Wpn-Sunburst", stat: "Rocket-Sunburst" }, // sunburst
			{ res: "R-Wpn-Missile-LtSAM", stat: "Missile-LtSAM" }, // avenger
			{ res: "R-Wpn-Missile-HvSAM", stat: "Missile-HvySAM" }, // vindicator
		],
		vtols: [
			{ res: "R-Wpn-Sunburst", stat: "Rocket-VTOL-Sunburst" }, // sunburst a2a
		],
		defenses: [
			{ res: "R-Defense-Sunburst", stat: "P0-AASite-Sunburst" }, // sunburst
			{ res: "R-Defense-SamSite1", stat: "P0-AASite-SAM1" }, // avenger
			//{ res: "R-Defense-WallTower-SamSite", stat: "WallTower-SamSite" }, // avenger
			{ res: "R-Defense-SamSite2", stat: "P0-AASite-SAM2" }, // vindicator
			//{ res: "R-Defense-WallTower-SamHvy", stat: "WallTower-SamHvy" }, // vindicator hardpoint
		],
		templates: [],
		extras: [
			"R-Wpn-Rocket-ROF02",
			"R-Wpn-Rocket-Damage03",
			"R-Wpn-Rocket-Accuracy01",
			"R-Wpn-Rocket-ROF03",
			"R-Wpn-Rocket-Damage09",
			"R-Wpn-Rocket-Accuracy02",
			"R-Wpn-RocketSlow-Accuracy02",
			"R-Wpn-Missile-Damage01",
			"R-Wpn-Missile-ROF01",
			"R-Wpn-Missile-Accuracy02",
			"R-Wpn-Missile-Damage03",
			"R-Wpn-Missile-ROF03",
		],
	},
	lasers:
	{
		alias: "las",
		weapons: [
			{ res: "R-Wpn-Laser01", stat: "Laser3BEAMMk1" }, // flash
			{ res: "R-Wpn-Laser02", stat: "Laser2PULSEMk1" }, // pulse
			{ res: "R-Wpn-HvyLaser", stat: "HeavyLaser" }, // hvy laser
		],
		vtols: [
			{ res: "R-Wpn-Laser01", stat: "Laser3BEAM-VTOL" }, // flash
			{ res: "R-Wpn-Laser02", stat: "Laser2PULSE-VTOL" }, // pulse
			{ res: "R-Wpn-HvyLaser", stat: "HeavyLaser-VTOL" }, // hvy laser
		],
		defenses: [
			{ res: "R-Defense-PrisLas", stat: "Emplacement-PrisLas" }, // flash empl
			{ res: "R-Defense-PulseLas", stat: "GuardTower-BeamLas" }, // pulse tower
			{ res: "R-Defense-WallTower-PulseLas", stat: "WallTower-PulseLas" }, // pulse hard
			{ res: "R-Defense-HeavyLas", stat: "Emplacement-HeavyLaser" }, // hvy empl
		],
		templates: [
			{ res: "R-Wpn-Laser01", body: "CyborgLightBody", prop: "CyborgLegs", weapons: [ "Cyb-Wpn-Laser", ] }, // flash borg
			{ res: "R-Cyborg-Hvywpn-PulseLsr", body: "CyborgHeavyBody", prop: "CyborgLegs", weapons: [ "Cyb-Hvywpn-PulseLsr", ] }, // pulse super
		],
		extras: [
			"R-Wpn-Energy-ROF01",
			"R-Wpn-Energy-Damage01",
			"R-Wpn-Energy-Accuracy01",
			"R-Wpn-Energy-ROF03",
			"R-Wpn-Energy-Damage03",
		],
	},
	fort_AT:
	{
		alias: "fort",
		weapons: [],
		vtols: [],
		defenses: [
			{ res: "R-Defense-Super-Cannon", stat: "X-Super-Cannon" },
			{ res: "R-Defense-Super-Rocket", stat: "X-Super-Rocket" },
			{ res: "R-Defense-Super-Missile", stat: "X-Super-Missile" },
			{ res: "R-Defense-MassDriver", stat: "X-Super-MassDriver" },
		],
		templates: [],
		extras: [],
	},
	AS:
	{
		alias: "as",
		weapons: [
			{ res: "R-Wpn-PlasmaCannon", stat: "Laser4-PlasmaCannon" }, // plasma cannon
			{ res: "R-Wpn-HeavyPlasmaLauncher", stat: "PlasmaHeavy" }, // Heavy Plasma launcher
		],
		vtols: [],
		defenses: [
			{ res: "R-Defense-PlasmaCannon", stat: "Emplacement-PlasmaCannon" },
			{ res: "R-Defense-HeavyPlasmaLauncher", stat: "Emplacement-HeavyPlasmaLauncher" },
		],
		templates: [],
		extras: [
			"R-Wpn-Cannon-Damage03",
			"R-Wpn-Cannon-ROF01",
			"R-Wpn-Cannon-Damage05",
			"R-Wpn-Cannon-Accuracy01",
			"R-Wpn-Cannon-ROF02",
			"R-Wpn-Cannon-Accuracy02",
			"R-Wpn-Cannon-ROF06",
			"R-Wpn-Cannon-Damage09",
			"R-Wpn-Flamer-ROF03",
			"R-Wpn-Flamer-Damage09",
		],
	},
	bombs:
	{
		alias: "bomb",
		weapons: [],
		vtols: [
		//	{ res: "R-Wpn-Bomb01", stat: "Bomb1-VTOL-LtHE" }, // cluster bomb
			//{ res: "R-Wpn-Bomb03", stat: "Bomb3-VTOL-LtINC" }, // Phosphor bomb
			{ res: "R-Wpn-Bomb02", stat: "Bomb2-VTOL-HvHE" }, // HEAP bomb
			{ res: "R-Wpn-Bomb04", stat: "Bomb4-VTOL-HvyINC" }, // Thermite bomb
			{ res: "R-Wpn-Bomb05", stat: "Bomb5-VTOL-Plasmite" }, // Plasmite bomb
		],
		empBomb: [
			{ res: "R-Wpn-Bomb06", stat: "Bomb6-VTOL-EMP" }, // EMP Missile Launcher
		],
		defenses: [],
		templates: [],
		extras: [
			"R-Struc-VTOLPad-Upgrade03",
			"R-Wpn-Bomb-Damage03",
			"R-Struc-VTOLPad-Upgrade06",
			"R-Wpn-Flamer-Damage09",
		],
	},
	AA:
	{
		alias: "aa",
		weapons: [
			{ res: "R-Wpn-AAGun03", stat: "QuadMg1AAGun" }, // hurricane
			{ res: "R-Wpn-AAGun04", stat: "QuadRotAAGun" }, // whirlwind
		],
		vtols: [
			{ res: "R-Wpn-Sunburst", stat: "Rocket-VTOL-Sunburst" },
		],
		defenses: [
			{ res: "R-Defense-AASite-QuadMg1", stat: "AASite-QuadMg1" }, // hurricane
			{ res: "R-Defense-AASite-QuadRotMg", stat: "AASite-QuadRotMg" }, // whirlwind
			//{ res: "R-Defense-WallTower-QuadRotAA", stat: "WallTower-QuadRotAAGun" },
		],
		templates: [],
		extras: [
			"R-Wpn-MG-ROF03",
			"R-Wpn-MG-Damage08",
		],
	},
	nexusTech:
	{
		alias: "nex",
		weapons: [
			{ res: "R-Wpn-EMPCannon", stat: "EMP-Cannon" },
			{ res: "R-Wpn-MortarEMP", stat: "MortarEMP" },
			{ res: "R-Sys-SpyTurret", stat: "SpyTurret01" },
			{ res: "R-Sys-SpyTurret", stat: "SpyTurret01" }, // weighted to favor Nexus Link
		],
		vtols: [],
		defenses: [
			{ res: "R-Sys-SpyTurret", stat: "Sys-SpyTower" },
			{ res: "R-Defense-EMPCannon", stat: "WallTower-EMP" },
			{ res: "R-Defense-EMPMortar", stat: "Emplacement-MortarEMP" },
		],
		templates: [],
		extras: [
			"R-Sys-Resistance-Circuits",
		],
	},
	lasers_AA:
	{
		alias: "lasaa",
		weapons: [
			{ res: "R-Wpn-AALaser", stat: "AAGunLaser" }, // stormbringer
		],
		vtols: [],
		defenses: [
			{ res: "R-Defense-AA-Laser", stat: "P0-AASite-Laser" }, // stormbringer
		],
		templates: [],
		extras: [
			"R-Wpn-Energy-ROF01",
			"R-Wpn-Energy-Damage01",
			"R-Wpn-Energy-Accuracy01",
			"R-Wpn-Energy-ROF03",
			"R-Wpn-Energy-Damage03",
		],
	},
	empBomb:
	{
		alias: "empbomb",
		weapons: [],
		vtols: [
			{ res: "R-Wpn-Bomb06", stat: "Bomb6-VTOL-EMP" }, // EMP Missile Launcher
		],
		defenses: [],
		templates: [],
		extras: [
			"R-Struc-VTOLPad-Upgrade03",
			"R-Wpn-Bomb-Damage03",
			"R-Struc-VTOLPad-Upgrade06",
		],
	},
};
