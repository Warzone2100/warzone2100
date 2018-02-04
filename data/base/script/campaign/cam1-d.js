
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEW_PARADIGM_RES = [
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF01", "R-Defense-WallUpgrade03",
	"R-Struc-Materials03", "R-Struc-Factory-Upgrade03",
	"R-Struc-Factory-Cyborg-Upgrade03", "R-Vehicle-Engine03",
	"R-Vehicle-Metals03", "R-Cyborg-Metals03", "R-Wpn-Cannon-Accuracy01",
	"R-Wpn-Cannon-Damage03", "R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01",
	"R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-Acc01", "R-Wpn-Rocket-Accuracy01",
	"R-Wpn-Rocket-Damage03", "R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage03", "R-Struc-RprFac-Upgrade03",
];


camAreaEvent("tankTrapTrig", function(droid)
{
	camEnableFactory("NPFactoryW");
	camEnableFactory("NPCybFactoryW");
	mrlGroupAttack();
});

camAreaEvent("northWayTrigger", function(droid)
{
	camEnableFactory("NPFactoryE");
	camEnableFactory("NPCybFactoryE");
	mrlGroupAttack();
});

camAreaEvent("causeWayTrig", function(droid)
{
	camEnableFactory("NPFactoryNE");
	camEnableFactory("NPCybFactoryNE");
	cyborgGroupPatrol();

	camDetectEnemyBase("NPLZGroup");
	camSetBaseReinforcements("NPLZGroup", camChangeOnDiff(600000), "getDroidsForNPLZ", CAM_REINFORCE_TRANSPORT, {
		entry: { x: 0, y: 0 },
		exit: { x: 0, y: 0 },
		data: {
			regroup: false,
			count: -1,
			repair: 40,
		},
	});
});

function getDroidsForNPLZ()
{
	const count = 4; //Last alpha mission always has 8 transport units
	var templates;
	with (camTemplates) templates = [ nphct, npsbb, npmorb ];

	var droids = [];
	for (var i = 0; i < count; ++i)
	{
		var t = templates[camRand(templates.length)];
		// two droids of each template
		droids[droids.length] = t;
		droids[droids.length] = t;
	}
	return droids;
}

function HoverGroupPatrol()
{
	camManageGroup(camMakeGroup("hoversAttack"), CAM_ORDER_ATTACK, {
		pos: camMakePos("attackPoint2"),
		fallback: camMakePos("cybRetreatPoint"),
		morale: 50,
		regroup: true
	});
	camManageGroup(camMakeGroup("hoversDefense"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("hoverDefense1"),
			camMakePos("hoverDefense2"),
			camMakePos("hoverDefense3"),
			camMakePos("hoverDefense4")
		],
		interval: 90000 //90 sec
	});
}

function cyborgGroupPatrol()
{
	camManageGroup(camMakeGroup("cyborgs1"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("genRetreatPoint"),
			camMakePos("cybRetreatPoint"),
			camMakePos("NPTransportPos")
		]
	});
	camManageGroup(camMakeGroup("cyborgs2"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("genRetreatPoint"),
			camMakePos("cybRetreatPoint"),
			camMakePos("NPTransportPos")
		]
	});
}

function mrlGroupAttack()
{
	camManageGroup(camMakeGroup("MRL1"), CAM_ORDER_ATTACK, {
		pos: camMakePos("attackPoint3"),
		fallback: camMakePos("genRetreatPoint"),
		morale: 60,
		regroup: true
	});
}

function sensorGroupAttack()
{
	camManageGroup(camMakeGroup("sensor1"), CAM_ORDER_ATTACK, {
		pos: camMakePos("attackPoint3"),
		fallback: camMakePos("genRetreatPoint"),
		morale: 50,
		regroup: true
	});
	camManageGroup(camMakeGroup("sensor2"), CAM_ORDER_ATTACK, {
		pos: camMakePos("attackPoint3"),
		fallback: camMakePos("genRetreatPoint"),
		morale: 50,
		regroup: true
	});
}

