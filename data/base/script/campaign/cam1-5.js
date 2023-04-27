
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEW_PARADIGM_RES = [
	"R-Wpn-MG1Mk1", "R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels",
	"R-Sys-Engineering01", "R-Wpn-MG-Damage04", "R-Wpn-MG-ROF02", "R-Wpn-Cannon-Damage03",
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Defense-WallUpgrade03", "R-Struc-Materials03", "R-Vehicle-Engine02",
	"R-Struc-RprFac-Upgrade03", "R-Wpn-Rocket-Damage02", "R-Wpn-Rocket-ROF03",
	"R-Vehicle-Metals02", "R-Wpn-Mortar-Damage03", "R-Wpn-Rocket-Accuracy02",
	"R-Wpn-RocketSlow-Damage02", "R-Wpn-Mortar-ROF01", "R-Cyborg-Metals03",
	"R-Wpn-Mortar-Acc01", "R-Wpn-RocketSlow-Accuracy01", "R-Wpn-Cannon-Accuracy01",
];
const SCAVENGER_RES = [
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF01", "R-Wpn-Rocket-Damage02",
	"R-Wpn-Cannon-Damage03", "R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-ROF01",
	"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-ROF03", "R-Vehicle-Metals02",
	"R-Defense-WallUpgrade03", "R-Struc-Materials03", "R-Wpn-Cannon-Accuracy01",
	"R-Wpn-Mortar-Acc01",
];
var useHeavyReinforcement;

//Get some droids for the New Paradigm transport
function getDroidsForNPLZ(args)
{
	var lightAttackerLimit = 8;
	var heavyAttackerLimit = 3;
	var unitTemplates;
	var list = [];

	if (difficulty === HARD)
	{
		lightAttackerLimit = 9;
		heavyAttackerLimit = 4;
	}
	else if (difficulty === INSANE)
	{
		lightAttackerLimit = 10;
		heavyAttackerLimit = 5;
	}

	if (useHeavyReinforcement)
	{
		var artillery = [cTempl.npmor];
		var other = [cTempl.npmmct];
		if (camRand(2) > 0)
		{
			//Add a sensor if artillery was chosen for the heavy units
			list.push(cTempl.npsens);
			unitTemplates = artillery;
		}
		else
		{
			unitTemplates = other;
		}
	}
	else
	{
		unitTemplates = [cTempl.nppod, cTempl.npmrl, cTempl.nphmgt];
	}

	var lim = useHeavyReinforcement ? heavyAttackerLimit : lightAttackerLimit;
	for (let i = 0; i < lim; ++i)
	{
		list.push(unitTemplates[camRand(unitTemplates.length)]);
	}

	useHeavyReinforcement = !useHeavyReinforcement; //switch it
	return list;
}

//These enable Scav and NP factories when close enough
camAreaEvent("NorthScavFactoryTrigger", function(droid)
{
	camEnableFactory("ScavNorthFactory");
	camEnableFactory("NPCyborgFactory");
	camEnableFactory("NPLeftFactory");
	camEnableFactory("NPRightFactory");
});

camAreaEvent("SouthWestScavFactoryTrigger", function(droid)
{
	camEnableFactory("ScavSouthWestFactory");
});

camAreaEvent("SouthEastScavFactoryTrigger", function(droid)
{
	camEnableFactory("ScavSouthEastFactory");
});

camAreaEvent("NPFactoryTrigger", function(droid)
{
	if (camIsTransporter(droid) === false)
	{
		camEnableFactory("NPCyborgFactory");
		camEnableFactory("NPLeftFactory");
		camEnableFactory("NPRightFactory");
	}
	else
	{
		resetLabel("NPFactoryTrigger", CAM_HUMAN_PLAYER);
	}
});

//Land New Paradigm transport in the LZ area (protected by four hardpoints in the New Paradigm base)
camAreaEvent("NPLZTriggerEast", function()
{
	camCallOnce("activateNPLZTransporter");
});

camAreaEvent("NPLZTrigger", function()
{
	camCallOnce("activateNPLZTransporter");
});

function activateNPLZTransporter()
{
	setTimer("sendNPTransport", camChangeOnDiff(camMinutesToMilliseconds(3)));
	sendNPTransport();
}

