
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const SCAVS = 7; //Scav player number
const NEW_PARADIGM_RESEARCH = [
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF01", "R-Defense-WallUpgrade03",
	"R-Struc-Materials03", "R-Struc-Factory-Upgrade03",
	"R-Struc-Factory-Cyborg-Upgrade03", "R-Vehicle-Engine03",
	"R-Vehicle-Metals03", "R-Cyborg-Metals03", "R-Wpn-Cannon-Accuracy01",
	"R-Wpn-Cannon-Damage03", "R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01",
	"R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-Acc01", "R-Wpn-Rocket-Accuracy01",
	"R-Wpn-Rocket-Damage03", "R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy02",
	"R-Wpn-RocketSlow-Damage03", "R-Struc-RprFac-Upgrade03",
];
const SCAVENGER_RES = [
	"R-Wpn-MG-Damage03", "R-Wpn-Rocket-Damage02", "R-Wpn-Cannon-Damage02",
];
var artiGroup;
var enemyHasArtifact;
var enemyStoleArtifact;


//These enable scav factories when close enough
camAreaEvent("northScavFactoryTrigger", function()
{
	camEnableFactory("scavNorthEastFactory");
});

camAreaEvent("southScavFactoryTrigger", function()
{
	camEnableFactory("scavSouthEastFactory");
});

camAreaEvent("middleScavFactoryTrigger", function()
{
	camEnableFactory("scavMiddleFactory");
});

//If a group member of artiGroup gets to the waypoint, then go to the
//New Paradigm landing zone.
camAreaEvent("NPWayPointTrigger", function()
{
	camManageGroup(artiGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("NPTransportPos"),
		regroup: false
	});
});

//Land New Paradigm transport if the New Paradigm have the artifact.
camAreaEvent("NPTransportTrigger", function()
{
	if (enemyHasArtifact)
	{
		var list = [];
		with (camTemplates) list = [npmrl, npmrl];
		camSendReinforcement(NEW_PARADIGM, camMakePos("NPTransportPos"), list, CAM_REINFORCE_TRANSPORT, {
			entry: { x: 39, y: 0 },
			exit: { x: 32, y: 62 }
		});
		playSound("pcv632.ogg"); //enemy transport escaping warning sound
	}
});

camAreaEvent("artifactCheckNP", function()
{
	enemyHasArtifact = true;
	var artifact = camGetArtifacts();
	camSafeRemoveObject(artifact[0], false);

	camPlayVideos("SB1_7_MSG3");
	hackAddMessage("C1-7_LZ2", PROX_MSG, CAM_HUMAN_PLAYER, true); //NPLZ blip
	camCallOnce("removeCanyonBlip");

	camManageGroup(artiGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("NPWayPoint"),
		regroup: false
	});
});

//Remove nearby droids. Make sure the player loses if the NP still has the artifact
//by the time it lands.
function eventTransporterLanded(transport)
{
	if ((transport.player === NEW_PARADIGM) && enemyHasArtifact)
	{
		enemyStoleArtifact = true;
		var crew = enumRange(transport.x, transport.y, 6, NEW_PARADIGM, false).filter(function(obj) {
			return obj.type === DROID && obj.group === artiGroup;
		});
		for (var i = 0, l = crew.length; i < l; ++i)
		{
			var obj = crew[i];
			camSafeRemoveObject(obj, false);
		}
	}
}

function eventTransporterExit(transport)
{
	if (transport.player === NEW_PARADIGM)
	{
		if (enemyStoleArtifact)
		{
			gameOverMessage(false);
		}
	}
}

//Check if the artifact group member are still alive and drop the artifact if needed.
function eventGroupLoss(obj, group, newsize)
{
	if ((group === artiGroup) && !newsize && enemyHasArtifact && !enemyStoleArtifact)
	{
		var acrate = addFeature("Crate", obj.x, obj.y);
		addLabel(acrate, "newArtiPos");

		camSetArtifacts({
			"newArtiPos": { tech: "R-Wpn-Cannon3Mk1" },
		});

		enemyHasArtifact = false;
		hackRemoveMessage("C1-7_LZ2", PROX_MSG, CAM_HUMAN_PLAYER);
	}
}

