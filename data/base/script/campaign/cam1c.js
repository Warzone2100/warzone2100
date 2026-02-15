include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_newParadigmRes = [
	"R-Wpn-MG1Mk1", "R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels",
	"R-Sys-Engineering01", "R-Wpn-MG-Damage03", "R-Wpn-MG-ROF01", "R-Wpn-Cannon-Damage02",
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Defense-WallUpgrade01", "R-Struc-Materials01", "R-Vehicle-Engine02",
	"R-Struc-RprFac-Upgrade02", "R-Wpn-Rocket-Damage01", "R-Wpn-Rocket-ROF03",
	"R-Vehicle-Metals01", "R-Wpn-Mortar-Damage02", "R-Wpn-Rocket-Accuracy01",
	"R-Wpn-RocketSlow-Damage01", "R-Wpn-Mortar-ROF01",
];
const mis_scavengerRes = [
	"R-Wpn-Flamer-Damage02", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage03", "R-Wpn-MG-ROF01", "R-Wpn-Rocket-Damage01",
	"R-Wpn-Cannon-Damage02", "R-Wpn-Mortar-Damage02", "R-Wpn-Mortar-ROF01",
	"R-Wpn-Rocket-ROF03", "R-Defense-WallUpgrade01", "R-Struc-Materials01",
];
const mis_newParadigmResClassic = [
	"R-Defense-WallUpgrade01", "R-Struc-Factory-Upgrade01", "R-Struc-Materials01",
	"R-Wpn-MG-ROF01", "R-Vehicle-Engine01", "R-Wpn-Cannon-Damage02", "R-Wpn-Flamer-Damage03",
	"R-Wpn-MG-Damage03", "R-Wpn-Rocket-Damage01", "R-Wpn-Flamer-ROF01", "R-Vehicle-Metals01",
	"R-Struc-RprFac-Upgrade03",
];
const mis_scavengerResClassic = [
	"R-Wpn-MG-Damage03", "R-Wpn-Rocket-Damage01"
];

function activateLZDefenders()
{
	if (camClassicMode())
	{
		return;
	}
	camManageGroup(camMakeGroup("PatrolForce"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("PatrolPos1"),
			camMakePos("PatrolPos2"),
			camMakePos("PatrolPos3"),
			camMakePos("PatrolPos4")
		],
		interval: camSecondsToMilliseconds(30),
		regroup: false,
	});
}

function activateScavBaseDefenders()
{
	if (camClassicMode())
	{
		return;
	}
	camManageGroup(camMakeGroup("DefendForce"), CAM_ORDER_DEFEND, {
		pos: camMakePos("defensePos"),
		radius: 16,
		regroup: false
	});
}

function sendRocketForce()
{
	camManageGroup(camMakeGroup("RocketForce"), CAM_ORDER_ATTACK, {
		regroup: true,
		count: -1,
	});
}

function sendTankScoutForce()
{
	camManageGroup(camMakeGroup("TankScoutForce"), CAM_ORDER_ATTACK, {
		regroup: true,
		count: -1,
	});
	// FIXME: Re-enable this when commander/formation movement
	// becomes good enough. Remove the call above then.
	/*
	camManageGroup(camMakeGroup("TankScoutForce"), CAM_ORDER_FOLLOW, {
		droid: "TankScoutForceCommander",
		order: CAM_ORDER_ATTACK
	});
	*/
}

function sendTankForce()
{
	camManageGroup(camMakeGroup("TankForce"), CAM_ORDER_ATTACK, {
		regroup: true,
		count: -1,
	});
	// FIXME: Re-enable this when commander/formation movement
	// becomes good enough. Remove the call above then.
	/*
	camManageGroup(camMakeGroup("TankForce"), CAM_ORDER_FOLLOW, {
		droid: "TankForceCommander",
		order: CAM_ORDER_ATTACK
	});
	*/
}

function enableNPFactory()
{
	camEnableFactory("NPCentralFactory");
}

function enableNorthScavFactory()
{
	camEnableFactory("ScavNorthFactory");
}

