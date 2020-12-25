include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEXUS_RES = [
	"R-Defense-WallUpgrade09", "R-Struc-Materials09", "R-Struc-Factory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals09",
	"R-Cyborg-Metals08", "R-Vehicle-Armor-Heat06", "R-Cyborg-Armor-Heat06",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Damage03", "R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage03",
	"R-Wpn-Energy-ROF03", "R-Wpn-Missile-Accuracy01", "R-Wpn-Missile-Damage03",
	"R-Wpn-Rail-Damage03", "R-Wpn-Rail-ROF03", "R-Sys-Sensor-Upgrade01",
	"R-Sys-NEXUSrepair", "R-Wpn-Flamer-Damage06",
];

camAreaEvent("factoryTriggerW", function() {
	enableAllFactories();
});

camAreaEvent("factoryTriggerS", function() {
	enableAllFactories();
});

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
		"NX-SWBase": {
			cleanup: "SWBaseCleanup",
			detectMsg: "CM34_OBJ2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NX-NWBase": {
			cleanup: "NWBaseCleanup",
			detectMsg: "CM34_BASEA",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NX-NEBase": {
			cleanup: "NEBaseCleanup",
			detectMsg: "CM34_BASEB",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NX-WBase": {
			cleanup: "WBaseCleanup",
			detectMsg: "CM34_BASEC",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NX-SEBase": {
			cleanup: "SEBaseCleanup",
			detectMsg: "CM34_BASED",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NX-VtolBase": {
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
			groupSize: 6,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmscouh, cTempl.nxmsamh, cTempl.nxmsens]
		},
		"NX-NWFactory2": {
			assembly: "NX-NWFactory2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 6,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmscouh, cTempl.nxmsamh, cTempl.nxmsens]
		},
		"NX-NWCyborgFactory": {
			assembly: "NX-NWCyborgFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(30)),
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds(30)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmscouh, cTempl.nxmsamh, cTempl.nxmsens]
		},
		"NX-SWFactory": {
			assembly: "NX-SWFactoryAssembly",
			order: CAM_ORDER_PATROL,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
			data: {
				pos: [
					camMakePos("SWPatrolPos1"),
					camMakePos("SWPatrolPos2"),
					camMakePos("SWPatrolPos3"),
					camMakePos("NEPatrolPos1"),
					camMakePos("NEPatrolPos2")
				],
				interval: camSecondsToMilliseconds(45),
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxmlinkh, cTempl.nxllinkh] //Nexus link factory
		},
		"NX-SWCyborgFactory1": {
			assembly: "NX-SWCyborgFactory1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(35)),
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
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(35)),
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
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(30)),
			data: {
				pos: [
					camMakePos("SEPatrolPos1"),
					camMakePos("NEPatrolPos1")
				],
				interval: camSecondsToMilliseconds(30),
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmscouh, cTempl.nxmsamh, cTempl.nxmsens]
		},
		"NX-VtolFactory1": {
			assembly: "NX-VtolFactory1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds(50)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxmpulsev]
		},
	});

	//Show Project transport flying video.
	hackAddMessage("MB3_4_MSG3", MISS_MSG, CAM_HUMAN_PLAYER, true);
	hackAddMessage("CM34_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);

	queue("enableAllFactories", camChangeOnDiff(camMinutesToMilliseconds(5)));
	setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(15)));
}
