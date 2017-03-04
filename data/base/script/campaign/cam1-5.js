
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var NPDefenseGroup;
const NP = 1; //New Paradigm player number
const scavs = 7; // Scav player number

//Get some droids for the NP transport
function getDroidsForNPLZ(args)
{
	with (camTemplates) {
		var scouts = [ npsens, nppod, nphmg ];
		var heavies = [ npsbb, npmmct, npmrl ];
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

//Land NP transport in the LZ area (protected by four hardpoints in the NP base)
camAreaEvent("NPLZTrigger", function()
{
	playSound("pcv395.ogg"); //enemy transport detected warning sound

	var list = getDroidsForNPLZ();
	camSendReinforcement(NP, camMakePos("NPTransportPos"), list, CAM_REINFORCE_TRANSPORT, {
		entry: { x: 5, y: 55 },
		exit: { x: 7, y: 57 },
		order: CAM_ORDER_ATTACK,
		data: { regroup: true, count: -1, pos: camMakePos("NPBase") }
	});
});

//What to do if the New Paradigm builds some droid
function eventDroidBuilt(droid, structure)
{
	if (!camDef(structure) || !structure || structure.player !== NP
                           || droid.droidType === DROID_CONSTRUCT)
		return;
	if (groupSize(NPDefenseGroup) < 4)
		groupAdd(NPDefenseGroup, droid);
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

//Destroying the NP base will activate all scav factories
//And make any unfound scavs attack the player
function camEnemyBaseEliminated_NPBaseGroup()
{
	//Enable all scav factories
	camEnableFactory("ScavNorthFactory");
	camEnableFactory("ScavSouthWestFactory");
	camEnableFactory("ScavSouthEastFactory");

	//All scavs on map attack
	camManageGroup(
		camMakeGroup(enumArea(0, 0, mapWidth, mapHeight, scavs, false)),
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
	setNoGoArea(lz2.x, lz2.y, lz2.x2, lz2.y2, NP);
	var tent = getObject("TransporterEntry");
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	var text = getObject("TransporterExit");
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	//Transporter is the only droid of the player's on the map
	var transporter = enumDroid();
	cameraTrack(transporter[0]);

	//Make sure the New Paradigm and Scavs are allies
	setAlliance(NP, scavs, true);

	setPower(2000, NP);
	setPower(500, scavs);

	//Just the ever growing amount of research for enemies
	//New Paradigm
	completeResearch("R-Defense-WallUpgrade02", NP);
	completeResearch("R-Struc-Materials02", NP);
	completeResearch("R-Struc-Factory-Upgrade02", NP);
	completeResearch("R-Struc-Factory-Cyborg-Upgrade02", NP);
	completeResearch("R-Vehicle-Engine02", NP);
	completeResearch("R-Vehicle-Metals02", NP);
	completeResearch("R-Cyborg-Metals02", NP);
	completeResearch("R-Wpn-Cannon-Damage03", NP);
	completeResearch("R-Wpn-Flamer-Damage03", NP);
	completeResearch("R-Wpn-Flamer-ROF01", NP);
	completeResearch("R-Wpn-MG-Damage04", NP);
	completeResearch("R-Wpn-MG-ROF01", NP);
	completeResearch("R-Wpn-Mortar-Damage03", NP);
	completeResearch("R-Wpn-Mortar-Acc01", NP);
	completeResearch("R-Wpn-Rocket-Accuracy01", NP);
	completeResearch("R-Wpn-Rocket-Damage03", NP);
	completeResearch("R-Wpn-Rocket-ROF02", NP);
	completeResearch("R-Wpn-RocketSlow-Damage02", NP);
	completeResearch("R-Struc-RprFac-Upgrade03", NP);

	//Scavs
	completeResearch("R-Wpn-MG-Damage03", scavs);
	completeResearch("R-Wpn-Rocket-Damage02", scavs);
	completeResearch("R-Wpn-Cannon-Damage02", scavs);

	
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
			player: NP
		},
	});

	//Will fly over New Paradigm base at the start
	camDetectEnemyBase("NPBaseGroup");

	camSetArtifacts({
		"NPCyborgFactory": { tech: "R-Struc-Factory-Upgrade03" },
		"NPRightFactory": { tech: "R-Vehicle-Engine02" },
		"NPLeftFactory": { tech: "R-Vehicle-Body08" }, //scorpion body
		"NPResearchFacility": { tech: "R-Comp-SynapticLink" },
	});

	//The NP factories had unused assembly points (off map also) 
	with (camTemplates) camSetFactories({
		"NPLeftFactory": {
			//assembly: "NPLeftAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 20000,
			templates: [ npmrl, npmmct, npsbb, nphmg ]
		},
		"NPRightFactory": {
			//assembly: "NPRightAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 20000,
			templates: [ npmor, npsens, npsbb, nphmg ]
		},
		"NPCyborgFactory": {
			//assembly: "NPCyborgAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 10000,
			templates: [ npcybc, npcybf, npcybm ]
		},
		"ScavSouthWestFactory": {
			assembly: "ScavSouthWestAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 4000,
			regroup: true,
			repair: 40,
			templates: [ firecan, rbjeep, rbuggy, bloke ]
		},
		"ScavSouthEastFactory": {
			assembly: "ScavSouthEastAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 4000,
			regroup: true,
			repair: 40,
			templates: [ firecan, rbjeep, rbuggy, bloke ]
		},
		"ScavNorthFactory": {
			assembly: "ScavNorthAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 4000,
			regroup: true,
			repair: 40,
			templates: [ firecan, rbjeep, rbuggy, bloke ]
		},
	});

	
	NPDefenseGroup = newGroup();

	queue("camCallOnce", 30000, "enableReinforcements");
	queue("enableNPFactories", 50000);
}