function sendNPTransport()
{
	var nearbyDefense = enumArea("LandingZone2", NEW_PARADIGM, false).filter((obj) => (
		obj.type === STRUCTURE && obj.stattype === DEFENSE
	));

	if (nearbyDefense.length > 0)
	{
		var list = getDroidsForNPLZ();
		camSendReinforcement(NEW_PARADIGM, camMakePos("NPTransportPos"), list, CAM_REINFORCE_TRANSPORT, {
			entry: { x: 2, y: 42 },
			exit: { x: 2, y: 42 },
			order: CAM_ORDER_ATTACK,
			data: {
				regroup: false,
				count: -1,
				pos: camMakePos("NPBase"),
				repair: 66,
			},
		});
	}
	else
	{
		removeTimer("sendNPTransport");
	}
}

function enableNPFactories()
{
	camEnableFactory("NPCyborgFactory");
	camEnableFactory("NPLeftFactory");
	camEnableFactory("NPRightFactory");
}

//Destroying the New Paradigm base will activate all scav factories
//And make any unfound scavs attack the player
function camEnemyBaseEliminated_NPBaseGroup()
{
	//Enable all scav factories
	camEnableFactory("ScavNorthFactory");
	camEnableFactory("ScavSouthWestFactory");
	camEnableFactory("ScavSouthEastFactory");

	//Make all scavengers on map attack
	camManageGroup(
		camMakeGroup(enumArea(0, 0, mapWidth, mapHeight, SCAV_7, false)),
		CAM_ORDER_ATTACK
	);
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_1A-C", {
		area: "RTLZ",
		message: "C1-5_LZ",
		reinforcements: camMinutesToSeconds(3),
		annihilate: true
	});

	useHeavyReinforcement = false; //Start with a light unit reinforcement first
	var lz = getObject("LandingZone1"); //player lz
	var lz2 = getObject("LandingZone2"); //new paradigm lz
	var tent = getObject("TransporterEntry");
	var text = getObject("TransporterExit");
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	setNoGoArea(lz2.x, lz2.y, lz2.x2, lz2.y2, 5);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	//Transporter is the only droid of the player's on the map
	var transporter = enumDroid();
	cameraTrack(transporter[0]);

	//Make sure the New Paradigm and Scavs are allies
	setAlliance(NEW_PARADIGM, SCAV_7, true);

	camCompleteRequiredResearch(NEW_PARADIGM_RES, NEW_PARADIGM);
	camCompleteRequiredResearch(SCAVENGER_RES, SCAV_7);

	camUpgradeOnMapTemplates(cTempl.bloke, cTempl.blokeheavy, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.trike, cTempl.trikeheavy, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.buggy, cTempl.buggyheavy, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.bjeep, cTempl.bjeepheavy, SCAV_7);
	camUpgradeOnMapTemplates(cTempl.rbjeep, cTempl.rbjeep8, SCAV_7);

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
		"NPRightFactory": { tech: "R-Vehicle-Engine02" },
		"NPLeftFactory": { tech: "R-Struc-Factory-Upgrade03" },
		"NPResearchFacility": { tech: "R-Comp-SynapticLink" },
	});

	camSetFactories({
		"NPLeftFactory": {
			assembly: "NPLeftAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			templates: [ cTempl.npmrl, cTempl.npmmct, cTempl.nphmgt, cTempl.nppod ],
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
		},
		"NPRightFactory": {
			assembly: "NPRightAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(50)),
			templates: [ cTempl.npmor, cTempl.npsens, cTempl.nphmgt ],
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
		},
		"NPCyborgFactory": {
			assembly: "NPCyborgAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(35)),
			templates: [ cTempl.npcybc, cTempl.npcybf, cTempl.npcybm ],
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
		},
		"ScavSouthWestFactory": {
			assembly: "ScavSouthWestAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(15)),
			templates: [ cTempl.firecan, cTempl.rbjeep8, cTempl.rbuggy, cTempl.blokeheavy ],
			data: {
				regroup: false,
				count: -1,
			},
		},
		"ScavSouthEastFactory": {
			assembly: "ScavSouthEastAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(15)),
			templates: [ cTempl.firecan, cTempl.rbjeep8, cTempl.rbuggy, cTempl.blokeheavy ],
			data: {
				regroup: false,
				count: -1,
			},
		},
		"ScavNorthFactory": {
			assembly: "ScavNorthAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(15)),
			templates: [ cTempl.firecan, cTempl.rbjeep8, cTempl.rbuggy, cTempl.blokeheavy ],
			data: {
				regroup: false,
				count: -1,
			},
		},
	});

	queue("enableNPFactories", camChangeOnDiff(camMinutesToMilliseconds(10)));
}
