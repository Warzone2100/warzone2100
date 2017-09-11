
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const civilians = 7; //Civilian player number.
var capturedCivCount; //How many civilians have been captured. 59 for defeat.
var civilianPosIndex; //Current location of civilian groups.
var shepardGroup; //Enemy group that protects civilians.
const COLLECTIVE_RES = [
	"R-Defense-WallUpgrade03", "R-Struc-Materials04",
	"R-Struc-Factory-Upgrade04", "R-Struc-VTOLFactory-Upgrade01",
	"R-Struc-VTOLPad-Upgrade01", "R-Struc-Factory-Cyborg-Upgrade04",
	"R-Vehicle-Engine04", "R-Vehicle-Metals03", "R-Cyborg-Metals04",
	"R-Vehicle-Armor-Heat02", "R-Cyborg-Armor-Heat02",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage04",
	"R-Wpn-Cannon-ROF03", "R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03",
	"R-Wpn-MG-Damage07", "R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02",
	"R-Wpn-Mortar-Damage06", "R-Wpn-Mortar-ROF03",
	"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage05", "R-Sys-Sensor-Upgrade01",
	"R-Struc-VTOLFactory-Upgrade01", "R-Struc-VTOLPad-Upgrade01",
	"R-Sys-Engineering02", "R-Wpn-Howitzer-Accuracy02",
	"R-Wpn-Howitzer-Damage02", "R-Wpn-RocketSlow-ROF03",
];

//Play video about civilians being captured by the Collective. Triggered
//by destroying the air base or crossing the base3Trigger area.
function videoTrigger()
{
	setMissionTime(getMissionTime() + camChangeOnDiff(1800)); //+30 minutes
	civilianOrders();
	captureCivilians();

	hackRemoveMessage("C2C_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);
	hackAddMessage("MB2_C_MSG2", MISS_MSG, CAM_HUMAN_PLAYER, true);
	hackAddMessage("C2C_OBJ2", PROX_MSG, CAM_HUMAN_PLAYER, true);
}

//Enable heavy factories and make groups do what they need to.
camAreaEvent("groupTrigger", function()
{
	activateGroups();
});

//Play second video and add 30 minutes to timer when base3Trigger is crossed.
camAreaEvent("base3Trigger", function()
{
	camCallOnce("videoTrigger");
});

//...Or if the player destroys the VTOL base.
function camEnemyBaseEliminated_COAirBase()
{
	camCallOnce("videoTrigger");
}


function enableFactories()
{
	camEnableFactory("COHeavyFac-Leopard");
	camEnableFactory("COHeavyFac-Upgrade");
	camEnableFactory("COVtolFacLeft-Prop");
	camEnableFactory("COVtolFacRight");
	camEnableFactory("COCyborgFactoryL");
	camEnableFactory("COCyborgFactoryR");
}

//Enable Groups after 8 minutes or player enters groupTrigger area.
//GroundWaypoint1 is included, but is unused in the WZScript version. Also
//the defense group patrols are unused, but cause path problems anyway.
function activateGroups()
{
	camManageGroup(camMakeGroup("heavyGroup1"), CAM_ORDER_PATROL, {
		pos: [
			//camMakePos("groundWayPoint1"),
			camMakePos("groundWayPoint2"),
			camMakePos("groundWayPoint3"),
		],
		//morale: 50,
		interval: 70000,
		regroup: false,
	});

	camManageGroup(camMakeGroup("cyborgGroup1"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("oilDerrick"),
			camMakePos("centerOfPlayerBase"),
		],
		//morale: 50,
		interval: 40000,
		regroup: false,
	});

	/*
	camManageGroup(camMakeGroup("defensegroup1"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("defensePoint1"),
			camMakePos("defensePoint2"),
		],
		//morale: 50,
		interval: 25000,
		regroup: false,
	});
	*/

	camManageGroup(camMakeGroup("vtolGroup1"), CAM_ORDER_PATROL, {
		pos: [
			//camMakePos("VtolWayPoint1"),
			camMakePos("vtolWayPoint2"),
			camMakePos("vtolWayPoint3"),
			camMakePos("centerOfPlayerBase"),
			camMakePos("oilDerrick"),
		],
		//morale: 50,
		interval: 30000,
		regroup: false,
	});
}

