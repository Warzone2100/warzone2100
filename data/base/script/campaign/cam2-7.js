include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const COLLECTIVE_RES = [
	"R-Defense-WallUpgrade05", "R-Struc-Materials05",
	"R-Struc-Factory-Upgrade05", "R-Struc-Factory-Cyborg-Upgrade05",
	"R-Struc-VTOLFactory-Upgrade03", "R-Struc-VTOLPad-Upgrade03",
	"R-Vehicle-Engine05", "R-Vehicle-Metals05", "R-Cyborg-Metals05",
	"R-Vehicle-Armor-Heat02", "R-Cyborg-Armor-Heat02",
	"R-Sys-Engineering02", "R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage05",
	"R-Wpn-Cannon-ROF03", "R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03",
	"R-Wpn-MG-Damage07", "R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02",
	"R-Wpn-Mortar-Damage06", "R-Wpn-Mortar-ROF03",
	"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage06", "R-Sys-Sensor-Upgrade01",
	"R-Wpn-Howitzer-Accuracy02", "R-Wpn-RocketSlow-ROF03",
	"R-Wpn-Howitzer-Damage03",
];

function camEnemyBaseDetected_COBase1()
{
	hackRemoveMessage("C27_OBJECTIVE1", PROX_MSG, CAM_HUMAN_PLAYER);
}

function camEnemyBaseDetected_COBase2()
{
	hackRemoveMessage("C27_OBJECTIVE2", PROX_MSG, CAM_HUMAN_PLAYER);

	var vt = enumArea("COBase2Cleanup", THE_COLLECTIVE, false).filter(function(obj) {
		return obj.type === DROID && isVTOL(obj);
	});
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
	var vt = enumArea("vtolGroupBase3", THE_COLLECTIVE, false).filter(function(obj) {
		return obj.type === DROID && isVTOL(obj);
	});
	camManageGroup(camMakeGroup(vt), CAM_ORDER_ATTACK, {
		regroup: false,
	});
}

function baseFourVtolAttack()
{
	var vt = enumArea("vtolGroupBase4", THE_COLLECTIVE, false).filter(function(obj) {
		return obj.type === DROID && isVTOL(obj);
	});
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

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2_8S", {
		eliminateBases: true,
		area: "RTLZ",
		message: "C27_LZ",
		reinforcements: camMinutesToSeconds(3)
	});

	var startpos = getObject("startPosition");
	var lz = getObject("landingZone"); //player lz
	var tent = getObject("transporterEntry");
	var text = getObject("transporterExit");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	var enemyLz = getObject("COLandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, THE_COLLECTIVE);

	camSetArtifacts({
		"COHeavyFac-Arti-b2": { tech: "R-Wpn-Cannon5" },
		"COTankKillerHardpoint": { tech: "R-Wpn-RocketSlow-ROF03" },
	});

	camCompleteRequiredResearch(COLLECTIVE_RES, THE_COLLECTIVE);

	camSetEnemyBases({
		"COBase1": {
			cleanup: "COBase1Cleanup",
			detectMsg: "C27_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"COBase2": {
			cleanup: "COBase2Cleanup",
			detectMsg: "C27_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"COBase3": {
			cleanup: "COBase3Cleanup",
			detectMsg: "C27_BASE3",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"COBase4": {
			cleanup: "COBase4Cleanup",
			detectMsg: "C27_BASE4",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
	});

	camSetFactories({
		"COHeavyFac-Arti-b2": {
			assembly: "base2HeavyAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds(70)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: [cTempl.comrotmh, cTempl.comhltat, cTempl.cohct]
		},
		"COCyborgFac-b4": {
			assembly: "base4CybAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
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
	hackAddMessage("C27_OBJECTIVE1", PROX_MSG, CAM_HUMAN_PLAYER, true);
	hackAddMessage("C27_OBJECTIVE2", PROX_MSG, CAM_HUMAN_PLAYER, true);
	hackAddMessage("C27_OBJECTIVE3", PROX_MSG, CAM_HUMAN_PLAYER, true);
	hackAddMessage("C27_OBJECTIVE4", PROX_MSG, CAM_HUMAN_PLAYER, true);

	queue("baseThreeVtolAttack", camSecondsToMilliseconds(30));
	queue("baseFourVtolAttack", camMinutesToMilliseconds(1));
	queue("enableFactoriesAndHovers", camChangeOnDiff(camMinutesToMilliseconds(1.5)));
}
