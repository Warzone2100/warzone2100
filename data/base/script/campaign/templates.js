var cTempl = {
////////////////////////////////////////////////////////////////////////////////

// CAM_1A
bloke: { body: "B1BaBaPerson01", prop: "BaBaLegs", weap: "BabaMG" },
trike: { body: "B4body-sml-trike01", prop: "BaBaProp", weap: "BabaTrikeMG" },
buggy: { body: "B3body-sml-buggy01", prop: "BaBaProp", weap: "BabaBuggyMG" },
bjeep: { body: "B2JeepBody", prop: "BaBaProp", weap: "BabaJeepMG" },

// CAM_1B
bloketwin: { body: "B1BaBaPerson01", prop: "BaBaLegs", weap: "BabaTwinMG" },
triketwin: { body: "B4body-sml-trike01", prop: "BaBaProp", weap: "BabaTrikeTwinMG" },
buggytwin: { body: "B3body-sml-buggy01", prop: "BaBaProp", weap: "BabaBuggyTwinMG" },
bjeeptwin: { body: "B2JeepBody", prop: "BaBaProp", weap: "BabaJeepTwinMG" },

// SUB_1_1
blokeheavy: { body: "B1BaBaPerson01", prop: "BaBaLegs", weap: "BabaHeavyMG" },
trikeheavy: { body: "B4body-sml-trike01", prop: "BaBaProp", weap: "BabaTrikeHeavyMG" },
buggyheavy: { body: "B3body-sml-buggy01", prop: "BaBaProp", weap: "BabaBuggyHeavyMG" },
bjeepheavy: { body: "B2JeepBody", prop: "BaBaProp", weap: "BabaJeepHeavyMG" },

// SUB_1_2

// SUB_1_3
rbjeep8: { body: "B2RKJeepBody", prop: "BaBaProp", weap: "BabaRocket8" },
rbjeep: { body: "B2RKJeepBody", prop: "BaBaProp", weap: "BabaRocket" },
rbuggy: { body: "B3bodyRKbuggy01", prop: "BaBaProp", weap: "BabaRocket" },
nppod: { body: "Body4ABT", prop: "wheeled01", weap: "Rocket-Pod" },
npblc: { body: "Body4ABT", prop: "HalfTrack", weap: "Cannon1Mk1" },
nphmg: { body: "Body4ABT", prop: "HalfTrack", weap: "MG3Mk1" },
npsmc: { body: "Body8MBT", prop: "HalfTrack", weap: "Cannon2A-TMk1" },
buscan: { body: "BusBody", prop: "BaBaProp", weap: "BabaBusCannon" },
firecan: { body: "FireBody", prop: "BaBaProp", weap: "BabaBusCannon" },

// CAM_1C
npsens: { body: "Body4ABT", prop: "wheeled01", weap: "SensorTurret1Mk1" },
npslc: { body: "Body8MBT", prop: "HalfTrack", weap: "Cannon1Mk1" },
npmor: { body: "Body8MBT", prop: "HalfTrack", weap: "Mortar1Mk1" },
npsmct: { body: "Body8MBT", prop: "tracked01", weap: "Cannon2A-TMk1" },

// CAM_1CA
npmrl: { body: "Body4ABT", prop: "HalfTrack", weap: "Rocket-MRL" },
npmmct: { body: "Body12SUP", prop: "tracked01", weap: "Cannon2A-TMk1" },
npsbb: { body: "Body8MBT", prop: "HalfTrack", weap: "Rocket-BB" },
npltat: { body: "Body4ABT", prop: "HalfTrack", weap: "Rocket-LtA-T" },

// SUB_1_4A

// CAM_1_5
nphmgt: { body: "Body8MBT", prop: "tracked01", weap: "MG3Mk1" },
npcybc: { body: "CyborgLightBody", prop: "CyborgLegs", weap: "CyborgCannon" },
npcybf: { body: "CyborgLightBody", prop: "CyborgLegs", weap: "CyborgFlamer01" },
npcybm: { body: "CyborgLightBody", prop: "CyborgLegs", weap: "CyborgChaingun" },

// CAM_1AC
nphct: { body: "Body12SUP", prop: "tracked01", weap: "Cannon375mmMk1" },
npmorb: { body: "Body8MBT", prop: "HalfTrack", weap: "Mortar2Mk1" },
npmsens: { body: "Body8MBT", prop: "HalfTrack", weap: "SensorTurret1Mk1" },

// SUB_1_7

// CAM_1_D
npcybr: { body: "CyborgLightBody", prop: "CyborgLegs", weap: "CyborgRocket" },
nphmgh: { body: "Body8MBT", prop: "hover01", weap: "MG3Mk1" },
npltath: { body: "Body8MBT", prop: "hover01", weap: "Rocket-LtA-T" },
nphch: { body: "Body12SUP", prop: "hover01", weap: "Cannon375mmMk1" },

// CAM_2_A
commgt: { body: "Body6SUPP", prop: "tracked01", weap: "MG3Mk1" },
comsens: { body: "Body6SUPP", prop: "tracked01", weap: "SensorTurret1Mk1" },
cohct: { body: "Body9REC", prop: "tracked01", weap: "Cannon375mmMk1" },
commrl: { body: "Body6SUPP", prop: "HalfTrack", weap: "Rocket-MRL" },
commrp: { body: "Body6SUPP", prop: "tracked01", weap: "Rocket-Pod" },
comorb: { body: "Body6SUPP", prop: "HalfTrack", weap: "Mortar2Mk1" },
colcbv: { body: "Body2SUP", prop: "V-Tol", weap: "Bomb1-VTOL-LtHE" },
prhct: { body: "Body11ABT", prop: "tracked01", weap: "Cannon375mmMk1" },
prltat: { body: "Body5REC", prop: "tracked01", weap: "Rocket-LtA-T" },
prrept: { body: "Body5REC", prop: "tracked01", weap: "LightRepair1" },

// SUB_2_1

// CAM_2_B
comatt: { body: "Body6SUPP", prop: "tracked01", weap: "Rocket-LtA-T" },
comit: { body: "Body6SUPP", prop: "tracked01", weap: "Flame2" },
colatv: { body: "Body2SUP", prop: "V-Tol", weap: "Rocket-VTOL-LtA-T" },

// SUB_2_2
comtath: { body: "Body6SUPP", prop: "hover01", weap: "Rocket-LtA-T" },
comtathh: { body: "Body6SUPP", prop: "HalfTrack", weap: "Rocket-LtA-T" },

// CAM_2_C
commorv: { body: "Body6SUPP", prop: "V-Tol", weap: "Bomb2-VTOL-HvHE" },
colagv: { body: "Body2SUP", prop: "V-Tol", weap: "MG4ROTARY-VTOL" },
comhpv: { body: "Body6SUPP", prop: "tracked01", weap: "Cannon4AUTOMk1" },
cohbbt: { body: "Body9REC", prop: "tracked01", weap: "Rocket-BB" },

// SUB_2_5
cocybag: { body: "CyborgLightBody", prop: "CyborgLegs", weap: "CyborgRotMG" },

// SUB_2_D
comhltat: { body: "Body6SUPP", prop: "tracked01", weap: "Rocket-HvyA-T" },
commorvt: { body: "Body6SUPP", prop: "V-Tol", weap: "Bomb4-VTOL-HvyINC" },
cohhpv: { body: "Body9REC", prop: "tracked01", weap: "Cannon4AUTOMk1" },
comagt: { body: "Body6SUPP", prop: "tracked01", weap: "MG4ROTARYMk1" },

// SUB_2_6
cohact: { body: "Body9REC", prop: "tracked01", weap: "Cannon5VulcanMk1" },
comrotm: { body: "Body6SUPP", prop: "HalfTrack", weap: "Mortar3ROTARYMk1" },
comsensh: { body: "Body6SUPP", prop: "HalfTrack", weap: "SensorTurret1Mk1" },

// SUB_2_7
comrotmh: { body: "Body6SUPP", prop: "tracked01", weap: "Mortar3ROTARYMk1" },

// SUB_2_8
comhvat: { body: "Body6SUPP", prop: "V-Tol", weap: "Rocket-VTOL-HvyA-T" },

// CAM_3_A
nxmscouh: { body: "Body7ABT", prop: "hover02", weap: "Missile-A-T" },
nxcyrail: { body: "CybNXRail1Jmp", prop: "CyborgLegs02", weap: "NX-Cyb-Rail1" },
nxcyscou: { body: "CybNXMissJmp", prop: "CyborgLegs02", weap: "NX-CyborgMiss" },
nxlneedv: { body: "Body3MBT", prop: "V-Tol02", weap: "RailGun1-VTOL" },
nxlscouv: { body: "Body3MBT", prop: "V-Tol02", weap: "Missile-VTOL-AT" },
nxmtherv: { body: "Body7ABT", prop: "V-Tol02", weap: "Bomb4-VTOL-HvyINC" },
prhasgnt: { body: "Body11ABT", prop: "tracked01", weap: "MG4ROTARYMk1" },
prhhpvt: { body: "Body11ABT", prop: "tracked01", weap: "Cannon4AUTOMk1" },
prhaacnt: { body: "Body11ABT", prop: "tracked01", weap: "AAGun2Mk1" },
prtruck: { body: "Body5REC", prop: "tracked01", weap: "Spade1Mk1" },

// SUB_3_1
nxcylas: { body: "CybNXPulseLasJmp", prop: "CyborgLegs02", weap: "NX-CyborgPulseLas" },
nxmrailh: { body: "Body7ABT", prop: "hover02", weap: "RailGun2Mk1" },

// CAM_3_B
nxmlinkh: { body: "Body7ABT", prop: "hover02", weap: "NEXUSlink" },
nxmsamh: { body: "Body7ABT", prop: "hover02", weap: "Missile-HvySAM" },
nxmheapv: { body: "Body7ABT", prop: "V-Tol02", weap: "Bomb2-VTOL-HvHE" },

// SUB_3_2
nxlflash: { body: "Body3MBT", prop: "hover02", weap: "Laser3BEAMMk1" },

// CAM_3_A_B
nxmsens: { body: "Body7ABT", prop: "hover02", weap: "SensorTurret1Mk1" },
nxmangel: { body: "Body7ABT", prop: "hover02", weap: "Missile-MdArt" },

// CAM_3_C

// CAM_3_A_D_1
nxmpulseh: { body: "Body7ABT", prop: "hover02", weap: "Laser2PULSEMk1" },

// CAM_3_A_D_2
nxhgauss: { body: "Body10MBT", prop: "hover02", weap: "RailGun3Mk1" },
nxlpulsev: { body: "Body3MBT", prop: "V-Tol02", weap: "Laser2PULSE-VTOL" },

// SUB_3_4
nxllinkh: { body: "Body3MBT", prop: "hover02", weap: "NEXUSlink" },
nxmpulsev: { body: "Body7ABT", prop: "V-Tol02", weap: "Laser2PULSE-VTOL" },


////////////////////////////////////////////////////////////////////////////////
};
