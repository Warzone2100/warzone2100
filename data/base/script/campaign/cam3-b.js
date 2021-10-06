include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");
include("script/campaign/transitionTech.js");

var trapActive;
var gammaAttackCount;
var truckLocCounter;
const GAMMA = 1; // Player 1 is Gamma team.
const NEXUS_RES = [
	"R-Sys-Engineering03", "R-Defense-WallUpgrade08", "R-Struc-Materials08",
	"R-Struc-VTOLPad-Upgrade06", "R-Wpn-Bomb-Damage03", "R-Sys-NEXUSrepair",
	"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
	"R-Wpn-Mortar-Acc03", "R-Wpn-MG-Damage09", "R-Wpn-Mortar-ROF04",
	"R-Vehicle-Engine07", "R-Vehicle-Metals07", "R-Vehicle-Armor-Heat04",
	"R-Cyborg-Metals07", "R-Cyborg-Armor-Heat04", "R-Wpn-RocketSlow-ROF05",
	"R-Wpn-AAGun-Damage06", "R-Wpn-AAGun-ROF06", "R-Wpn-Howitzer-Damage09",
	"R-Wpn-Howitzer-ROF04", "R-Wpn-Cannon-Damage09", "R-Wpn-Cannon-ROF06",
	"R-Wpn-Missile-Damage01", "R-Wpn-Missile-ROF01", "R-Wpn-Missile-Accuracy01",
	"R-Wpn-Rail-Damage01", "R-Wpn-Rail-ROF01", "R-Wpn-Rail-Accuracy01",
	"R-Wpn-Energy-Damage03", "R-Wpn-Energy-ROF03", "R-Wpn-Energy-Accuracy01",
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

	camSetVtolData(NEXUS, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(camMinutesToMilliseconds(2)), "NXCommandCenter", ext);
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
	}
	else
	{
		removeTimer("sendNXTransporter");
	}
}

//Send Nexus land units
function sendNXlandReinforcements()
{
	if (!enumArea("NXWestBaseCleanup", NEXUS, false).length)
	{
		removeTimer("sendNXlandReinforcements");
		return;
	}

	camSendReinforcement(NEXUS, camMakePos("westPhantomFactory"), getDroidsForNXLZ(),
		CAM_REINFORCE_GROUND, {
			data: {regroup: true, count: -1,},
		}
	);
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
		interval: camSecondsToMilliseconds(45),
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
		interval: camSecondsToMilliseconds(45),
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
		interval: camSecondsToMilliseconds(30),
		regroup: false,
		count: -1
	});
}

function truckDefense()
{
	if (enumDroid(GAMMA, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	var list = ["Emplacement-Howitzer105", "Emplacement-MdART-pit", "Emplacement-RotHow"];
	var position;

	if (truckLocCounter === 0)
	{
		position = camMakePos("buildPos1");
		truckLocCounter += 1;
	}
	else
	{
		position = camMakePos("buildPos2");
		truckLocCounter = 0;
	}

	camQueueBuilding(GAMMA, list[camRand(list.length)], position);
}

//Take everything Gamma has and donate to Nexus.
function trapSprung()
{
	setAlliance(GAMMA, NEXUS, true);
	setAlliance(GAMMA, CAM_HUMAN_PLAYER, false);
	camPlayVideos({video: "MB3_B_MSG3", type: CAMP_MSG});
	hackRemoveMessage("CM3B_GAMMABASE", PROX_MSG, CAM_HUMAN_PLAYER);

	setMissionTime(camChangeOnDiff(camMinutesToSeconds(90)));
	camCallOnce("activateNexusGroups");
	enableAllFactories();

	sendNXTransporter();
	changePlayerColour(GAMMA, NEXUS); // Black painting.
	playSound(SYNAPTICS_ACTIVATED);

	setTimer("sendNXTransporter", camChangeOnDiff(camMinutesToMilliseconds(3)));
	setTimer("sendNXlandReinforcements", camChangeOnDiff(camMinutesToMilliseconds(4)));
	setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(4.5)));
}

function setupCapture()
{
	trapActive = true;
	playSound("pcv455.ogg"); //Incoming message.
	setAlliance(GAMMA, NEXUS, false);

	queue("trapSprung", camSecondsToMilliseconds(2)); //call this a few seconds later
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
	truckLocCounter = 0;
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");

     camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_3_2S");
	setMissionTime(camChangeOnDiff(camMinutesToSeconds(30))); // For the rescue mission.

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
		"gammaFactory": { tech: "R-Wpn-Cannon-ROF04" },
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds(45)),
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
		}
	});

	if (difficulty >= HARD)
	{
		addDroid(GAMMA, 28, 5, "Truck Python Tracks", "Body11ABT", "tracked01", "", "", "Spade1Mk1");

		camManageTrucks(GAMMA);
	}

	setAlliance(GAMMA, CAM_HUMAN_PLAYER, true);
	hackAddMessage("CM3B_GAMMABASE", PROX_MSG, CAM_HUMAN_PLAYER, false);
	camPlayVideos([{video: "MB3_B_MSG", type: CAMP_MSG}, {video: "MB3_B_MSG2", type: MISS_MSG}]);

	changePlayerColour(GAMMA, 0);
	setAlliance(GAMMA, CAM_HUMAN_PLAYER, true);

	queue("transferPower", camSecondsToMilliseconds(3));
	queue("vtolAttack", camChangeOnDiff(camMinutesToMilliseconds(5)));
}
