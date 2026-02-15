include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");
include("script/campaign/transitionTech.js");

const MIS_GAMMA_PLAYER = 1; //Gamma is player one.
const mis_nexusRes = [
	"R-Sys-Engineering03", "R-Defense-WallUpgrade09", "R-Struc-Materials09",
	"R-Struc-VTOLPad-Upgrade06", "R-Wpn-Bomb-Damage03", "R-Sys-NEXUSrepair",
	"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
	"R-Wpn-Mortar-Acc03", "R-Wpn-MG-Damage10", "R-Wpn-Mortar-ROF04",
	"R-Vehicle-Engine08", "R-Vehicle-Metals09", "R-Vehicle-Armor-Heat06",
	"R-Cyborg-Metals09", "R-Cyborg-Armor-Heat06", "R-Wpn-RocketSlow-ROF06",
	"R-Wpn-AAGun-Damage06", "R-Wpn-AAGun-ROF06", "R-Wpn-Howitzer-Damage09",
	"R-Wpn-Howitzer-ROF04", "R-Wpn-Cannon-Damage09", "R-Wpn-Cannon-ROF06",
	"R-Wpn-Missile-Damage02", "R-Wpn-Missile-ROF02", "R-Wpn-Missile-Accuracy02",
	"R-Wpn-Rail-Damage02", "R-Wpn-Rail-ROF02", "R-Wpn-Rail-Accuracy01",
	"R-Wpn-Energy-Damage03", "R-Wpn-Energy-ROF03", "R-Wpn-Energy-Accuracy01",
	"R-Wpn-AAGun-Accuracy03", "R-Wpn-Howitzer-Accuracy03", "R-Sys-NEXUSsensor",
];
const mis_nexusResClassic = [
	"R-Defense-WallUpgrade09", "R-Struc-Materials09", "R-Struc-Factory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals07",
	"R-Cyborg-Metals08", "R-Vehicle-Armor-Heat05", "R-Cyborg-Armor-Heat05",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Damage03", "R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage03",
	"R-Wpn-Energy-ROF03", "R-Wpn-Missile-Accuracy01", "R-Wpn-Missile-Damage02",
	"R-Wpn-Rail-Damage02", "R-Wpn-Rail-ROF02", "R-Sys-Sensor-Upgrade01",
	"R-Sys-NEXUSrepair", "R-Wpn-Flamer-Damage06", "R-Sys-NEXUSsensor",
];
const mis_vtolAppearPositions = ["vtolSpawnPos1", "vtolSpawnPos2"];
var reunited;
var betaUnitIds;
var truckLocCounter;

camAreaEvent("gammaBaseTrigger", function(droid) {
	discoverGammaBase();
});

