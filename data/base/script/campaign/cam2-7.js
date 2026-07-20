include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_collectiveRes = [
	"R-Defense-WallUpgrade06", "R-Struc-Materials06", "R-Sys-Engineering02",
	"R-Vehicle-Engine05", "R-Vehicle-Metals05", "R-Cyborg-Metals05",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage06", "R-Wpn-Cannon-ROF03",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage08",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc03", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF04", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage06",
	"R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF03", "R-Wpn-Howitzer-ROF02",
	"R-Wpn-Howitzer-Damage08", "R-Cyborg-Armor-Heat02", "R-Vehicle-Armor-Heat02",
	"R-Wpn-Bomb-Damage02", "R-Wpn-AAGun-Damage03", "R-Wpn-AAGun-ROF03",
	"R-Wpn-AAGun-Accuracy02", "R-Wpn-Howitzer-Accuracy01", "R-Struc-VTOLPad-Upgrade03",
];
const mis_collectiveResClassic = [
	"R-Defense-WallUpgrade05", "R-Struc-Materials05", "R-Struc-Factory-Upgrade05",
	"R-Struc-VTOLPad-Upgrade03", "R-Vehicle-Engine05", "R-Vehicle-Metals05",
	"R-Cyborg-Metals05", "R-Vehicle-Armor-Heat02", "R-Cyborg-Armor-Heat02",
	"R-Sys-Engineering02", "R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage05",
	"R-Wpn-Cannon-ROF03", "R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03",
	"R-Wpn-Howitzer-Accuracy02", "R-Wpn-Howitzer-Damage03", "R-Sys-Sensor-Upgrade01",
	"R-Wpn-MG-Damage07", "R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage06",
	"R-Wpn-RocketSlow-ROF03"
];
const mis_vtolAppearPositions = ["vtolAppearPos1", "vtolAppearPos2"];

