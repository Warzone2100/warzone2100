
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NP = 1; //New Paradigm player number

camAreaEvent("tankTrapTrig", function()
{
	camEnableFactory("NPFactoryW");
	camEnableFactory("NPCybFactoryW");
	camEnableFactory("NPFactoryE");
	camEnableFactory("NPCybFactoryE");
	mrlGroupAttack();
});

camAreaEvent("causeWayTrig", function()
{
	camEnableFactory("NPFactoryNE");
	camEnableFactory("NPCybFactoryNE");
	cyborgGroupPatrol();
	sendNPTransporter();
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

function sendNPTransporter()
{
	var list = getDroidsForNPLZ();
	camSendReinforcement(NP, camMakePos("NPTransportPos"), list, 
		CAM_REINFORCE_TRANSPORT, {
			entry: { x: 0, y: 0 },
			exit: { x: 0, y: 0 },
			message: "C1D_LZ2"
		}
	);

	if(enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false).length > 0)
	{
		queue("sendNPTransporter", 600000); //10 min
	}
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
	camManageGroup(camMakeGroup("IDF1"), CAM_ORDER_ATTACK,
		{ pos: camMakePos("attackPoint3") });
	camManageGroup(camMakeGroup("IDF2"), CAM_ORDER_ATTACK, 
		{ pos: camMakePos("attackPoint3") });
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
		reinforcements: 60 //1 min
	});
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_1END", {
		area: "RTLZ",
		message: "C1D_LZ",
		reinforcements: -1
	});

	var startpos = getObject("startPosition");
	centreView(startpos.x, startpos.y);
	var lz = getObject("landingZone"); //player lz
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	var tent = getObject("transporterEntry");
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	var text = getObject("transporterExit");
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	//Get rid of the already existing crate and replace with another
	camSafeRemoveObject("artifact1", false);
	camSetArtifacts({
		"artifactLocation": { tech: "R-Vehicle-Prop-Hover" }, //SE base
		"NPFactoryW": { tech: "R-Vehicle-Metals03" }, //West factory
		"NPFactoryNE": { tech: "R-Vehicle-Body12" }, //Main base factory
	});

	setPower(5000, NP);

	//New Paradigm research
	completeResearch("R-Defense-WallUpgrade03", NP);
	completeResearch("R-Struc-Materials03", NP);
	completeResearch("R-Struc-Factory-Upgrade03", NP);
	completeResearch("R-Struc-Factory-Cyborg-Upgrade03", NP);
	completeResearch("R-Vehicle-Engine03", NP);
	completeResearch("R-Vehicle-Metals03", NP);
	completeResearch("R-Cyborg-Metals03", NP);
	completeResearch("R-Wpn-Cannon-Accuracy01", NP);
	completeResearch("R-Wpn-Cannon-Damage03", NP);
	completeResearch("R-Wpn-Flamer-Damage03", NP);
	completeResearch("R-Wpn-Flamer-ROF01", NP);
	completeResearch("R-Wpn-MG-Damage04", NP);
	completeResearch("R-Wpn-MG-ROF01", NP);
	completeResearch("R-Wpn-Mortar-Damage03", NP);
	completeResearch("R-Wpn-Mortar-Acc01", NP);
	completeResearch("R-Wpn-Rocket-Accuracy01", NP);
	completeResearch("R-Wpn-Rocket-Damage03", NP);
	completeResearch("R-Wpn-Rocket-ROF03", NP);
	completeResearch("R-Wpn-RocketSlow-Accuracy03", NP);
	completeResearch("R-Wpn-RocketSlow-Damage03", NP);
	completeResearch("R-Struc-RprFac-Upgrade03", NP);

	
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
	});
	
	with (camTemplates) camSetFactories({
		"NPFactoryW": {
			assembly: camMakePos("genRetreatPoint"),
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 50000,
			regroup: true,
			repair: 40,
			templates: [ nphmgh, npltath, nphch ] //Hover factory
		},
		"NPFactoryE": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 50000,
			regroup: true,
			repair: 40,
			templates: [ npltat, npmsens, npmorb, npsmct, nphct ] //variety
		},
		"NPFactoryNE": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 50000,
			regroup: true,
			repair: 40,
			templates: [ nphct, npsbb, npmorb ] //tough units
		},
		"NPCybFactoryW": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 40000,
			regroup: true,
			repair: 40,
			templates: [ npcybc, npcybf, npcybr ]
		},
		"NPCybFactoryE": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 40000,
			regroup: true,
			repair: 40,
			templates: [ npcybc, npcybf, npcybr ]
		},
		"NPCybFactoryNE": {
			assembly: camMakePos("cybRetreatPoint"),
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 40000,
			regroup: true,
			repair: 40,
			templates: [ npcybc, npcybf, npcybr ]
		},
	});

	hackAddMessage("C1D_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true); //hover arti

	queue("camCallOnce", 10000, "enableReinforcements");
	queue("setupPatrols", 40000);
}
