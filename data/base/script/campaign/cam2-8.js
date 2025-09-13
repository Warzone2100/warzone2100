include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_collectiveRes = [
	"R-Defense-WallUpgrade06", "R-Struc-Materials06", "R-Sys-Engineering02",
	"R-Vehicle-Engine06", "R-Vehicle-Metals06", "R-Cyborg-Metals06",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage06", "R-Wpn-Cannon-ROF03",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage08",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage06",
	"R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF03", "R-Wpn-Howitzer-ROF03",
	"R-Wpn-Howitzer-Damage09", "R-Cyborg-Armor-Heat03", "R-Vehicle-Armor-Heat03",
	"R-Wpn-Bomb-Damage02", "R-Wpn-AAGun-Damage03", "R-Wpn-AAGun-ROF03",
	"R-Wpn-AAGun-Accuracy02", "R-Wpn-Howitzer-Accuracy02", "R-Struc-VTOLPad-Upgrade03",
];
const mis_collectiveResClassic = [
	"R-Defense-WallUpgrade05", "R-Struc-Materials06", "R-Struc-Factory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade03", "R-Vehicle-Engine05", "R-Vehicle-Metals05",
	"R-Cyborg-Metals06", "R-Vehicle-Armor-Heat02", "R-Cyborg-Armor-Heat02",
	"R-Sys-Engineering02", "R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage06",
	"R-Wpn-Cannon-ROF03", "R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03",
	"R-Wpn-Howitzer-Accuracy02", "R-Wpn-Howitzer-Damage03", "R-Sys-Sensor-Upgrade01",
	"R-Wpn-MG-Damage07", "R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage06",
	"R-Wpn-RocketSlow-ROF03"
];

camAreaEvent("vtolRemoveZone", function(droid)
{
	if ((droid.player !== CAM_HUMAN_PLAYER) && camVtolCanDisappear(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	resetLabel("vtolRemoveZone", CAM_THE_COLLECTIVE);
});

function vtolGroupAttack()
{
	camManageGroup(camMakeGroup("COVtolGroup"), CAM_ORDER_ATTACK, {
		regroup: false,
	});
}

function setupLandGroups()
{
	const hovers = enumArea("NWTankGroup", CAM_THE_COLLECTIVE, false).filter((obj) => (
		obj.type === DROID && obj.propulsion === tProp.tank.hover
	));
	const tanks = enumArea("NWTankGroup", CAM_THE_COLLECTIVE, false).filter((obj) => (
		obj.type === DROID && obj.propulsion !== tProp.tank.hover
	));

	camManageGroup(camMakeGroup(hovers), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("NWTankPos1"),
			camMakePos("NWTankPos2"),
			camMakePos("NWTankPos3"),
		],
		interval: camSecondsToMilliseconds(25),
		regroup: false,
	});

	camManageGroup(camMakeGroup(tanks), CAM_ORDER_ATTACK, {
		regroup: false,
	});

	if (difficulty >= HARD)
	{
		camManageGroup(camMakeGroup("ETankGroup"), CAM_ORDER_ATTACK, {
			regroup: false,
		});
	}

	camManageGroup(camMakeGroup("WCyborgGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("WCybPos1"),
			camMakePos("WCybPos2"),
			camMakePos("WCybPos3"),
		],
		//fallback: camMakePos("COHeavyFacR-b2Assembly"),
		//morale: 90,
		interval: camSecondsToMilliseconds(30),
		regroup: false,
	});
}

function enableFactories()
{
	camEnableFactory("COCyborgFac-b1");
	camEnableFactory("COHeavyFacL-b2");
	camEnableFactory("COHeavyFacR-b2");
	camEnableFactory("COVtolFac-b3");
}

