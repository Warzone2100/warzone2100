
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

//New base blip, new base area, new factory data

const NEW_PARADIGM_RES = [
	"R-Wpn-MG1Mk1", "R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels",
	"R-Sys-Engineering01", "R-Wpn-MG-Damage03", "R-Wpn-MG-ROF01", "R-Wpn-Cannon-Damage01",
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Defense-WallUpgrade02","R-Struc-Materials02", "R-Vehicle-Engine01",
	"R-Struc-RprFac-Upgrade01", "R-Wpn-Rocket-Damage01", "R-Wpn-Rocket-ROF02",
	"R-Wpn-Mortar-Damage02", "R-Wpn-Mortar-ROF01",
];
const SCAVENGER_RES = [
	"R-Wpn-Flamer-Damage02", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage03", "R-Wpn-MG-ROF01", "R-Wpn-Cannon-Damage01",
	"R-Wpn-Mortar-Damage02", "R-Wpn-Mortar-ROF01", "R-Wpn-Rocket-ROF03",
	"R-Defense-WallUpgrade02","R-Struc-Materials02",
];
var NPDefenseGroup, NPScoutGroup, NPFactory;

camAreaEvent("RemoveBeacon", function(droid)
{
	hackRemoveMessage("C1-3_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
});

camAreaEvent("NorthConvoyTrigger", function(droid)
{
	camManageGroup(camMakeGroup("NorthConvoyForce"), CAM_ORDER_DEFEND, {
		pos: camMakePos("NorthConvoyLoc")
	});
});

camAreaEvent("SouthConvoyTrigger", function(droid)
{
	var scout = getObject("ScoutDroid");
	if (camDef(scout) && scout)
	{
		camTrace("New Paradigm sensor scout retreating");
		var pos = camMakePos("ScoutDroidTarget");
		orderDroidLoc(scout, DORDER_MOVE, pos.x, pos.y);
	}
});

camAreaEvent("WestConvoyTrigger", function(droid)
{
	camManageGroup(camMakeGroup("WestConvoyForce"), CAM_ORDER_DEFEND, {
		pos: camMakePos("WestConvoyLoc"),
		radius: 6,
	});
});

function enableNP(args)
{
	camEnableFactory("ScavFactory");
	camEnableFactory("ScavFactorySouth");
	camEnableFactory("NPFactory");

	if (difficulty === INSANE)
	{
		queue("NPReinforce", camSecondsToMilliseconds(10));
	}

	camManageGroup(NPScoutGroup, CAM_ORDER_COMPROMISE, {
		pos: camMakePos("RTLZ"),
		repair: 66,
		regroup: true,
		removable: false,
	});
	camManageGroup(NPDefenseGroup, CAM_ORDER_FOLLOW, {
		droid: "NPCommander",
		order: CAM_ORDER_DEFEND,
		data: {
			pos: camMakePos("NPCommander"),
			radius: 22,
			repair: 66,
		},
		repair: 66,
	});

	camPlayVideos(["pcv455.ogg", {video: "SB1_3_MSG4", type: MISS_MSG}]);
}

function NPReinforce()
{
	if (getObject("NPHQ") !== null)
	{
		var list = [];
		var count = 5 + camRand(5);
		var scouts = [cTempl.nphmg, cTempl.npblc, cTempl.nppod, cTempl.nphmg, cTempl.npblc];

		for (var i = 0; i < count; ++i)
		{
			list.push(scouts[camRand(scouts.length)]);
		}
		camSendReinforcement(NEW_PARADIGM, camMakePos("NPReinforcementPos"), list, CAM_REINFORCE_GROUND, {
			data: {
				regroup: false,
				repair: 66,
				count: -1,
			},
		});
		queue("NPReinforce", camSecondsToMilliseconds(180));
	}
}

function sendScouts()
{
	camManageGroup(camMakeGroup("ScavScoutForce"), CAM_ORDER_COMPROMISE, {
		pos: camMakePos("RTLZ")
	});
}

camAreaEvent("ScavTrigger", function(droid)
{
	camEnableFactory("ScavFactory");
});

camAreaEvent("NPTrigger", function(droid)
{
	camCallOnce("enableReinforcements");
});

function eventAttacked(victim, attacker) {
	if (!camDef(victim) || !victim || victim.player === CAM_HUMAN_PLAYER)
	{
		return;
	}
	if (victim.player === NEW_PARADIGM)
	{
		camCallOnce("enableNP");
		var commander = getObject("NPCommander");
		if (camDef(attacker) && attacker && camDef(commander) && commander &&
			commander.order !== DORDER_SCOUT && commander.order !== DORDER_RTR)
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
		reinforcements: camMinutesToSeconds(2), // changes!
		annihilate: true
	});
}

