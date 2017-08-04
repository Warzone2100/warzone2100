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
		"R-Wpn-Howitzer-Damage02",
];

camAreaEvent("groupTrigger", function(droid)
{
	camManageGroup(camMakeGroup("mediumBaseGroup"), CAM_ORDER_DEFEND, {
		pos: camMakePos("uplinkBaseCorner"),
		regroup: false,
	});

	camManageGroup(camMakeGroup("southEastGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("playerLZ"),
			camMakePos("grp2Pos2"),
			camMakePos("uplinkBaseCorner"),
		],
		interval: 20000,
		//fallback: camMakePos("heavyFacAssembly"),
		regroup: false,
	});

	camEnableFactory("COCyborgFactory-b1");
	camEnableFactory("COCyborgFactory-b2");
	camEnableFactory("COHeavyFactory-b2R");
});

function camEnemyBaseEliminated_COUplinkBase()
{
	hackRemoveMessage("C26_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);
}

//Attacks after 1 minute.
function northWestAttack()
{
	camManageGroup(camMakeGroup("uplinkBaseGroup"), CAM_ORDER_ATTACK, {
		pos: [
			camMakePos("hillPos1"),
			camMakePos("playerLZ"),
			camMakePos("hillPos1"),
		],
		morale: 50,
		fallback: camMakePos("base1CybAssembly"),
		regroup: false,
	});

	//May not be used?
	/*
	camManageGroup(camMakeGroup("regionGroup4"), CAM_ORDER_ATTACK, {
		pos: [
			camMakePos("playerLZ"),
		],
		morale: 70,
		fallback: camMakePos("heavyFacAssembly"),
		regroup: false,
	});
	*/
}

//Attack after 2 minutes.
function mainBaseAttackGroup()
{
	camManageGroup(camMakeGroup("mainBaseGroup"), CAM_ORDER_ATTACK, {
		pos: [
			camMakePos("grp2Pos1"),
			camMakePos("playerLZ"),
			camMakePos("grp2Pos2"),
		],
		morale: 40,
		fallback: camMakePos("grp2Pos2"),
		regroup: false,
	});
}

//Order the truck to build some defenses.
function truckDefense()
{
	var truck = enumDroid(THE_COLLECTIVE, DROID_CONSTRUCT);
	if(enumDroid(THE_COLLECTIVE, DROID_CONSTRUCT).length > 0)
		queue("truckDefense", 160000);

	var list = ["Emplacement-MortarPit02", "WallTower04", "CO-Tower-LtATRkt", "Sys-CB-Tower01"];
	camQueueBuilding(THE_COLLECTIVE, list[camRand(list.length)], camMakePos("heavyFacAssembly"));
}

//After 3 minutes.
function enableTimeBasedFactories()
{
	camEnableFactory("COMediumFactory");
	camEnableFactory("COCyborgFactory-Arti");
	camEnableFactory("COHeavyFactory-b2L");
}


function enableReinforcements()
{
	playSound("pcv440.ogg"); // Reinforcements are available.
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2_7S", {
		area: "RTLZ",
		message: "C26_LZ",
		reinforcements: 180, true //3 min
	});
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2_7S", {
		area: "RTLZ",
		message: "C26_LZ",
		reinforcements: -1
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
		"COCyborgFactory-Arti": { tech: "R-Wpn-Rocket07-Tank-Killer" },
		"COCommandCenter": { tech: "R-Wpn-Mortar3" },
		"uplink": { tech: "R-Sys-VTOLCBS-Tower01" },
	});

	setPower(camChangeOnDiff(50000, true), THE_COLLECTIVE);
	camCompleteRequiredResearch(COLLECTIVE_RES, THE_COLLECTIVE);

	camSetEnemyBases({
		"COUplinkBase": {
			cleanup: "uplinkBaseCleanup",
			detectMsg: "C26_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"COMainBase": {
			cleanup: "mainBaseCleanup",
			detectMsg: "C26_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"COMediumBase": {
			cleanup: "mediumBaseCleanup",
			detectMsg: "C26_BASE3",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
	});

	with (camTemplates) camSetFactories({
		"COCyborgFactory-Arti": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(60000),
			regroup: false,
			repair: 40,
			templates: [npcybc, npcybf, cocybag, npcybr]
		},
		"COCyborgFactory-b1": {
			assembly: camMakePos("base1CybAssembly"),
			order: CAM_ORDER_ATTACK,
			groupSize: 6,
			throttle: camChangeOnDiff(50000),
			regroup: false,
			repair: 40,
			templates: [cocybag, npcybr]
		},
		"COCyborgFactory-b2": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(70000),
			regroup: false,
			repair: 40,
			templates: [npcybc, npcybf]
		},
		"COHeavyFactory-b2L": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(90000),
			regroup: false,
			repair: 40,
			templates: [cohact, comhpv, comrotm]
		},
		"COHeavyFactory-b2R": {
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(80000),
			regroup: false,
			repair: 40,
			templates: [comrotm, comhltat, cohact, comsensh]
		},
		"COMediumFactory": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(40000),
			regroup: false,
			repair: 40,
			templates: [comhpv, comagt, comrotm]
		},
	});

	camManageTrucks(THE_COLLECTIVE);
	truckDefense();
	hackAddMessage("C26_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);

	queue("enableReinforcements", 20000);
	queue("northWestAttack", 120000);
	queue("mainBaseAttackGroup", 180000);
	queue("enableTimeBasedFactories", 180000);
}
