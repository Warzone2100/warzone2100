
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
	"R-Wpn-MG-Damage03", "R-Wpn-Rocket-Damage02"
];

function camEnemyBaseDetected_NPBaseGroup()
{
	// First wave of trucks
	camQueueBuilding(1, "GuardTower6", "BuildTower0");
	camQueueBuilding(1, "PillBox3",    "BuildTower3");
	camQueueBuilding(1, "PillBox3",    "BuildTower6");

	// Second wave of trucks
	camQueueBuilding(1, "GuardTower3", "BuildTower1");
	camQueueBuilding(1, "GuardTower6", "BuildTower2");
	camQueueBuilding(1, "GuardTower6", "BuildTower4");

	// Third wave of trucks
	camQueueBuilding(1, "GuardTower3", "BuildTower5");
	camQueueBuilding(1, "GuardTower6", "BuildTower7");

	// Send tanks
	camManageGroup(camMakeGroup("AttackGroupLight"), CAM_ORDER_COMPROMISE, {
		pos: camMakePos("RTLZ"),
		regroup: true,
	});
	camManageGroup(camMakeGroup("AttackGroupMedium"), CAM_ORDER_COMPROMISE, {
		pos: camMakePos("RTLZ"),
		regroup: true,
	});
}

function enableSouthScavFactory()
{
	camEnableFactory("SouthScavFactory");
}

camAreaEvent("NorthScavFactoryTrigger", function()
{
	camEnableFactory("NorthScavFactory");
});

camAreaEvent("HeavyNPFactoryTrigger", function()
{
	camEnableFactory("HeavyNPFactory");
});

camAreaEvent("MediumNPFactoryTrigger", function()
{
	camEnableFactory("MediumNPFactory");
});

camAreaEvent("LandingZoneTrigger", function()
{
	var lz = getObject("LandingZone2"); // will override later
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, 0);
	playSound("pcv456.ogg");
	// continue after 4 seconds
	queue("moreLandingZoneTrigger", 4000);
});

function moreLandingZoneTrigger()
{
	hackAddMessage("SB1_4_B", MISS_MSG, 0, true);
	// Give extra 30 minutes.
	setMissionTime(1800 + getMissionTime());
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_5S", {
		area: "RTLZ",
		message: "C1-4_LZ",
		reinforcements: 90 // changes!
	});
	// enables all factories
	camEnableFactory("SouthScavFactory");
	camEnableFactory("NorthScavFactory");
	camEnableFactory("HeavyNPFactory");
	camEnableFactory("MediumNPFactory");
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_5S", {
		area: "RTLZ",
		message: "C1-4_LZ",
		reinforcements: -1 // will override later
	});
	var startpos = getObject("StartPosition");
	centreView(startpos.x, startpos.y);
	var lz = getObject("LandingZone1"); // will override later
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, 0);
	var tent = getObject("TransporterEntry");
	startTransporterEntry(tent.x, tent.y, 0);
	var text = getObject("TransporterExit");
	setTransporterExit(text.x, text.y, 0);

	// Hmm, these tech lists are getting pretty long.
	// But there isn't much we can do to reduce duplication.
	setPower(5000, 1);
	setPower(200, 7);
	camCompleteRequiredResearch(NEW_PARADIGM_RES, 1);
	camCompleteRequiredResearch(SCAVENGER_RES, 7);
	setAlliance(1, 7, true);

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

	// These seem to be in a different order this time,
	// first PROX then MISS, not sure if matters.
	hackAddMessage("C1-4_OBJ1", PROX_MSG, 0, false);
	hackAddMessage("SB1_4_MSG", MISS_MSG, 0, false);

	camSetArtifacts({
		"NPCommandCenter": { tech: "R-Vehicle-Metals01" },
		"NPResearchFacility": { tech: "R-Vehicle-Body04" },
		"MediumNPFactory": { tech: "R-Wpn-Rocket02-MRL" },
	});

	with (camTemplates) camSetFactories({
		"SouthScavFactory": {
			assembly: "SouthScavFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			maxSize: 6,
			throttle: 35000,
			templates: [ rbuggy, bjeep, buscan, trike ]
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
			throttle: 35000,
			templates: [ firecan, rbjeep, bloke, buggy ]
		},
		"HeavyNPFactory": {
			assembly: "HeavyNPFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			maxSize: 6,         // this one was exclusively producing trucks
			throttle: 40000,    // but we simplify this out
			templates: [ npmmct, npsmct, npsmc ]
		},
		"MediumNPFactory": {
			assembly: "MediumNPFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			maxSize: 6,
			throttle: 40000,
			templates: [ npmrl, nphmg, npsbb, npmor ]
		},
	});

	// To be able to use camEnqueueBuilding() later,
	// and also to rebuild dead trucks.
	camManageTrucks(1);

	queue("enableSouthScavFactory", 10000);
}
