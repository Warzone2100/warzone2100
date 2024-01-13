include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var capturedCivCount; //How many civilians have been captured. 59 for defeat.
var civilianPosIndex; //Current location of civilian groups.
var shepardGroup; //Enemy group that protects civilians.
var lastSoundTime; //Only play the "civilian rescued" sound every so often.
const mis_collectiveRes = [
	"R-Defense-WallUpgrade05", "R-Struc-Materials05", "R-Sys-Engineering02",
	"R-Vehicle-Engine04", "R-Vehicle-Metals05", "R-Cyborg-Metals05",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage05", "R-Wpn-Cannon-ROF01",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage06",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage05",
	"R-Wpn-Mortar-ROF02", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage05",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage05",
	"R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF01", "R-Wpn-Howitzer-ROF01",
	"R-Wpn-Howitzer-Damage07", "R-Cyborg-Armor-Heat01", "R-Vehicle-Armor-Heat01",
	"R-Wpn-Bomb-Damage01", "R-Wpn-AAGun-Damage03", "R-Wpn-AAGun-ROF02",
	"R-Wpn-AAGun-Accuracy01", "R-Struc-VTOLPad-Upgrade01"
];

//Play video about civilians being captured by the Collective. Triggered
//by destroying the air base or crossing the base3Trigger area.
function videoTrigger()
{
	camSetExtraObjectiveMessage(_("Rescue the civilians from The Collective before too many are captured"));

	setMissionTime(getMissionTime() + camChangeOnDiff(camMinutesToSeconds(30)));
	setTimer("civilianOrders", camSecondsToMilliseconds(2));
	setTimer("captureCivilians", camChangeOnDiff(camSecondsToMilliseconds(10)));

	hackRemoveMessage("C2C_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
	camPlayVideos({video: "MB2_C_MSG2", type: MISS_MSG});
	hackAddMessage("C2C_OBJ2", PROX_MSG, CAM_HUMAN_PLAYER, false);
}

//Enable heavy factories and make groups do what they need to.
camAreaEvent("groupTrigger", function(droid)
{
	activateGroups();
});

//Play second video and add 30 minutes to timer when base3Trigger is crossed.
camAreaEvent("base3Trigger", function(droid)
{
	camCallOnce("videoTrigger");
});

//Send idle droids in this base to attack when the player spots the base
function camEnemyBaseDetected_COAirBase()
{
	const droids = enumArea("airBaseCleanup", CAM_THE_COLLECTIVE, false).filter((obj) => (
		obj.type === DROID && obj.group === null
	));

	camManageGroup(camMakeGroup(droids), CAM_ORDER_ATTACK, {
		count: -1,
		regroup: false,
		repair: 67
	});
}

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
		interval: camSecondsToMilliseconds(70),
		regroup: false,
	});

	camManageGroup(camMakeGroup("cyborgGroup1"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("oilDerrick"),
			camMakePos("centerOfPlayerBase"),
		],
		//morale: 50,
		interval: camSecondsToMilliseconds(40),
		regroup: false,
	});

	camManageGroup(camMakeGroup("vtolGroup1"), CAM_ORDER_PATROL, {
		pos: [
			//camMakePos("VtolWayPoint1"),
			camMakePos("vtolWayPoint2"),
			camMakePos("vtolWayPoint3"),
			camMakePos("centerOfPlayerBase"),
			camMakePos("oilDerrick"),
		],
		//morale: 50,
		interval: camSecondsToMilliseconds(30),
		regroup: false,
	});
}