//Moves some New Paradigm forces to the artifact
function getArtifact()
{
	camManageGroup(artiGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("artifactLocation"),
		regroup: false
	});
}

//New Paradigm truck builds six lancer hardpoints around LZ
function buildLancers()
{
	for (var i = 1; i <= 6; ++i)
	{
		camQueueBuilding(NEW_PARADIGM, "WallTower06", "hardPoint" + i);
	}
}

//Must destroy all of the New Paradigm droids and make sure the artifact is safe.
function extraVictory()
{
	if (!enumDroid(NEW_PARADIGM).length)
	{
		return true;
	}
}

//Enable transport reinforcements
function enableReinforcements()
{
	playSound("pcv440.ogg"); // Reinforcements are available.
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_DS", {
		area: "RTLZ",
		message: "C1-7_LZ",
		reinforcements: 60, //1 min
		callback: "extraVictory",
		retlz: true,
	});
}

function removeCanyonBlip()
{
	hackRemoveMessage("C1-7_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
}

function eventPickup(feature, droid)
{
	if (feature.stattype === ARTIFACT)
	{
		if (droid.player === CAM_HUMAN_PLAYER)
		{
			if (enemyHasArtifact)
			{
				hackRemoveMessage("C1-7_LZ2", PROX_MSG, CAM_HUMAN_PLAYER);
			}
			enemyHasArtifact = false;
			camCallOnce("removeCanyonBlip");
		}
	}
}

//Mission setup stuff
function eventStartLevel()
{
	enemyHasArtifact = false;
	enemyStoleArtifact = false;
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone"); //player lz
	var tent = getObject("transporterEntry");
	var text = getObject("transporterExit");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_DS", {
		area: "RTLZ",
		message: "C1-7_LZ",
		reinforcements: -1,
		callback: "extraVictory",
		retlz: true,
	});

	//Make sure the New Paradigm and Scavs are allies
	setAlliance(NEW_PARADIGM, SCAVS, true);

	//Get rid of the already existing crate and replace with another
	camSafeRemoveObject("artifact1", false);
	camSetArtifacts({
		"artifactLocation": { tech: "R-Wpn-Cannon3Mk1" },
	});

	setPower(AI_POWER, NEW_PARADIGM);
	setPower(AI_POWER, SCAVS);

	camCompleteRequiredResearch(NEW_PARADIGM_RESEARCH, NEW_PARADIGM);
	camCompleteRequiredResearch(SCAVENGER_RES, SCAVS);

	camSetEnemyBases({
		"ScavMiddleGroup": {
			assembly: "middleAssembly",
			cleanup: "scavMiddle",
			detectMsg: "C1-7_BASE1",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"ScavSouthEastGroup": {
			assembly: "southAssembly",
			cleanup: "scavSouthEast",
			detectMsg: "C1-7_BASE2",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"ScavNorthEastGroup": {
			assembly: "northAssembly",
			cleanup: "scavNorth",
			detectMsg: "C1-7_BASE3",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
	});

	with (camTemplates) camSetFactories({
		"scavMiddleFactory": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(10000),
			data: {
				regroup: true,
				count: -1,
			},
			templates: [ firecan, rbjeep, rbuggy, bloke ]
		},
		"scavSouthEastFactory": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(10000),
			data: {
				regroup: true,
				count: -1,
			},
			templates: [ firecan, rbjeep, rbuggy, bloke ]
		},
		"scavNorthEastFactory": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(10000),
			rdata: {
				regroup: true,
				count: -1,
			},
			templates: [ firecan, rbjeep, rbuggy, bloke ]
		},
	});

	artiGroup = camMakeGroup(enumArea("NPArtiGroup", NEW_PARADIGM, false));
	camManageTrucks(NEW_PARADIGM);
	buildLancers();

	hackAddMessage("C1-7_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true); //Canyon
	queue("enableReinforcements", 15000);
	queue("getArtifact", camChangeOnDiff(90000));
}
