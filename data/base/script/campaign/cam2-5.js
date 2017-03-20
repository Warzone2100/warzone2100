include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const CO = 2; //The Collective player number
const collectiveRes = [
		"R-Defense-WallUpgrade03", "R-Struc-Materials05",
		"R-Struc-Factory-Upgrade05", "R-Struc-Factory-Cyborg-Upgrade05",
		"R-Struc-VTOLFactory-Upgrade02", "R-Struc-VTOLPad-Upgrade01", 
		"R-Vehicle-Engine04", "R-Vehicle-Metals04", "R-Cyborg-Metals05", 
		"R-Vehicle-Armor-Heat02", "R-Cyborg-Armor-Heat02",
		"R-Sys-Engineering02", "R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage04", 
		"R-Wpn-Cannon-ROF02", "R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03",
		"R-Wpn-MG-Damage07", "R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02",
		"R-Wpn-Mortar-Damage06", "R-Wpn-Mortar-ROF03", 
		"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06", 
		"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", 
		"R-Wpn-RocketSlow-Damage05", "R-Sys-Sensor-Upgrade01",
		"R-Wpn-Howitzer-Accuracy01", "R-Wpn-RocketSlow-ROF03",
		"R-Wpn-Howitzer-Damage01"
];

camAreaEvent("factoryTrigger", function(droid)
{
	camEnableFactory("COMediumFactory");
	camEnableFactory("COCyborgFactoryL");
	camEnableFactory("COCyborgFactoryR");

	camManageGroup(camMakeGroup("canalGuards"), CAM_ORDER_ATTACK, {
		morale: 60,
		fallback: camMakePos("mediumFactoryAssembly"),
		repair: 30,
		regroup: false,
	});
});

camAreaEvent("damTrigger", function(droid)
{
	camManageGroup(camMakeGroup("damGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("damWaypoint1"),
			camMakePos("damWaypoint2"),
			camMakePos("damWaypoint3"),
		],
		morale: 10,
		fallback: camMakePos("damWaypoint1"),
		repair: 40,
		regroup: false,
	});
});

function camEnemyBaseEliminated_COEastBase()
{
	hackRemoveMessage("C25_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);
}

function setupCyborgsNorth()
{
	camManageGroup(camMakeGroup("northCyborgs"), CAM_ORDER_ATTACK, {
		morale: 70,
		fallback: camMakePos("mediumFactoryAssembly"),
		repair: 30,
		regroup: false,
	});
}

function setupCyborgsEast()
{
	camManageGroup(camMakeGroup("eastCyborgs"), CAM_ORDER_ATTACK, {
		pos: camMakePos("playerLZ"),
		morale: 90,
		fallback: camMakePos("crossroadWaypoint"),
		repair: 30,
		regroup: false,
	});
}

function enableReinforcements()
{
	playSound("pcv440.ogg"); // Reinforcements are available.
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2DS", {
		area: "RTLZ",
		message: "C25_LZ",
		reinforcements: 180 //3 min
	});
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2DS",{
		area: "RTLZ",
		message: "C25_LZ",
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
		"NuclearReactor": { tech: "R-Struc-Power-Upgrade01" },
		"COMediumFactory": { tech: "R-Wpn-Cannon4AMk1" },
		"COCyborgFactoryL": { tech: "R-Wpn-MG4" },
	});

	setPower(10000, CO);
	camEnableRes(collectiveRes, CO);

	camSetEnemyBases({
		"COEastBase": {
			cleanup: "baseCleanup",
			detectMsg: "C25_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv393.ogg",
		}
	});

	with (camTemplates) camSetFactories({
		"COMediumFactory": {
			assembly: camMakePos("mediumFactoryAssembly"),
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 40000,
			regroup: true,
			repair: 40,
			templates: [comct, comatt, comhpv]
		},
		"COCyborgFactoryL": {
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: 25000,
			regroup: true,
			repair: 40,
			templates: [cocybag, npcybf, npcybr]
		},
		"COCyborgFactoryR": {
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: 25000,
			regroup: true,
			repair: 40,
			templates: [npcybr, npcybc]
		},
	});
	
	hackAddMessage("C25_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);

	queue("enableReinforcements", 15000);
	queue("setupCyborgsEast", 180000);//3 min
	queue("setupCyborgsNorth", 600000);//10 min
}

