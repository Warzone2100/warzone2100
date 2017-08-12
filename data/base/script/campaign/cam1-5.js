
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const SCAVS = 7; // Scav player number
const NEW_PARADIGM_RES = [
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF01", "R-Defense-WallUpgrade02",
	"R-Struc-Materials02", "R-Struc-Factory-Upgrade02",
	"R-Struc-Factory-Cyborg-Upgrade02", "R-Vehicle-Engine02",
	"R-Vehicle-Metals02", "R-Cyborg-Metals02", "R-Wpn-Cannon-Damage03",
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01", "R-Wpn-Mortar-Damage03",
	"R-Wpn-Mortar-Acc01", "R-Wpn-Rocket-Accuracy01", "R-Wpn-Rocket-Damage03",
	"R-Wpn-Rocket-ROF02", "R-Wpn-RocketSlow-Damage02", "R-Struc-RprFac-Upgrade03",
];

const SCAVENGER_RES = [
	"R-Wpn-MG-Damage03", "R-Wpn-Rocket-Damage02", "R-Wpn-Cannon-Damage02",
];


//Get some droids for the New Paradigm transport
function getDroidsForNPLZ(args)
{
	var scouts;
	var heavies;

	with (camTemplates) {
		scouts = [ npsens, nppod, nphmg ];
		heavies = [ npsbb, npmmct, npmrl ];
	}

	var numScouts = camRand(5) + 1;
	var heavy = heavies[camRand(heavies.length)];
	var list = [];

	for (var i = 0; i < numScouts; ++i)
	{
		list[list.length] = scouts[camRand(scouts.length)];
	}

	for (var i = numScouts; i < 8; ++i)
	{
		list[list.length] = heavy;
	}

	return list;
}

//These enable scav factories when close enough
camAreaEvent("NorthScavFactoryTrigger", function()
{
	camEnableFactory("ScavNorthFactory");
});

camAreaEvent("SouthWestScavFactoryTrigger", function()
{
	camEnableFactory("ScavSouthWestFactory");
});

camAreaEvent("SouthEastScavFactoryTrigger", function()
{
	camEnableFactory("ScavSouthEastFactory");
});

//Land New Paradigm transport in the LZ area (protected by four hardpoints in the New Paradigm base)
camAreaEvent("NPLZTrigger", function()
{
	sendNPTransport();
});

function sendNPTransport()
{
	var tPos = getObject("NPTransportPos");
	var nearbyDefense = enumRange(tPos.x, tPos.y, 6, NEW_PARADIGM, false);

	if(nearbyDefense.length)
	{
		var list = getDroidsForNPLZ();
		camSendReinforcement(NEW_PARADIGM, camMakePos("NPTransportPos"), list, CAM_REINFORCE_TRANSPORT, {
			entry: { x: 1, y: 42 },
			exit: { x: 1, y: 42 },
			order: CAM_ORDER_ATTACK,
			data: { regroup: true, count: -1, pos: camMakePos("NPBase") }
		});

		queue("sendNPTransport", camChangeOnDiff(180000)); //3 min
	}
}

//Enable transport reinforcements
function enableReinforcements()
{
	playSound("pcv440.ogg"); // Reinforcements are available.
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_1A-C", {
		area: "RTLZ",
		message: "C1-5_LZ",
		reinforcements: 180 //3 min
	});
}

function enableNPFactories()
{
	camEnableFactory("NPCyborgFactory");
	camEnableFactory("NPLeftFactory");
	camEnableFactory("NPRightFactory");
}

//Destroying the NEW_PARADIGM base will activate all scav factories
//And make any unfound SCAVS attack the player
function camEnemyBaseEliminated_NPBaseGroup()
{
	//Enable all scav factories
	camEnableFactory("ScavNorthFactory");
	camEnableFactory("ScavSouthWestFactory");
	camEnableFactory("ScavSouthEastFactory");

	//All SCAVS on map attack
	camManageGroup(
		camMakeGroup(enumArea(0, 0, mapWidth, mapHeight, SCAVS, false)),
		CAM_ORDER_ATTACK
	);
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_1A-C", {
		area: "RTLZ",
		message: "C1-5_LZ",
		reinforcements: -1
	});

	var lz = getObject("LandingZone1"); //player lz
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	var lz2 = getObject("LandingZone2"); //new paradigm lz
	setNoGoArea(lz2.x, lz2.y, lz2.x2, lz2.y2, NEW_PARADIGM);
	var tent = getObject("TransporterEntry");
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	var text = getObject("TransporterExit");
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	//Transporter is the only droid of the player's on the map
	var transporter = enumDroid();
	cameraTrack(transporter[0]);

	//Make sure the New Paradigm and Scavs are allies
	setAlliance(NEW_PARADIGM, SCAVS, true);

	setPower(camChangeOnDiff(15000, true), NEW_PARADIGM);
	setPower(camChangeOnDiff(800, true), SCAVS);

	camCompleteRequiredResearch(NEW_PARADIGM_RES, NEW_PARADIGM);
	camCompleteRequiredResearch(SCAVENGER_RES, SCAVS);


	camSetEnemyBases({
		"ScavNorthGroup": {
			cleanup: "ScavNorth",
			detectMsg: "C1-5_BASE1",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"ScavSouthWestGroup": {
			cleanup: "ScavSouthWest",
			detectMsg: "C1-5_BASE2",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"ScavSouthEastGroup": {
			cleanup: "ScavSouthEast",
			detectMsg: "C1-5_BASE3",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"NPBaseGroup": {
			cleanup: "NPBase",
			detectMsg: "C1-5_OBJ1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
			player: NEW_PARADIGM
		},
	});

	camSetArtifacts({
		"NPCyborgFactory": { tech: "R-Struc-Factory-Upgrade03" },
		"NPRightFactory": { tech: "R-Vehicle-Engine02" },
		"NPLeftFactory": { tech: "R-Vehicle-Body08" }, //scorpion body
		"NPResearchFacility": { tech: "R-Comp-SynapticLink" },
	});

	with (camTemplates) camSetFactories({
		"NPLeftFactory": {
			//assembly: "NPLeftAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(20000),
			templates: [ npmrl, npmmct, npsbb, nphmg ]
		},
		"NPRightFactory": {
			//assembly: "NPRightAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(20000),
			templates: [ npmor, npsens, npsbb, nphmg ]
		},
		"NPCyborgFactory": {
			//assembly: "NPCyborgAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(10000),
			templates: [ npcybc, npcybf, npcybm ]
		},
		"ScavSouthWestFactory": {
			assembly: "ScavSouthWestAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(4000),
			regroup: true,
			repair: 40,
			templates: [ firecan, rbjeep, rbuggy, bloke ]
		},
		"ScavSouthEastFactory": {
			assembly: "ScavSouthEastAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(4000),
			regroup: true,
			repair: 40,
			templates: [ firecan, rbjeep, rbuggy, bloke ]
		},
		"ScavNorthFactory": {
			assembly: "ScavNorthAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(4000),
			regroup: true,
			repair: 40,
			templates: [ firecan, rbjeep, rbuggy, bloke ]
		},
	});

	queue("camCallOnce", 30000, "enableReinforcements");
	queue("enableNPFactories", 50000);
}
