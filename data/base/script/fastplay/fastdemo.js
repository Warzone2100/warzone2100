include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

camAreaEvent("removeObjectiveBlip", function()
{
	hackRemoveMessage("FAST_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
});

camAreaEvent("factory1Trigger", function()
{
	camEnableFactory("base1Factory");
});

camAreaEvent("factory2Trigger", function()
{
	camEnableFactory("base2Factory");
});

function enableLastBase()
{
	camEnableFactory("base3Factory");
}

function enabledBase3()
{
	camCallOnce("enableLastBase");
}

camAreaEvent("factory3Trigger", function()
{
	queue("enabledBase3", camChangeOnDiff(camMinutesToMilliseconds(5)));
	camManageGroup(camMakeGroup("scavBase3"), CAM_ORDER_DEFEND, {
		pos: [
			camMakePos("hillGroupPos1")
		],
		regroup: false,
		count: -1
	});
});

camAreaEvent("factory3TriggerInstant", function()
{
	enabledBase3();
});

function grantStartTech()
{
	const tech = [
		"R-Wpn-MG1Mk1","R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels"
	];
	const structs = [
		cam_base_structures.commandCenter, cam_base_structures.powerGenerator,
		cam_base_structures.derrick, cam_base_structures.researchLab,
		cam_base_structures.factory,
	];

	camCompleteRequiredResearch(tech, CAM_HUMAN_PLAYER);
	for (let i = 0, l = structs.length; i < l; ++i)
	{
		enableStructure(structs[i], CAM_HUMAN_PLAYER);
	}

	enableResearch("R-Wpn-MG-Damage01", CAM_HUMAN_PLAYER);
}

function sendAttackGroup1()
{
	camManageGroup(camMakeGroup("regionGroup0"), CAM_ORDER_ATTACK, {
		morale: 50,
		fallback: camMakePos("base1Assembly"),
		regroup: false,
		count: -1
	});

	camManageGroup(camMakeGroup("regionGroup1"), CAM_ORDER_ATTACK, {
		morale: 50,
		fallback: camMakePos("base1Assembly"),
		regroup: false,
		count: -1
	});
}

function sendAttackGroup2()
{
	camManageGroup(camMakeGroup("regionGroup2"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("valleyPlayerBasePos"),
			camMakePos("valleyReinforcePos")
		],
		interval: camSecondsToMilliseconds(20),
		regroup: false,
		count: -1
	});

	camManageGroup(camMakeGroup("regionGroup3"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("valleyPlayerBasePos"),
			camMakePos("valleyReinforcePos")
		],
		interval: camSecondsToMilliseconds(20),
		regroup: false,
		count: -1
	});
}

function activateDefenders()
{
	camManageGroup(camMakeGroup("regionGroup4"), CAM_ORDER_DEFEND, {
		pos: camMakePos("regionGroup4"),
		radius: 12,
		fallback: camMakePos("base1Assembly"),
		morale: 50,
		regroup: false,
		count: -1
	});

	camManageGroup(camMakeGroup("regionGroup5"), CAM_ORDER_DEFEND, {
		pos: camMakePos("regionGroup5"),
		radius: 10,
		fallback: camMakePos("base2Assembly"),
		morale: 67,
		regroup: false,
		count: -1
	});

	camManageGroup(camMakeGroup("scavBase2"), CAM_ORDER_DEFEND, {
		pos: camMakePos("scavBase2"),
		radius: 14,
		fallback: camMakePos("base2Assembly"),
		morale: 20,
		regroup: false,
		count: -1
	});
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, undefined);
	const startPos = getObject("startPosition");
	const lz = getObject("landingZone");
	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	setReinforcementTime(-1);
	setMissionTime(-1);
	grantStartTech();

	setPower(1500, CAM_HUMAN_PLAYER);

	camSetEnemyBases({
		"northBase": {
			cleanup: "scavBase1",
			detectMsg: "FAST_BASE1",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"northEastBase": {
			cleanup: "scavBase2",
			detectMsg: "FAST_BASE2",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"southBase": {
			cleanup: "scavBase3",
			detectMsg: "FAST_BASE3",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"middleBase": {
			cleanup: "scavBase4",
			detectMsg: "FAST_BASE4",
			detectSnd: "pcv375.ogg",
			eliminateSnd: "pcv391.ogg"
		},
	});

	camSetArtifacts({
		"base1Factory": { tech: ["R-Defense-Tower01", "R-Wpn-MG-Damage02"] },
		"base1PowerGenerator": { tech: "R-Struc-PowerModuleMk1" },
		"flamerArti": { tech: "R-Wpn-Flamer01Mk1" },
		"radarTower": { tech: "R-Sys-Sensor-Turret01" },
		"base2Factory": { tech: ["R-Vehicle-Prop-Halftracks", "R-Wpn-Flamer-Damage01"] },
		"bunkerArti": { tech: ["R-Sys-Engineering01", "R-Sys-MobileRepairTurret01" ]},
	});

	camSetFactories({
		"base1Factory": {
			assembly: "base1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(15)),
			templates: [cTempl.bloke, cTempl.trike, cTempl.buggy]
		},
		"base2Factory": {
			assembly: "base2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(15)),
			templates: [cTempl.bloke, cTempl.trike, cTempl.buggy, cTempl.bjeep]
		},
		"base3Factory": {
			assembly: "base3Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(15)),
			templates: [cTempl.rbjeep, cTempl.trike, cTempl.buggy, cTempl.bloke, cTempl.bjeep]
		}
	});

	camPlayVideos({video: "MBDEMO_MSG", type: MISS_MSG});
	hackAddMessage("FAST_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);

	queue("sendAttackGroup1", camSecondsToMilliseconds(10));
	queue("sendAttackGroup2", camSecondsToMilliseconds(20));
	queue("activateDefenders", camSecondsToMilliseconds(30));
}
