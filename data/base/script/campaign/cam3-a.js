include("script/campaign/transitionTech.js");
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var index; //Number of bonus transports that have flown in.
var startedFromMenu;

//Remove Nexus VTOL droids.
camAreaEvent("vtolRemoveZone", function(droid)
{
	if (droid.player !== CAM_HUMAN_PLAYER)
	{
		if (isVTOL(droid))
		{
			camSafeRemoveObject(droid, false);
		}
	}

	resetLabel("vtolRemoveZone", NEXUS);
});

//Order base three groups to do stuff.
camAreaEvent("cybAttackers", function(droid)
{
	enableAllFactories();

	camManageGroup(camMakeGroup("NEAttackerGroup"), CAM_ORDER_ATTACK, {
		regroup: true,
		morale: 90,
		fallback: camMakePos("SWBaseRetreat")
	});

	camManageGroup(camMakeGroup("NEDefenderGroup"), CAM_ORDER_DEFEND, {
		pos: [
			camMakePos("genericasAssembly"),
			camMakePos("northFacAssembly"),
		],
		regroup: true,
	});
});

camAreaEvent("westFactoryTrigger", function(droid)
{
	enableAllFactories();
});

//make the first batch or extra transport droids hero rank.
function setHeroUnits()
{
	const DROID_EXP = 512;
	var droids = enumDroid(CAM_HUMAN_PLAYER).filter(function(dr) {
		return (!camIsSystemDroid(dr) && !camIsTransporter(dr));
	});

	for (var j = 0, i = droids.length; j < i; ++j)
	{
		setDroidExperience(droids[j], DROID_EXP);
	}
}

function eventTransporterLanded(transport)
{
	if (startedFromMenu)
	{
		camCallOnce("setHeroUnits");
	}
}

//Enable all factories.
function enableAllFactories()
{
	const FACTORY_NAMES = [
		"NXcybFac-b3", "NXcybFac-b2-1", "NXcybFac-b2-2", "NXHvyFac-b2", "NXcybFac-b4",
	];

	for (var j = 0, i = FACTORY_NAMES.length; j < i; ++j)
	{
		camEnableFactory(FACTORY_NAMES[j]);
	}
}


//Extra transport units are only awarded to those who start Gamma campaign
//from the main menu.
function sendPlayerTransporter()
{
	const transportLimit = 4; //Max of four transport loads if starting from menu.
	if (!camDef(index))
	{
		index = 0;
	}

	if (index === transportLimit)
	{
		return;
	}

	var droids = [];
	var list = [cTempl.prhasgnt, cTempl.prhhpvt, cTempl.prhaacnt, cTempl.prtruck];

	// send 4 Assault Guns, 2 Hyper Velocity Cannons, 2 Cyclone AA Turrets and 2 Trucks
	for (var i = 0, d = list.length; i < 10; ++i)
	{
		droids.push(i < d * 2 ? list[i % 4] : list[0]);
	}

	camSendReinforcement(CAM_HUMAN_PLAYER, camMakePos("landingZone"), droids,
		CAM_REINFORCE_TRANSPORT, {
			entry: { x: 63, y: 118 },
			exit: { x: 63, y: 118 }
		}
	);

	index = index + 1;
	queue("sendPlayerTransporter", 300000); //5 min
}

//Setup Nexus VTOL hit and runners.
function vtolAttack()
{
	var list = [cTempl.nxlneedv, cTempl.nxlscouv, cTempl.nxmtherv];
	camSetVtolData(NEXUS, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(300000), "NXCommandCenter"); //5 min
}

//These groups are active immediately.
function groupPatrolNoTrigger()
{
	camManageGroup(camMakeGroup("cybAttackers"), CAM_ORDER_ATTACK, {
		pos: [
			camMakePos("northFacAssembly"),
			camMakePos("ambushPlayerPos"),
		],
		regroup: true,
		count: -1,
		morale: 90,
		fallback: camMakePos("healthRetreatPos")
	});

	camManageGroup(camMakeGroup("hoverPatrolGrp"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("hoverGrpPos1"),
			camMakePos("hoverGrpPos2"),
			camMakePos("hoverGrpPos3"),
		]
	});

	camManageGroup(camMakeGroup("cybValleyPatrol"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("southWestBasePatrol"),
			camMakePos("westEntrancePatrol"),
			camMakePos("playerLZPatrol"),
		]
	});

	camManageGroup(camMakeGroup("NAmbushCyborgs"), CAM_ORDER_ATTACK);
}

//Build defenses.
function truckDefense()
{
	if (enumDroid(NEXUS, DROID_CONSTRUCT).length > 0)
	{
		queue("truckDefense", 160000);
	}

	const DEFENSE = ["NX-Tower-Rail1", "NX-Tower-ATMiss"];
	camQueueBuilding(NEXUS, DEFENSE[camRand(DEFENSE.length)]);
}