camAreaEvent("vtolRemoveZone", function(droid)
{
	if ((droid.player !== CAM_HUMAN_PLAYER) && camVtolCanDisappear(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	resetLabel("vtolRemoveZone", CAM_THE_COLLECTIVE);
});

function camEnemyBaseDetected_COBase1()
{
	hackRemoveMessage("C27_OBJECTIVE1", PROX_MSG, CAM_HUMAN_PLAYER);
}

function camEnemyBaseDetected_COBase2()
{
	hackRemoveMessage("C27_OBJECTIVE2", PROX_MSG, CAM_HUMAN_PLAYER);

	const vt = enumArea("COBase2Cleanup", CAM_THE_COLLECTIVE, false).filter((obj) => (
		obj.type === DROID && isVTOL(obj)
	));
	camManageGroup(camMakeGroup(vt), CAM_ORDER_ATTACK, {
		regroup: false,
	});
}

function camEnemyBaseDetected_COBase3()
{
	hackRemoveMessage("C27_OBJECTIVE3", PROX_MSG, CAM_HUMAN_PLAYER);
}

function camEnemyBaseDetected_COBase4()
{
	hackRemoveMessage("C27_OBJECTIVE4", PROX_MSG, CAM_HUMAN_PLAYER);
}

function baseThreeVtolAttack()
{
	const vt = enumArea("vtolGroupBase3", CAM_THE_COLLECTIVE, false).filter((obj) => (
		obj.type === DROID && isVTOL(obj)
	));
	camManageGroup(camMakeGroup(vt), CAM_ORDER_ATTACK, {
		regroup: false,
	});
}

function baseFourVtolAttack()
{
	const vt = enumArea("vtolGroupBase4", CAM_THE_COLLECTIVE, false).filter((obj) => (
		obj.type === DROID && isVTOL(obj)
	));
	camManageGroup(camMakeGroup(vt), CAM_ORDER_ATTACK, {
		regroup: false,
	});
}

function enableFactoriesAndHovers()
{
	camEnableFactory("COHeavyFac-Arti-b2");
	camEnableFactory("COCyborgFac-b2");
	camEnableFactory("COCyborgFac-b3");
	camEnableFactory("COVtolFactory-b4");
	camEnableFactory("COHeavyFac-b4");
	camEnableFactory("COCyborgFac-b4");

	camManageGroup(camMakeGroup("grp2Hovers"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("hoverPos1"),
			camMakePos("playerLZ"),
			camMakePos("hoverPos2"),
		],
		//fallback: camMakePos("base2HeavyAssembly"),
		//morale: 10,
		interval: camSecondsToMilliseconds(22),
		regroup: false,
	});
}

function truckDefense()
{
	if (enumDroid(CAM_THE_COLLECTIVE, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	const list = ["Emplacement-Howitzer105", "Emplacement-Rocket06-IDF", "Sys-CB-Tower01", "Emplacement-Howitzer105", "Emplacement-Rocket06-IDF", "Sys-SensoTower02"];
	camQueueBuilding(CAM_THE_COLLECTIVE, list[camRand(list.length)], camMakePos("buildPos1"));
	camQueueBuilding(CAM_THE_COLLECTIVE, list[camRand(list.length)], camMakePos("buildPos2"));
}

function insaneWave2()
{
	const list = [cTempl.colhvat, cTempl.colhvat];
	const ext = {limit: [2, 2], alternate: true, altIdx: 0};
	camSetVtolData(CAM_THE_COLLECTIVE, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
}

function insaneWave3()
{
	const list = [cTempl.commorv, cTempl.comacv];
	const ext = {limit: [2, 3], alternate: true, altIdx: 0};
	camSetVtolData(CAM_THE_COLLECTIVE, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
}

function insaneVtolAttack()
{
	if (camClassicMode())
	{
		const list = [cTempl.colhvat, cTempl.commorvt];
		const ext = {limit: [4, 5], alternate: true, altIdx: 0};
		camSetVtolData(CAM_THE_COLLECTIVE, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
	}
	else
	{
		const list = [cTempl.commorvt, cTempl.commorvt];
		const ext = {limit: [2, 2], alternate: true, altIdx: 0};
		camSetVtolData(CAM_THE_COLLECTIVE, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
		queue("insaneWave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("insaneWave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

function insaneReinforcementSpawn()
{
	const units = [cTempl.cohhvch, cTempl.comagh, cTempl.cohach, cTempl.comltath];
	const limits = {minimum: 4, maxRandom: 4};
	const location = ["insaneSpawnPos1", "insaneSpawnPos2", "insaneSpawnPos3", "insaneSpawnPos4"];
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function insaneTransporterAttack()
{
	const DISTANCE_FROM_POS = 30;
	const units = [cTempl.cohact, cTempl.comrotmh, cTempl.cohhpv, cTempl.comhltat];
	const limits = {minimum: 4, maxRandom: 4};
	const location = camGenerateRandomMapCoordinate(getObject("startPosition"), CAM_GENERIC_LAND_STAT, DISTANCE_FROM_POS);
	camSendGenericSpawn(CAM_REINFORCE_TRANSPORT, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function insaneSetupSpawns()
{
	if (camAllowInsaneSpawns())
	{
		queue("insaneVtolAttack", camMinutesToMilliseconds(2));
		setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(4.5));
		setTimer("insaneTransporterAttack", camMinutesToMilliseconds(5.5));
	}
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, cam_levels.beta10.pre, {
		eliminateBases: true,
		area: "RTLZ",
		message: "C27_LZ",
		reinforcements: camMinutesToSeconds(3)
	});

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone"); //player lz
	const tEnt = getObject("transporterEntry");
	const tExt = getObject("transporterExit");
	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);

	if (camClassicMode())
	{
		camClassicResearch(mis_collectiveResClassic, CAM_THE_COLLECTIVE);

		camSetArtifacts({
			"COHeavyFac-Arti-b2": { tech: "R-Wpn-Cannon5" },
		});
	}
	else
	{
		camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);

		camUpgradeOnMapTemplates(cTempl.commc, cTempl.cohact, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybf, cTempl.cocybth, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybc, cTempl.cocybsn, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybr, cTempl.cocybtk, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybm, cTempl.cocybag, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.colatv, cTempl.colhvat, CAM_THE_COLLECTIVE);

		camSetArtifacts({
			"COHeavyFac-Arti-b2": { tech: ["R-Wpn-Cannon5", "R-Wpn-MG-Damage08"] },
			"COTankKillerHardpoint": { tech: "R-Wpn-RocketSlow-Damage06" },
			"COVtolFactory-b4": { tech: "R-Wpn-Bomb-Damage02" },
		});
	}

	camSetEnemyBases({
		"COBase1": {
			cleanup: "COBase1Cleanup",
			detectMsg: "C27_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COBase2": {
			cleanup: "COBase2Cleanup",
			detectMsg: "C27_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COBase3": {
			cleanup: "COBase3Cleanup",
			detectMsg: "C27_BASE3",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COBase4": {
			cleanup: "COBase4Cleanup",
			detectMsg: "C27_BASE4",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"COHeavyFac-Arti-b2": {
			assembly: "base2HeavyAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 54 : 60)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.comagt, cTempl.cohact, cTempl.cohhpv, cTempl.comhltat] : [cTempl.comagt, cTempl.cohact, cTempl.cohhpv, cTempl.comtath]
		},
		"COCyborgFac-b2": {
			assembly: "base2CybAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 36 : 40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.cocybsn, cTempl.cocybag] : [cTempl.npcybc, cTempl.cocybag]
		},
		"COCyborgFac-b3": {
			assembly: "base3CybAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 36 : 40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.cocybth, cTempl.cocybtk] : [cTempl.npcybf, cTempl.npcybr]
		},
		"COHeavyFac-b4": {
			assembly: "base4HeavyAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 63 : 70)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.comrotmh, cTempl.comhltat, cTempl.cohact, cTempl.cohhpv] : [cTempl.comrotmh, cTempl.comhltat, cTempl.cohct]
		},
		"COCyborgFac-b4": {
			assembly: "base4CybAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 36 : 40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.cocybag, cTempl.cocybsn, cTempl.cocybtk] : [cTempl.cocybag, cTempl.npcybc, cTempl.npcybr]
		},
		"COVtolFactory-b4": {
			assembly: "base4VTOLAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 54 : 60)),
			data: {
				regroup: false,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.colagv, cTempl.commorv, cTempl.commorvt, cTempl.colhvat, cTempl.comacv] : [cTempl.colagv, cTempl.commorv]
		},
	});

	//This mission shows you the approximate base locations at the start.
	//These are removed once the base it is close to is seen and is replaced
	//with a more precise proximity blip.
	hackAddMessage("C27_OBJECTIVE1", PROX_MSG, CAM_HUMAN_PLAYER, false);
	hackAddMessage("C27_OBJECTIVE2", PROX_MSG, CAM_HUMAN_PLAYER, false);
	hackAddMessage("C27_OBJECTIVE3", PROX_MSG, CAM_HUMAN_PLAYER, false);
	hackAddMessage("C27_OBJECTIVE4", PROX_MSG, CAM_HUMAN_PLAYER, false);

	if (difficulty >= HARD)
	{
		addDroid(CAM_THE_COLLECTIVE, 55, 25, "Truck Panther Tracks", tBody.tank.panther, tProp.tank.tracks, "", "", tConstruct.truck);
		camManageTrucks(CAM_THE_COLLECTIVE);
		setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(4.5)));
	}

	queue("baseThreeVtolAttack", camChangeOnDiff(camSecondsToMilliseconds(90)));
	queue("baseFourVtolAttack", camChangeOnDiff(camMinutesToMilliseconds(2)));
	queue("enableFactoriesAndHovers", camSecondsToMilliseconds(90));
	queue("insaneSetupSpawns", camMinutesToMilliseconds(7));
}
