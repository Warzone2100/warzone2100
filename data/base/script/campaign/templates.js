
const tBody = {
    scav: {
        //Human
        human: "B1BaBaPerson01",
        //Scavenger vehicle
        trike: "B4body-sml-trike01",
        buggy: "B3body-sml-buggy01",
        jeep: "B2JeepBody",
        rocketJeep: "B2RKJeepBody",
        rocketBuggy: "B3bodyRKbuggy01",
        bus: "BusBody",
        fireTruck: "FireBody",
    },
    tank: {
        //Light
        viper: "Body1REC",
        bug: "Body4ABT",
        leopard: "Body2SUP",
        retaliation: "Body3MBT",
        //Medium
        cobra: "Body5REC",
        scorpion: "Body8MBT",
        panther: "Body6SUPP",
        retribution: "Body7ABT",
        //Heavy
        python: "Body11ABT",
        mantis: "Body12SUP",
        tiger: "Body9REC",
        vengeance: "Body10MBT",
    },
    cyborg: {
        //Light cyborg
        lightBody: "CyborgLightBody",
        //Nexus light cyborg
        nexusRailBody: "CybNXRail1Jmp",
        nexusScourgeBody: "CybNXMissJmp",
        nexusLaserBody: "CybNXPulseLasJmp",
    }
};
const tProp = {
    scav: {
        //Scavenger Legs
        legs: "BaBaLegs",
        //Scavenger Vehicle
        vehicle: "BaBaProp",
    },
    tank: {
        //Wheels
        wheels: "wheeled01",
        wheels2: "wheeled02",
        wheels3: "wheeled03",
        //HalfTrack
        halfTracks: "HalfTrack",
        halfTracks2: "HalfTrack02",
        halfTracks3: "HalfTrack03",
        //Tracks
        tracks: "tracked01",
        tracks2: "tracked02",
        tracks3: "tracked03",
        //Hover
        hover: "hover01",
        hover2: "hover02",
        hover3: "hover03",
    },
    air: {
        //VTOL
        vtol: "V-Tol",
        vtol2: "V-Tol02",
        vtol3: "V-Tol03",
    },
    cyborg: {
        //Cyborg Legs
        legs: "CyborgLegs",
        legs2: "CyborgLegs02",
        legs3: "CyborgLegs03",
    }
};
const tWeap = {
    scav: {
        //MG
        machinegun: "BabaMG",
        twinMachinegun: "BabaTwinMG",
        heavyMachinegun: "BabaHeavyMG",
        trikeMachinegun: "BabaTrikeMG",
        trikeTwinMachinegun: "BabaTrikeTwinMG",
        trikeHeavyMachinegun: "BabaTrikeHeavyMG",
        buggyMachinegun: "BabaBuggyMG",
        buggyTwinMachinegun: "BabaBuggyTwinMG",
        buggyHeavyMachinegun: "BabaBuggyHeavyMG",
        jeepMachinegun: "BabaJeepMG",
        jeepTwinMachinegun: "BabaJeepTwinMG",
        jeepHeavyMachinegun: "BabaJeepHeavyMG",
        //cannon
        busCannon: "BabaBusCannon",
        //rocket
        miniRocketPod: "BabaRocket",
        //artillery
        miniRocketArray: "BabaRocket8",
    },
    tank: {
        //MG
        machinegun: "MG1Mk1",
        twinMachinegun: "MG2Mk1",
        heavyMachinegun: "MG3Mk1",
        assaultGun: "MG4ROTARYMk1",
        //Flamer
        flamer: "Flame1Mk1",
        inferno: "Flame2",
        //Cannon
        lightCannon: "Cannon1Mk1",
        mediumCannon: "Cannon2A-TMk1",
        heavyCannon: "Cannon375mmMk1",
        hyperVelocityCannon: "Cannon4AUTOMk1",
        assaultCannon: "Cannon5VulcanMk1",
        //Rocket
        miniRocketPod: "Rocket-Pod",
        lancer: "Rocket-LtA-T",
        tankKiller: "Rocket-HvyA-T",
        bunkerBuster: "Rocket-BB",
        //Artillery
        mortar: "Mortar1Mk1",
        bombard: "Mortar2Mk1",
        pepperpot: "Mortar3ROTARYMk1",
        miniRocketArray: "Rocket-MRL",
        rippleRocket: "Rocket-IDF",
        howitzer: "Howitzer105Mk1",
        groundShaker: "Howitzer150Mk1",
        hellstorm: "Howitzer03-Rot",
        angel: "Missile-MdArt",
        archAngel: "Missile-HvyArt",
        heavyPlasmaLauncher: "PlasmaHeavy",
        //Anti-Air
        hurricane: "QuadMg1AAGun",
        cyclone: "AAGun2Mk1",
        whirlwind: "QuadRotAAGun",
        avenger: "Missile-LtSAM",
        vindicator: "Missile-HvySAM",
        //Laser
        flashlight: "Laser3BEAMMk1",
        pulseLaser: "Laser2PULSEMk1",
        //Rail
        needle: "RailGun1Mk1",
        railGun: "RailGun2Mk1",
        gaussCannon: "RailGun3Mk1",
        //Missile
        scourgeMissile: "Missile-A-T",
        //Electronic
        nexusLink: "NEXUSlink",
    },
    cyborg: {
        //MG
        heavyMachinegun: "CyborgChaingun",
        assaultGun: "CyborgRotMG",
        //Flamer
        flamer: "CyborgFlamer01",
        thermite: "CyborgFlamer02",
        //Cannon
        heavyCannon: "CyborgCannon",
        sniperCannon: "CyborgCannon02",
        //Rocket
        lancer: "CyborgRocket",
        tankKiller: "CyborgRocket02",
        //Laser
        flashlight: "Cyb-Wpn-Laser",
        nexusFlashlight: "NX-CyborgPulseLas",
        //Rail
        needle: "Cyb-Wpn-Rail1",
        nexusNeedle: "NX-Cyb-Rail1",
        //Missile
        scourgeMissile: "Cyb-Wpn-Atmiss",
        nexusScourgeMissile: "NX-CyborgMiss",
    },
    air: {
        //MG
        machinegun: "MG1-VTOL",
        twinMachinegun: "MG2-VTOL",
        heavyMachinegun: "MG3-VTOL",
        assaultGun: "MG4ROTARY-VTOL",
        //Cannon
        lightCannon: "Cannon1-VTOL",
        heavyCannon: "Cannon375mm-VTOL",
        hyperVelocityCannon: "Cannon4AUTO-VTOL",
        assaultCannon: "Cannon5Vulcan-VTOL",
        //Rocket
        miniRocketPod: "Rocket-VTOL-Pod",
        lancer: "Rocket-VTOL-LtA-T",
        tankKiller: "Rocket-VTOL-HvyA-T",
        bunkerBuster: "Rocket-VTOL-BB",
        //Bomb
        clusterBomb: "Bomb1-VTOL-LtHE",
        heapBomb: "Bomb2-VTOL-HvHE",
        phosphorBomb: "Bomb3-VTOL-LtINC",
        thermiteBomb: "Bomb4-VTOL-HvyINC",
        //Laser
        flashlight: "Laser3BEAM-VTOL",
        pulseLaser: "Laser2PULSE-VTOL",
        //Rail
        needle: "RailGun1-VTOL",
        railGun: "RailGun2-VTOL",
        //Missile
        scourgeMissile: "Missile-VTOL-AT",
    }
};
const tSensor = {
    sensor: "SensorTurret1Mk1",
    counterBattery: "Sys-CBTurret01",
    vtolStrike: "Sys-VstrikeTurret01",
    vtolCounterBattery: "Sys-VTOLCBTurret01",
};
const tRepair = {
    lightRepair: "LightRepair1",
};
const tConstruct = {
    truck: "Spade1Mk1",
};
const tCommand = {
    commander: "CommandBrain01",
};

