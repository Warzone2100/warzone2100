
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var NPDefenseGroup; // no particular orders, just stay near factories

function sendRocketForce()
{
	camManageGroup(camMakeGroup("RocketForce"), CAM_ORDER_ATTACK, {
		regroup: true,
		count: -1,
	});
}

function sendTankScoutForce()
{
	camManageGroup(camMakeGroup("TankScoutForce"), CAM_ORDER_ATTACK, {
		regroup: true,
		count: -1,
	});
	// FIXME: Re-enable this when commander/formation movement
	// becomes good enough. Remove the call above then.
	/*
	camManageGroup(camMakeGroup("TankScoutForce"), CAM_ORDER_FOLLOW, {
		droid: "TankScoutForceCommander",
		order: CAM_ORDER_ATTACK
	});
	*/
}

function sendTankForce()
{
	camManageGroup(camMakeGroup("TankForce"), CAM_ORDER_ATTACK, {
		regroup: true,
		count: -1,
	});
	// FIXME: Re-enable this when commander/formation movement
	// becomes good enough. Remove the call above then.
	/*
	camManageGroup(camMakeGroup("TankForce"), CAM_ORDER_FOLLOW, {
		droid: "TankForceCommander",
		order: CAM_ORDER_ATTACK
	});
	*/
}

function enableNPFactory()
{
	camEnableFactory("NPCentralFactory");
}

camAreaEvent("RemoveBeacon", function()
{
	hackRemoveMessage("C1C_OBJ1", PROX_MSG, 0);
});

camAreaEvent("AmbushTrigger", function()
{
	// wzcam enables factory here, even though it's quite early
	camEnableFactory("ScavCentralFactory");

	camManageGroup(camMakeGroup("AmbushForce"), CAM_ORDER_ATTACK, {
		pos: "AmbushTarget",
		regroup: true,
		count: -1,
	});
	// FIXME: Re-enable this when commander/formation movement
	// becomes good enough. Remove the call above then.
	// FIXME: This group has more droids than the commander can handle!
	/*
	camManageGroup(camMakeGroup("AmbushForce"), CAM_ORDER_FOLLOW, {
		droid: "AmbushForceCommander",
		order: CAM_ORDER_ATTACK,
		pos: "AmbushTarget",
	});
	*/
});

camAreaEvent("ScavCentralFactoryTrigger", function()
{
	// doesn't make much sense because the player
	// passes through AmbushTrigger anyway
	// before getting there
	camEnableFactory("ScavCentralFactory");
});

camAreaEvent("ScavNorthFactoryTrigger", function()
{
	camEnableFactory("ScavNorthFactory");
});

camAreaEvent("NPNorthFactoryTrigger", function()
{
	camEnableFactory("NPNorthFactory");
});

function camEnemyBaseEliminated_NPCentralFactory()
{
	camEnableFactory("NPNorthFactory");
}

function getDroidsForNPLZ(args)
{
	with (camTemplates) {
		var scouts = [ npsens, nppod, nphmg ];
		var heavies = [ npslc, npsmct, npmor ];
	}
	var numScouts = camRand(5) + 1;
	var list = [];
	for (var i = 0; i < numScouts; ++i)
		list[list.length] = scouts[camRand(scouts.length)];
	var heavy = heavies[camRand(heavies.length)];
	for (var i = numScouts; i < 8; ++i)
		list[list.length] = heavy;
	return list;
}

camAreaEvent("NPLZ1Trigger", function()
{
	// Message4 here, Message3 for the second LZ, and
	// please don't ask me why they did it this way
	hackAddMessage("MB1C4_MSG", MISS_MSG, 0, true);
	camDetectEnemyBase("NPLZ1Group");
	camSetEnemyReinforcements(
		"NPLZ1Group", CAM_REINFORCE_TRANSPORT,
		300000, "getDroidsForNPLZ", {
			entry: { x: 126, y: 76 },
			exit: { x: 126, y: 36 }
		});
});

camAreaEvent("NPLZ2Trigger", function()
{
	hackAddMessage("MB1C3_MSG", MISS_MSG, 0, true);
	camDetectEnemyBase("NPLZ2Group");
	camSetEnemyReinforcements(
		"NPLZ2Group", CAM_REINFORCE_TRANSPORT,
		300000, "getDroidsForNPLZ", {
			entry: { x: 126, y: 76 },
			exit: { x: 126, y: 36 }
		});
});

function eventDroidBuilt(droid, structure)
{
	if (!camDef(structure) || !structure || structure.player !== 1
                           || droid.droidType === DROID_CONSTRUCT)
		return;
	if (groupSize(NPDefenseGroup) < 4)
		groupAdd(NPDefenseGroup, droid);
}

