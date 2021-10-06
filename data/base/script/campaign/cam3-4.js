include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEXUS_RES = [
	"R-Sys-Engineering03", "R-Defense-WallUpgrade11", "R-Struc-Materials11",
	"R-Struc-VTOLPad-Upgrade06", "R-Wpn-Bomb-Damage03", "R-Sys-NEXUSrepair",
	"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
	"R-Wpn-Mortar-Acc03", "R-Wpn-MG-Damage09", "R-Wpn-Mortar-ROF04",
	"R-Vehicle-Engine09", "R-Vehicle-Metals11", "R-Vehicle-Armor-Heat08",
	"R-Cyborg-Metals11", "R-Cyborg-Armor-Heat08", "R-Wpn-RocketSlow-ROF06",
	"R-Wpn-AAGun-Damage06", "R-Wpn-AAGun-ROF06", "R-Wpn-Howitzer-Damage09",
	"R-Wpn-Howitzer-ROF04", "R-Wpn-Cannon-Damage09", "R-Wpn-Cannon-ROF06",
	"R-Wpn-Missile-Damage03", "R-Wpn-Missile-ROF03", "R-Wpn-Missile-Accuracy02",
	"R-Wpn-Rail-Damage03", "R-Wpn-Rail-ROF03", "R-Wpn-Rail-Accuracy01",
	"R-Wpn-Energy-Damage03", "R-Wpn-Energy-ROF03", "R-Wpn-Energy-Accuracy01",
	"R-Wpn-AAGun-Accuracy03", "R-Wpn-Howitzer-Accuracy03",
];

function eventDestroyed(obj)
{
	if (obj.player === NEXUS && obj.type === STRUCTURE && obj.stattype === HQ)
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
	let hackFailChance = 60;

	if (camRand(100) < hackFailChance)
	{
		return;
	}

	camHackIntoPlayer(CAM_HUMAN_PLAYER, NEXUS);
}

// A little suprise absorbption attack when discovering the SW base.
function firstAbsorbAttack()
{
	var objects = enumArea(0, 0, mapWidth, mapHeight, CAM_HUMAN_PLAYER, false).filter(function(obj) {
		return (obj.type !== DROID) || (obj.type === DROID && obj.droidType !== DROID_SUPERTRANSPORTER);
	});

	for (var i = 0, len = objects.length; i < len; ++i)
	{
		var obj = objects[i];
		//Destroy all the VTOLs to prevent a player from instantly defeating the HQ in a rush.
		if (obj.type === DROID && isVTOL(obj))
		{
			camSafeRemoveObject(obj, true);
			continue;
		}
		if ((camRand(100) < 10) && !donateObject(obj, NEXUS))
		{
			camSafeRemoveObject(obj, true);
		}
	}
}

function activateNexus()
{
	camSetExtraObjectiveMessage(_("Destroy the Nexus HQ to disable the Nexus Intruder Program"));
	playSound(SYNAPTICS_ACTIVATED);
	camSetNexusState(true);
	setTimer("nexusHackFeature", camSecondsToMilliseconds((difficulty <= MEDIUM) ? 20 : 10));
}

function camEnemyBaseDetected_NX_SWBase()
{
	camPlayVideos({video: "MB3_4_MSG4", type: MISS_MSG});
	firstAbsorbAttack(); //before Nexus state activation to prevent sound spam.
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
	const FACTORY_LIST = [
		"NX-NWFactory1", "NX-NWFactory2", "NX-NEFactory", "NX-SWFactory",
		"NX-SEFactory", "NX-VtolFactory1", "NX-NWCyborgFactory",
		"NX-VtolFactory2", "NX-SWCyborgFactory1", "NX-SWCyborgFactory2",
	];

	for (var i = 0, l = FACTORY_LIST.length; i < l; ++i)
	{
		camEnableFactory(FACTORY_LIST[i]);
	}

	//Set the already placed VTOL fighters into action
	camManageGroup(camMakeGroup("vtolBaseCleanup"), CAM_ORDER_ATTACK, {
		regroup: false, count: -1
	});
}

function truckDefense()
{
	var truckNum = countDroid(NEXUS, DROID_CONSTRUCT);
	if (truckNum > 0)
	{
		var list = [
			"Sys-NEXUSLinkTOW", "P0-AASite-SAM2", "Emplacement-PrisLas",
			"NX-Tower-ATMiss", "Sys-NX-CBTower", "Emplacement-HvART-pit",
			"Sys-SensoTower02"
		];

		for (var i = 0; i < truckNum; ++i)
		{
			camQueueBuilding(NEXUS, list[camRand(list.length)]);
		}
	}
	else
	{
		removeTimer("truckDefense");
	}
}

function eventStartLevel()
{
	var startpos = getObject("startPosition");
	var tpos = getObject("transportEntryExit");
	var lz = getObject("landingZone");

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "GAMMA_OUT", {
		area: "RTLZ",
		reinforcements: camMinutesToSeconds(1),
		annihilate: true
	});

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tpos.x, tpos.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tpos.x, tpos.y, CAM_HUMAN_PLAYER);
	setMissionTime(-1); //Infinite time

	var enemyLz = getObject("NXlandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, NEXUS);

	camCompleteRequiredResearch(NEXUS_RES, NEXUS);
	setupNexusPatrols();
	camManageTrucks(NEXUS);

	camSetArtifacts({
		"NX-NWCyborgFactory": { tech: "R-Wpn-RailGun03" },
		"NX-NEFactory": { tech: "R-Vehicle-Body10" }, //Vengeance
		"NX-MissileEmplacement": { tech: "R-Wpn-HvArtMissile" },
	});

	camSetEnemyBases({
		"NX_SWBase": {
			cleanup: "SWBaseCleanup",
			detectMsg: "CM34_OBJ2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NX_NWBase": {
			cleanup: "NWBaseCleanup",
			detectMsg: "CM34_BASEA",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NX_NEBase": {
			cleanup: "NEBaseCleanup",
			detectMsg: "CM34_BASEB",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NX_WBase": {
			cleanup: "WBaseCleanup",
			detectMsg: "CM34_BASEC",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NX_SEBase": {
			cleanup: "SEBaseCleanup",
			detectMsg: "CM34_BASED",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NX_VtolBase": {
			cleanup: "vtolBaseCleanup",
			detectMsg: "CM34_BASEE",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
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
	setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(15)));
}
