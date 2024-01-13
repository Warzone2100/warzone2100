include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_nexusRes = [
	"R-Sys-Engineering03", "R-Defense-WallUpgrade12", "R-Struc-Materials11",
	"R-Struc-VTOLPad-Upgrade06", "R-Wpn-Bomb-Damage03", "R-Sys-NEXUSrepair",
	"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
	"R-Wpn-Mortar-Acc03", "R-Wpn-MG-Damage10", "R-Wpn-Mortar-ROF04",
	"R-Vehicle-Engine09", "R-Vehicle-Metals11", "R-Vehicle-Armor-Heat08",
	"R-Cyborg-Metals11", "R-Cyborg-Armor-Heat08", "R-Wpn-RocketSlow-ROF06",
	"R-Wpn-AAGun-Damage06", "R-Wpn-AAGun-ROF06", "R-Wpn-Howitzer-Damage09",
	"R-Wpn-Howitzer-ROF04", "R-Wpn-Cannon-Damage09", "R-Wpn-Cannon-ROF06",
	"R-Wpn-Missile-Damage03", "R-Wpn-Missile-ROF03", "R-Wpn-Missile-Accuracy02",
	"R-Wpn-Rail-Damage03", "R-Wpn-Rail-ROF03", "R-Wpn-Rail-Accuracy01",
	"R-Wpn-Energy-Damage03", "R-Wpn-Energy-ROF03", "R-Wpn-Energy-Accuracy01",
	"R-Wpn-AAGun-Accuracy03", "R-Wpn-Howitzer-Accuracy03", "R-Sys-NEXUSsensor",
];

function eventDestroyed(obj)
{
	if (camGetNexusState() && obj.player === CAM_NEXUS && obj.type === STRUCTURE && obj.stattype === HQ)
	{
		camSetNexusState(false);
		removeTimer("nexusHackFeature");
	}
	else if (obj.player === CAM_HUMAN_PLAYER)
	{
		if (enumDroid(CAM_HUMAN_PLAYER).length === 0)
		{
			//Play an addition special video when losing on the last Gamma mission.
			camPlayVideos({video: "MB3_4_MSG5", type: CAMP_MSG});
		}
	}
}

camAreaEvent("factoryTriggerW", function() {
	enableAllFactories();
});

camAreaEvent("factoryTriggerS", function() {
	enableAllFactories();
});

function nexusHackFeature()
{
	let hackFailChance = 0;

	switch (difficulty)
	{
		case SUPEREASY: hackFailChance = 95; break;
		case EASY: hackFailChance = 85; break;
		case MEDIUM: hackFailChance = 75; break;
		case HARD: hackFailChance = 65; break;
		case INSANE: hackFailChance = 55; break;
		default: hackFailChance = 70;
	}

	if (camRand(100) < hackFailChance)
	{
		return;
	}

	camHackIntoPlayer(CAM_HUMAN_PLAYER, CAM_NEXUS);
}

// A little suprise absorbption attack when discovering the SW base.
function takeoverChanceAttack()
{
	let chance = 0;
	switch (difficulty)
	{
		case SUPEREASY: chance = 1; break;
		case EASY: chance = 3; break;
		case MEDIUM: chance = 5; break;
		case HARD: chance = 7; break;
		case INSANE: chance = 9; break;
		default: chance = 5;
	}

	const objects = enumArea(0, 0, mapWidth, mapHeight, CAM_HUMAN_PLAYER, false).filter((obj) => (
		(obj.type !== DROID) || (obj.type === DROID && obj.droidType !== DROID_SUPERTRANSPORTER)
	));

	for (let i = 0, len = objects.length; i < len; ++i)
	{
		const obj = objects[i];
		if (obj.type === DROID && obj.droidType === DROID_COMMAND)
		{
			continue; //A little too hectic to take a Commander immediately.
		}
		if (camRand(100) < chance)
		{
			if (obj.type === STRUCTURE && obj.stattype === WALL)
			{
				camSafeRemoveObject(obj, true); // Just remove walls and tank traps.
			}
			else if (!donateObject(obj, CAM_NEXUS))
			{
				camSafeRemoveObject(obj, true); // If can't transfer then get rid of it too.
			}
		}
	}
}