function playSecondVideo()
{
	hackAddMessage("MB1C2_MSG", MISS_MSG, 0, true);
}

function eventVideoDone()
{
	camCallOnce("playSecondVideo");
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "CAM_1CA");
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, 0);

	setReinforcementTime(-1);
	setMissionTime(7200);
	setAlliance(1, 7, true);

	setPower(5000, 1);
	setPower(100, 7);

	camSetEnemyBases({
		"ScavSouthDerrickGroup": {
			cleanup: "ScavSouthDerrick",
			detectMsg: "C1C_BASE1",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv391.ogg"
		},
		"ScavSouthEastHighgroundGroup": {
			cleanup: "ScavSouthEastHighground",
			detectMsg: "C1C_BASE2",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv391.ogg"
		},
		"ScavNorthBaseGroup": {
			cleanup: "ScavNorthBase",
			detectMsg: "C1C_BASE3",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv391.ogg"
		},
		"ScavSouthPodPitsGroup": {
			cleanup: "ScavSouthPodPits",
			detectMsg: "C1C_BASE4",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv391.ogg"
		},
		"ScavCentralBaseGroup": {
			cleanup: "MixedCentralBase", // two bases with same cleanup region
			detectMsg: "C1C_BASE5",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv391.ogg",
			player: 7 // hence discriminate by player filter
		},
		"ScavSouthEastBaseGroup": {
			cleanup: "ScavSouthEastBase",
			detectMsg: "C1C_BASE6",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv391.ogg",
		},
		"NPEastBaseGroup": {
			cleanup: "NPEastBase",
			detectMsg: "C1C_BASE7",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NPNorthEastGeneratorGroup": {
			cleanup: "NPNorthEastGenerator",
			detectMsg: "C1C_BASE8",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NPNorthEastBaseGroup": {
			cleanup: "NPNorthEastBase",
			detectMsg: "C1C_BASE9",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NPCentralBaseGroup": {
			cleanup: "MixedCentralBase", // two bases with same cleanup region
			detectMsg: "C1C_BASE10",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
			player: 1 // hence discriminate by player filter
		},
		"NPLZ1Group": {
			cleanup: "NPLZ1", // kill the four towers to disable LZ
			detectMsg: "C1C_LZ1",
			eliminateSnd: "pcv394.ogg",
			player: 1 // required for LZ-type bases
		},
		"NPLZ2Group": {
			cleanup: "NPLZ2", // kill the four towers to disable LZ
			detectMsg: "C1C_LZ2",
			eliminateSnd: "pcv394.ogg",
			player: 1 // required for LZ-type bases
		},
	});

	hackAddMessage("C1C_OBJ1", PROX_MSG, 0, false); // initial beacon
	hackAddMessage("MB1C_MSG", MISS_MSG, 0, true);

	camSetArtifacts({
		"ScavSouthFactory": { tech: "R-Wpn-Rocket05-MiniPod" },
		"NPResearchFacility": { tech: "R-Struc-Research-Module" },
		"NPCentralFactory": { tech: "R-Vehicle-Prop-Tracks" },
		"NPNorthFactory": { tech: "R-Vehicle-Engine01" },
	});

	with (camTemplates) camSetFactories({
		"ScavSouthFactory": {
			assembly: "ScavSouthFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 90000,
			regroup: true,
			templates: [ buscan, rbjeep, trike, buggy ]
		},
		"ScavCentralFactory": {
			// no assembly was defined in wzcam for this factory
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 60000,
			regroup: true,
			templates: [ firecan, rbuggy, bjeep, bloke ]
		},
		"ScavNorthFactory": {
			assembly: "ScavNorthFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 30000,
			regroup: true,
			templates: [ firecan, rbuggy, buscan, trike ]
		},
		"NPCentralFactory": {
			assembly: "NPCentralFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 60000,
			regroup: true,
			templates: [ npmor, npsens, npslc ]
		},
		"NPNorthFactory": {
			assembly: "NPNorthFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 30000,
			regroup: true,
			templates: [ nppod, npsmct, npmor ]
		},
	});

	camManageTrucks(1);

	NPDefenseGroup = newGroup();
	camEnableFactory("ScavSouthFactory");
	camManageGroup(camMakeGroup("RocketScoutForce"), CAM_ORDER_ATTACK, {
		regroup: true,
		count: -1,
	});
	queue("sendRocketForce", 25000);
	queue("sendTankScoutForce", 30000);
	queue("sendTankForce", 100000); // in wzcam it moves back and then forward
	queue("enableNPFactory", 300000);
}
