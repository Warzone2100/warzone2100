
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEW_PARADIGM_RES = [
	"R-Wpn-MG-Damage02", "R-Wpn-Flamer-Damage03", "R-Struc-RprFac-Upgrade03",
];
const SCAVENGER_RES = [
	"R-Wpn-MG-Damage02",
];

var NPDefenseGroup, NPScoutGroup, NPFactory;

camAreaEvent("RemoveBeacon", function(droid)
{
	hackRemoveMessage("C1-3_OBJ1", PROX_MSG, 0);
});

camAreaEvent("NorthConvoyTrigger", function(droid)
{
	camManageGroup(camMakeGroup("NorthConvoyForce"), CAM_ORDER_DEFEND, {
		pos: camMakePos("NorthConvoyLoc")
	});
});

camAreaEvent("SouthConvoyTrigger", function(droid)
{
	camManageGroup(camMakeGroup("SouthConvoyForce"), CAM_ORDER_DEFEND, {
		pos: camMakePos("SouthConvoyLoc")
	});
	var scout = getObject("ScoutDroid");
	if (camDef(scout) && scout) {
		camTrace("New Paradigm sensor scout retreating");
		var pos = camMakePos("ScoutDroidTarget");
		orderDroidLoc(scout, DORDER_MOVE, pos.x, pos.y);
	}
});

camAreaEvent("WestConvoyTrigger", function(droid)
{
	camManageGroup(camMakeGroup("WestConvoyForce"), CAM_ORDER_DEFEND, {
		pos: camMakePos("WestConvoyLoc")
	});
});

function playYouAreInContraventionOfTheNewParadigm()
{
	hackAddMessage("SB1_3_MSG4", MISS_MSG, 0, true);
	camManageGroup(NPScoutGroup, CAM_ORDER_COMPROMISE, {
		pos: camMakePos("RTLZ"),
		repair: 30,
		regroup: true,
	});
	camManageGroup(NPDefenseGroup, CAM_ORDER_FOLLOW, {
		droid: "NPCommander",
		order: CAM_ORDER_DEFEND,
		data: {
			pos: camMakePos("NPCommander"),
			radius: 15,
			repair: 30,
		},
		repair: 30
	});
}

function enableNP(args)
{
	camEnableFactory("ScavFactory");
	camEnableFactory("NPFactory");
	playSound("pcv455.ogg");
	queue("playYouAreInContraventionOfTheNewParadigm", 2000);
}

function sendScouts()
{
	camManageGroup(camMakeGroup("ScavScoutForce"), CAM_ORDER_COMPROMISE,
	               { pos: camMakePos("RTLZ") });
}

camAreaEvent("ScavTrigger", function(droid)
{
	camEnableFactory("ScavFactory");
});

camAreaEvent("NPTrigger", function(droid)
{
	camCallOnce("enableReinforcements");
	camCallOnce("enableNP");
});

function eventAttacked(victim, attacker) {
	if (!camDef(victim) || !victim || victim.player === 0)
		return;
	if (victim.player === 1)
	{
		camCallOnce("enableNP");
		var commander = getObject("NPCommander");
		if (camDef(attacker) && attacker && camDef(commander) && commander
		    && commander.order !== DORDER_SCOUT && commander.order !== DORDER_RTR)
		{
			orderDroidLoc(commander, DORDER_SCOUT, attacker.x, attacker.y);
		}
	}
}

function enableReinforcements()
{
	playSound("pcv440.ogg"); // Reinforcements are available.
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_1C", {
		area: "RTLZ",
		message: "C1-3_LZ",
		reinforcements: 120 // changes!
	});
}

function camEnemyBaseDetected_ScavBaseGroup()
{
	queue("camCallOnce", 1000, "enableReinforcements");
}

function camEnemyBaseEliminated_ScavBaseGroup()
{
	//make enemy easier to find if all his buildings destroyed
	camManageGroup(
		camMakeGroup(enumArea(0, 0, mapWidth, mapHeight, 7, false)),
		CAM_ORDER_ATTACK
	);
}

function playNPWarningMessage()
{
	hackAddMessage("SB1_3_MSG3", MISS_MSG, 0, true);
}

function playNPWarningSound()
{
	var startpos = getObject("StartPosition");
	playSound("pcv455.ogg");
	queue("playNPWarningMessage", 2000);
}

function eventDroidBuilt(droid, structure)
{
	// An example of manually managing factory groups.
	if (!camDef(structure) || !structure || structure.id !== NPFactory.id)
		return;
	if (groupSize(NPDefenseGroup) < 6) // watch out! commander control limit
		groupAdd(NPDefenseGroup, droid);
	else if (groupSize(NPScoutGroup) < 4 && droid.body !== camTemplates.npsmc.body)
		groupAdd(NPScoutGroup, droid);// heavy tanks don't go scouting
	// As libcampaign.js pre-hook has already fired,
	// the droid would remain assigned to the factory's
	// managed group if not reassigned here,
	// hence fall-through.
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_1C", {
		area: "RTLZ",
		message: "C1-3_LZ",
		reinforcements: -1 // will override later
	});
	var startpos = getObject("StartPosition");
	centreView(startpos.x, startpos.y);
	var lz = getObject("LandingZone");
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, 0);
	var tent = getObject("TransporterEntry");
	startTransporterEntry(tent.x, tent.y, 0);
	var text = getObject("TransporterExit");
	setTransporterExit(text.x, text.y, 0);

	setPower(50000, 1);
	setPower(200, 7);
	camCompleteRequiredResearch(NEW_PARADIGM_RES, 1);
	camCompleteRequiredResearch(SCAVENGER_RES, 7);
	setAlliance(1, 7, true);

	camSetEnemyBases({
		"ScavBaseGroup": {
			cleanup: "ScavBase",
			detectMsg: "C1-3_BASE1",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"NPBaseGroup": {
			cleanup: "NPBase",
			detectMsg: "C1-3_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg"
		},
	});

	hackAddMessage("SB1_3_MSG", MISS_MSG, 0, false);
	hackAddMessage("C1-3_OBJ1", PROX_MSG, 0, false); // south-west beacon

	camSetArtifacts({
		"ScavFactory": { tech: "R-Wpn-Cannon1Mk1" },
		"NPFactory": { tech: "R-Struc-Factory-Module" },
		"NPHQ": { tech: "R-Defense-HardcreteWall" },
		"NPCRC": { tech: "R-Struc-CommandRelay" },
	});

	with (camTemplates) camSetFactories({
		"ScavFactory": {
			assembly: "ScavAssembly",
			order: CAM_ORDER_COMPROMISE,
			data: {
				pos: camMakePos("RTLZ"),
				radius: 8
			},
			groupSize: 4,
			maxSize: 10,
			throttle: 40000,
			templates: [ rbuggy, bloke, rbjeep, buggy ]
		},
		"NPFactory": {
			assembly: "NPAssembly",
			order: CAM_ORDER_ATTACK,
			data: {
				regroup: true,
			},
			groupSize: 4, // sic! scouts, at most
			maxSize: 20,
			throttle: 60000,
			templates: [ nppod, nphmg, npsmc, npsmc ]
		},
	});

	NPScoutGroup = camMakeGroup("NPScoutForce");
	NPDefenseGroup = camMakeGroup("NPDefense");
	NPFactory = getObject("NPFactory");

	queue("playNPWarningSound", 3000);
	queue("sendScouts", 60000);
}
