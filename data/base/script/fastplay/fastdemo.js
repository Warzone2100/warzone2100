include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");
const SCAVENGER_PLAYER = 7;

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

camAreaEvent("factory3Trigger", function()
{
	camEnableFactory("base3Factory");
	camManageGroup(camMakeGroup("scavBase3"), CAM_ORDER_DEFEND, {
		pos: [
			camMakePos("hillGroupPos1")
		],
		regroup: false,
		count: -1
	});
});

function grantStartTech()
{
	const TECH = [
		"R-Wpn-MG1Mk1","R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels"
	];
	const STRUCTS = [
		"A0CommandCentre", "A0PowerGenerator", "A0ResourceExtractor",
		"A0ResearchFacility", "A0LightFactory"
	];

	camCompleteRequiredResearch(TECH, CAM_HUMAN_PLAYER);
	for (var i = 0, l = STRUCTS.length; i < l; ++i)
	{
		enableStructure(STRUCTS[i], CAM_HUMAN_PLAYER);
	}

	//NOTE: To prevent extra research from being exposed from the MG damage
	//we are going to give the player the third damage upgrade.
	enableResearch("R-Wpn-MG-Damage03", CAM_HUMAN_PLAYER);
	completeResearch("R-Wpn-MG-Damage03", CAM_HUMAN_PLAYER);
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
		interval: 20000,
		regroup: false,
		count: -1
	});

	camManageGroup(camMakeGroup("regionGroup3"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("valleyPlayerBasePos"),
			camMakePos("valleyReinforcePos")
		],
		interval: 20000,
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
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	setReinforcementTime(-1);
	setMissionTime(-1);
	grantStartTech();

	setPower(1000, CAM_HUMAN_PLAYER);
	setPower(AI_POWER, SCAVENGER_PLAYER);

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

	camSafeRemoveObject("flamerArti", false);
	camSetArtifacts({
		"base1Factory": { tech: "R-Defense-Tower01" },
		"artifactPos": { tech: "R-Wpn-Flamer01Mk1" },
		"radarTower": { tech: "R-Sys-Sensor-Turret01" },
		"base2Factory": { tech: "R-Vehicle-Prop-Halftracks" },
		"bunkerArti": { tech: "R-Sys-Engineering01" },
	});

	with (camTemplates) camSetFactories({
		"base1Factory": {
			assembly: "base1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(15000),
			templates: [bloke, trike, buggy]
		},
		"base2Factory": {
			assembly: "base2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(15000),
			templates: [bloke, trike, buggy, bjeep]
		},
		"base3Factory": {
			assembly: "base3Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(15000),
			templates: [rbjeep, trike, buggy, rbjeep]
		}
	});

	camPlayVideos("MBDEMO_MSG");
	hackAddMessage("FAST_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);

	queue("sendAttackGroup1", 10000);
	queue("sendAttackGroup2", 20000);
	queue("activateDefenders", 30000);
}
