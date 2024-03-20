include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_newParadigmRes = [
	"R-Wpn-MG1Mk1", "R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels",
	"R-Sys-Engineering01", "R-Wpn-MG-Damage04", "R-Wpn-MG-ROF01", "R-Wpn-Cannon-Damage03",
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Defense-WallUpgrade02", "R-Struc-Materials02", "R-Vehicle-Engine02",
	"R-Struc-RprFac-Upgrade03", "R-Wpn-Rocket-Damage02", "R-Wpn-Rocket-ROF03",
	"R-Vehicle-Metals02", "R-Wpn-Mortar-Damage03", "R-Wpn-Rocket-Accuracy01",
	"R-Wpn-RocketSlow-Damage02", "R-Wpn-Mortar-ROF01",
];
const mis_scavengerRes = [
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF01", "R-Wpn-Rocket-Damage02",
	"R-Wpn-Cannon-Damage02", "R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-ROF01",
	"R-Wpn-Rocket-ROF03", "R-Vehicle-Metals02",
	"R-Defense-WallUpgrade02", "R-Struc-Materials02",
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
	hackAddMessage("C1-4_LZ", PROX_MSG, CAM_HUMAN_PLAYER, false);
});

camAreaEvent("LandingZoneTrigger", function()
{
	camPlayVideos([cam_sounds.incoming.incomingIntelligenceReport, {video: "SB1_4_B", type: MISS_MSG}]);
	hackRemoveMessage("C1-4_LZ", PROX_MSG, CAM_HUMAN_PLAYER); //Remove LZ 2 blip.

	const lz = getObject("LandingZone2"); // will override later
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	// Give extra 40 minutes.
	setMissionTime(camChangeOnDiff(camMinutesToSeconds(40)) + getMissionTime());
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
	camManageGroup(camMakeGroup("AttackGroupLight"), CAM_ORDER_ATTACK, {
		pos: camMakePos("nearSensor"),
		radius: 10,
	});

	camManageGroup(camMakeGroup("AttackGroupMedium"), CAM_ORDER_ATTACK, {
		pos: camMakePos("nearSensor"),
		radius: 10,
	});

	camEnableFactory("HeavyNPFactory");
	camEnableFactory("MediumNPFactory");
}

function buildDefenses()
{
	// First wave of trucks
	camQueueBuilding(CAM_NEW_PARADIGM, "GuardTower6", "BuildTower0");
	camQueueBuilding(CAM_NEW_PARADIGM, "PillBox1",    "BuildTower3");
	camQueueBuilding(CAM_NEW_PARADIGM, "PillBox1",    "BuildTower6");

	// Second wave of trucks
	camQueueBuilding(CAM_NEW_PARADIGM, "GuardTower3", "BuildTower1");
	camQueueBuilding(CAM_NEW_PARADIGM, "GuardTower6", "BuildTower2");
	camQueueBuilding(CAM_NEW_PARADIGM, "GuardTower6", "BuildTower4");

	// Third wave of trucks
	camQueueBuilding(CAM_NEW_PARADIGM, "GuardTower3", "BuildTower5");
	camQueueBuilding(CAM_NEW_PARADIGM, "GuardTower6", "BuildTower7");
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_5S", {
		area: "RTLZ",
		message: "C1-4_LZ",
		reinforcements: -1, // will override later
		retlz: true
	});

	const startPos = getObject("StartPosition");
	const lz = getObject("LandingZone1"); // will override later
	const tEnt = getObject("TransporterEntry");
	const tExt = getObject("TransporterExit");

	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);

	camCompleteRequiredResearch(mis_newParadigmRes, CAM_NEW_PARADIGM);
	camCompleteRequiredResearch(mis_scavengerRes, CAM_SCAV_7);
	setAlliance(CAM_NEW_PARADIGM, CAM_SCAV_7, true);

	camUpgradeOnMapTemplates(cTempl.bloke, cTempl.blokeheavy, CAM_SCAV_7);
	camUpgradeOnMapTemplates(cTempl.trike, cTempl.trikeheavy, CAM_SCAV_7);
	camUpgradeOnMapTemplates(cTempl.buggy, cTempl.buggyheavy, CAM_SCAV_7);
	camUpgradeOnMapTemplates(cTempl.bjeep, cTempl.bjeepheavy, CAM_SCAV_7);
	camUpgradeOnMapTemplates(cTempl.rbjeep, cTempl.rbjeep8, CAM_SCAV_7);

	camSetEnemyBases({
		"SouthScavBaseGroup": {
			cleanup: "SouthScavBase",
			detectMsg: "C1-4_BASE1",
			detectSnd: cam_sounds.baseDetection.scavengerBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerBaseEradicated
		},
		"NorthScavBaseGroup": {
			cleanup: "NorthScavBase",
			detectMsg: "C1-4_BASE3",
			detectSnd: cam_sounds.baseDetection.scavengerBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerBaseEradicated
		},
		"NPBaseGroup": {
			cleanup: "NPBase",
			detectMsg: "C1-4_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated
		},
	});

	hackAddMessage("C1-4_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);

	camSetArtifacts({
		"NPCommandCenter": { tech: "R-Vehicle-Metals01" },
		"NPResearchFacility": { tech: "R-Wpn-MG-Damage04" },
		"MediumNPFactory": { tech: "R-Wpn-Rocket02-MRL" },
		"HeavyNPFactory": { tech: "R-Wpn-Rocket-Damage02" },
	});

	camSetFactories({
		"SouthScavFactory": {
			assembly: "SouthScavFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			maxSize: 6,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(20)),
			templates: [ cTempl.rbuggy, cTempl.bjeepheavy, cTempl.buscan, cTempl.trikeheavy ]
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
			templates: [ cTempl.firecan, cTempl.rbjeep8, cTempl.blokeheavy, cTempl.buggyheavy ]
		},
		"HeavyNPFactory": {
			assembly: "HeavyNPFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			maxSize: 6, // this one was exclusively producing trucks
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)), // but we simplify this out
			templates: [ cTempl.npsmc, cTempl.npmmct, cTempl.npsmc, cTempl.npsmct ]
		},
		"MediumNPFactory": {
			assembly: "MediumNPFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			maxSize: 6,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(50)),
			templates: [ cTempl.npmrl, cTempl.nphmg, cTempl.npsbb, cTempl.npmor ]
		},
	});

	// To be able to use camEnqueueBuilding() later,
	// and also to rebuild dead trucks.
	camManageTrucks(CAM_NEW_PARADIGM);

	queue("enableSouthScavFactory", camChangeOnDiff(camSecondsToMilliseconds(10)));
}