camAreaEvent("RemoveBeacon", function()
{
	hackRemoveMessage("C1C_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
});

camAreaEvent("AmbushTrigger", function()
{
	// wzcam enables factory here, even though it's quite early
	camEnableFactory("ScavCentralFactory");

	camManageGroup(camMakeGroup("AmbushForce"), CAM_ORDER_ATTACK, {
		pos: "AmbushTarget",
		regroup: true,
		count: -1,
	});
	// FIXME: Re-enable this when commander/formation movement
	// becomes good enough. Remove the call above then.
	// FIXME: This group has more droids than the commander can handle!
	/*
	camManageGroup(camMakeGroup("AmbushForce"), CAM_ORDER_FOLLOW, {
		droid: "AmbushForceCommander",
		order: CAM_ORDER_ATTACK,
		pos: "AmbushTarget",
	});
	*/
});

camAreaEvent("ScavCentralFactoryTrigger", function()
{
	// doesn't make much sense because the player
	// passes through AmbushTrigger anyway
	// before getting there
	camEnableFactory("ScavCentralFactory");
});

camAreaEvent("ScavNorthFactoryTrigger", function()
{
	camCallOnce("enableNorthScavFactory");
});

camAreaEvent("NPNorthFactoryTrigger", function()
{
	camEnableFactory("NPNorthFactory");
});

function camEnemyBaseEliminated_NPCentralFactory()
{
	camEnableFactory("NPNorthFactory");
}

function getDroidsForNPLZ(args)
{
	const scouts = [ cTempl.nppod, cTempl.nphmg ];
	const heavies = [ cTempl.npslc, cTempl.npsmct ];
	const list = [];
	const LIMIT = ((difficulty >= INSANE) ? 10 : 8);
	let numScouts = camRand(5) + 1;
	let heavy = heavies[camRand(heavies.length)];
	let loopRuns = 0;

	if (camRand(100) < 50)
	{
		list.push(cTempl.npsens); //sensor will count towards scout total
		numScouts -= 1;
		heavy = cTempl.npmor;
	}

	while (list.length < LIMIT)
	{
		list.push((loopRuns < numScouts) ? scouts[camRand(scouts.length)] : heavy);
		++loopRuns;
	}

	return list;
}

camAreaEvent("NPLZ1Trigger", function()
{
	// Message4 here, Message3 for the second LZ, and
	// please don't ask me why they did it this way
	camPlayVideos({video: "MB1C4_MSG", type: MISS_MSG});
	camDetectEnemyBase("NPLZ1Group");

	camSetBaseReinforcements("NPLZ1Group", camChangeOnDiff(camMinutesToMilliseconds((difficulty >= INSANE) ? 4.5 : 5)), "getDroidsForNPLZ",
		CAM_REINFORCE_TRANSPORT, {
			entry: { x: 126, y: 76 },
			exit: { x: 126, y: 36 },
			posLZ: camMakePos("EastNPLZ")
		}
	);

	// Increase the reinforcement spawn spawn speed at this point.
	if (camAllowInsaneSpawns())
	{
		removeTimer("insaneReinforcementSpawn");
		setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(3));
	}

	camCallOnce("activateLZDefenders");
});

camAreaEvent("NPLZ2Trigger", function()
{
	camPlayVideos({video: "MB1C3_MSG", type: MISS_MSG});
	camDetectEnemyBase("NPLZ2Group");
	camCallOnce("enableNorthScavFactory");

	camSetBaseReinforcements("NPLZ2Group", camChangeOnDiff(camMinutesToMilliseconds(5)), "getDroidsForNPLZ",
		CAM_REINFORCE_TRANSPORT, {
			entry: { x: 126, y: 76 },
			exit: { x: 126, y: 36 },
			posLZ: camMakePos("WestNPLZ")
		}
	);
});

