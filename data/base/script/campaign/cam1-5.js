include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_newParadigmRes = [
	"R-Wpn-MG1Mk1", "R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels",
	"R-Sys-Engineering01", "R-Wpn-MG-Damage04", "R-Wpn-MG-ROF02", "R-Wpn-Cannon-Damage03",
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Defense-WallUpgrade02", "R-Struc-Materials02", "R-Vehicle-Engine02",
	"R-Struc-RprFac-Upgrade03", "R-Wpn-Rocket-Damage02", "R-Wpn-Rocket-ROF03",
	"R-Vehicle-Metals02", "R-Wpn-Mortar-Damage03", "R-Wpn-Rocket-Accuracy02",
	"R-Wpn-RocketSlow-Damage02", "R-Wpn-Mortar-ROF01", "R-Cyborg-Metals03",
	"R-Wpn-Mortar-Acc01", "R-Wpn-RocketSlow-Accuracy01", "R-Wpn-Cannon-Accuracy01",
];
const mis_scavengerRes = [
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF01", "R-Wpn-Rocket-Damage02",
	"R-Wpn-Cannon-Damage03", "R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-ROF01",
	"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-ROF03", "R-Vehicle-Metals02",
	"R-Defense-WallUpgrade02", "R-Struc-Materials02", "R-Wpn-Cannon-Accuracy01",
	"R-Wpn-Mortar-Acc01",
];
const mis_newParadigmResClassic = [
	"R-Defense-WallUpgrade02", "R-Struc-Materials02", "R-Struc-Factory-Upgrade02",
	"R-Vehicle-Engine02", "R-Vehicle-Metals02", "R-Cyborg-Metals02", "R-Wpn-Cannon-Accuracy01",
	"R-Wpn-Cannon-Damage03", "R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF01", "R-Wpn-Mortar-Acc01", "R-Wpn-Mortar-Damage03",
	"R-Wpn-Rocket-Accuracy01", "R-Wpn-Rocket-Damage03", "R-Wpn-Rocket-ROF02",
	"R-Wpn-RocketSlow-Damage02", "R-Struc-RprFac-Upgrade03"
];
const mis_scavengerResClassic = [
	"R-Wpn-MG-Damage03", "R-Wpn-Rocket-Damage02"
];
var useHeavyReinforcement;

