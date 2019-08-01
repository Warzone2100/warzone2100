
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEW_PARADIGM_RES = [
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF01", "R-Defense-WallUpgrade02",
	"R-Struc-Materials02", "R-Struc-Factory-Upgrade02",
	"R-Struc-Factory-Cyborg-Upgrade02", "R-Vehicle-Engine02",
	"R-Vehicle-Metals02", "R-Cyborg-Metals02", "R-Wpn-Cannon-Damage03",
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01",
	"R-Wpn-Mortar-Damage02", "R-Wpn-Rocket-Accuracy01",
	"R-Wpn-Rocket-Damage02", "R-Wpn-Rocket-ROF01",
	"R-Wpn-RocketSlow-Damage02", "R-Struc-RprFac-Upgrade03",
];

const SCAVENGER_RES = [
	"R-Wpn-MG-Damage03", "R-Wpn-Rocket-Damage02",
];

//Pursue player when nearby but do not go too far away from defense zone.
function camEnemyBaseDetected_NPBaseGroup()
{
	camCallOnce("NPBaseDetect");
}

function enableSouthScavFactory()
{
	camEnableFactory("SouthScavFactory");
}

camAreaEvent("NorthScavFactoryTrigger", function()
{
	camEnableFactory("NorthScavFactory");
});

camAreaEvent("NPBaseDetectTrigger", function()
{
	camDetectEnemyBase("NPBaseGroup");
});

camAreaEvent("removeRedObjectiveBlip", function()
{
	hackRemoveMessage("C1-4_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER); //Remove mission objective.
});

camAreaEvent("triggerLZ2Blip", function()
{
	hackAddMessage("C1-4_LZ", PROX_MSG, CAM_HUMAN_PLAYER, false);
});

camAreaEvent("LandingZoneTrigger", function()
{
	camPlayVideos(["pcv456.ogg", "SB1_4_B"]);
	hackRemoveMessage("C1-4_LZ", PROX_MSG, CAM_HUMAN_PLAYER); //Remove LZ 2 blip.

	var lz = getObject("LandingZone2"); // will override later
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	// Give extra 30 minutes.
	setMissionTime(camChangeOnDiff(camMinutesToSeconds(30)) + getMissionTime());
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_5S", {
		area: "RTLZ",
		message: "C1-4_LZ",
		reinforcements: camMinutesToSeconds(1.5), // changes!
		retlz: true
	});
	// enables all factories
	camEnableFactory("SouthScavFactory");
	camEnableFactory("NorthScavFactory");
	camEnableFactory("HeavyNPFactory");
	camEnableFactory("MediumNPFactory");
	buildDefenses();
});

function NPBaseDetect()
{
	// Send tanks
	camManageGroup(camMakeGroup("AttackGroupLight"), CAM_ORDER_DEFEND, {
		pos: camMakePos("nearSensor"),
		radius: 10,
		regroup: true
	});

	camManageGroup(camMakeGroup("AttackGroupMedium"), CAM_ORDER_DEFEND, {
		pos: camMakePos("nearSensor"),
		radius: 10,
		regroup: true
	});

	camEnableFactory("HeavyNPFactory");
	camEnableFactory("MediumNPFactory");
}

function buildDefenses()
{
	// First wave of trucks
	camQueueBuilding(NEW_PARADIGM, "GuardTower6", "BuildTower0");
	camQueueBuilding(NEW_PARADIGM, "PillBox3",    "BuildTower3");
	camQueueBuilding(NEW_PARADIGM, "PillBox3",    "BuildTower6");

	// Second wave of trucks
	camQueueBuilding(NEW_PARADIGM, "GuardTower3", "BuildTower1");
	camQueueBuilding(NEW_PARADIGM, "GuardTower6", "BuildTower2");
	camQueueBuilding(NEW_PARADIGM, "GuardTower6", "BuildTower4");

	// Third wave of trucks
	camQueueBuilding(NEW_PARADIGM, "GuardTower3", "BuildTower5");
	camQueueBuilding(NEW_PARADIGM, "GuardTower6", "BuildTower7");
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_5S", {
		area: "RTLZ",
		message: "C1-4_LZ",
		reinforcements: -1, // will override later
		retlz: true
	});

	var startpos = getObject("StartPosition");
	var lz = getObject("LandingZone1"); // will override later
	var tent = getObject("TransporterEntry");
	var text = getObject("TransporterExit");

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	camCompleteRequiredResearch(NEW_PARADIGM_RES, NEW_PARADIGM);
	camCompleteRequiredResearch(SCAVENGER_RES, SCAV_7);
	setAlliance(NEW_PARADIGM, SCAV_7, true);

	camSetEnemyBases({
		"SouthScavBaseGroup": {
			cleanup: "SouthScavBase",
			detectMsg: "C1-4_BASE1",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"NorthScavBaseGroup": {
			cleanup: "NorthScavBase",
			detectMsg: "C1-4_BASE3",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"NPBaseGroup": {
			cleanup: "NPBase",
			detectMsg: "C1-4_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg"
		},
	});

	hackAddMessage("C1-4_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);

	camSetArtifacts({
		"NPCommandCenter": { tech: "R-Vehicle-Metals01" },
		"NPResearchFacility": { tech: "R-Vehicle-Body04" },
		"MediumNPFactory": { tech: "R-Wpn-Rocket02-MRL" },
	});

	camSetFactories({
		"SouthScavFactory": {
			assembly: "SouthScavFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			maxSize: 6,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(20)),
			templates: [ cTempl.rbuggy, cTempl.bjeep, cTempl.buscan, cTempl.trike ]
		},
		"NorthScavFactory": {
			assembly: "NorthScavFactoryAssembly",
			order: CAM_ORDER_COMPROMISE,
			data: {
				pos: camMakePos("RTLZ"),
				radius: 8
			},
			groupSize: 4,
			maxSize: 6,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(20)),
			templates: [ cTempl.firecan, cTempl.rbjeep, cTempl.bloke, cTempl.buggy ]
		},
		"HeavyNPFactory": {
			assembly: "HeavyNPFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			maxSize: 6,         // this one was exclusively producing trucks
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),    // but we simplify this out
			templates: [ cTempl.npmmct, cTempl.npsmct, cTempl.npsmc ]
		},
		"MediumNPFactory": {
			assembly: "MediumNPFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			maxSize: 6,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			templates: [ cTempl.npmrl, cTempl.nphmg, cTempl.npsbb, cTempl.npmor ]
		},
	});

	// To be able to use camEnqueueBuilding() later,
	// and also to rebuild dead trucks.
	camManageTrucks(NEW_PARADIGM);

	queue("enableSouthScavFactory", camChangeOnDiff(camSecondsToMilliseconds(10)));
}