function insaneReinforcementSpawn()
{
	const units = [cTempl.npsmct, cTempl.nppod];
	const limits = {minimum: 6, maxRandom: 4};
	const location = ["reinforceNorth", "reinforceSouthEast"];
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_NEW_PARADIGM, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, cam_levels.alpha7);
	const startPos = getObject("startPosition");
	const lz = getObject("landingZone");
	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	camSetMissionTimer(camChangeOnDiff(camHoursToSeconds(2)));

	setAlliance(CAM_NEW_PARADIGM, CAM_SCAV_7, true);

	if (camClassicMode())
	{
		camClassicResearch(mis_newParadigmResClassic, CAM_NEW_PARADIGM);
		camClassicResearch(mis_scavengerResClassic, CAM_SCAV_7);

		camSetArtifacts({
			"ScavSouthFactory": { tech: "R-Wpn-Rocket05-MiniPod" },
			"NPResearchFacility": { tech: "R-Struc-Research-Module" },
			"NPCentralFactory": { tech: "R-Vehicle-Prop-Tracks" },
			"NPNorthFactory": { tech: "R-Vehicle-Engine01" },
		});
	}
	else
	{
		camCompleteRequiredResearch(mis_newParadigmRes, CAM_NEW_PARADIGM);
		camCompleteRequiredResearch(mis_scavengerRes, CAM_SCAV_7);

		camUpgradeOnMapTemplates(cTempl.bloke, cTempl.blokeheavy, CAM_SCAV_7);
		camUpgradeOnMapTemplates(cTempl.trike, cTempl.trikeheavy, CAM_SCAV_7);
		camUpgradeOnMapTemplates(cTempl.buggy, cTempl.buggyheavy, CAM_SCAV_7);
		camUpgradeOnMapTemplates(cTempl.bjeep, cTempl.bjeepheavy, CAM_SCAV_7);

		if (camAllowInsaneSpawns())
		{
			addDroid(CAM_NEW_PARADIGM, 90, 107, "MRP Bug Wheels", tBody.tank.bug, tProp.tank.wheels, "", "", tWeap.tank.miniRocketPod);
			addDroid(CAM_NEW_PARADIGM, 89, 107, "MRP Bug Wheels", tBody.tank.bug, tProp.tank.wheels, "", "", tWeap.tank.miniRocketPod);
			addDroid(CAM_NEW_PARADIGM, 88, 107, "MRP Bug Wheels", tBody.tank.bug, tProp.tank.wheels, "", "", tWeap.tank.miniRocketPod);
			addDroid(CAM_NEW_PARADIGM, 90, 110, "MRP Bug Wheels", tBody.tank.bug, tProp.tank.wheels, "", "", tWeap.tank.miniRocketPod);
			addDroid(CAM_NEW_PARADIGM, 89, 110, "MRP Bug Wheels", tBody.tank.bug, tProp.tank.wheels, "", "", tWeap.tank.miniRocketPod);
			addDroid(CAM_NEW_PARADIGM, 88, 110, "MRP Bug Wheels", tBody.tank.bug, tProp.tank.wheels, "", "", tWeap.tank.miniRocketPod);
			addDroid(CAM_NEW_PARADIGM, 114, 25, "MRP Scorpion Halftracks", tBody.tank.scorpion, tProp.tank.halfTracks, "", "", tWeap.tank.miniRocketPod);
			addDroid(CAM_NEW_PARADIGM, 114, 26, "MRP Scorpion Halftracks", tBody.tank.scorpion, tProp.tank.halfTracks, "", "", tWeap.tank.miniRocketPod);
			addDroid(CAM_NEW_PARADIGM, 114, 27, "MRP Scorpion Halftracks", tBody.tank.scorpion, tProp.tank.halfTracks, "", "", tWeap.tank.miniRocketPod);
			addDroid(CAM_NEW_PARADIGM, 116, 25, "Lancer Scorpion Halftracks", tBody.tank.scorpion, tProp.tank.halfTracks, "", "", tWeap.tank.lancer);
			addDroid(CAM_NEW_PARADIGM, 116, 26, "Lancer Scorpion Halftracks", tBody.tank.scorpion, tProp.tank.halfTracks, "", "", tWeap.tank.lancer);
			addDroid(CAM_NEW_PARADIGM, 116, 27, "Lancer Scorpion Halftracks", tBody.tank.scorpion, tProp.tank.halfTracks, "", "", tWeap.tank.lancer);
		}

		camSetArtifacts({
			"ScavSouthFactory": { tech: ["R-Wpn-Rocket05-MiniPod", "R-Wpn-Cannon2Mk1"] },
			"NPResearchFacility": { tech: "R-Struc-Research-Module" },
			"NPCentralFactory": { tech: "R-Vehicle-Prop-Tracks" },
			"NPNorthFactory": { tech: "R-Vehicle-Engine01" },
		});
	}

	camSetEnemyBases({
		"ScavSouthDerrickGroup": {
			cleanup: "ScavSouthDerrick",
			detectMsg: "C1C_BASE1",
			detectSnd: cam_sounds.baseDetection.scavengerOutpostDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerOutpostEradicated
		},
		"ScavSouthEastHighgroundGroup": {
			cleanup: "ScavSouthEastHighground",
			detectMsg: "C1C_BASE6",
			detectSnd: cam_sounds.baseDetection.scavengerBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerBaseEradicated
		},
		"ScavNorthBaseGroup": {
			cleanup: "ScavNorthBase",
			detectMsg: "C1C_BASE3",
			detectSnd: cam_sounds.baseDetection.scavengerBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerBaseEradicated
		},
		"ScavSouthPodPitsGroup": {
			cleanup: "ScavSouthPodPits",
			detectMsg: "C1C_BASE4",
			detectSnd: cam_sounds.baseDetection.scavengerOutpostDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerOutpostEradicated
		},
		"ScavCentralBaseGroup": {
			cleanup: "MixedCentralBase", // two bases with same cleanup region
			detectMsg: "C1C_BASE5",
			detectSnd: cam_sounds.baseDetection.scavengerBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerBaseEradicated,
			player: CAM_SCAV_7 // hence discriminate by player filter
		},
		"NPEastBaseGroup": {
			cleanup: "NPEastBase",
			detectMsg: "C1C_BASE7",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NPNorthEastGeneratorGroup": {
			cleanup: "NPNorthEastGenerator",
			detectMsg: "C1C_BASE8",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NPNorthEastBaseGroup": {
			cleanup: "NPNorthEastBase",
			detectMsg: "C1C_BASE9",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NPCentralBaseGroup": {
			cleanup: "MixedCentralBase", // two bases with same cleanup region
			detectMsg: "C1C_BASE10",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
			player: CAM_NEW_PARADIGM // hence discriminate by player filter
		},
		"NPLZ1Group": {
			cleanup: "NPLZ1", // kill the four towers to disable LZ
			detectMsg: "C1C_LZ1",
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
			player: CAM_NEW_PARADIGM // required for LZ-type bases
		},
		"NPLZ2Group": {
			cleanup: "NPLZ2", // kill the four towers to disable LZ
			detectMsg: "C1C_LZ2",
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
			player: CAM_NEW_PARADIGM // required for LZ-type bases
		},
	});

	hackAddMessage("C1C_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false); // initial beacon
	camPlayVideos([{video: "MB1C_MSG", type: CAMP_MSG}, {video: "MB1C2_MSG", type: CAMP_MSG}]);

	camSetFactories({
		"ScavSouthFactory": {
			assembly: "ScavSouthFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty >= INSANE) ? 20 : 25)),
			templates: (!camClassicMode()) ? [ cTempl.buscan, cTempl.rbjeep8, cTempl.trikeheavy, cTempl.buggyheavy ] : [ cTempl.buscan, cTempl.rbjeep, cTempl.trike, cTempl.buggy ]
		},
		"ScavCentralFactory": {
			assembly: "ScavCentralFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty >= INSANE) ? 20 : 25)),
			templates: (!camClassicMode()) ? [ cTempl.firecan, cTempl.rbuggy, cTempl.bjeepheavy, cTempl.blokeheavy ] : [ cTempl.firecan, cTempl.rbuggy, cTempl.bjeep, cTempl.bloke ]
		},
		"ScavNorthFactory": {
			assembly: "ScavNorthFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty >= INSANE) ? 15 : 20)),
			templates: (!camClassicMode()) ? [ cTempl.firecan, cTempl.rbuggy, cTempl.buscan, cTempl.trikeheavy ] : [ cTempl.firecan, cTempl.rbuggy, cTempl.buscan, cTempl.trike ]
		},
		"NPCentralFactory": {
			assembly: "NPCentralFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty >= INSANE) ? 40 : 60)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: (difficulty >= INSANE) ? [ cTempl.npmor, cTempl.npsens, cTempl.npsmc ] : [ cTempl.npmor, cTempl.npsens, cTempl.npslc ]
		},
		"NPNorthFactory": {
			assembly: "NPNorthFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty >= INSANE) ? 40 : 50)),
			data: {
				regroup: false,
				repair: 66,
				count: -1,
			},
			templates: [ cTempl.nppod, cTempl.npsmct, cTempl.npmor ]
		},
	});

	camManageTrucks(CAM_NEW_PARADIGM);
	replaceTexture("page-7-barbarians-arizona.png", "page-7-barbarians-kevlar.png");

	camEnableFactory("ScavSouthFactory");
	camManageGroup(camMakeGroup("RocketScoutForce"), CAM_ORDER_ATTACK, {
		regroup: true,
		count: -1,
	});
	queue("sendRocketForce", camSecondsToMilliseconds(25));
	queue("sendTankScoutForce", camSecondsToMilliseconds(30));
	queue("sendTankForce", camSecondsToMilliseconds(100)); // in wzcam it moves back and then forward
	queue("enableNPFactory", camMinutesToMilliseconds(5));
	queue("activateScavBaseDefenders", camSecondsToMilliseconds(3));
	if (camAllowInsaneSpawns())
	{
		setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(4));
	}
}