function destroyPlayerVtols()
{
	const hq = getObject("NX-HQ");
	if (hq === null)
	{
		removeTimer("destroyPlayerVtols");
		return;
	}
	const __SCAN_RADIUS = 11;
	const objects = enumRange(hq.x, hq.y, __SCAN_RADIUS, CAM_HUMAN_PLAYER, false);
	for (let i = 0, len = objects.length; i < len; ++i)
	{
		const obj = objects[i];
		if (obj.type === DROID && isVTOL(obj))
		{
			camSafeRemoveObject(obj, true);
		}
	}
}

function activateNexus()
{
	camSetExtraObjectiveMessage(_("Destroy the Nexus HQ to disable the Nexus Intruder Program"));
	playSound(cam_sounds.nexus.synapticLinksActivated);
	camSetNexusState(true);
	setTimer("nexusHackFeature", camSecondsToMilliseconds((difficulty <= EASY) ? 20 : 10));
	setTimer("destroyPlayerVtols", camSecondsToMilliseconds(0.2));
}

function camEnemyBaseDetected_NX_SWBase()
{
	if (getObject("NX-HQ") === null)
	{
		return; //Probably destroyed through cheats?
	}
	camPlayVideos({video: "MB3_4_MSG4", type: MISS_MSG});
	//Do these before Nexus state activation to prevent sound spam.
	queue("takeoverChanceAttack", camSecondsToMilliseconds(0.5));
	queue("activateNexus", camSecondsToMilliseconds(1));
}

function setupNexusPatrols()
{
	camManageGroup(camMakeGroup("SWBaseCleanup"), CAM_ORDER_PATROL, {
		pos:[
			"SWPatrolPos1",
			"SWPatrolPos2",
			"SWPatrolPos3"
		],
		interval: camSecondsToMilliseconds(20),
		regroup: false,
		repair: 45,
		count: -1
	});

	camManageGroup(camMakeGroup("NEBaseCleanup"), CAM_ORDER_PATROL, {
		pos:[
			"NEPatrolPos1",
			"NEPatrolPos2"
		],
		interval: camSecondsToMilliseconds(30),
		regroup: false,
		repair: 45,
		count: -1
	});

	camManageGroup(camMakeGroup("SEBaseCleanup"), CAM_ORDER_PATROL, {
		pos:[
			"SEPatrolPos1",
			"SEPatrolPos2"
		],
		interval: camSecondsToMilliseconds(20),
		regroup: false,
		repair: 45,
		count: -1
	});

	camManageGroup(camMakeGroup("NWBaseCleanup"), CAM_ORDER_PATROL, {
		pos:[
			"NWPatrolPos1",
			"NWPatrolPos2",
			"NWPatrolPos3"
		],
		interval: camSecondsToMilliseconds(35),
		regroup: false,
		repair: 45,
		count: -1
	});
}

function enableAllFactories()
{
	const factoryList = [
		"NX-NWFactory1", "NX-NWFactory2", "NX-NEFactory", "NX-SWFactory",
		"NX-SEFactory", "NX-VtolFactory1", "NX-NWCyborgFactory",
		"NX-VtolFactory2", "NX-SWCyborgFactory1", "NX-SWCyborgFactory2",
	];

	for (let i = 0, l = factoryList.length; i < l; ++i)
	{
		camEnableFactory(factoryList[i]);
	}

	//Set the already placed VTOL fighters into action
	camManageGroup(camMakeGroup("vtolBaseCleanup"), CAM_ORDER_ATTACK, {
		regroup: false, count: -1
	});
}

function truckDefense()
{
	const TRUCK_NUM = countDroid(CAM_NEXUS, DROID_CONSTRUCT);
	if (TRUCK_NUM > 0)
	{
		const list = [
			"Sys-NEXUSLinkTOW", "P0-AASite-SAM2", "Emplacement-PrisLas",
			"NX-Tower-ATMiss", "Sys-NX-CBTower", "NX-Emp-MultiArtMiss-Pit",
			"Sys-NX-SensorTower"
		];

		for (let i = 0; i < TRUCK_NUM; ++i)
		{
			camQueueBuilding(CAM_NEXUS, list[camRand(list.length)]);
		}
	}
	else
	{
		removeTimer("truckDefense");
	}
}