function truckDefense()
{
	if (enumDroid(CAM_THE_COLLECTIVE, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	const list = ["CO-Tower-LtATRkt", "PillBox1", "CO-WallTower-HvCan"];
	camQueueBuilding(CAM_THE_COLLECTIVE, list[camRand(list.length)]);
}

//This controls the collective cyborg shepard groups and moving civilians
//to the transporter pickup zone. Civilians spawn around each waypoint (stops
//when all of the shepard group members are destroyed).
function captureCivilians()
{
	const wayPoints = [
		"civPoint1", "civPoint2", "civPoint3", "civPoint4",
		"civPoint5", "civPoint6", "civPoint7", "civCapturePos"
	];
	const currPos = getObject(wayPoints[civilianPosIndex]);
	const shepardDroids = enumGroup(shepardGroup);

	if (shepardDroids.length > 0)
	{
		//add some civs
		const NUM = 1 + camRand(3);
		for (let i = 0; i < NUM; ++i)
		{
			addDroid(CAM_SCAV_7, currPos.x, currPos.y, "Civilian",
					tBody.scav.human, tProp.scav.legs, "", "", tWeap.scav.machinegun);
		}

		//Only count civilians that are not in the the transporter base.
		const civs = enumArea(0, 0, 35, mapHeight, CAM_SCAV_7, false);
		//Move them
		for (let i = 0; i < civs.length; ++i)
		{
			orderDroidLoc(civs[i], DORDER_MOVE, currPos.x, currPos.y);
		}

		if (civilianPosIndex <= 5)
		{
			for (let i = 0; i < shepardDroids.length; ++i)
			{
				orderDroidLoc(shepardDroids[i], DORDER_MOVE, currPos.x, currPos.y);
			}
		}

		if (civilianPosIndex === 7)
		{
			queue("sendCOTransporter", camSecondsToMilliseconds(6));
		}
		civilianPosIndex = (civilianPosIndex > 6) ? 0 : (civilianPosIndex + 1);
	}
	else
	{
		removeTimer("captureCivilians");
	}
}

//When rescued, the civilians will make their way towards the player's LZ
//before removal.
function civilianOrders()
{
	const lz = getObject("startPosition");
	const civs = enumDroid(CAM_SCAV_7);
	let rescued = false;

	//Check if a civilian is close to a player droid.
	for (let i = 0; i < civs.length; ++i)
	{
		const objs = enumRange(civs[i].x, civs[i].y, 6, CAM_HUMAN_PLAYER, false);
		for (let j = 0; j < objs.length; ++j)
		{
			if (objs[j].type === DROID)
			{
				rescued = true;
				orderDroidLoc(civs[i], DORDER_MOVE, lz.x, lz.y);
				break;
			}
		}
	}

	//Play the "Civilian rescued" sound and throttle it.
	if (rescued && ((lastSoundTime + camSecondsToMilliseconds(30)) < gameTime))
	{
		lastSoundTime = gameTime;
		playSound(cam_sounds.rescue.civilianRescued);
	}
}

//Capture civilans.
function eventTransporterLanded(transport)
{
	const position = getObject("COTransportPos");
	const civs = enumRange(position.x, position.y, 15, CAM_SCAV_7, false);

	if (civs.length)
	{
		playSound(cam_sounds.enemyEscaping);
		capturedCivCount += civs.length - 1;
		for (let i = 0; i < civs.length; ++i)
		{
			camSafeRemoveObject(civs[i], false);
		}
	}
}

//Send Collective transport as long as the player has not entered the base.
function sendCOTransporter()
{
	const list = [cTempl.npcybr, cTempl.npcybr];
	const tPos = getObject("COTransportPos");
	const pDroid = enumRange(tPos.x, tPos.y, 6, CAM_HUMAN_PLAYER, false);

	if (!pDroid.length)
	{
		camSendReinforcement(CAM_THE_COLLECTIVE, camMakePos("COTransportPos"), list,
			CAM_REINFORCE_TRANSPORT, {
				entry: { x: 2, y: 80 },
				exit: { x: 2, y: 80 }
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
		const lz = getObject("startPosition");
		const civs = enumRange(lz.x, lz.y, 30, CAM_SCAV_7, false);

		for (let i = 0; i < civs.length; ++i)
		{
			camSafeRemoveObject(civs[i], false);
		}

		//Win regardless if all civilians do not make it to the LZ.
		return true;
	}
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_2_5S", {
		callback: "extraVictoryCondition"
	});

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone"); //player lz
	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	const enemyLz = getObject("COLandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, 5);

	camSetArtifacts({
		"rippleRocket": { tech: "R-Wpn-Rocket06-IDF" },
		"quadbof": { tech: "R-Wpn-AAGun02" },
		"howitzer": { tech: "R-Wpn-HowitzerMk1" },
		"COHeavyFac-Leopard": { tech: "R-Vehicle-Body06" }, //Panther
		"COHeavyFac-Upgrade": { tech: "R-Struc-Factory-Upgrade04" },
		"COVtolFacLeft-Prop": { tech: "R-Vehicle-Prop-VTOL" },
		"COInfernoEmplacement-Arti": { tech: "R-Wpn-Flamer-ROF02" },
	});

	setMissionTime(camChangeOnDiff(camHoursToSeconds(2)));

	setAlliance(CAM_THE_COLLECTIVE, CAM_SCAV_7, true);
	setAlliance(CAM_HUMAN_PLAYER, CAM_SCAV_7, true);
	camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);

	if (difficulty >= MEDIUM)
	{
		camUpgradeOnMapTemplates(cTempl.commc, cTempl.commrp, CAM_THE_COLLECTIVE);
	}
	camUpgradeOnMapTemplates(cTempl.npcybf, cTempl.cocybth, CAM_THE_COLLECTIVE);

	camSetEnemyBases({
		"COAirBase": {
			cleanup: "airBaseCleanup",
			detectMsg: "C2C_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COCyborgBase": {
			cleanup: "cyborgBaseCleanup",
			detectMsg: "C2C_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COtransportBase": {
			cleanup: "transportBaseCleanup",
			detectMsg: "C2C_BASE3",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"COHeavyFac-Upgrade": {
			assembly: "COHeavyFac-UpgradeAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: [cTempl.comorb, cTempl.comit, cTempl.cohct, cTempl.comhpv, cTempl.cohbbt, cTempl.comsens]
		},
		"COHeavyFac-Leopard": {
			assembly: "COHeavyFac-LeopardAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: [cTempl.comsens, cTempl.cohbbt, cTempl.comhpv, cTempl.cohct, cTempl.comit, cTempl.comorb]
		},
		"COCyborgFactoryL": {
			assembly: "COCyborgFactoryLAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.cocybth, cTempl.npcybc, cTempl.npcybr]
		},
		"COCyborgFactoryR": {
			assembly: "COCyborgFactoryRAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.cocybth, cTempl.npcybc, cTempl.npcybr]
		},
		"COVtolFacLeft-Prop": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(50)),
			data: {
				regroup: false,
				count: -1,
			},
			templates: [cTempl.commorv, cTempl.colagv]
		},
		"COVtolFacRight": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(50)),
			data: {
				regroup: false,
				count: -1,
			},
			templates: [cTempl.colagv, cTempl.commorv]
		},
	});

	camManageTrucks(CAM_THE_COLLECTIVE);
	capturedCivCount = 0;
	civilianPosIndex = 0;
	lastSoundTime = 0;
	shepardGroup = camMakeGroup("heavyGroup2");
	enableFactories();

	camPlayVideos({video: "MB2_C_MSG", type: MISS_MSG});
	hackAddMessage("C2C_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);

	queue("activateGroups", camChangeOnDiff(camMinutesToMilliseconds(8)));
	setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(3)));

	truckDefense();
}
