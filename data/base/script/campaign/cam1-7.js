
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NP = 1; //New Paradigm player number
const scavs = 7; //Scav player number
var artiGroup;
var lastPosX, lastPosY;

//Determine if NP has artifact values:
//0 - Does not have artifact
//1 - Has artifact; Can be dropped
//2 - Has artifact; Can not be dropped
var enemyHasArtifact;


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
//New Paradigm landing zone
camAreaEvent("NPWayPointTrigger", function()
{
	camManageGroup(artiGroup, CAM_ORDER_DEFEND, 
		{ pos: camMakePos("NPTransportPos") }
	);
});

//The artifact group move to waypoint 1
camAreaEvent("artifactCheckNP", function()
{
	var artifact = getArtifacts();
	camSafeRemoveObject(artifact[0], false);
	enemyHasArtifact = 1;

	hackAddMessage("SB1_7_MSG3", MISS_MSG, CAM_HUMAN_PLAYER, true);
	hackRemoveMessage("C1-7_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true); //canyon
	hackAddMessage("C1-7_LZ2", PROX_MSG, CAM_HUMAN_PLAYER, true); //NPLZ blip

	camManageGroup(artiGroup, CAM_ORDER_DEFEND, 
		{ pos: camMakePos("NPWayPoint") }
	);

	checkArtifactGroup();
});

//Land New Paradigm transport if the New Paradigm have the artifact
//Also prevent it from being dropped anymore
camAreaEvent("NPTransportTrigger", function()
{
	if(enemyHasArtifact === 1)
	{
		enemyHasArtifact = 2;
		var list = [];
		with(camTemplates) list = [npmrl, npmrl];
		camSendReinforcement(NP, camMakePos("NPTransportPos"), list, CAM_REINFORCE_TRANSPORT, {
			entry: { x: 39, y: 0 },
			exit: { x: 32, y: 62 }
		});
		playSound("pcv632.ogg"); //enemy transport escaping warning sound
		queue("timedGameLoss", 18000);
	}
});

//Check if the artifact group member are still alive
//If not, then drop the artifact unless enemyHasArtifact = 2
function checkArtifactGroup()
{
	var droids = enumGroup(artiGroup);

	if(droids.length !== 0)
	{
		lastPosX = droids[0].x;
		lastPosY = droids[0].y;
		queue("checkArtifactGroup", 1000);
	}
	else
	{
		if(enemyHasArtifact !== 2)
		{
			var acrate = addFeature("Crate", lastPosX, lastPosY);
			addLabel(acrate, "newArtiPos");
		
			camSetArtifacts({
				"newArtiPos": { tech: "R-Wpn-Cannon3Mk1" },
			});
			enemyHasArtifact = 0;
			hackRemoveMessage("C1-7_LZ2", PROX_MSG, CAM_HUMAN_PLAYER, true);
		}
	}
}

//Give New Paradigm transporter time to lift off before showing game over
function timedGameLoss()
{
	camTrace("Enemy stole artifact");
	gameOverMessage(false);
}

//Moves some New Paradigm forces to the artifact
function getArtifact()
{
	camManageGroup(artiGroup, CAM_ORDER_DEFEND,
		{ pos: camMakePos("artifactLocation") }
	);
}

//New Paradigm truck builds six lancer hardpoints around NP LZ
function buildLancers()
{
	for(var i = 1; i <= 6; ++i)
	{
		camQueueBuilding(NP, "WallTower06", "hardPoint" + i);
	}
}

//Enable transport reinforcements
function enableReinforcements()
{
	playSound("pcv440.ogg"); // Reinforcements are available.
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_DS", {
		area: "RTLZ",
		message: "C1-7_LZ",
		reinforcements: 60 //1 min
	});
}

///Remove canyon blip should the player reach it before the New Paradigm does
function eventPickup(droid, feature)
{
	if(droid.player === CAM_HUMAN_PLAYER && enemyHasArtifact === 0)
		hackRemoveMessage("C1-7_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);
};

//Mission setup stuff
function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_1_DS", {
		area: "RTLZ",
		message: "C1-7_LZ",
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

	//Make sure the New Paradigm and Scavs are allies
	setAlliance(NP, scavs, true);

	//Get rid of the already existing crate and replace with another
	camSafeRemoveObject("artifact1", false);
	camSetArtifacts({
		"artifactLocation": { tech: "R-Wpn-Cannon3Mk1" },
	});

	setPower(2000, NP);
	setPower(200, scavs);

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
	completeResearch("R-Wpn-RocketSlow-Accuracy02", NP);
	completeResearch("R-Wpn-RocketSlow-Damage03", NP);
	completeResearch("R-Struc-RprFac-Upgrade03", NP);

	//Scav research
	completeResearch("R-Wpn-MG-Damage03", scavs);
	completeResearch("R-Wpn-Rocket-Damage02", scavs);
	completeResearch("R-Wpn-Cannon-Damage02", scavs);

	
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
			throttle: 4000,
			regroup: true,
			repair: 40,
			templates: [ firecan, rbjeep, rbuggy, bloke ]
		},
		"scavSouthEastFactory": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 4000,
			regroup: true,
			repair: 40,
			templates: [ firecan, rbjeep, rbuggy, bloke ]
		},
		"scavNorthEastFactory": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 4000,
			regroup: true,
			repair: 40,
			templates: [ firecan, rbjeep, rbuggy, bloke ]
		},
	});
	
	artiGroup = camMakeGroup(enumArea("NPArtiGroup", NP, false));
	enemyHasArtifact = 0;
	camManageTrucks(NP);
	buildLancers();


	hackAddMessage("C1-7_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true); //Canyon

	queue("camCallOnce", 15000, "enableReinforcements");
	queue("getArtifact", 120000);
}
