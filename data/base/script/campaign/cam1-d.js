
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
	camCallOnce("mrlGroupAttack");
});

camAreaEvent("northWayTrigger", function(droid)
{
	camEnableFactory("NPFactoryE");
	camEnableFactory("NPCybFactoryE");
	camCallOnce("mrlGroupAttack");
});

camAreaEvent("causeWayTrig", function(droid)
{
	camEnableFactory("NPFactoryNE");
	camEnableFactory("NPCybFactoryNE");
	camCallOnce("transportBaseSetup");
});

camAreaEvent("westWayTrigger", function(droid)
{
	camEnableFactory("NPFactoryNE");
	camEnableFactory("NPCybFactoryNE");
	camEnableFactory("NPFactoryE");
	camEnableFactory("NPCybFactoryE");
	camCallOnce("mrlGroupAttack");
	camCallOnce("transportBaseSetup");
});

function transportBaseSetup()
{
	camDetectEnemyBase("NPLZGroup");
	camSetBaseReinforcements("NPLZGroup", camChangeOnDiff(600000), "getDroidsForNPLZ", CAM_REINFORCE_TRANSPORT, {
		entry: { x: 2, y: 2 },
		exit: { x: 2, y: 2 },
		data: {
			regroup: false,
			count: -1,
			repair: 40,
		},
	});
}

function getDroidsForNPLZ()
{
	const LIM = 8; //Last alpha mission always has 8 transport units
	var templates = [ cTempl.nphct, cTempl.nphct, cTempl.npmorb, cTempl.npmorb, cTempl.npsbb ];

	var droids = [];
	for (var i = 0; i < LIM; ++i)
	{
		droids.push(templates[camRand(templates.length)]);
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
		interval: 90000, //90 sec
		repair: 70
	});
}

function cyborgGroupPatrol()
{
	camManageGroup(camMakeGroup("cyborgs1"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("genRetreatPoint"),
			camMakePos("cybRetreatPoint"),
			camMakePos("NPTransportPos")
		],
		repair: 66
	});
	camManageGroup(camMakeGroup("cyborgs2"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("genRetreatPoint"),
			camMakePos("cybRetreatPoint"),
			camMakePos("NPTransportPos")
		],
		repair: 66
	});
}

function mrlGroupAttack()
{
	camManageGroup(camMakeGroup("MRL1"), CAM_ORDER_ATTACK, {
		count: -1,
		repair: 80,
		regroup: false
	});
}

function IDFGroupAmbush()
{
	camManageGroup(camMakeGroup("IDF1"), CAM_ORDER_ATTACK, {
		count: -1,
		regroup: false
	});
	camManageGroup(camMakeGroup("IDF2"), CAM_ORDER_ATTACK, {
		count: -1,
		regroup: false
	});
}

function setupPatrols()
{
	IDFGroupAmbush();
	HoverGroupPatrol();
	cyborgGroupPatrol();
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

	camSetFactories({
		"NPFactoryW": {
			assembly: "NPFactoryWAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(55000),
			data: {
				regroup: false,
				repair: 66,
				count: -1,
			},
			templates: [ cTempl.nphmgh, cTempl.npltath, cTempl.nphch ] //Hover factory
		},
		"NPFactoryE": {
			assembly: "NPFactoryEAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(65000),
			data: {
				regroup: false,
				repair: 66,
				count: -1,
			},
			templates: [ cTempl.npltat, cTempl.npmsens, cTempl.npmorb, cTempl.npsmct, cTempl.nphct ] //variety
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
			templates: [ cTempl.nphct, cTempl.npsbb, cTempl.npmorb ] //tough units
		},
		"NPCybFactoryW": {
			assembly: "NPCybFactoryWAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(35000),
			data: {
				regroup: false,
				repair: 66,
				count: -1,
			},
			templates: [ cTempl.npcybc, cTempl.npcybf, cTempl.npcybr ]
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
			templates: [ cTempl.npcybc, cTempl.npcybf, cTempl.npcybr ]
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
			templates: [ cTempl.npcybc, cTempl.npcybf, cTempl.npcybr ]
		},
	});

	hackAddMessage("C1D_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);

	queue("enableReinforcements", 12000);
	queue("setupPatrols", 160000); // 2.5 min.
}
