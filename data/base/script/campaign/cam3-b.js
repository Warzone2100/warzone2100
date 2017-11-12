include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");
include("script/campaign/transitionTech.js");

const GAMMA = 1; // Player 1 is Gamma team.
const NEXUS_RES = [
	"R-Defense-WallUpgrade08", "R-Struc-Materials08", "R-Struc-Factory-Upgrade06",
	"R-Struc-Factory-Cyborg-Upgrade06", "R-Struc-VTOLFactory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals06",
	"R-Cyborg-Metals07", "R-Vehicle-Armor-Heat05", "R-Cyborg-Armor-Heat05",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Accuracy03", "R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage01",
	"R-Wpn-Energy-ROF01", "R-Wpn-Missile-Accuracy01", "R-Wpn-Missile-Damage01",
	"R-Wpn-Rail-Damage02", "R-Wpn-Rail-ROF02", "R-Sys-Sensor-Upgrade01",
	"R-Sys-NEXUSrepair", "R-Wpn-Flamer-Damage06",
];

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

camAreaEvent("trapTrigger", function(droid)
{
	if (droid.player === CAM_HUMAN_PLAYER)
	{
		playSound("pcv455.ogg"); //Incoming message.
		queue("trapSprung", 2000);
	}
	else
	{
		resetLabel("trapTrigger", CAM_HUMAN_PLAYER);
	}
});

//Setup Nexus VTOL hit and runners.
function vtolAttack()
{
	var list; with (camTemplates) list = [nxmheapv, nxmtherv, nxlscouv];
	camSetVtolData(NEXUS, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(120000), "NXCommandCenter"); //2 min
}

function enableAllFactories()
{
	camEnableFactory("gammaFactory");
	camEnableFactory("gammaCyborgFactory");
}

//return 10 units if for a transport and up to 15 for land.
function getDroidsForNXLZ(isTransport)
{
	if (!camDef(isTransport))
	{
		isTransport = false;
	}

	const COUNT = isTransport ? 10 : 8 + camRand(8);
	var units;
	with (camTemplates) units = [nxcyrail, nxcyscou, nxcylas, nxmlinkh, nxmrailh, nxmsamh];

	var droids = [];
	for (var i = 0; i < COUNT; ++i)
	{
		droids.push(units[camRand(units.length)]);
	}

	return droids;
}

//Send Nexus transport units
function sendNXTransporter()
{
	if (!enumArea("NXEastBaseCleanup", NEXUS, false).length && !enumArea("NXWestBaseCleanup", NEXUS, false).length)
	{
		return; //Call off transport when both west and east Nexus bases are destroyed.
	}

	const LZ_ALIAS = "CM3B_TRANS"; //1 and 2
	var lzNum = camRand(2) + 1;
	var list = getDroidsForNXLZ(true);
	var pos = (lzNum === 1) ? "nexusEastTransportPos" : "nexusWestTransportPos";

	camSendReinforcement(NEXUS, camMakePos(pos), list,
		CAM_REINFORCE_TRANSPORT, {
			message: LZ_ALIAS + lzNum,
			entry: { x: 63, y: 4 },
			exit: { x: 63, y: 4 }
		}
	);

	queue("sendNXTransporter", camChangeOnDiff(180000)); //3 min
}

//Send Nexus transport units
function sendNXlandReinforcements()
{
	if (!enumArea("NXWestBaseCleanup", NEXUS, false).length)
	{
		return;
	}

	camSendReinforcement(NEXUS, camMakePos("westPhantomFactory"), getDroidsForNXLZ(),
		CAM_REINFORCE_GROUND, {
			data: {regroup: true, count: -1,},
		}
	);

	queue("sendNXlandReinforcements", camChangeOnDiff(240000)); //4 min
}

function transferPower()
{
    const AWARD = 5000;
    var powerTransferSound = "power-transferred.ogg";
    setPower(playerPower(me) + AWARD, me);
    playSound(powerTransferSound);
}

function activateNexusGroups()
{
	camManageGroup(camMakeGroup("eastNXGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("northEndOfPass"),
			camMakePos("southOfRidge"),
			camMakePos("westRidge"),
			camMakePos("eastRidge"),
		],
		interval: 45000,
		regroup: false,
		count: -1
		//morale: 90,
		//fallback: camMakePos("eastRetreat")
	});

	camManageGroup(camMakeGroup("westNXGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("westDoorOfBase"),
			camMakePos("eastDoorOfBase"),
			camMakePos("playerLZ"),
		],
		interval: 45000,
		regroup: false,
		count: -1
		//morale: 90,
		//fallback: camMakePos("westRetreat")
	});

	camManageGroup(camMakeGroup("gammaBaseCleanup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("gammaBase"),
			camMakePos("northEndOfPass"),
		],
		interval: 30000,
		regroup: false,
		count: -1
	});
}

//Take everything Gamma has and donate to Nexus.
function trapSprung()
{
	camPlayVideos("MB3_B_MSG3");
	hackRemoveMessage("CM3B_GAMMABASE", PROX_MSG, CAM_HUMAN_PLAYER);

	setMissionTime(camChangeOnDiff(5400));

	activateNexusGroups();
	enableAllFactories();

	sendNXlandReinforcements();
	sendNXTransporter();
	changePlayerColour(GAMMA, NEXUS); // Black painting.
	playSound(SYNAPTICS_ACTIVATED);
}

function eventStartLevel()
{
	const MISSION_TIME = camChangeOnDiff(1800); //30 minutes. Rescue part.
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");

     camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_3_2S");
	setMissionTime(MISSION_TIME);

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	setPower(AI_POWER, NEXUS);
	setPower(AI_POWER, GAMMA);

	camCompleteRequiredResearch(NEXUS_RES, NEXUS);
	camCompleteRequiredResearch(GAMMA_ALLY_RES, GAMMA);
	camCompleteRequiredResearch(NEXUS_RES, GAMMA); //They get even more research.

	setAlliance(GAMMA, NEXUS, true);

	camSetArtifacts({
		"NXCommandCenter": { tech: "R-Struc-Research-Upgrade07" },
		"NXBeamTowerArti": { tech: "R-Wpn-Laser01" },
		"gammaResLabArti": { tech: "R-Wpn-Mortar-Acc03" },
		"gammaCommandArti": { tech: "R-Vehicle-Body03" }, //retalitation
	});

	camSetEnemyBases({
		"GammaBase": {
			cleanup: "gammaBaseCleanup",
			detectMsg: "CM3B_GAMMABASE",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NXEastBase": {
			cleanup: "NXEastBaseCleanup",
			detectMsg: "CM3B_BASE4",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NXWestBase": {
			cleanup: "NXWestBaseCleanup",
			detectMsg: "CM3B_BASE6",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		}
	});

	with (camTemplates) camSetFactories({
		"gammaFactory": {
			assembly: "gammaFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(45000),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [nxmrailh, nxmscouh]
		},
		"gammaCyborgFactory": {
			assembly: "gammaCyborgFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			group: camMakeGroup("gammaBaseCleanup"),
			groupSize: 6,
			throttle: camChangeOnDiff(40000),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [nxcyrail, nxcyscou, nxcylas]
		}
	});

	hackAddMessage("CM3B_GAMMABASE", PROX_MSG, CAM_HUMAN_PLAYER, true);
	camPlayVideos(["MB3_B_MSG", "MB3_B_MSG2"]);

	changePlayerColour(GAMMA, 0);

	queue("transferPower", 3000);
	queue("vtolAttack", camChangeOnDiff(300000)); //5 min
	queue("enableAllFactories", camChangeOnDiff(301000)); //5 min
}