//Remove Nexus VTOL droids.
camAreaEvent("vtolRemoveZone", function(droid)
{
	if (droid.player !== CAM_HUMAN_PLAYER && camVtolCanDisappear(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	resetLabel("vtolRemoveZone", CAM_NEXUS);
});

function insaneWave2()
{
	const list = [cTempl.nxlscouv, cTempl.nxlscouv];
	const ext = {limit: [2, 2], alternate: true, altIdx: 0};
	camSetVtolData(CAM_NEXUS, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
}

function insaneWave3()
{
	const list = [cTempl.nxlpulsev, cTempl.nxlpulsev];
	const ext = {limit: [3, 3], alternate: true, altIdx: 0};
	camSetVtolData(CAM_NEXUS, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
}

function insaneVtolAttack()
{
	if (camClassicMode())
	{
		const list = [cTempl.nxlpulsev, cTempl.nxmheapv];
		const ext = {limit: [5, 5], alternate: true, altIdx: 0};
		camSetVtolData(CAM_NEXUS, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4.5), CAM_REINFORCE_CONDITION_NONE, ext);
	}
	else
	{
		const list = [cTempl.nxmheapv, cTempl.nxmtherv];
		const ext = {limit: [2, 2], alternate: true, altIdx: 0};
		camSetVtolData(CAM_NEXUS, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4.5), CAM_REINFORCE_CONDITION_NONE, ext);
		queue("insaneWave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("insaneWave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

function setupPatrolGroups()
{
	camManageGroup(camMakeGroup("NEgroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("southBaseRetreat"),
			camMakePos("southOfVtolBase"),
			camMakePos("SWrampToGammaBase"),
			camMakePos("eastRidgeGammaBase"),
		],
		//fallback: camMakePos("southBaseRetreat"),
		//morale: 90,
		interval: camSecondsToMilliseconds(35),
		regroup: true,
	});

	camManageGroup(camMakeGroup("Egroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("southBaseRetreat"),
			camMakePos("southOfVtolBase"),
			camMakePos("SWrampToGammaBase"),
			camMakePos("eastRidgeGammaBase"),
		],
		//fallback: camMakePos("southBaseRetreat"),
		//morale: 90,
		interval: camSecondsToMilliseconds(35),
		regroup: true,
	});

	camManageGroup(camMakeGroup("vtolGroup1"), CAM_ORDER_ATTACK, {
		regroup: false, count: -1
	});

	camManageGroup(camMakeGroup("vtolGroup2"), CAM_ORDER_ATTACK, {
		regroup: false, count: -1
	});
}

//Either time based or triggered by discovering Gamma base.
function enableAllFactories()
{
	camEnableFactory("NXbase1HeavyFacArti");
	camEnableFactory("NXsouthCybFac");
	camEnableFactory("NXcybFacArti");
	camEnableFactory("NXvtolFacArti");
}

function truckDefense()
{
	if (enumDroid(CAM_NEXUS, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	const list = ["Emplacement-Howitzer150", "NX-Emp-MedArtMiss-Pit"];
	let position;

	if (truckLocCounter === 0)
	{
		position = camMakePos("buildPos1");
		truckLocCounter += 1;
	}
	else
	{
		position = camMakePos("buildPos2");
		truckLocCounter = 0;
	}

	camQueueBuilding(CAM_NEXUS, list[camRand(list.length)], position);
}

function insaneReinforcementSpawn()
{
	const units = {units: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas, cTempl.nxmscouh, cTempl.nxmrailh, cTempl.nxmangel], appended: cTempl.nxmsens};
	const limits = {minimum: 8, maxRandom: 4};
	const location = ["southSpawnPos", "eastSpawnPos"];
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_NEXUS, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function discoverGammaBase()
{
	reunited = true;
	const lz = getObject("landingZone");
	setScrollLimits(0, 0, 64, 192); //top and middle portion.
	restoreLimboMissionData();
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	camSetMissionTimer(camChangeOnDiff(camMinutesToSeconds(90)));
	setPower(playerPower(CAM_HUMAN_PLAYER) + camChangeOnDiff(10000));

	playSound(cam_sounds.powerTransferred);
	playSound(cam_sounds.rescue.groupRescued);

	camAbsorbPlayer(MIS_GAMMA_PLAYER, CAM_HUMAN_PLAYER); //Take everything they got!
	setAlliance(CAM_NEXUS, MIS_GAMMA_PLAYER, false);

	hackRemoveMessage("CM3C_GAMMABASE", PROX_MSG, CAM_HUMAN_PLAYER);
	hackRemoveMessage("CM3C_BETATEAM", PROX_MSG, CAM_HUMAN_PLAYER);

	setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(4.5)));
	truckDefense();
	enableAllFactories();
	if (camAllowInsaneSpawns())
	{
		setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(2.5));
		queue("insaneVtolAttack", camMinutesToMilliseconds(5));
	}
}

function findBetaUnitIds()
{
	const droids = enumArea("betaUnits", CAM_HUMAN_PLAYER, false).filter((obj) => (
		obj.type === DROID
	));

	for (let i = 0, len = droids.length; i < len; ++i)
	{
		betaUnitIds.push(droids[i].id);
	}
}

function betaAlive()
{
	if (reunited)
	{
		return true; //Don't need to see if Beta is still alive if reunited with base.
	}

	let alive = false;
	const myDroids = enumDroid(CAM_HUMAN_PLAYER);

	for (let i = 0, l = betaUnitIds.length; i < l; ++i)
	{
		for (let x = 0, c = myDroids.length; x < c; ++x)
		{
			if (myDroids[x].id === betaUnitIds[i])
			{
				alive = true;
				break;
			}
		}

		if (alive)
		{
			break;
		}
	}

	if (!alive)
	{
		return false;
	}
}

function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Reunite a part of Beta team with a Gamma team outpost"));

	const startPos = getObject("startPosition");
	const limboLZ = getObject("limboDroidLZ");
	reunited = false;
	betaUnitIds = [];
	truckLocCounter = 0;

	findBetaUnitIds();

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, cam_levels.gamma7, {
		callback: "betaAlive"
	});

	centreView(startPos.x, startPos.y);
	setNoGoArea(limboLZ.x, limboLZ.y, limboLZ.x2, limboLZ.y2, -1);
	setMissionTime(camChangeOnDiff(camMinutesToSeconds(10)));

	if (camClassicMode())
	{
		camClassicResearch(mis_nexusResClassic, CAM_NEXUS);
		camClassicResearch(mis_gammaAllyResClassic, MIS_GAMMA_PLAYER);

		camSetArtifacts({
			"NXbase1HeavyFacArti": { tech: "R-Vehicle-Body07" }, //retribution
			"NXcybFacArti": { tech: "R-Wpn-RailGun01" },
			"NXvtolFacArti": { tech: "R-Struc-VTOLPad-Upgrade04" },
			"NXcommandCenter": { tech: "R-Wpn-Missile-LtSAM" },
		});
	}
	else
	{
		camCompleteRequiredResearch(mis_nexusRes, CAM_NEXUS);
		camCompleteRequiredResearch(mis_gammaAllyRes, MIS_GAMMA_PLAYER);

		camSetArtifacts({
			"NXbase1HeavyFacArti": { tech: "R-Vehicle-Body07" }, //retribution
			"NXcybFacArti": { tech: "R-Wpn-RailGun01" },
			"NXvtolFacArti": { tech: "R-Struc-VTOLPad-Upgrade04" },
			"NXcommandCenter": { tech: ["R-Wpn-Missile-LtSAM", "R-Defense-WallUpgrade10"] },
		});
	}

	hackAddMessage("CM3C_GAMMABASE", PROX_MSG, CAM_HUMAN_PLAYER, false);
	hackAddMessage("CM3C_BETATEAM", PROX_MSG, CAM_HUMAN_PLAYER, false);

	setAlliance(CAM_HUMAN_PLAYER, MIS_GAMMA_PLAYER, true);
	setAlliance(CAM_NEXUS, MIS_GAMMA_PLAYER, true);

	camSetEnemyBases({
		"NXNorthBase": {
			cleanup: "northBaseCleanup",
			detectMsg: "CM3C_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NXVtolBase": {
			cleanup: "vtolBaseCleanup",
			detectMsg: "CM3C_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NXSouthBase": {
			cleanup: "southBaseCleanup",
			detectMsg: "CM3C_BASE3",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"NXbase1HeavyFacArti": {
			assembly: "NXHeavyAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
			data: {
				regroup: false,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxmrailh, cTempl.nxlflash, cTempl.nxmlinkh] //nxmsamh
		},
		"NXsouthCybFac": {
			assembly: "NXsouthCybFacAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(45)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
		},
		"NXcybFacArti": {
			assembly: "NXcybFacArtiAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
		},
		"NXvtolFacArti": {
			assembly: "NXvtolAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.nxmheapv, cTempl.nxmtherv]
		},
	});

	if (difficulty >= HARD)
	{
		addDroid(CAM_NEXUS, 31, 185, "Truck Retribution Hover", tBody.tank.retribution, tProp.tank.hover2, "", "", tConstruct.truck);
		camManageTrucks(CAM_NEXUS, false);
	}

	camPlayVideos([{video: "MB3_C_MSG", type: CAMP_MSG}, {video: "MB3_C_MSG2", type: MISS_MSG}]);
	setScrollLimits(0, 137, 64, 192); //Show the middle section of the map.
	changePlayerColour(MIS_GAMMA_PLAYER, 0);

	queue("setupPatrolGroups", camSecondsToMilliseconds(10));
	queue("enableAllFactories", camChangeOnDiff(camMinutesToMilliseconds(3)));
}
