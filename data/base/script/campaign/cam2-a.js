include("script/campaign/transitionTech.js");
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const TRANSPORT_LIMIT = 4;
var index; //Number of transport loads sent into the level
var startedFromMenu;


camAreaEvent("vtolRemoveZone", function(droid)
{
	camSafeRemoveObject(droid, false);
	resetLabel("vtolRemoveZone", THE_COLLECTIVE);
});

//Attack and destroy all those who resist the Machine! -The Collective
function secondVideo()
{
	camPlayVideos("MB2A_MSG2");
}

//Damage the base and droids for the player
function preDamageStuff()
{
	var droids = enumDroid(CAM_HUMAN_PLAYER);
	var structures = enumStruct(CAM_HUMAN_PLAYER);
	var x = 0;

	for (x = 0; x < droids.length; ++x)
	{
		var droid = droids[x];
		if (!camIsTransporter(droid))
		{
			setHealth(droid, 45 + camRand(20));
		}
	}

	for (x = 0; x < structures.length; ++x)
	{
		var struc = structures[x];
		setHealth(struc, 45 + camRand(45));
	}
}

function getDroidsForCOLZ()
{
	var droids = [];
	var count = 6 + camRand(5);
	var templates;
	var sensors = [cTempl.comsens, cTempl.comsens];
	var usingHeavy = false;

	if (camRand(100) < 50)
	{
		templates = [cTempl.npcybm, cTempl.commgt, cTempl.npcybc, cTempl.npcybr];
	}
	else
	{
		templates = [cTempl.cohct, cTempl.comct, cTempl.comorb];
		usingHeavy = true;
	}

	for (var i = 0; i < count; ++i)
	{
		if (!i && usingHeavy)
		{
			droids.push(sensors[camRand(sensors.length)]); //bring a sensor
		}
		else
		{
			droids.push(templates[camRand(templates.length)]);
		}
	}

	return droids;
}

//Send Collective transport units
function sendCOTransporter()
{
	var tPos = getObject("COTransportPos");
	var nearbyDefense = enumRange(tPos.x, tPos.y, 15, THE_COLLECTIVE, false);

	if (nearbyDefense.length)
	{
		var list = getDroidsForCOLZ();
		camSendReinforcement(THE_COLLECTIVE, camMakePos("COTransportPos"), list,
			CAM_REINFORCE_TRANSPORT, {
				entry: { x: 125, y: 100 },
				exit: { x: 125, y: 70 }
			}
		);

		queue("sendCOTransporter", camChangeOnDiff(240000)); //4 min
	}
}

