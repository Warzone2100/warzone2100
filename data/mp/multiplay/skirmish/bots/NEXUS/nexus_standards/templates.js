
//Original Nexus AI template set. These are selected randomly within a certain limit.


const HOVER_PROPULSIONS = [
	"hover01",
];

const VTOL_PROPULSIONS = [
	"V-Tol",
];

const HELICOPTER_PROPULSIONS = [
	"helicopter",
];

const CYBORG_COMPONENTS = [
	"CyborgLightBody",
	"CyborgHeavyBody",
	"CyborgLegs",
];

const TWO_TURRET_BODY = [
	"Body14SUP",
];

//const THREE_TURRET_BODY = [];

const STANDARD_TEMPLATES = [
	{ body: "Body1REC", prop: "wheeled01", weaps: ["MG1Mk1",] },
	{ body: "Body1REC", prop: "wheeled01", weaps: ["MG2Mk1",] },
	{ body: "Body1REC", prop: "wheeled01", weaps: ["MG3Mk1",] },
	{ body: "Body1REC", prop: "wheeled01", weaps: ["Rocket-Pod",] },
	{ body: "Body1REC", prop: "wheeled01", weaps: ["Rocket-LtA-T",] },
	{ body: "Body1REC", prop: "wheeled01", weaps: ["Flame1Mk1",] },
	{ body: "Body1REC", prop: "wheeled01", weaps: ["Rocket-LtA-T",] },
	{ body: "Body5REC", prop: "HalfTrack", weaps: ["Rocket-MRL",] },
	{ body: "Body1REC", prop: "wheeled01", weaps: ["Cannon1Mk1",] },
	{ body: "Body1REC", prop: "wheeled01", weaps: ["Rocket-Pod",] },
	{ body: "Body1REC", prop: "HalfTrack", weaps: ["Cannon1Mk1",] },
	{ body: "Body5REC", prop: "HalfTrack", weaps: ["Rocket-LtA-T",] },
	{ body: "Body5REC", prop: "HalfTrack", weaps: ["Rocket-MRL",] },
	{ body: "Body5REC", prop: "HalfTrack", weaps: ["Rocket-MRL-Hvy",] },
	{ body: "Body5REC", prop: "HalfTrack", weaps: ["Flame2",] },
	{ body: "Body5REC", prop: "hover01", weaps: ["Rocket-LtA-T", ] },
	{ body: "Body5REC", prop: "hover01", weaps: ["Rocket-BB",] },
	{ body: "Body5REC", prop: "tracked01", weaps: ["Cannon2A-TMk1",] },
	{ body: "Body5REC", prop: "tracked01", weaps: ["Cannon4AUTOMk1",] },
	{ body: "Body5REC", prop: "tracked01", weaps: ["Cannon375mmMk1",] },
	{ body: "Body11ABT", prop: "hover01", weaps: ["Cannon4AUTOMk1",] },
	{ body: "Body5REC", prop: "tracked01", weaps: ["Rocket-HvyA-T",] },
	{ body: "Body5REC", prop: "tracked01", weaps: ["Rocket-MRL-Hvy",] },
	{ body: "Body11ABT", prop: "tracked01", weaps: ["Cannon375mmMk1",] },
	{ body: "Body11ABT", prop: "tracked01", weaps: ["Cannon375mmMk1",] },
	{ body: "Body6SUPP", prop: "hover01", weaps: ["Cannon4AUTOMk1",] },
	{ body: "Body6SUPP", prop: "hover01", weaps: ["Rocket-HvyA-T",] },
	{ body: "Body9REC", prop: "tracked01", weaps: ["Cannon375mmMk1",] },
	{ body: "Body9REC", prop: "tracked01", weaps: ["Cannon4AUTOMk1",] },
	{ body: "Body9REC", prop: "tracked01", weaps: ["Cannon375mmMk1",] },
	{ body: "Body9REC", prop: "hover01", weaps: ["Rocket-HvyA-T",] },
	{ body: "Body2SUP", prop: "HalfTrack", weaps: ["RailGun1Mk1",] },
	{ body: "Body9REC", prop: "tracked01", weaps: ["Cannon375mmMk1",] },
	{ body: "Body6SUPP", prop: "tracked01", weaps: ["RailGun2Mk1",] },
	{ body: "Body6SUPP", prop: "hover01", weaps: ["Rocket-HvyA-T",] },
	{ body: "Body9REC", prop: "tracked01", weaps: ["Missile-A-T",] },
	{ body: "Body9REC", prop: "tracked01", weaps: ["RailGun3Mk1",] },
	{ body: "Body9REC", prop: "hover01", weaps: ["RailGun3Mk1",] },
	{ body: "Body7ABT", prop: "tracked01", weaps: ["Laser3BEAMMk1",] },
	{ body: "Body7ABT", prop: "tracked01", weaps: ["Laser2PULSEMk1",] },
	{ body: "Body7ABT", prop: "tracked01", weaps: ["RailGun2Mk1",] },
	{ body: "Body10MBT", prop: "tracked01", weaps: ["Missile-A-T",] },
	{ body: "Body7ABT", prop: "hover01", weaps: ["RailGun2Mk1",] },
	{ body: "Body7ABT", prop: "tracked01", weaps: ["Missile-A-T",] },
	{ body: "Body10MBT", prop: "tracked01", weaps: ["RailGun3Mk1",] },
	{ body: "Body10MBT", prop: "hover01", weaps: ["Missile-A-T",] },
	{ body: "Body10MBT", prop: "tracked01", weaps: ["HeavyLaser",] },
	{ body: "Body10MBT", prop: "hover01", weaps: ["RailGun3Mk1",] },
	{ body: "Body10MBT", prop: "hover01", weaps: ["Missile-MdArt",] },
	{ body: "Body14SUP", prop: "hover01", weaps: ["Missile-MdArt", "RailGun3Mk1",] },
];

