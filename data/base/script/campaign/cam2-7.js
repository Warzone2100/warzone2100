include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_collectiveRes = [
	"R-Defense-WallUpgrade06", "R-Struc-Materials06", "R-Sys-Engineering02",
	"R-Vehicle-Engine05", "R-Vehicle-Metals05", "R-Cyborg-Metals05",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage06", "R-Wpn-Cannon-ROF03",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage08",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage06",
	"R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF03", "R-Wpn-Howitzer-ROF02",
	"R-Wpn-Howitzer-Damage08", "R-Cyborg-Armor-Heat02", "R-Vehicle-Armor-Heat02",
	"R-Wpn-Bomb-Damage02", "R-Wpn-AAGun-Damage03", "R-Wpn-AAGun-ROF03",
	"R-Wpn-AAGun-Accuracy02", "R-Wpn-Howitzer-Accuracy01", "R-Struc-VTOLPad-Upgrade03",
];

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

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2_8S", {
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

	const enemyLz = getObject("COLandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, CAM_THE_COLLECTIVE);

	camSetArtifacts({
		"COHeavyFac-Arti-b2": { tech: ["R-Wpn-Cannon5", "R-Wpn-MG-Damage08"] },
		"COTankKillerHardpoint": { tech: "R-Wpn-RocketSlow-Damage06" },
		"COVtolFactory-b4": { tech: "R-Wpn-Bomb-Damage02" },
	});

	camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);

	camUpgradeOnMapTemplates(cTempl.commc, cTempl.cohact, CAM_THE_COLLECTIVE);

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
			templates: [cTempl.comagt, cTempl.cohact, cTempl.cohhpv, cTempl.comtath]
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
			templates: [cTempl.npcybc, cTempl.cocybag]
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
			templates: [cTempl.npcybf, cTempl.npcybr]
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
			templates: [cTempl.comrotmh, cTempl.comhltat, cTempl.cohact, cTempl.cohhpv]
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
			templates: [cTempl.cocybag, cTempl.npcybc, cTempl.npcybr]
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
			templates: [cTempl.colagv, cTempl.commorv, cTempl.commorvt]
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
}
