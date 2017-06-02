include("script/campaign/transitionTech.js");
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var index; //Number of transport loads sent into the level


camAreaEvent("vtolRemoveZone", function(droid)
{
	camSafeRemoveObject(droid, false);
	resetLabel("vtolRemoveZone", THE_COLLECTIVE);
});

//Attack and destroy all those who resist the Machine! -The Collective
function secondVideo()
{
	hackAddMessage("MB2A_MSG2", MISS_MSG, CAM_HUMAN_PLAYER, true);
}

//Damage the base and droids for the player
function preDamageStuff()
{
	var droids = enumDroid(CAM_HUMAN_PLAYER);
	var structures = enumStruct(CAM_HUMAN_PLAYER);

	for(var x = 0; x < droids.length; ++x)
	{
		var droid = droids[x];
		setHealth(droid, 45 + camRand(20));
	}

	for(var x = 0; x < structures.length; ++x)
	{
		var struc = structures[x];
		setHealth(struc, 45 + camRand(45));
	}
}

function getDroidsForCOLZ()
{
	const count = [3,3,3,4,4,4,4,4,4][camRand(9)];
	var t;
	var scouts;
	var artillery;
	with (camTemplates) scouts = [npcybm, npcybc, commgt, comsens];
	with (camTemplates) artillery = [npcybc, npcybr, cohct, comct, comorb];

	var droids = [];
	for (var i = 0; i < count; ++i)
	{
		if(camRand(3) === 0)
		{
			t = scouts[camRand(scouts.length)];
		}
		else
		{
			t = artillery[camRand(artillery.length)];
		}

		droids.push(t);
		droids.push(t);
	}

	return droids;
}

//Send Collective tranport units
function sendCOTransporter()
{
	var list = getDroidsForCOLZ();
	camSendReinforcement(THE_COLLECTIVE, camMakePos("COTransportPos"), list,
		CAM_REINFORCE_TRANSPORT, {
			entry: { x: 126, y: 100 },
			exit: { x: 126, y: 70 }
		}
	);

	queue("sendCOTransporter", camChangeOnDiff(300000)); //5 min
}

//Extra transport units are only awarded to those who start Beta campaign
//from the main menu. Otherwise a player can just bring in there Alpha units
function sendPlayerTransporter()
{
	const TRANSPORT_LIMIT = 4;

	if(!camDef(index))
	{
		index = 0;
	}

	if(index === TRANSPORT_LIMIT)
	{
		return;
	}

	var droids = [];
	var list;
	with (camTemplates) list = [prhct, prltat, npcybr, prrept];

	for(var i = 0; i < 10; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	camSendReinforcement(CAM_HUMAN_PLAYER, camMakePos("landingZone"), droids,
		CAM_REINFORCE_TRANSPORT, {
			entry: { x: 87, y: 126 },
			exit: { x: 87, y: 126 }
		}
	);

	index = index + 1;
	queue("sendPlayerTransporter", camChangeOnDiff(300000, true)); //5 min
}

//Continuously spawns heavy units on the north part of the map every 7 minutes
function mapEdgeDroids()
{
	var TankNum = 8 + camRand(6);
	var list;
	with (camTemplates) list = [npcybm, npcybr, comct, cohct];

	var droids = [];
	for (var i = 0; i < TankNum; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	camSendReinforcement(THE_COLLECTIVE, camMakePos("groundUnitPos"), droids, CAM_REINFORCE_GROUND);
	queue("mapEdgeDroids", camChangeOnDiff(420000)); //7 min
}

//Stop VTOLs from coming if HQ is destroyed.
function checkCollectiveHQ()
{
	if(getObject("COCommandCenter") === null)
	{
		camToggleVtolSpawn();
	}
	else
	{
		queue("checkCollectiveHQ", 8000);
	}
}

function vtolAttack()
{
	var list; with (camTemplates) list = [colcbv, colatv];
	camSetVtolData(THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(300000)); //5 min
	checkCollectiveHQ();
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
	var truck = enumDroid(THE_COLLECTIVE, DROID_CONSTRUCT);
	if(enumDroid(THE_COLLECTIVE, DROID_CONSTRUCT).length > 0)
		queue("truckDefense", 160000);

	const DEFENSES = ["WallTower06", "PillBox1", "WallTower03"];
	camQueueBuilding(THE_COLLECTIVE, DEFENSES[camRand(DEFENSES.length)]);
}

//Gives starting tech and research.
function cam2Setup()
{
	const COLLECTIVE_RES = [
		"R-Wpn-MG1Mk1", "R-Sys-Engineering02",
		"R-Defense-WallUpgrade03", "R-Struc-Materials03",
		"R-Struc-Factory-Upgrade03", "R-Struc-Factory-Cyborg-Upgrade03",
		"R-Vehicle-Engine03", "R-Vehicle-Metals03", "R-Cyborg-Metals03",
		"R-Vehicle-Armor-Heat01", "R-Cyborg-Armor-Heat01",
		"R-Wpn-Cannon-Accuracy01", "R-Wpn-Cannon-Damage03",
		"R-Wpn-Cannon-ROF01", "R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01",
		"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF02", "R-Wpn-Mortar-Acc01",
		"R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-ROF01",
		"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage03",
		"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
		"R-Wpn-RocketSlow-Damage03", "R-Sys-Sensor-Upgrade01"
	];

	for( var x = 0; x < ALPHA_TECH.length; ++x)
	{
		makeComponentAvailable(ALPHA_TECH[x], CAM_HUMAN_PLAYER);
	}

	for( var x = 0; x < STRUCTS_ALPHA.length; ++x)
	{
		enableStructure(STRUCTS_ALPHA[x], CAM_HUMAN_PLAYER);
	}

	camCompleteRequiredResearch(PLAYER_RES_BETA, CAM_HUMAN_PLAYER);
	camCompleteRequiredResearch(COLLECTIVE_RES, THE_COLLECTIVE);
	camCompleteRequiredResearch(ALPHA_RESEARCH, CAM_HUMAN_PLAYER);
	enableResearch("R-Wpn-Cannon-Accuracy02", CAM_HUMAN_PLAYER);
	preDamageStuff();
}


function eventStartLevel()
{
	const MISSION_TIME = camChangeOnDiff(3600); //60 minutes.
	const PLAYER_POWER = 5000;
	const COLLECTIVE_POWER = camChangeOnDiff(10000, true);
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
	setPower(COLLECTIVE_POWER, THE_COLLECTIVE);
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
	hackAddMessage("MB2A_MSG", MISS_MSG, CAM_HUMAN_PLAYER, true);

	//Only if starting Beta directly rather than going through Alpha
	if(enumDroid(CAM_HUMAN_PLAYER, DROID_TRANSPORTER).length === 0) {
		sendPlayerTransporter();
	}
	else {
		setReinforcementTime(camChangeOnDiff(300, true)); // 5 min.
	}

	queue("secondVideo", 12000); // 12 sec
	queue("truckDefense", 15000);// 15 sec.
	queue("sendCOTransporter", 30000); //30 sec
	queue("groupPatrol", 160000); // ~3 min
	queue("vtolAttack", 300000); //5 min
	queue("mapEdgeDroids", 420000); //7 min
}
