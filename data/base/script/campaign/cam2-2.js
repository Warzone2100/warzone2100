
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const CO = 2; //The Collective player number
const warning = "pcv632.ogg"; // Collective commander escaping

var commandGroup;

const collectiveRes = [
		"R-Defense-WallUpgrade03", "R-Struc-Materials04",
		"R-Struc-Factory-Upgrade04", "R-Struc-Factory-Cyborg-Upgrade04",
		"R-Vehicle-Engine04", "R-Vehicle-Metals03", "R-Cyborg-Metals03", 
		"R-Vehicle-Armor-Heat02", "R-Cyborg-Armor-Heat02", 
		"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage04", 
		"R-Wpn-Cannon-ROF02", "R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03",
		"R-Wpn-MG-Damage06", "R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02",
		"R-Wpn-Mortar-Damage06", "R-Wpn-Mortar-ROF03", 
		"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06", 
		"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", 
		"R-Wpn-RocketSlow-Damage05", "R-Sys-Sensor-Upgrade01",
		"R-Struc-VTOLFactory-Upgrade01", "R-Struc-VTOLPad-Upgrade01",
		"R-Sys-Engineering02", "R-Wpn-Howitzer-Accuracy01", 
		"R-Wpn-Howitzer-Damage01", "R-Wpn-RocketSlow-ROF03",
];

camAreaEvent("vtolRemoveZone", function(droid)
{
	if(isVTOL(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	
	resetLabel("vtolRemoveZone", CO);
});


camAreaEvent("group1Trigger", function()
{
	hackRemoveMessage("C22_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);
	camEnableFactory("COFactoryEast");

	camManageGroup(commandGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("wayPoint1"),
		regroup: false,
	});
});

camAreaEvent("wayPoint1Rad", function(droid)
{
	if(isVTOL(droid))
	{
		resetLabel("wayPoint1Rad", CO);
		return;
	}
	camManageGroup(commandGroup, CAM_ORDER_DEFEND, { 
		pos: camMakePos("wayPoint3"),
		regroup: false,
	});
});

//Tell player that Collective Commander is leaving and group all droids
//that can attack together to defend the enemy commander.
camAreaEvent("wayPoint2Rad", function(droid)
{
	if(droid.droidType != DROID_COMMAND)
	{
		resetLabel("wayPoint2Rad", CO);
		return;
	}

	var point = getObject("wayPoint3");
	var defGroup = enumRange(point.x, point.y, 10, CO, false).filter(function(obj) {
		return obj.droidType == DROID_WEAPON
	});


	camManageGroup(commandGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("wayPoint4"),
		regroup: false
	});

	camManageGroup(camMakeGroup(defGroup), CAM_ORDER_DEFEND, {
		pos: camMakePos("defensePos"),
		regroup: false
	});

	playSound(warning);
});

camAreaEvent("failZone", function(droid)
{
	if(droid.droidType == DROID_COMMAND)
	{
		camSafeRemoveObject(droid, false);
		failSequence();
	}
	else
		resetLabel("failZone");
});

function vtolRetreat()
{
	camRetreatVtols(CO, "vtolRemovePoint");
	queue("vtolRetreat", 2000);
}

function vtolAttack()
{
	var list; with (camTemplates) list = [colatv, colatv];
	camSetVtolData(CO, "vtolAppearPoint", "vtolRemovePoint", list, 120000);
}

//Order the truck to build some defenses.
function truckDefense()
{
	var truck = enumDroid(CO, DROID_CONSTRUCT);
	if(enumDroid(CO, DROID_CONSTRUCT).length > 0)
		queue("truckDefense", 160000);

	const list = ["WallTower06", "PillBox1", "WallTower03"];
	camQueueBuilding(CO, list[camRand(list.length)]);
}

function showGameOver()
{
	var arti = getArtifacts();
	camSafeRemoveObject(arti[0], false);
	gameOverMessage(false);
}

function failSequence()
{
	camTrace("Collective Commander escaped with artifact");
	queue("showGameOver", 300);
}

function enableReinforcements()
{
	playSound("pcv440.ogg"); // Reinforcements are available.
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_2C", {
		area: "RTLZ",
		message: "C22_LZ",
		reinforcements: 180 //3 min
	});
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_2C",{
		area: "RTLZ",
		message: "C22_LZ",
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
		"COCommander": { tech: "R-Wpn-RocketSlow-Accuracy03" },
	});

	setPower(8000, CO); //used to be 10000

	camSetEnemyBases({
		"COEastBase": {
			cleanup: "eastBaseCleanup",
			detectMsg: "C22_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"COWestBase": {
			cleanup: "westBaseCleanup",
			detectMsg: "C22_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		}
	});

	with (camTemplates) camSetFactories({
		"COFactoryEast": {
			assembly: camMakePos("eastAssembly"),
			order: CAM_ORDER_ATTACK,
			groupSize: 6,
			throttle: 70000,
			regroup: true,
			repair: 40,
			templates: [cohct, comtathh, comorb] //Heavy factory
		},
		"COFactoryWest": {
			assembly: camMakePos("westAssembly"),
			throttle: 20000,
			templates: [comtath, comih] //Hover lancers/infernos
		},
	});
	
	commandGroup = camMakeGroup("group1NBase");
	camManageTrucks(CO);
	truckDefense();
	camEnableFactory("COFactoryWest");
	

	hackAddMessage("C22_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);

	queue("enableReinforcements", 20000);
	queue("vtolAttack", 60000);
}
