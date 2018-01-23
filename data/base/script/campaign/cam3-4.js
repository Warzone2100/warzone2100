include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEXUS_RES = [
	"R-Defense-WallUpgrade09", "R-Struc-Materials09", "R-Struc-Factory-Upgrade06",
	"R-Struc-Factory-Cyborg-Upgrade06", "R-Struc-VTOLFactory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals09",
	"R-Cyborg-Metals08", "R-Vehicle-Armor-Heat06", "R-Cyborg-Armor-Heat06",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Accuracy03", "R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage03",
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
		interval: 20000,
		regroup: false,
		repair: 45,
		count: -1
	});

	camManageGroup(camMakeGroup("NEBaseCleanup"), CAM_ORDER_PATROL, {
		pos:[
			"NEPatrolPos1",
			"NEPatrolPos2"
		],
		interval: 30000,
		regroup: false,
		repair: 45,
		count: -1
	});

	camManageGroup(camMakeGroup("SEBaseCleanup"), CAM_ORDER_PATROL, {
		pos:[
			"SEPatrolPos1",
			"SEPatrolPos2"
		],
		interval: 20000,
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
		interval: 35000,
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

function enableReinforcements()
{
	playSound("pcv440.ogg"); // Reinforcements are available.
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "GAMMA_OUT", {
		area: "RTLZ",
		reinforcements: 60, // 1 min
		annihilate: true
	});
}

function truckDefense()
{
	var truckNum = countDroid(NEXUS, DROID_CONSTRUCT);
	if (truckNum > 0)
	{
		var list = [
			"Sys-NEXUSLinkTOW", "P0-AASite-SAM2", "Emplacement-PrisLas",
			"NX-Tower-ATMiss", "Sys-NX-CBTower"
		];

		for (var i = 0; i < truckNum * 2; ++i)
		{
			camQueueBuilding(NEXUS, list[camRand(list.length)]);
		}
	}

	queue("truckDefense", camChangeOnDiff(300000)); // 5 min
}

//This mission has a heavy load on resources for Nexus so reset it every so often.
function resetNexusPower()
{
	setPower(AI_POWER, NEXUS);
	queue("resetNexusPower", 300000); // 5 min
}

function eventStartLevel()
{
	var startpos = getObject("startPosition");
	var tpos = getObject("transportEntryExit");
	var lz = getObject("landingZone");

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "GAMMA_OUT", {
		area: "RTLZ",
		reinforcements: -1,
		annihilate: true
	});

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tpos.x, tpos.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tpos.x, tpos.y, CAM_HUMAN_PLAYER);
	setMissionTime(-1); //Infinite time

	setPower(AI_POWER, NEXUS);
	camCompleteRequiredResearch(NEXUS_RES, NEXUS);
	setupNexusPatrols();
	camManageTrucks(NEXUS);
	truckDefense();

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

	with (camTemplates) camSetFactories({
		"NX-NWFactory1": {
			assembly: "NX-NWFactory1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 6,
			throttle: camChangeOnDiff(40000),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [nxhgauss, nxmpulseh, nxmscouh, nxmsamh, nxmstrike]
		},
		"NX-NWFactory2": {
			assembly: "NX-NWFactory2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 6,
			throttle: camChangeOnDiff(40000),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [nxhgauss, nxmpulseh, nxmscouh, nxmsamh, nxmstrike]
		},
		"NX-NWCyborgFactory": {
			assembly: "NX-NWCyborgFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(30000),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [nxcyrail, nxcyscou, nxcylas]
		},
		"NX-NEFactory": {
			assembly: "NX-NEFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(30000),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [nxhgauss, nxmpulseh, nxmscouh, nxmsamh, nxmstrike]
		},
		"NX-SWFactory": {
			assembly: "NX-SWFactoryAssembly",
			order: CAM_ORDER_PATROL,
			groupSize: 4,
			throttle: camChangeOnDiff(60000),
			data: {
				pos: [
					camMakePos("SWPatrolPos1"),
					camMakePos("SWPatrolPos2"),
					camMakePos("SWPatrolPos3"),
					camMakePos("NEPatrolPos1"),
					camMakePos("NEPatrolPos2")
				],
				interval: 45000,
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [nxmlinkh, nxllinkh] //Nexus link factory
		},
		"NX-SWCyborgFactory1": {
			assembly: "NX-SWCyborgFactory1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(35000),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [nxcyrail, nxcyscou, nxcylas]
		},
		"NX-SWCyborgFactory2": {
			assembly: "NX-SWCyborgFactory2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(35000),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [nxcyrail, nxcyscou, nxcylas]
		},
		"NX-SEFactory": {
			assembly: "NX-SEFactoryAssembly",
			order: CAM_ORDER_PATROL,
			groupSize: 5,
			throttle: camChangeOnDiff(30000),
			data: {
				pos: [
					camMakePos("SEPatrolPos1"),
					camMakePos("NEPatrolPos1")
				],
				interval: 30000,
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [nxhgauss, nxmpulseh, nxmscouh, nxmsamh, nxmstrike]
		},
		"NX-VtolFactory1": {
			assembly: "NX-VtolFactory1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(60000),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [nxmheapv, nxlscouv]
		},
		"NX-VtolFactory2": {
			assembly: "NX-VtolFactory2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(50000),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [nxmpulsev]
		},
	});

	const START_FACTORIES = [
		"NX-VtolFactory1", "NX-VtolFactory2", "NX-SEFactory", "NX-NEFactory",
		"NX-NWCyborgFactory"
	];
	for (var i = 0, l = START_FACTORIES.length; i < l; ++i)
	{
		camEnableFactory(START_FACTORIES[i]);
	}

	//Show Project transport flying video.
	hackAddMessage("MB3_4_MSG3", MISS_MSG, CAM_HUMAN_PLAYER, true);
	hackAddMessage("CM34_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);

	queue("enableReinforcements", 16000);
	queue("resetNexusPower", 300000); // 5 min
	queue("enableAllFactories", camChangeOnDiff(600000)); // 10 min.
}