//Extra transport units are only awarded to those who start Beta campaign
//from the main menu. Otherwise a player can just bring in there Alpha units
function sendPlayerTransporter()
{
	if (!camDef(index))
	{
		index = 0;
	}

	if (index === TRANSPORT_LIMIT)
	{
		downTransporter();
		return;
	}

	var ti = (index === (TRANSPORT_LIMIT - 1)) ? 60000 : 300000; //5 min
	var droids = [];
	var list = [cTempl.prhct, cTempl.prhct, cTempl.prhct, cTempl.prltat, cTempl.prltat, cTempl.npcybr, cTempl.prrept];

	for (var i = 0; i < 10; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	camSendReinforcement(CAM_HUMAN_PLAYER, camMakePos("landingZone"), droids,
		CAM_REINFORCE_TRANSPORT, {
			entry: { x: 87, y: 126 },
			exit: { x: 87, y: 126 }
		}
	);

	queue("sendPlayerTransporter", ti);
}

//Continuously spawns heavy units on the north part of the map every 7 minutes
function mapEdgeDroids()
{
	var TankNum = 8 + camRand(6);
	var list = [cTempl.npcybm, cTempl.npcybr, cTempl.comct, cTempl.cohct];

	var droids = [];
	for (var i = 0; i < TankNum; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	camSendReinforcement(THE_COLLECTIVE, camMakePos("groundUnitPos"), droids, CAM_REINFORCE_GROUND);
	queue("mapEdgeDroids", camChangeOnDiff(420000)); //7 min
}

function vtolAttack()
{
	var list = [cTempl.colcbv];
	camSetVtolData(THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(180000), "COCommandCenter"); //3 min
}

function groupPatrol()
{
	camManageGroup(camMakeGroup("edgeGroup"), CAM_ORDER_ATTACK, {
		regroup: true,
		count: -1,
	});

	camManageGroup(camMakeGroup("IDFGroup"), CAM_ORDER_DEFEND, {
		pos: [
			camMakePos("waypoint1"),
			camMakePos("waypoint2")
		]
	});

	camManageGroup(camMakeGroup("sensorGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("waypoint1"),
			camMakePos("waypoint2")
		]
	});
}

//Build defenses around oil resource
function truckDefense()
{
	if (enumDroid(THE_COLLECTIVE, DROID_CONSTRUCT).length > 0)
	{
		const DEFENSES = ["WallTower06", "PillBox1", "WallTower03"];
		camQueueBuilding(THE_COLLECTIVE, DEFENSES[camRand(DEFENSES.length)]);

		queue("truckDefense", 160000);
	}
}

//Gives starting tech and research.
function cam2Setup()
{
	var x = 0;
	const COLLECTIVE_RES = [
		"R-Wpn-MG1Mk1", "R-Sys-Engineering02",
		"R-Defense-WallUpgrade03", "R-Struc-Materials03",
		"R-Struc-Factory-Upgrade03", "R-Struc-Factory-Cyborg-Upgrade03",
		"R-Vehicle-Engine03", "R-Vehicle-Metals03", "R-Cyborg-Metals03",
		"R-Wpn-Cannon-Accuracy01", "R-Wpn-Cannon-Damage03",
		"R-Wpn-Cannon-ROF01", "R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01",
		"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF02", "R-Wpn-Mortar-Acc01",
		"R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-ROF01",
		"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage03",
		"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
		"R-Wpn-RocketSlow-Damage03", "R-Sys-Sensor-Upgrade01"
	];

	for (x = 0; x < ALPHA_TECH.length; ++x)
	{
		makeComponentAvailable(ALPHA_TECH[x], CAM_HUMAN_PLAYER);
	}

	for (x = 0; x < STRUCTS_ALPHA.length; ++x)
	{
		enableStructure(STRUCTS_ALPHA[x], CAM_HUMAN_PLAYER);
	}

	camCompleteRequiredResearch(PLAYER_RES_BETA, CAM_HUMAN_PLAYER);
	camCompleteRequiredResearch(COLLECTIVE_RES, THE_COLLECTIVE);
	camCompleteRequiredResearch(ALPHA_RESEARCH, CAM_HUMAN_PLAYER);
	enableResearch("R-Wpn-Cannon-Damage04", CAM_HUMAN_PLAYER);
	enableResearch("R-Wpn-Rocket-Damage04", CAM_HUMAN_PLAYER);
	preDamageStuff();
}

//Get some higher rank droids at start and first Transport drop.
function setUnitRank()
{
	const DROID_EXP = 32;
	const MIN_TO_AWARD = 16;
	var droids = enumDroid(CAM_HUMAN_PLAYER).filter(function(dr) {
		return (!camIsSystemDroid(dr) && !camIsTransporter(dr));
	});

	for (var j = 0, i = droids.length; j < i; ++j)
	{
		var droid = droids[j];
		if (Math.floor(droid.experience) < MIN_TO_AWARD)
		{
			setDroidExperience(droids[j], DROID_EXP);
		}
	}
}

//Bump the rank of the first batch of transport droids as a reward.
function eventTransporterLanded(transport)
{
	if (transport.player === CAM_HUMAN_PLAYER)
	{
		if (startedFromMenu)
		{
			camCallOnce("setUnitRank");
		}

		if (!camDef(index))
		{
			index = 0;
		}

		index = index + 1;
		if (index >= TRANSPORT_LIMIT)
		{
			queue("downTransporter", 60000);
		}
	}
}

//Warn that something bad happened to the fifth transport
function reallyDownTransporter()
{
	setReinforcementTime(LZ_COMPROMISED_TIME);
	playSound("pcv443.ogg");
}

function downTransporter()
{
	camCallOnce("reallyDownTransporter");
}

function eventTransporterLaunch(transport)
{
	if (index >= TRANSPORT_LIMIT)
	{
		queue("downTransporter", 60000);
	}
}

function eventGameLoaded()
{
	if (index >= TRANSPORT_LIMIT)
	{
		setReinforcementTime(LZ_COMPROMISED_TIME);
	}
}

function eventStartLevel()
{
	const MISSION_TIME = camChangeOnDiff(3600); //60 minutes.
	const PLAYER_POWER = (difficulty === INSANE) ? 9000 : 5000;
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone"); //player lz
	var enemyLz = getObject("COLandingZone");
	var tent = getObject("transporterEntry");
	var text = getObject("transporterExit");

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_2_1S");
	setReinforcementTime(LZ_COMPROMISED_TIME);

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, THE_COLLECTIVE);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	camSetArtifacts({
		"COCommandCenter": { tech: "R-Sys-Engineering02" },
		"COArtiPillbox": { tech: "R-Wpn-MG-ROF02" },
		"COArtiCBTower": { tech: "R-Sys-Sensor-Upgrade01" },
	});

	setMissionTime(MISSION_TIME);
	setPower(PLAYER_POWER, CAM_HUMAN_PLAYER);
	cam2Setup();

	//C2A_BASE2 is not really a base
	camSetEnemyBases({
		"CONorthBase": {
			cleanup: "CONorth",
			detectMsg: "C2A_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"CONorthWestBase": {
			cleanup: "CONorthWest",
			detectMsg: "C2A_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
	});

	camManageTrucks(THE_COLLECTIVE);
	truckDefense();
	setUnitRank(); //All pre-placed player droids are ranked.
	camPlayVideos("MB2A_MSG");
	startedFromMenu = false;

	//Only if starting Beta directly rather than going through Alpha
	if (enumDroid(CAM_HUMAN_PLAYER, DROID_TRANSPORTER).length === 0)
	{
		startedFromMenu = true;
		sendPlayerTransporter();
	}
	else
	{
		setReinforcementTime(300); // 5 min.
	}

	queue("secondVideo", 12000); // 12 sec
	queue("truckDefense", 15000);// 15 sec.
	queue("groupPatrol", camChangeOnDiff(60000)); // 60 sec
	queue("sendCOTransporter", camChangeOnDiff(240000)); //4 min
	queue("vtolAttack", camChangeOnDiff(180000)); //3 min
	queue("mapEdgeDroids", camChangeOnDiff(420000)); //7 min
}
