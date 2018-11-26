include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");
include("script/campaign/transitionTech.js");

var trapActive;
var gammaAttackCount;
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
	camCallOnce("setupCapture");
});

camAreaEvent("mockBattleTrigger", function(droid)
{
	setAlliance(GAMMA, NEXUS, false); //brief mockup battle
	camCallOnce("activateNexusGroups"); //help destroy Gamma base
});

function camEnemyBaseEliminated_NXEastBase()
{
	camRemoveEnemyTransporterBlip();
}

function camEnemyBaseEliminated_NXWestBase()
{
	camRemoveEnemyTransporterBlip();
}

//Setup Nexus VTOL hit and runners.
function vtolAttack()
{
	var list = [cTempl.nxmheapv, cTempl.nxlscouv, cTempl.nxmtherv, cTempl.nxlscouv];
	var ext = {
		limit: [5, 2, 5, 2], //paired with template list
		alternate: true,
		altIdx: 0
	};

	camSetVtolData(NEXUS, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(120000), "NXCommandCenter", ext); //2 min
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

	const COUNT = isTransport ? 10 : 10 + camRand(6);
	var units = [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas, cTempl.nxmlinkh, cTempl.nxmrailh, cTempl.nxmsamh];

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
	if (camCountStructuresInArea("NXEastBaseCleanup", NEXUS) === 0 &&
		camCountStructuresInArea("NXWestBaseCleanup", NEXUS) === 0)
	{
		return; //Call off transport when both west and east Nexus bases are destroyed.
	}

	const LZ_ALIAS = "CM3B_TRANS"; //1 and 2
	var list = getDroidsForNXLZ(true);
	var lzNum;
	var pos;

	if (camCountStructuresInArea("NXEastBaseCleanup", NEXUS) > 0)
	{
		lzNum = 1;
		pos = "nexusEastTransportPos";
	}

	if (camCountStructuresInArea("NXWestBaseCleanup", NEXUS) > 0 && (camRand(2) || !camDef(pos)))
	{
		lzNum = 2;
		pos = "nexusWestTransportPos";
	}

	if (camDef(pos))
	{
		camSendReinforcement(NEXUS, camMakePos(pos), list, CAM_REINFORCE_TRANSPORT, {
			message: LZ_ALIAS + lzNum,
			entry: { x: 62, y: 4 },
			exit: { x: 62, y: 4 }
		});

		queue("sendNXTransporter", camChangeOnDiff(180000)); //3 min
	}
}

//Send Nexus land units
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
    setPower(playerPower(CAM_HUMAN_PLAYER) + AWARD, CAM_HUMAN_PLAYER);
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
	if (!trapActive)
	{
		playSound("pcv455.ogg"); //Incoming message.
		trapActive = true;
		setAlliance(GAMMA, NEXUS, false);
		queue("trapSprung", 2000); //call this a few seconds later
		return;
	}

	setAlliance(GAMMA, NEXUS, true);
	setAlliance(GAMMA, CAM_HUMAN_PLAYER, false);
	camPlayVideos("MB3_B_MSG3");
	hackRemoveMessage("CM3B_GAMMABASE", PROX_MSG, CAM_HUMAN_PLAYER);

	setMissionTime(camChangeOnDiff(5400));
	camCallOnce("activateNexusGroups");
	enableAllFactories();

	queue("sendNXlandReinforcements", camChangeOnDiff(300000)); //5 min
	sendNXTransporter();
	changePlayerColour(GAMMA, NEXUS); // Black painting.
	playSound(SYNAPTICS_ACTIVATED);
}

function setupCapture()
{
	trapSprung();
}

function eventAttacked(victim, attacker)
{
	if (!trapActive && gammaAttackCount > 4)
	{
		camCallOnce("setupCapture");
	}

	if (victim.player === GAMMA && attacker.player === NEXUS)
	{
		gammaAttackCount = gammaAttackCount + 1;
	}
}

function eventStartLevel()
{
	trapActive = false;
	gammaAttackCount = 0;
	const MISSION_TIME = camChangeOnDiff(1800); //30 minutes. Rescue part.
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");

     camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_3_2S");
	setMissionTime(MISSION_TIME);

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	var enemyLz = getObject("NXlandingZone");
	var enemyLz2 = getObject("NXlandingZone2");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, NEXUS);
	setNoGoArea(enemyLz2.x, enemyLz2.y, enemyLz2.x2, enemyLz2.y2, 5);

	camCompleteRequiredResearch(NEXUS_RES, NEXUS);
	camCompleteRequiredResearch(GAMMA_ALLY_RES, GAMMA);
	camCompleteRequiredResearch(NEXUS_RES, GAMMA); //They get even more research.

	setAlliance(GAMMA, CAM_HUMAN_PLAYER, false);
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

	camSetFactories({
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
			templates: [cTempl.nxmrailh, cTempl.nxmscouh]
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
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
		}
	});

	setAlliance(GAMMA, CAM_HUMAN_PLAYER, true);
	hackAddMessage("CM3B_GAMMABASE", PROX_MSG, CAM_HUMAN_PLAYER, true);
	camPlayVideos(["MB3_B_MSG", "MB3_B_MSG2"]);

	changePlayerColour(GAMMA, 0);
	setAlliance(GAMMA, CAM_HUMAN_PLAYER, true);

	queue("transferPower", 3000);
	queue("vtolAttack", camChangeOnDiff(300000)); //5 min
}