function eventStartLevel()
{
	const startPos = getObject("startPosition");
	const tpos = getObject("transportEntryExit");
	const lz = getObject("landingZone");

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, CAM_GAMMA_OUT, {
		area: "RTLZ",
		reinforcements: camMinutesToSeconds(1),
		annihilate: true
	});

	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tpos.x, tpos.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tpos.x, tpos.y, CAM_HUMAN_PLAYER);
	setMissionTime(-1); //Infinite time

	const enemyLz = getObject("NXlandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, CAM_NEXUS);

	camCompleteRequiredResearch(mis_nexusRes, CAM_NEXUS);
	if (difficulty === INSANE)
	{
		completeResearch("R-Defense-WallUpgrade13", CAM_NEXUS);
	}
	setupNexusPatrols();
	camManageTrucks(CAM_NEXUS);

	camSetArtifacts({
		"NX-NWCyborgFactory": { tech: "R-Wpn-RailGun03" },
		"NX-NEFactory": { tech: "R-Vehicle-Body10" }, //Vengeance
		"NX-MissileEmplacement": { tech: "R-Wpn-HvArtMissile" },
	});

	camSetEnemyBases({
		"NX_SWBase": {
			cleanup: "SWBaseCleanup",
			detectMsg: "CM34_OBJ2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NX_NWBase": {
			cleanup: "NWBaseCleanup",
			detectMsg: "CM34_BASEA",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NX_NEBase": {
			cleanup: "NEBaseCleanup",
			detectMsg: "CM34_BASEB",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NX_WBase": {
			cleanup: "WBaseCleanup",
			detectMsg: "CM34_BASEC",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NX_SEBase": {
			cleanup: "SEBaseCleanup",
			detectMsg: "CM34_BASED",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NX_VtolBase": {
			cleanup: "vtolBaseCleanup",
			detectMsg: "CM34_BASEE",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"NX-NWFactory1": {
			assembly: "NX-NWFactory1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(80)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmscouh, cTempl.nxmsamh]
		},
		"NX-NWFactory2": {
			assembly: "NX-NWFactory2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(70)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmscouh, cTempl.nxmsamh]
		},
		"NX-NWCyborgFactory": {
			assembly: "NX-NWCyborgFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(55)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
		},
		"NX-NEFactory": {
			assembly: "NX-NEFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(70)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmscouh, cTempl.nxmsamh]
		},
		"NX-SWFactory": {
			assembly: "NX-SWFactoryAssembly",
			order: CAM_ORDER_PATROL,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(80)),
			data: {
				pos: [
					camMakePos("SWPatrolPos3"),
					camMakePos("NEPatrolPos1"),
					camMakePos("NEPatrolPos2")
				],
				interval: camSecondsToMilliseconds(90),
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxmlinkh, cTempl.nxllinkh] //Nexus link factory
		},
		"NX-SWCyborgFactory1": {
			assembly: "NX-SWCyborgFactory1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(65)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
		},
		"NX-SWCyborgFactory2": {
			assembly: "NX-SWCyborgFactory2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
		},
		"NX-SEFactory": {
			assembly: "NX-SEFactoryAssembly",
			order: CAM_ORDER_PATROL,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(75)),
			data: {
				pos: [
					camMakePos("SEPatrolPos1"),
					camMakePos("NEPatrolPos1")
				],
				interval: camSecondsToMilliseconds(90),
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmscouh, cTempl.nxmsamh]
		},
		"NX-VtolFactory1": {
			assembly: "NX-VtolFactory1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(80)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxmheapv, cTempl.nxlscouv]
		},
		"NX-VtolFactory2": {
			assembly: "NX-VtolFactory2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(70)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxmpulsev]
		},
	});

	//Show Project transport flying video.
	camPlayVideos({video: "MB3_4_MSG3", type: CAMP_MSG});
	hackAddMessage("CM34_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);

	queue("enableAllFactories", camChangeOnDiff(camMinutesToMilliseconds(5)));
	setTimer("truckDefense", camMinutesToMilliseconds(20));
}