const cTempl = {
////////////////////////////////////////////////////////////////////////////////

// CAM_1A
bloke: { body: tBody.scav.human, prop: tProp.scav.legs, weap: tWeap.scav.machinegun },
trike: { body: tBody.scav.trike, prop: tProp.scav.vehicle, weap: tWeap.scav.trikeMachinegun },
buggy: { body: tBody.scav.buggy, prop: tProp.scav.vehicle, weap: tWeap.scav.buggyMachinegun },
bjeep: { body: tBody.scav.jeep, prop: tProp.scav.vehicle, weap: tWeap.scav.jeepMachinegun },

// CAM_1B
bloketwin: { body: tBody.scav.human, prop: tProp.scav.legs, weap: tWeap.scav.twinMachinegun },
triketwin: { body: tBody.scav.trike, prop: tProp.scav.vehicle, weap: tWeap.scav.trikeTwinMachinegun },
buggytwin: { body: tBody.scav.buggy, prop: tProp.scav.vehicle, weap: tWeap.scav.buggyTwinMachinegun },
bjeeptwin: { body: tBody.scav.jeep, prop: tProp.scav.vehicle, weap: tWeap.scav.jeepTwinMachinegun },

// SUB_1_1
blokeheavy: { body: tBody.scav.human, prop: tProp.scav.legs, weap: tWeap.scav.heavyMachinegun },
trikeheavy: { body: tBody.scav.trike, prop: tProp.scav.vehicle, weap: tWeap.scav.trikeHeavyMachinegun },
buggyheavy: { body: tBody.scav.buggy, prop: tProp.scav.vehicle, weap: tWeap.scav.buggyHeavyMachinegun },
bjeepheavy: { body: tBody.scav.jeep, prop: tProp.scav.vehicle, weap: tWeap.scav.jeepHeavyMachinegun },

// SUB_1_2

// SUB_1_3
rbjeep8: { body: tBody.scav.rocketJeep, prop: tProp.scav.vehicle, weap: tWeap.scav.miniRocketArray },
rbjeep: { body: tBody.scav.rocketJeep, prop: tProp.scav.vehicle, weap: tWeap.scav.miniRocketPod },
rbuggy: { body: tBody.scav.rocketBuggy, prop: tProp.scav.vehicle, weap: tWeap.scav.miniRocketPod },
nppod: { body: tBody.tank.bug, prop: tProp.tank.wheels, weap: tWeap.tank.miniRocketPod },
npblc: { body: tBody.tank.bug, prop: tProp.tank.halfTracks, weap: tWeap.tank.lightCannon },
nphmg: { body: tBody.tank.bug, prop: tProp.tank.halfTracks, weap: tWeap.tank.heavyMachinegun },
npsmc: { body: tBody.tank.scorpion, prop: tProp.tank.halfTracks, weap: tWeap.tank.mediumCannon },
buscan: { body: tBody.scav.bus, prop: tProp.scav.vehicle, weap: tWeap.scav.busCannon },
firecan: { body: tBody.scav.fireTruck, prop: tProp.scav.vehicle, weap: tWeap.scav.busCannon },

// CAM_1C
npsens: { body: tBody.tank.bug, prop: tProp.tank.wheels, weap: tSensor.sensor },
npslc: { body: tBody.tank.scorpion, prop: tProp.tank.halfTracks, weap: tWeap.tank.lightCannon },
npmor: { body: tBody.tank.scorpion, prop: tProp.tank.halfTracks, weap: tWeap.tank.mortar },
npsmct: { body: tBody.tank.scorpion, prop: tProp.tank.tracks, weap: tWeap.tank.mediumCannon },

// CAM_1CA
npmrl: { body: tBody.tank.bug, prop: tProp.tank.halfTracks, weap: tWeap.tank.miniRocketArray },
npmmct: { body: tBody.tank.mantis, prop: tProp.tank.tracks, weap: tWeap.tank.mediumCannon },
npsbb: { body: tBody.tank.scorpion, prop: tProp.tank.halfTracks, weap: tWeap.tank.bunkerBuster },
npltat: { body: tBody.tank.bug, prop: tProp.tank.halfTracks, weap: tWeap.tank.lancer },

// SUB_1_4A
npmorb: { body: tBody.tank.scorpion, prop: tProp.tank.halfTracks, weap: tWeap.tank.bombard },

// CAM_1_5
nphmgt: { body: tBody.tank.scorpion, prop: tProp.tank.tracks, weap: tWeap.tank.heavyMachinegun },
npcybc: { body: tBody.cyborg.lightBody, prop: tProp.cyborg.legs, weap: tWeap.cyborg.heavyCannon },
npcybf: { body: tBody.cyborg.lightBody, prop: tProp.cyborg.legs, weap: tWeap.cyborg.flamer },
npcybm: { body: tBody.cyborg.lightBody, prop: tProp.cyborg.legs, weap: tWeap.cyborg.heavyMachinegun },

// CAM_1AC
nphct: { body: tBody.tank.mantis, prop: tProp.tank.tracks, weap: tWeap.tank.heavyCannon },
npmsens: { body: tBody.tank.scorpion, prop: tProp.tank.halfTracks, weap: tSensor.sensor },

// SUB_1_7

// CAM_1_D
npcybr: { body: tBody.cyborg.lightBody, prop: tProp.cyborg.legs, weap: tWeap.cyborg.lancer },
nphmgh: { body: tBody.tank.scorpion, prop: tProp.tank.hover, weap: tWeap.tank.heavyMachinegun },
npltath: { body: tBody.tank.scorpion, prop: tProp.tank.hover, weap: tWeap.tank.lancer },
nphch: { body: tBody.tank.mantis, prop: tProp.tank.hover, weap: tWeap.tank.heavyCannon },
nphbb: { body: tBody.tank.mantis, prop: tProp.tank.hover, weap: tWeap.tank.bunkerBuster },

// CAM_2_A
commgt: { body: tBody.tank.panther, prop: tProp.tank.tracks, weap: tWeap.tank.heavyMachinegun },
comsens: { body: tBody.tank.panther, prop: tProp.tank.tracks, weap: tSensor.sensor },
cohct: { body: tBody.tank.tiger, prop: tProp.tank.tracks, weap: tWeap.tank.heavyCannon },
commc: { body: tBody.tank.panther, prop: tProp.tank.tracks, weap: tWeap.tank.mediumCannon },
commrl: { body: tBody.tank.panther, prop: tProp.tank.halfTracks, weap: tWeap.tank.miniRocketArray },
commrp: { body: tBody.tank.panther, prop: tProp.tank.tracks, weap: tWeap.tank.miniRocketPod },
comorb: { body: tBody.tank.panther, prop: tProp.tank.halfTracks, weap: tWeap.tank.bombard },
colcbv: { body: tBody.tank.leopard, prop: tProp.air.vtol, weap: tWeap.air.clusterBomb },
colpbv: { body: tBody.tank.leopard, prop: tProp.air.vtol, weap: tWeap.air.phosphorBomb },
colatv: { body: tBody.tank.leopard, prop: tProp.air.vtol, weap: tWeap.air.lancer },
prhct: { body: tBody.tank.python, prop: tProp.tank.tracks, weap: tWeap.tank.heavyCannon },
prltat: { body: tBody.tank.cobra, prop: tProp.tank.tracks, weap: tWeap.tank.lancer },
prrept: { body: tBody.tank.cobra, prop: tProp.tank.tracks, weap: tRepair.lightRepair },

// SUB_2_1

// CAM_2_B
comatt: { body: tBody.tank.panther, prop: tProp.tank.tracks, weap: tWeap.tank.lancer },
comit: { body: tBody.tank.panther, prop: tProp.tank.tracks, weap: tWeap.tank.inferno },
cocybth: { body: tBody.cyborg.lightBody, prop: tProp.cyborg.legs, weap: tWeap.cyborg.thermite },

// SUB_2_2
comtath: { body: tBody.tank.panther, prop: tProp.tank.hover, weap: tWeap.tank.lancer },
comtathh: { body: tBody.tank.panther, prop: tProp.tank.halfTracks, weap: tWeap.tank.lancer },
cohhch: { body: tBody.tank.tiger, prop: tProp.tank.hover, weap: tWeap.tank.heavyCannon },

// CAM_2_C
comhcv: { body: tBody.tank.panther, prop: tProp.air.vtol, weap: tWeap.air.heavyCannon },
commorv: { body: tBody.tank.panther, prop: tProp.air.vtol, weap: tWeap.air.heapBomb },
colagv: { body: tBody.tank.leopard, prop: tProp.air.vtol, weap: tWeap.air.assaultGun },
comhpv: { body: tBody.tank.panther, prop: tProp.tank.tracks, weap: tWeap.tank.hyperVelocityCannon },
cohbbt: { body: tBody.tank.tiger, prop: tProp.tank.tracks, weap: tWeap.tank.bunkerBuster },

// SUB_2_5
cocybag: { body: tBody.cyborg.lightBody, prop: tProp.cyborg.legs, weap: tWeap.cyborg.assaultGun },
cocybsn: { body: tBody.cyborg.lightBody, prop: tProp.cyborg.legs, weap: tWeap.cyborg.sniperCannon },
cocybtk: { body: tBody.cyborg.lightBody, prop: tProp.cyborg.legs, weap: tWeap.cyborg.tankKiller },
comhltat: { body: tBody.tank.panther, prop: tProp.tank.tracks, weap: tWeap.tank.tankKiller },
cohhvch: { body: tBody.tank.tiger, prop: tProp.tank.hover, weap: tWeap.tank.hyperVelocityCannon },
comagh: { body: tBody.tank.panther, prop: tProp.tank.hover, weap: tWeap.tank.assaultGun },
comhvcv: { body: tBody.tank.panther, prop: tProp.air.vtol, weap: tWeap.air.hyperVelocityCannon },

// SUB_2_D
commorvt: { body: tBody.tank.panther, prop: tProp.air.vtol, weap: tWeap.air.thermiteBomb },
cohhpv: { body: tBody.tank.tiger, prop: tProp.tank.tracks, weap: tWeap.tank.hyperVelocityCannon },
comagt: { body: tBody.tank.panther, prop: tProp.tank.tracks, weap: tWeap.tank.assaultGun },
colhvat: { body: tBody.tank.leopard, prop: tProp.air.vtol, weap: tWeap.air.tankKiller },

// SUB_2_6
cohact: { body: tBody.tank.tiger, prop: tProp.tank.tracks, weap: tWeap.tank.assaultCannon },
comrotm: { body: tBody.tank.panther, prop: tProp.tank.halfTracks, weap: tWeap.tank.pepperpot },
comsensh: { body: tBody.tank.panther, prop: tProp.tank.halfTracks, weap: tSensor.sensor },

// SUB_2_7
comrotmh: { body: tBody.tank.panther, prop: tProp.tank.tracks, weap: tWeap.tank.pepperpot },
comltath: { body: tBody.tank.panther, prop: tProp.tank.hover, weap: tWeap.tank.tankKiller },
comacv: { body: tBody.tank.panther, prop: tProp.air.vtol, weap: tWeap.air.assaultCannon },
cohach: { body: tBody.tank.tiger, prop: tProp.tank.hover, weap: tWeap.tank.assaultCannon },

// SUB_2_8
comhvat: { body: tBody.tank.panther, prop: tProp.air.vtol, weap: tWeap.air.tankKiller },

// CAM_2_END
cowwt: { body: tBody.tank.tiger, prop: tProp.tank.tracks, weap: tWeap.tank.whirlwind },

// CAM_3_A
nxmscouh: { body: tBody.tank.retribution, prop: tProp.tank.hover2, weap: tWeap.tank.scourgeMissile },
nxcyrail: { body: tBody.cyborg.nexusRailBody, prop: tProp.cyborg.legs2, weap: tWeap.cyborg.nexusNeedle },
nxcyscou: { body: tBody.cyborg.nexusScourgeBody, prop: tProp.cyborg.legs2, weap: tWeap.cyborg.nexusScourgeMissile },
nxlneedv: { body: tBody.tank.retaliation, prop: tProp.air.vtol2, weap: tWeap.air.needle },
nxlscouv: { body: tBody.tank.retaliation, prop: tProp.air.vtol2, weap: tWeap.air.scourgeMissile },
nxmtherv: { body: tBody.tank.retribution, prop: tProp.air.vtol2, weap: tWeap.air.thermiteBomb },
prhasgnt: { body: tBody.tank.python, prop: tProp.tank.tracks, weap: tWeap.tank.assaultGun },
prhhpvt: { body: tBody.tank.python, prop: tProp.tank.tracks, weap: tWeap.tank.hyperVelocityCannon },
prhaawwt: { body: tBody.tank.python, prop: tProp.tank.tracks, weap: tWeap.tank.whirlwind },
prtruck: { body: tBody.tank.cobra, prop: tProp.tank.tracks, weap: tConstruct.truck },

// SUB_3_1
nxcylas: { body: tBody.cyborg.nexusLaserBody, prop: tProp.cyborg.legs2, weap: tWeap.cyborg.nexusFlashlight },
nxmrailh: { body: tBody.tank.retribution, prop: tProp.tank.hover2, weap: tWeap.tank.railGun },

// CAM_3_B
nxmlinkh: { body: tBody.tank.retribution, prop: tProp.tank.hover2, weap: tWeap.tank.nexusLink },
nxmsamh: { body: tBody.tank.retribution, prop: tProp.tank.hover2, weap: tWeap.tank.vindicator },
nxmheapv: { body: tBody.tank.retribution, prop: tProp.air.vtol2, weap: tWeap.air.heapBomb },

// SUB_3_2
nxlflash: { body: tBody.tank.retaliation, prop: tProp.tank.hover2, weap: tWeap.tank.flashlight },

// CAM_3_A_B
nxmsens: { body: tBody.tank.retribution, prop: tProp.tank.hover2, weap: tSensor.sensor },
nxmangel: { body: tBody.tank.retribution, prop: tProp.tank.hover2, weap: tWeap.tank.angel },

// CAM_3_C

// CAM_3_A_D_1
nxmpulseh: { body: tBody.tank.retribution, prop: tProp.tank.hover2, weap: tWeap.tank.pulseLaser },

// CAM_3_A_D_2
nxhgauss: { body: tBody.tank.vengeance, prop: tProp.tank.hover2, weap: tWeap.tank.gaussCannon },
nxlpulsev: { body: tBody.tank.retaliation, prop: tProp.air.vtol2, weap: tWeap.air.pulseLaser },

// SUB_3_4
nxllinkh: { body: tBody.tank.retaliation, prop: tProp.tank.hover2, weap: tWeap.tank.nexusLink },
nxmpulsev: { body: tBody.tank.retribution, prop: tProp.air.vtol2, weap: tWeap.air.pulseLaser },


////////////////////////////////////////////////////////////////////////////////
};
