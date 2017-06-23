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
	hackRemoveMessage("C27_OBJECTIVE1", PROX_MSG, CAM_HUMAN_PLAYER, true);
}

function camEnemyBaseDetected_COBase2()
{
	hackRemoveMessage("C27_OBJECTIVE2", PROX_MSG, CAM_HUMAN_PLAYER, true);
}

function camEnemyBaseDetected_COBase3()
{
	hackRemoveMessage("C27_OBJECTIVE3", PROX_MSG, CAM_HUMAN_PLAYER, true);
}

function camEnemyBaseDetected_COBase4()
{
	hackRemoveMessage("C27_OBJECTIVE4", PROX_MSG, CAM_HUMAN_PLAYER, true);
}

function baseFourGroupAttack()
{
	camManageGroup(camMakeGroup("grp2Hovers"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("hoverPos1"),
			camMakePos("playerLZ"),
			camMakePos("hoverPos2"),
		],
		//fallback: camMakePos("base2HeavyAssembly"),
		//morale: 10,
		interval: 22000,
		regroup: false,
	});

	camManageGroup(camMakeGroup("vtolGroupBase4"), CAM_ORDER_ATTACK, {
		regroup: false,
	});
}

function enableFactories()
{
	camEnableFactory("COVtolFactory-b4");
	camEnableFactory("COHeavyFac-Arti-b2");
	camEnableFactory("COCyborgFac-b2");
	camEnableFactory("COCyborgFac-b3");
	camEnableFactory("COHeavyFac-b4");
	camEnableFactory("COCyborgFac-b4");
}

//Player must destroy all bases.
function checkEnemyBases()
{
	if(camAllEnemyBasesEliminated())
	{
		return true;
	}
}


function enableReinforcements()
{
	playSound("pcv440.ogg"); // Reinforcements are available.
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2_8S", {
		callback: "checkEnemyBases",
		area: "RTLZ",
		message: "C27_LZ",
		reinforcements: camChangeOnDiff(180, true) //3 min
	});
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2_8S", {
		callback: "checkEnemyBases",
		area: "RTLZ",
		message: "C27_LZ",
		reinforcements: -1,
	});

	var startpos = getObject("startPosition");
	centreView(startpos.x, startpos.y);
	var lz = getObject("landingZone"); //player lz
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	var tent = getObject("transporterEntry");
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	var text = getObject("transporterExit");
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	camSetArtifacts({
		"COHeavyFac-Arti-b2": { tech: "R-Wpn-Cannon5" },
	});

	setPower(camChangeOnDiff(80000, true), THE_COLLECTIVE); //10000.
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

	with (camTemplates) camSetFactories({
		"COHeavyFac-Arti-b2": {
			assembly: camMakePos("base2HeavyAssembly"),
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(60000),
			regroup: true,
			repair: 40,
			templates: [comagt, cohact, cohhpv, comtath]
		},
		"COCyborgFac-b2": {
			assembly: camMakePos("base2CybAssembly"),
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(120000),
			regroup: true,
			repair: 40,
			templates: [npcybc, cocybag]
		},
		"COCyborgFac-b3": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(120000),
			regroup: true,
			repair: 40,
			templates: [npcybf, npcybr]
		},
		"COHeavyFac-b4": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(30000),
			regroup: true,
			repair: 40,
			templates: [comrotmh, comhltat, cohct]
		},
		"COCyborgFac-b4": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(120000),
			regroup: true,
			repair: 40,
			templates: [cocybag, npcybc, npcybr]
		},
		"COVtolFactory-b4": {
			assembly: camMakePos("base4VTOLAssembly"),
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(120000),
			regroup: false,
			repair: 40,
			templates: [colagv, commorv]
		},
	});

	enableFactories();

	//This mission shows you the approximate base locations at the start.
	//These are removed once the base it is close to is seen and is replaced
	//with a more precise proximity blip.
	hackAddMessage("C27_OBJECTIVE1", PROX_MSG, CAM_HUMAN_PLAYER, true);
	hackAddMessage("C27_OBJECTIVE2", PROX_MSG, CAM_HUMAN_PLAYER, true);
	hackAddMessage("C27_OBJECTIVE3", PROX_MSG, CAM_HUMAN_PLAYER, true);
	hackAddMessage("C27_OBJECTIVE4", PROX_MSG, CAM_HUMAN_PLAYER, true);


	queue("enableReinforcements", 15000);
	queue("baseFourGroupAttack", 100000);
}