function truckDefense()
{
	if (enumDroid(THE_COLLECTIVE, DROID_CONSTRUCT).length > 0)
	{
		queue("truckDefense", 160000);
	}

	const LIST = ["WallTower06", "PillBox1", "WallTower03"];
	camQueueBuilding(THE_COLLECTIVE, LIST[camRand(LIST.length)]);
}

//This controls the collective cyborg shepard groups and moving civilians
//to the transporter pickup zone. Civilians spawn around each waypoint (stops
//when all of the shepard group members are destroyed).
function captureCivilians()
{
	var wayPoints = [
		"civPoint1", "civPoint2", "civPoint3", "civPoint4",
		"civPoint5", "civPoint6", "civPoint7", "civCapturePos"
	];
	var currPos = getObject(wayPoints[civilianPosIndex]);
	var shepardDroids = enumGroup(shepardGroup);

	if (shepardDroids.length)
	{
		//add some civs
		var num = 1 + camRand(3);
		for (var i = 0; i < num; ++i)
		{
			addDroid(civilians, currPos.x, currPos.y, "Civilian",
					"B1BaBaPerson01", "BaBaLegs", "", "", "BaBaMG");
		}

		//Only count civilians that are not in the the transporter base.
		var civs = enumArea(0, 0, 35, mapHeight, civilians, false);
		//Move them
		for (var i = 0; i < civs.length; ++i)
		{
			orderDroidLoc(civs[i], DORDER_MOVE, currPos.x, currPos.y);
		}

		if (civilianPosIndex <= 5)
		{
			for (var i = 0; i < shepardDroids.length; ++i)
			{
				orderDroidLoc(shepardDroids[i], DORDER_MOVE, currPos.x, currPos.y);
			}
		}

		if (civilianPosIndex === 7)
		{
			queue("sendCOTransporter", 6000);
		}
		civilianPosIndex = (civilianPosIndex > 6) ? 0 : (civilianPosIndex + 1);
		queue("captureCivilians", camChangeOnDiff(10000)); //10 sec.
	}
}

//When rescued, the civilians will make their way towards the player's LZ
//before removal.
function civilianOrders()
{
	var lz = getObject("startPosition");
	var rescueSound = "pcv612.ogg";	//"Civilian Rescued".
	var civs = enumDroid(civilians);
	var rescued = false;

	//Check if a civilian is close to a player droid.
	for (var i = 0; i < civs.length; ++i)
	{
		var pDroids = enumRange(civs[i].x, civs[i].y, 6, CAM_HUMAN_PLAYER, false);
		if (pDroids.length)
		{
			rescued = true;
			orderDroidLoc(civs[i], DORDER_MOVE, lz.x, lz.y);
		}
	}

	if (rescued === true)
	{
		playSound(rescueSound);
	}

	queue("civilianOrders", 2000);
}

//Capture civilans.
function eventTransporterLanded(transport)
{
	var escaping = "pcv632.ogg"; //"Enemy escaping".
	var position = getObject("COTransportPos");
	var civs = enumRange(position.x, position.y, 15, civilians, false);

	if (civs.length)
	{
		playSound(escaping);
		capturedCivCount += civs.length - 1;
		for (var i = 0; i < civs.length; ++i)
		{
			camSafeRemoveObject(civs[i], false);
		}
	}
}

//Send Collective transport as long as the player has not entered the base.
function sendCOTransporter()
{
	var list; with (camTemplates) list = [npcybr, npcybr];
	var tPos = getObject("COTransportPos");
	var pDroid = enumRange(tPos.x, tPos.y, 6, CAM_HUMAN_PLAYER, false);

	if (!pDroid.length)
	{
		camSendReinforcement(THE_COLLECTIVE, camMakePos("COTransportPos"), list,
			CAM_REINFORCE_TRANSPORT, {
				entry: { x: 1, y: 80 },
				exit: { x: 1, y: 80 }
			}
		);
	}
}