function truckDefense()
{
	if (enumDroid(CAM_THE_COLLECTIVE, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	const list = ["Emplacement-Rocket06-IDF", "Emplacement-Howitzer150", "CO-Tower-HvATRkt", "CO-Tower-HVCan", "Sys-CB-Tower01"];
	camQueueBuilding(CAM_THE_COLLECTIVE, list[camRand(list.length)], camMakePos("buildPos1"));
	camQueueBuilding(CAM_THE_COLLECTIVE, list[camRand(list.length)], camMakePos("buildPos2"));
	camQueueBuilding(CAM_THE_COLLECTIVE, list[camRand(list.length)], camMakePos("buildPos3"));
}

function insaneWave2()
{
	const list = [cTempl.colhvat, cTempl.comacv];
	const ext = {limit: [2, 4], alternate: true, altIdx: 0};
	camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camMinutesToMilliseconds(3), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
}

function insaneWave3()
{
	const list = [cTempl.comacv, cTempl.commorv];
	const ext = {limit: [4, 2], alternate: true, altIdx: 0};
	camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camMinutesToMilliseconds(3), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
}

function insaneVtolAttack()
{
	if (camClassicMode())
	{
		const list = [cTempl.commorvt, cTempl.comacv];
		const ext = {limit: [2, 4], alternate: true, altIdx: 0};
		camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camMinutesToMilliseconds(3), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
	}
	else
	{
		const list = [cTempl.commorvt, cTempl.commorvt];
		const ext = {limit: [2, 2], alternate: true, altIdx: 0};
		camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camMinutesToMilliseconds(3), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
		queue("insaneWave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("insaneWave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

function insaneReinforcementSpawn()
{
	const SCAN_DISTANCE = 1;
	const DISTANCE_FROM_POS = 25;
	const units = [cTempl.cohhvch, cTempl.comagh, cTempl.cohach, cTempl.comltath];
	const limits = {minimum: 5, maxRandom: 3};
	const location = camGenerateRandomMapEdgeCoordinate(getObject("startPosition"), CAM_GENERIC_WATER_STAT, DISTANCE_FROM_POS, SCAN_DISTANCE);
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function insaneTransporterAttack()
{
	const DISTANCE_FROM_POS = 25;
	const units = (!camClassicMode()) ? [cTempl.cocybsn, cTempl.cocybtk, cTempl.comltath, cTempl.cohhvch] : [cTempl.cohact, cTempl.cohct, cTempl.comhltat];
	const limits = {minimum: 7, maxRandom: 3};
	const location = camGenerateRandomMapCoordinate(getObject("startPosition"), CAM_GENERIC_LAND_STAT, DISTANCE_FROM_POS);
	camSendGenericSpawn(CAM_REINFORCE_TRANSPORT, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function insaneSetupSpawns()
{
	if (camAllowInsaneSpawns())
	{
		queue("insaneVtolAttack", camMinutesToMilliseconds(2));
		setTimer("insaneTransporterAttack", camMinutesToMilliseconds(3.5));
		setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(4.5));
	}
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, cam_levels.betaEnd, {
		area: "RTLZ",
		reinforcements: camMinutesToSeconds(3),
		annihilate: true
	});

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone"); //player lz
	const tEnt = getObject("transporterEntry");
	const tExt = getObject("transporterExit");
	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);

	camSetArtifacts({
		"COVtolFac-b3": { tech: "R-Vehicle-Body09" }, //Tiger body
		"COHeavyFacL-b2": { tech: "R-Wpn-HvyHowitzer" },
	});

	if (camClassicMode())
	{
		camClassicResearch(mis_collectiveResClassic, CAM_THE_COLLECTIVE);
	}
	else
	{
		camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);

		camUpgradeOnMapTemplates(cTempl.commc, cTempl.cohhpv, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.comtath, cTempl.comltath, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybf, cTempl.cocybth, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybc, cTempl.cocybsn, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybr, cTempl.cocybtk, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybm, cTempl.cocybag, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.colatv, cTempl.colhvat, CAM_THE_COLLECTIVE);

		//New AC Tiger tracked units for Hard and Insane difficulty
		if (difficulty >= HARD)
		{
			addDroid(CAM_THE_COLLECTIVE, 30, 22, "AC Tiger Tracks", tBody.tank.tiger, tProp.tank.tracks, "", "", tWeap.tank.assaultCannon);
			addDroid(CAM_THE_COLLECTIVE, 30, 23, "AC Tiger Tracks", tBody.tank.tiger, tProp.tank.tracks, "", "", tWeap.tank.assaultCannon);
			addDroid(CAM_THE_COLLECTIVE, 31, 22, "AC Tiger Tracks", tBody.tank.tiger, tProp.tank.tracks, "", "", tWeap.tank.assaultCannon);
			addDroid(CAM_THE_COLLECTIVE, 31, 23, "AC Tiger Tracks", tBody.tank.tiger, tProp.tank.tracks, "", "", tWeap.tank.assaultCannon);
		}
	}

	camSetEnemyBases({
		"COBase1": {
			cleanup: "COBase1Cleanup",
			detectMsg: "C28_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COBase2": {
			cleanup: "COBase2Cleanup",
			detectMsg: "C28_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COBase3": {
			cleanup: "COBase3Cleanup",
			detectMsg: "C28_BASE3",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"COCyborgFac-b1": {
			assembly: "COCyborgFac-b1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 30 : 35)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.cocybag, cTempl.cocybtk] : [cTempl.cocybag, cTempl.npcybr, cTempl.npcybf, cTempl.npcybc]
		},
		"COHeavyFacL-b2": {
			assembly: "COHeavyFacL-b2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 70 : 75)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.cohhpv, cTempl.cohact] : [cTempl.comhpv, cTempl.cohact]
		},
		"COHeavyFacR-b2": {
			assembly: "COHeavyFacR-b2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 6,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 55 : 60)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.comrotmh, cTempl.cohact] : [cTempl.comrotmh, cTempl.cohct]
		},
		"COVtolFac-b3": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 45 : 50)),
			data: {
				regroup: false,
				count: -1,
			},
			templates: [cTempl.comhvat]
		},
	});

	camManageTrucks(CAM_THE_COLLECTIVE);

	queue("setupLandGroups", camSecondsToMilliseconds(90));
	queue("vtolGroupAttack", camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 100 : 110)));
	queue("enableFactories", camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 135 : 150)));
	queue("insaneSetupSpawns", camMinutesToMilliseconds(5));
	setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(3)));
	truckDefense();
}