function camEnemyBaseDetected_ScavBaseGroup()
{
	queue("camCallOnce", camSecondsToMilliseconds(1), "enableReinforcements");
}

function camEnemyBaseDetected_ScavBaseGroupSouth()
{
	camEnableFactory("ScavFactory");
	camEnableFactory("ScavFactorySouth");
	camManageGroup(camMakeGroup("SouthConvoyForce"), CAM_ORDER_COMPROMISE, {
		pos: camMakePos("SouthConvoyLoc"),
		regroup: false, //true when movement gets better. Very big group this one is.
	});
}

function camEnemyBaseEliminated_ScavBaseGroup()
{
	//make enemy easier to find if all his buildings destroyed
	camManageGroup(
		camMakeGroup(enumArea(0, 0, mapWidth, mapHeight, SCAV_7, false)),
		CAM_ORDER_ATTACK
	);
}

function playNPWarningMessage()
{
	camPlayVideos(["pcv455.ogg", {video: "SB1_3_MSG3", type: CAMP_MSG}]);
}

function eventDroidBuilt(droid, structure)
{
	// An example of manually managing factory groups.
	if (!camDef(structure) || !structure || structure.id !== NPFactory.id)
	{
		return;
	}
	if (getObject("NPCommander") !== null && groupSize(NPDefenseGroup) < 6) // watch out! commander control limit
	{
		groupAdd(NPDefenseGroup, droid);
	}
	else if (groupSize(NPScoutGroup) < 4 && droid.body !== cTempl.npsmc.body)
	{
		groupAdd(NPScoutGroup, droid); // heavy tanks don't go scouting
	}
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
		reinforcements: -1, // will override later
		annihilate: true
	});

	var startpos = getObject("StartPosition");
	var lz = getObject("LandingZone");
	var tent = getObject("TransporterEntry");
	var text = getObject("TransporterExit");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	camCompleteRequiredResearch(NEW_PARADIGM_RES, NEW_PARADIGM);
	camCompleteRequiredResearch(SCAVENGER_RES, SCAV_7);
	setAlliance(NEW_PARADIGM, SCAV_7, true);

	camUpgradeOnMapTemplates(cTempl.bloke, cTempl.blokeheavy, 7);
	camUpgradeOnMapTemplates(cTempl.trike, cTempl.trikeheavy, 7);
	camUpgradeOnMapTemplates(cTempl.buggy, cTempl.buggyheavy, 7);
	camUpgradeOnMapTemplates(cTempl.bjeep, cTempl.bjeepheavy, 7);
	camUpgradeOnMapTemplates(cTempl.rbjeep, cTempl.rbjeep8, 7);

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
		"ScavBaseGroupSouth": {
			cleanup: "SouthScavBase",
			detectMsg: "C1-3_OBJ2",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
	});

	hackAddMessage("C1-3_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false); // south-west beacon

	camSetArtifacts({
		"ScavFactory": { tech: "R-Wpn-Flamer-Damage03" },
		"NPFactory": { tech: ["R-Struc-Factory-Module", "R-Vehicle-Body04"] },
		"NPLab": { tech: "R-Defense-HardcreteWall" },
		"NPCRC": { tech: "R-Struc-CommandRelay" },
	});

	camSetFactories({
		"ScavFactory": {
			assembly: "ScavAssembly",
			order: CAM_ORDER_COMPROMISE,
			data: {
				pos: camMakePos("RTLZ"),
				radius: 8
			},
			groupSize: 4,
			maxSize: 10,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(15)),
			templates: [ cTempl.buggyheavy, cTempl.blokeheavy, cTempl.bjeepheavy, cTempl.trikeheavy ]
		},
		"NPFactory": {
			assembly: "NPAssembly",
			order: CAM_ORDER_ATTACK,
			data: {
				regroup: false,
				repair: 30,
			},
			groupSize: 4, // sic! scouts, at most
			maxSize: 20,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			templates: [ cTempl.nppod, cTempl.nphmg, cTempl.npsmc, cTempl.npsmc ]
		},
		"ScavFactorySouth": {
			assembly: "ScavAssemblySouth",
			order: CAM_ORDER_ATTACK,
			data: {
				regroup: false,
				count: -1,
			},
			groupSize: 4,
			maxSize: 10,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(25)),
			templates: [ cTempl.rbjeep8, cTempl.buscan, cTempl.rbuggy, cTempl.firecan ]
		},
	});

	NPScoutGroup = camMakeGroup("NPScoutForce");
	NPDefenseGroup = camMakeGroup("NPDefense");
	NPFactory = getObject("NPFactory");

	queue("playNPWarningMessage", camSecondsToMilliseconds(3));
	queue("sendScouts", camSecondsToMilliseconds(60));
}