const STANDARD_TRUCK_TEMPLATES = [
	{ body: "Body1REC", prop: "wheeled01", weaps: ["Spade1Mk1",] },
	{ body: "Body8MBT", prop: "hover01", weaps: ["Spade1Mk1",] },
];

const STANDARD_TANK_REPAIRS = [
	{ body: "Body1REC", prop: "wheeled01", weaps: ["LightRepair1",] },
	{ body: "Body4ABT", prop: "hover01", weaps: ["LightRepair1",] },
	{ body: "Body2SUP", prop: "hover01", weaps: ["LightRepair1",] },
	{ body: "Body6SUPP", prop: "tracked01", weaps: ["LightRepair1",] },
	{ body: "Body3MBT", prop: "hover01", weaps: ["LightRepair1",] },
];

const STANDARD_SENSOR_TEMPLATES = [
	{ body: "Body5REC", prop: "HalfTrack", weaps: ["SensorTurret1Mk1",] },
	{ body: "Body4ABT", prop: "hover01", weaps: ["SensorTurret1Mk1",] },
	{ body: "Body2SUP", prop: "hover01", weaps: ["SensorTurret1Mk1",] },
	{ body: "Body3MBT", prop: "hover01", weaps: ["SensorTurret1Mk1",] },
];

const STANDARD_LIGHT_CYBORG_TEMPLATES = [
	{ body: "CyborgLightBody", prop: "CyborgLegs", weaps: ["CyborgChaingun",] },
	{ body: "CyborgLightBody", prop: "CyborgLegs", weaps: ["CyborgCannon",] },
	{ body: "CyborgLightBody", prop: "CyborgLegs", weaps: ["CyborgFlamer01",] },
	{ body: "CyborgLightBody", prop: "CyborgLegs", weaps: ["CyborgRocket",] },
	{ body: "CyborgLightBody", prop: "CyborgLegs", weaps: ["Cyb-Wpn-Laser",] },
	{ body: "CyborgLightBody", prop: "CyborgLegs", weaps: ["Cyb-Wpn-Atmiss",] },
	{ body: "CyborgLightBody", prop: "CyborgLegs", weaps: ["Cyb-Wpn-Rail1",] },
];

const STANDARD_CYBORG_MECHANIC_TEMPLATES = [
	{ body: "CyborgLightBody", prop: "CyborgLegs", weaps: ["CyborgRepair",] },
];

const STANDARD_CYBORG_ENGINEER_TEMPLATES = [
	{ body: "CyborgLightBody", prop: "CyborgLegs", weaps: ["CyborgSpade",] },
];

const STANDARD_SUPER_CYBORG_TEMPLATES = [
	{ body: "CyborgHeavyBody", prop: "CyborgLegs", weaps: ["Cyb-Hvywpn-Mcannon",] },
	{ body: "CyborgHeavyBody", prop: "CyborgLegs", weaps: ["Cyb-Hvywpn-HPV",] },
	{ body: "CyborgHeavyBody", prop: "CyborgLegs", weaps: ["Cyb-Hvywpn-Acannon",] },
	{ body: "CyborgHeavyBody", prop: "CyborgLegs", weaps: ["Cyb-Hvywpn-TK",] },
	{ body: "CyborgHeavyBody", prop: "CyborgLegs", weaps: ["Cyb-Hvywpn-A-T",] },
	{ body: "CyborgHeavyBody", prop: "CyborgLegs", weaps: ["Cyb-Hvywpn-PulseLsr",] },
	{ body: "CyborgHeavyBody", prop: "CyborgLegs", weaps: ["Cyb-Hvywpn-RailGunner",] },
];

const STANDARD_VTOL_TEMPLATES = [
	{ body: "Body1REC", prop: "V-Tol", weaps: ["Rocket-VTOL-LtA-T",] },
	{ body: "Body1REC", prop: "V-Tol", weaps: ["Bomb1-VTOL-LtHE",] },
	{ body: "Body4ABT", prop: "V-Tol", weaps: ["Bomb1-VTOL-LtHE",] },
	{ body: "Body8MBT", prop: "V-Tol", weaps: ["Bomb2-VTOL-HvHE",] },
	{ body: "Body8MBT", prop: "V-Tol", weaps: ["Rocket-VTOL-LtA-T",] },
	{ body: "Body8MBT", prop: "V-Tol", weaps: ["Rocket-VTOL-BB",] },
	{ body: "Body7ABT", prop: "V-Tol", weaps: ["Bomb4-VTOL-HvyINC",] },
	{ body: "Body3MBT", prop: "V-Tol", weaps: ["Missile-VTOL-AT",] },
	{ body: "Body7ABT", prop: "V-Tol", weaps: ["RailGun2-VTOL",] },
	{ body: "Body7ABT", prop: "V-Tol", weaps: ["Bomb5-VTOL-Plasmite",] },
];