//Check if too many civilians have been captured by the Collective.
//This will automatically check for civs near landing zones and remove them.
function extraVictoryCondition()
{
	const LIMIT = 59;

	if (capturedCivCount >= LIMIT)
	{
		return false; //instant defeat
	}
	else
	{
		var lz = getObject("startPosition");
		var civs = enumRange(lz.x, lz.y, 30, civilians, false);

		for (var i = 0; i < civs.length; ++i)
		{
			camSafeRemoveObject(civs[i], false);
		}

		if (!enumArea(0, 0, mapWidth, mapHeight, civilians, false).length)
		{
			return true;
		}
	}
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_2_5S", {
		callback: "extraVictoryCondition"
	});

	var startpos = getObject("startPosition");
	var lz = getObject("landingZone"); //player lz
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	camSetArtifacts({
		"rippleRocket": { tech: "R-Wpn-Rocket06-IDF" },
		"quadbof": { tech: "R-Wpn-AAGun02" },
		"howitzer": { tech: "R-Wpn-HowitzerMk1" },
		"COHeavyFac-Leopard": { tech: "R-Vehicle-Body02" }, //leopard
		"COHeavyFac-Upgrade": { tech: "R-Struc-Factory-Upgrade04" },
		"COVtolFacLeft-Prop": { tech: "R-Vehicle-Prop-VTOL" },
	});

	setPower(camChangeOnDiff(200000, true), THE_COLLECTIVE);
	setMissionTime(camChangeOnDiff(7200)); //120 min

	setAlliance(THE_COLLECTIVE, civilians, true);
	setAlliance(CAM_HUMAN_PLAYER, civilians, true);
	camCompleteRequiredResearch(COLLECTIVE_RES, THE_COLLECTIVE);

	camSetEnemyBases({
		"COAirBase": {
			cleanup: "airBaseCleanup",
			detectMsg: "C2C_BASE1",
			detectSnd: "pcv393.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"COCyborgBase": {
			cleanup: "cyborgBaseCleanup",
			detectMsg: "C2C_BASE2",
			detectSnd: "pcv393.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"COtransportBase": {
			cleanup: "transportBaseCleanup",
			detectMsg: "C2C_BASE3",
			detectSnd: "pcv393.ogg",
			eliminateSnd: "pcv394.ogg",
		},
	});

	with (camTemplates) camSetFactories({
		"COHeavyFac-Upgrade": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(120000),
			regroup: false,
			repair: 40,
			templates: [comit, cohct, comhpv, cohbbt]
		},
		"COHeavyFac-Leopard": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(100000),
			regroup: false,
			repair: 40,
			templates: [comit, cohct, comhpv, cohbbt]
		},
		"COCyborgFactoryL": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(80000),
			regroup: true,
			repair: 40,
			templates: [npcybf, npcybc, npcybr]
		},
		"COCyborgFactoryR": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(80000),
			regroup: true,
			repair: 40,
			templates: [npcybf, npcybc, npcybr]
		},
		"COVtolFacLeft-Prop": {
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(100000),
			regroup: false,
			repair: 40,
			templates: [commorv, colagv, colatv]
		},
		"COVtolFacRight": {
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(100000),
			regroup: false,
			repair: 40,
			templates: [colatv, colagv, commorv]
		},
	});

	camManageTrucks(THE_COLLECTIVE);
	truckDefense();
	capturedCivCount = 0;
	civilianPosIndex = 0;
	shepardGroup = camMakeGroup("heavyGroup2");
	enableFactories();

	hackAddMessage("MB2_C_MSG", MISS_MSG, CAM_HUMAN_PLAYER, true);
	hackAddMessage("C2C_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);

	queue("activateGroups", camChangeOnDiff(480000)); //8 min
}
