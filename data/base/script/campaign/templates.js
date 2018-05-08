var cTempl = {
////////////////////////////////////////////////////////////////////////////////

// CAM_1A
bloke: { body: "B1BaBaPerson01", prop: "BaBaLegs", weap: "BabaMG" },
trike: { body: "B4body-sml-trike01", prop: "BaBaProp", weap: "BabaTrikeMG" },
buggy: { body: "B3body-sml-buggy01", prop: "BaBaProp", weap: "BabaBuggyMG" },
bjeep: { body: "B2JeepBody", prop: "BaBaProp", weap: "BabaJeepMG" },

// SUB_1_2
rbjeep: { body: "B2RKJeepBody", prop: "BaBaProp", weap: "BabaRocket" },
rbuggy: { body: "B3bodyRKbuggy01", prop: "BaBaProp", weap: "BabaRocket" },

// SUB_1_3
nppod: { body: "Body4ABT", prop: "wheeled01", weap: "Rocket-Pod" },
nphmg: { body: "Body4ABT", prop: "HalfTrack", weap: "MG3Mk1" },
npsmc: { body: "Body8MBT", prop: "HalfTrack", weap: "Cannon2A-TMk1" },

// CAM_1C
npsens: { body: "Body4ABT", prop: "wheeled01", weap: "SensorTurret1Mk1" },
buscan: { body: "BusBody", prop: "BaBaProp", weap: "BabaBusCannon" },
firecan: { body: "FireBody", prop: "BaBaProp", weap: "BabaBusCannon" },
npslc: { body: "Body8MBT", prop: "HalfTrack", weap: "Cannon1Mk1" },
npmor: { body: "Body8MBT", prop: "HalfTrack", weap: "Mortar1Mk1" },
npsmct: { body: "Body8MBT", prop: "tracked01", weap: "Cannon2A-TMk1" },
nptruck: { body: "Body8MBT", prop: "HalfTrack", weap: "Spade1Mk1" },

// CAM_1CA
npmrl: { body: "Body4ABT", prop: "HalfTrack", weap: "Rocket-MRL" },
npmmct: { body: "Body12SUP", prop: "tracked01", weap: "Cannon2A-TMk1" },
npsbb: { body: "Body8MBT", prop: "HalfTrack", weap: "Rocket-BB" },

// CAM_1_5
npcybc: { body: "CyborgLightBody", prop: "CyborgLegs", weap: "CyborgCannon" },
npcybf: { body: "CyborgLightBody", prop: "CyborgLegs", weap: "CyborgFlamer01" },
npcybm: { body: "CyborgLightBody", prop: "CyborgLegs", weap: "CyborgChaingun" },

// CAM_1AC
nphct: { body: "Body12SUP", prop: "tracked01", weap: "Cannon375mmMk1" },
npmorb: { body: "Body8MBT", prop: "HalfTrack", weap: "Mortar2Mk1" },
npmsens: { body: "Body8MBT", prop: "HalfTrack", weap: "SensorTurret1Mk1" },

// CAM_1_D
npcybr: { body: "CyborgLightBody", prop: "CyborgLegs", weap: "CyborgRocket" },
npltat: { body: "Body4ABT", prop: "HalfTrack", weap: "Rocket-LtA-T" },
nphmgh: { body: "Body8MBT", prop: "hover01", weap: "MG3Mk1" },
npltath: { body: "Body8MBT", prop: "hover01", weap: "Rocket-LtA-T" },
nphch: { body: "Body12SUP", prop: "hover01", weap: "Cannon375mmMk1" },

// CAM_2_A
commgt: { body: "Body6SUPP", prop: "tracked01", weap: "MG3Mk1" },
comsens: { body: "Body6SUPP", prop: "tracked01", weap: "SensorTurret1Mk1" },
cohct: { body: "Body9REC", prop: "tracked01", weap: "Cannon375mmMk1" },
comct: { body: "Body6SUPP", prop: "tracked01", weap: "Cannon2A-TMk1" },
comorb: { body: "Body6SUPP", prop: "HalfTrack", weap: "Mortar2Mk1" },
colcbv: { body: "Body2SUP", prop: "V-Tol", weap: "Bomb1-VTOL-LtHE" },
colatv: { body: "Body2SUP", prop: "V-Tol", weap: "Rocket-VTOL-LtA-T" },
prtruck: { body: "Body5REC", prop: "tracked01", weap: "Spade1Mk1" },
prhct: { body: "Body11ABT", prop: "tracked01", weap: "Cannon375mmMk1" },
prltat: { body: "Body5REC", prop: "tracked01", weap: "Rocket-LtA-T" },
prrept: { body: "Body5REC", prop: "tracked01", weap: "LightRepair1" },

// CAM_2_B
cotruck: { body: "Body6SUPP", prop: "tracked01", weap: "Spade1Mk1" },
comatt: { body: "Body6SUPP", prop: "tracked01", weap: "Rocket-LtA-T" },
comit: { body: "Body6SUPP", prop: "tracked01", weap: "Flame2" },
comrept: { body: "Body6SUPP", prop: "tracked01", weap: "LightRepair1" },
comorbt: { body: "Body6SUPP", prop: "tracked01", weap: "Mortar2Mk1" },

// CAM_2_2
comtath: { body: "Body6SUPP", prop: "hover01", weap: "Rocket-LtA-T" },
comtathh: { body: "Body6SUPP", prop: "HalfTrack", weap: "Rocket-LtA-T" },
comih: { body: "Body6SUPP", prop: "hover01", weap: "Flame2" },

// CAM_2_C
commorv: { body: "Body6SUPP", prop: "V-Tol", weap: "Bomb2-VTOL-HvHE" },
colagv: { body: "Body2SUP", prop: "V-Tol", weap: "MG4ROTARY-VTOL" },
comhpv: { body: "Body6SUPP", prop: "tracked01", weap: "Cannon4AUTOMk1" },
cohbbt: { body: "Body9REC", prop: "tracked01", weap: "Rocket-BB" },
cohhot: { body: "Body9REC", prop: "tracked01", weap: "Howitzer105Mk1" },

// CAM_2_5
cohhpv: { body: "Body9REC", prop: "tracked01", weap: "Cannon4AUTOMk1" },
comagt: { body: "Body6SUPP", prop: "tracked01", weap: "MG4ROTARYMk1" },
cocybag: { body: "CyborgLightBody", prop: "CyborgLegs", weap: "CyborgRotMG" },
cohaaq: { body: "Body9REC", prop: "tracked01", weap: "QuadRotAAGun" },

// CAM_2_D
comhltat: { body: "Body6SUPP", prop: "tracked01", weap: "Rocket-HvyA-T" },

// CAM_2_6
cohact: { body: "Body9REC", prop: "tracked01", weap: "Cannon5VulcanMk1" },
comrotm: { body: "Body6SUPP", prop: "HalfTrack", weap: "Mortar3ROTARYMk1" },
comsensh: { body: "Body6SUPP", prop: "HalfTrack", weap: "SensorTurret1Mk1" },

// CAM_2_7
comrotmh: { body: "Body6SUPP", prop: "tracked01", weap: "Mortar3ROTARYMk1" },

// CAM_2_8
comhvat: { body: "Body6SUPP", prop: "V-Tol", weap: "Rocket-VTOL-HvyA-T" },

// CAM_3_A
nxtruckh: { body: "Body7ABT", prop: "hover02", weap: "Spade1Mk1" },
nxmserh: { body: "Body7ABT", prop: "hover02", weap: "Missile-MdArt" },
nxmreph: { body: "Body7ABT", prop: "hover02", weap: "LightRepair1" },
nxlsensh: { body: "Body3MBT", prop: "hover02", weap: "SensorTurret1Mk1" },
nxmrailh: { body: "Body7ABT", prop: "hover02", weap: "RailGun2Mk1" },
nxmscouh: { body: "Body7ABT", prop: "hover02", weap: "Missile-A-T" },
nxcyrail: { body: "CyborgLightBody", prop: "CyborgLegs02", weap: "Cyb-Wpn-Rail1" },
nxcyscou: { body: "CyborgLightBody", prop: "CyborgLegs02", weap: "Cyb-Wpn-Atmiss" },
nxlneedv: { body: "Body3MBT", prop: "V-Tol02", weap: "RailGun1-VTOL" },
nxlscouv: { body: "Body3MBT", prop: "V-Tol02", weap: "Missile-VTOL-AT" },
nxmtherv: { body: "Body7ABT", prop: "V-Tol02", weap: "Bomb4-VTOL-HvyINC" },
prhasgnt: { body: "Body11ABT", prop: "tracked01", weap: "MG4ROTARYMk1" },
prhhpvt: { body: "Body11ABT", prop: "tracked01", weap: "Cannon4AUTOMk1" },
prhaacnt: { body: "Body11ABT", prop: "tracked01", weap: "AAGun2Mk1" },

// CAM_3_1
nxmcommh: { body: "Body7ABT", prop: "hover02", weap: "CommandTurret1" },
nxcylas: { body: "CyborgLightBody", prop: "CyborgLegs02", weap: "Cyb-Wpn-Laser" },

// CAM_3_B
nxmlinkh: { body: "Body7ABT", prop: "hover02", weap: "NEXUSlink" },
nxmsamh: { body: "Body7ABT", prop: "hover02", weap: "Missile-HvySAM" },
nxmheapv: { body: "Body7ABT", prop: "V-Tol02", weap: "Bomb2-VTOL-HvHE" },

// CAM_3_2
nxlflash: { body: "Body3MBT", prop: "hover02", weap: "Laser3BEAMMk1" },

// CAM_3_A_B
nxmstrike: { body: "Body7ABT", prop: "hover02", weap: "Sys-VstrikeTurret01" },

// CAM_3_A_D_1
nxmpulseh: { body: "Body7ABT", prop: "hover02", weap: "Laser2PULSEMk1" },
nxlpulsev: { body: "Body3MBT", prop: "V-Tol02", weap: "Laser2PULSE-VTOL" },

// CAM_3_A_D_2
nxhgauss: { body: "Body10MBT", prop: "hover02", weap: "RailGun3Mk1" },
nxhrailv: { body: "Body10MBT", prop: "V-Tol02", weap: "RailGun2-VTOL" },

// CAM_3_4
nxllinkh: { body: "Body3MBT", prop: "hover02", weap: "NEXUSlink" },
nxmpulsev: { body: "Body7ABT", prop: "V-Tol02", weap: "Laser2PULSE-VTOL" },


////////////////////////////////////////////////////////////////////////////////
};