//Gives starting tech and research.
function cam3Setup()
{
	var x = 0;
	var l = 0;
	const NEXUS_RES = [
		"R-Wpn-MG1Mk1", "R-Sys-Engineering03", "R-Defense-WallUpgrade07",
		"R-Struc-Materials07", "R-Struc-Factory-Upgrade06",
		"R-Struc-Factory-Cyborg-Upgrade06", "R-Struc-VTOLFactory-Upgrade06",
		"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals07",
		"R-Cyborg-Metals07", "R-Vehicle-Armor-Heat03", "R-Cyborg-Armor-Heat03",
		"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
		"R-Wpn-Bomb-Accuracy03", "R-Wpn-Missile-Damage01", "R-Wpn-Missile-ROF01",
		"R-Sys-Sensor-Upgrade01", "R-Sys-NEXUSrepair", "R-Wpn-Rail-Damage01",
		"R-Wpn-Rail-ROF01", "R-Wpn-Rail-Accuracy01", "R-Wpn-Flamer-Damage06",
	];

	for (x = 0, l = BETA_TECH.length; x < l; ++x)
	{
		makeComponentAvailable(BETA_TECH[x], CAM_HUMAN_PLAYER);
	}

	for (x = 0, l = STRUCTS_GAMMA.length; x < l; ++x)
	{
		enableStructure(STRUCTS_GAMMA[x], CAM_HUMAN_PLAYER);
	}

	camCompleteRequiredResearch(ALPHA_RESEARCH, CAM_HUMAN_PLAYER);
	camCompleteRequiredResearch(ALPHA_RESEARCH, NEXUS);

	camCompleteRequiredResearch(PLAYER_RES_GAMMA, CAM_HUMAN_PLAYER);
	camCompleteRequiredResearch(NEXUS_RES, NEXUS);

	enableResearch("R-Wpn-Howitzer03-Rot", CAM_HUMAN_PLAYER);
	enableResearch("R-Wpn-MG-Damage08", CAM_HUMAN_PLAYER);
}


function eventStartLevel()
{
	const MISSION_TIME = camChangeOnDiff(7200); //120 minutes.
	const PLAYER_POWER = 16000;
	const REINFORCEMENT_TIME = 300; //5 minutes.
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	var tent = getObject("transporterEntry");
	var text = getObject("transporterExit");

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_3_1S");
	setMissionTime(MISSION_TIME);

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	var enemyLz = getObject("NXlandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, NEXUS);

	camSetArtifacts({
		"NXPowerGenArti": { tech: "R-Struc-Power-Upgrade02" },
		"NXResearchLabArti": { tech: "R-Sys-Engineering03" },
	});

	setPower(PLAYER_POWER, CAM_HUMAN_PLAYER);
	cam3Setup();

	camSetEnemyBases({
		"NEXUS-WBase": {
			cleanup: "westBaseCleanup",
			detectMsg: "CM3A_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NEXUS-SWBase": {
			cleanup: "southWestBaseCleanup",
			detectMsg: "CM3A_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NEXUS-NEBase": {
			cleanup: "northEastBaseCleanup",
			detectMsg: "CM3A_BASE3",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NEXUS-NWBase": {
			cleanup: "northWestBaseCleanup",
			detectMsg: "CM3A_BASE4",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
	});

	camSetFactories({
		"NXcybFac-b3": {
			assembly: "NXcybFac-b3Assembly",
			order: CAM_ORDER_ATTACK,
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			groupSize: 4,
			throttle: camChangeOnDiff(40000),
			group: camMakeGroup("NEAttackerGroup"),
			templates: [cTempl.nxcyrail, cTempl.nxcyscou]
		},
		"NXcybFac-b2-1": {
			assembly: "NXcybFac-b2-1Assembly",
			order: CAM_ORDER_ATTACK,
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			groupSize: 4,
			throttle: camChangeOnDiff(30000),
			group: camMakeGroup("cybAttackers"),
			templates: [cTempl.nxcyrail, cTempl.nxcyscou]
		},
		"NXcybFac-b2-2": {
			assembly: "NXcybFac-b2-2Assembly",
			order: CAM_ORDER_PATROL,
			data: {
				pos: [
					camMakePos("southWestBasePatrol"),
					camMakePos("westEntrancePatrol"),
					camMakePos("playerLZPatrol"),
				],
				regroup: false,
				repair: 40,
				count: -1,
			},
			groupSize: 4,
			throttle: camChangeOnDiff(30000),
			group: camMakeGroup("cybValleyPatrol"),
			templates: [cTempl.nxcyrail, cTempl.nxcyscou]
		},
		"NXHvyFac-b2": {
			assembly: "NXHvyFac-b2Assembly",
			order: CAM_ORDER_PATROL,
			data: {
				pos: [
					camMakePos("hoverGrpPos1"),
					camMakePos("hoverGrpPos2"),
					camMakePos("hoverGrpPos3"),
				],
				regroup: false,
				repair: 45,
				count: -1,
			},
			groupSize: 4,
			throttle: camChangeOnDiff(60000),
			group: camMakeGroup("hoverPatrolGrp"),
			templates: [cTempl.nxmscouh]
		},
		"NXcybFac-b4": {
			assembly: "NXcybFac-b4Assembly",
			order: CAM_ORDER_PATROL,
			data: {
				pos: [
					camMakePos("genericasAssembly"),
					camMakePos("northFacAssembly"),
				],
				regroup: false,
				repair: 40,
				count: -1,
			},
			groupSize: 4,
			throttle: camChangeOnDiff(30000),
			group: camMakeGroup("NEDefenderGroup"),
			templates: [cTempl.nxcyrail, cTempl.nxcyscou]
		},
	});

	camManageTrucks(NEXUS);
	truckDefense();
	camPlayVideos(["MB3A_MSG", "MB3A_MSG2"]);
	startedFromMenu = false;

	//Only if starting Gamma directly rather than going through Beta
	if (enumDroid(CAM_HUMAN_PLAYER, DROID_TRANSPORTER).length === 0)
	{
		startedFromMenu = true;
		setReinforcementTime(LZ_COMPROMISED_TIME);
		sendPlayerTransporter();
	}
	else
	{
		setReinforcementTime(REINFORCEMENT_TIME);
	}

	groupPatrolNoTrigger();
	queue("vtolAttack", camChangeOnDiff(480000)); //8 min
	queue("enableAllFactories", camChangeOnDiff(1200000)); //20 minutes.
}