function IDFGroupAmbush()
{
	camManageGroup(camMakeGroup("IDF1"), CAM_ORDER_ATTACK, {
		pos: camMakePos("attackPoint3")
	});
	camManageGroup(camMakeGroup("IDF2"), CAM_ORDER_ATTACK, {
		pos: camMakePos("attackPoint3")
	});
}

function setupPatrols()
{
	IDFGroupAmbush();
	sensorGroupAttack();
	HoverGroupPatrol();
}

function enableReinforcements()
{
	playSound("pcv440.ogg"); // Reinforcements are available.
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_1END", {
		area: "RTLZ",
		message: "C1D_LZ",
		reinforcements: 120, //2 min
		eliminateBases: true
	});
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_1END", {
		area: "RTLZ",
		message: "C1D_LZ",
		reinforcements: -1,
		eliminateBases: true
	});

	var startpos = getObject("startPosition");
	var lz = getObject("landingZone"); //player lz
	var tent = getObject("transporterEntry");
	var text = getObject("transporterExit");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	//Get rid of the already existing crate and replace with another
	camSafeRemoveObject("artifact1", false);
	camSetArtifacts({
		"artifactLocation": { tech: "R-Vehicle-Prop-Hover" }, //SE base
		"NPFactoryW": { tech: "R-Vehicle-Metals03" }, //West factory
		"NPFactoryNE": { tech: "R-Vehicle-Body12" }, //Main base factory
	});

	camCompleteRequiredResearch(NEW_PARADIGM_RES, NEW_PARADIGM);

	camSetEnemyBases({
		"NPSouthEastGroup": {
			cleanup: "NPSouthEast",
			detectMsg: "C1D_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NPMiddleGroup": {
			cleanup: "NPMiddle",
			detectMsg: "C1D_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NPNorthEastGroup": {
			cleanup: "NPNorthEast",
			detectMsg: "C1D_BASE3",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NPLZGroup": {
			cleanup: "NPLZ1",
			detectMsg: "C1D_LZ2",
			eliminateSnd: "pcv394.ogg",
			player: NEW_PARADIGM // required for LZ-type bases
		},
	});

	with (camTemplates) camSetFactories({
		"NPFactoryW": {
			assembly: "NPFactoryWAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(50000),
			data: {
				regroup: false,
				repair: 66,
				count: -1,
			},
			templates: [ nphmgh, npltath, nphch ] //Hover factory
		},
		"NPFactoryE": {
			assembly: "NPFactoryEAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(60000),
			data: {
				regroup: false,
				repair: 66,
				count: -1,
			},
			templates: [ npltat, npmsens, npmorb, npsmct, nphct ] //variety
		},
		"NPFactoryNE": {
			assembly: "NPFactoryNEAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(90000),
			data: {
				regroup: false,
				repair: 66,
				count: -1,
			},
			templates: [ nphct, npsbb, npmorb ] //tough units
		},
		"NPCybFactoryW": {
			assembly: "NPCybFactoryWAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(30000),
			data: {
				regroup: false,
				repair: 66,
				count: -1,
			},
			templates: [ npcybc, npcybf, npcybr ]
		},
		"NPCybFactoryE": {
			assembly: "NPCybFactoryEAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(30000),
			data: {
				regroup: false,
				repair: 66,
				count: -1,
			},
			templates: [ npcybc, npcybf, npcybr ]
		},
		"NPCybFactoryNE": {
			assembly: "NPCybFactoryNEAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(40000),
			data: {
				regroup: false,
				repair: 66,
				count: -1,
			},
			templates: [ npcybc, npcybf, npcybr ]
		},
	});

	hackAddMessage("C1D_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);

	queue("enableReinforcements", 15000);
	queue("setupPatrols", 160000); // 2.5 min.
}