//Get some droids for the New Paradigm transport
function getDroidsForNPLZ(args)
{
	let lightAttackerLimit = 8;
	let heavyAttackerLimit = (camClassicMode()) ? 3 : 6;
	let unitTemplates;
	const list = [];

	if (difficulty === HARD)
	{
		lightAttackerLimit = 9;
		heavyAttackerLimit = (camClassicMode()) ? 4 : 7;
	}
	else if (difficulty >= INSANE)
	{
		lightAttackerLimit = 10;
		heavyAttackerLimit = (camClassicMode()) ? 5 : 8;
	}

	if (useHeavyReinforcement)
	{
		if (camClassicMode())
		{
			unitTemplates = [cTempl.npsbb, cTempl.npmmct, cTempl.npmrl];
		}
		else
		{
			const artillery = (!camClassicMode()) ? [cTempl.npmorb] : [cTempl.npmor];
			const other = [cTempl.npmmct];
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
	}
	else
	{
		unitTemplates = (!camClassicMode()) ? [cTempl.nppod, cTempl.npmrl, cTempl.nphmgt] : [cTempl.npsens, cTempl.nppod, cTempl.nphmg];
	}

	const LIM = useHeavyReinforcement ? heavyAttackerLimit : lightAttackerLimit;
	for (let i = 0; i < LIM; ++i)
	{
		list.push(unitTemplates[camRand(unitTemplates.length)]);
	}

	useHeavyReinforcement = !useHeavyReinforcement; //switch it
	return list;
}

function insaneReinforcementSpawn()
{
	const units = [cTempl.npcybc, cTempl.npcybr];
	const limits = {minimum: 6, maxRandom: 4};
	const location = ["insaneSpawnPos1", "insaneSpawnPos2", "insaneSpawnPos3", "insaneSpawnPos4"];
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_NEW_PARADIGM, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function insaneSetupSpawns()
{
	if (camAllowInsaneSpawns())
	{
		setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(2.5));
	}
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
		camCallOnce("insaneSetupSpawns");
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
	const nearbyDefense = enumArea("LandingZone2", CAM_NEW_PARADIGM, false).filter((obj) => (
		obj.type === STRUCTURE && obj.stattype === DEFENSE
	));

	if (nearbyDefense.length > 0)
	{
		const list = getDroidsForNPLZ();
		camSendReinforcement(CAM_NEW_PARADIGM, camMakePos("NPTransportPos"), list, CAM_REINFORCE_TRANSPORT, {
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
	camCallOnce("insaneSetupSpawns");
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
		camMakeGroup(enumArea(0, 0, mapWidth, mapHeight, CAM_SCAV_7, false)),
		CAM_ORDER_ATTACK
	);
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, cam_levels.alpha10, {
		area: "RTLZ",
		message: "C1-5_LZ",
		reinforcements: camMinutesToSeconds(3),
		annihilate: true
	});

	useHeavyReinforcement = false; //Start with a light unit reinforcement first
	const lz = getObject("LandingZone1"); //player lz
	const tEnt = getObject("TransporterEntry");
	const tExt = getObject("TransporterExit");
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);

	//Transporter is the only droid of the player's on the map
	const transporter = enumDroid();
	cameraTrack(transporter[0]);

	//Make sure the New Paradigm and Scavs are allies
	setAlliance(CAM_NEW_PARADIGM, CAM_SCAV_7, true);

	if (camClassicMode())
	{
		camClassicResearch(mis_newParadigmResClassic, CAM_NEW_PARADIGM);
		camClassicResearch(mis_scavengerResClassic, CAM_SCAV_7);

		camSetArtifacts({
			"NPCyborgFactory": { tech: "R-Struc-Factory-Upgrade03" },
			"NPRightFactory": { tech: "R-Vehicle-Engine02" },
			"NPLeftFactory": { tech: "R-Vehicle-Body08" },
			"NPResearchFacility": { tech: "R-Comp-SynapticLink" },
		});
	}
	else
	{
		camCompleteRequiredResearch(mis_newParadigmRes, CAM_NEW_PARADIGM);
		camCompleteRequiredResearch(mis_scavengerRes, CAM_SCAV_7);

		camUpgradeOnMapTemplates(cTempl.bloke, cTempl.blokeheavy, CAM_SCAV_7);
		camUpgradeOnMapTemplates(cTempl.trike, cTempl.trikeheavy, CAM_SCAV_7);
		camUpgradeOnMapTemplates(cTempl.buggy, cTempl.buggyheavy, CAM_SCAV_7);
		camUpgradeOnMapTemplates(cTempl.bjeep, cTempl.bjeepheavy, CAM_SCAV_7);
		camUpgradeOnMapTemplates(cTempl.rbjeep, cTempl.rbjeep8, CAM_SCAV_7);

		camSetArtifacts({
			"NPRightFactory": { tech: "R-Vehicle-Engine02" },
			"NPLeftFactory": { tech: "R-Struc-Factory-Upgrade03" },
			"NPResearchFacility": { tech: "R-Comp-SynapticLink" },
		});
	}

	camSetEnemyBases({
		"ScavNorthGroup": {
			cleanup: "ScavNorth",
			detectMsg: "C1-5_BASE1",
			detectSnd: cam_sounds.baseDetection.scavengerBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerBaseEradicated
		},
		"ScavSouthWestGroup": {
			cleanup: "ScavSouthWest",
			detectMsg: "C1-5_BASE2",
			detectSnd: cam_sounds.baseDetection.scavengerBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerBaseEradicated
		},
		"ScavSouthEastGroup": {
			cleanup: "ScavSouthEast",
			detectMsg: "C1-5_BASE3",
			detectSnd: cam_sounds.baseDetection.scavengerBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerBaseEradicated
		},
		"NPBaseGroup": {
			cleanup: "NPBase",
			detectMsg: "C1-5_OBJ1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
			player: CAM_NEW_PARADIGM
		},
		"ScavOutpostGroup": {
			cleanup: "ScavMiddle",
			detectMsg: "C1-5_BASE4",
			detectSnd: cam_sounds.baseDetection.scavengerOutpostDetected,
			eliminateSnd: cam_sounds.baseElimination.scavengerOutpostEradicated
		},
	});

	camSetFactories({
		"NPLeftFactory": {
			assembly: "NPLeftAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			templates: (!camClassicMode()) ? [ cTempl.npmrl, cTempl.npmmct, cTempl.nphmgt, cTempl.nppod ] : [ cTempl.npmrl, cTempl.npmmct, cTempl.npsbb, cTempl.nphmg ],
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
			templates: (!camClassicMode()) ? [ cTempl.npmorb, cTempl.npsens, cTempl.nphmgt ] : [ cTempl.npmor, cTempl.npsens, cTempl.npsbb, cTempl.nphmg ],
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
			templates: (!camClassicMode()) ? [ cTempl.firecan, cTempl.rbjeep8, cTempl.rbuggy, cTempl.blokeheavy ] : [ cTempl.firecan, cTempl.rbjeep, cTempl.rbuggy, cTempl.bloke ],
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
			templates: (!camClassicMode()) ? [ cTempl.firecan, cTempl.rbjeep8, cTempl.rbuggy, cTempl.blokeheavy ] : [ cTempl.firecan, cTempl.rbjeep, cTempl.rbuggy, cTempl.bloke ],
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
			templates: (!camClassicMode()) ? [ cTempl.firecan, cTempl.rbjeep8, cTempl.rbuggy, cTempl.blokeheavy ] : [ cTempl.firecan, cTempl.rbjeep, cTempl.rbuggy, cTempl.bloke ],
			data: {
				regroup: false,
				count: -1,
			},
		},
	});

	queue("enableNPFactories", camChangeOnDiff(camMinutesToMilliseconds(10)));
}
