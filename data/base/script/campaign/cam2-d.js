include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const UPLINK = 1; //The satellite uplink player number.
const CO = 2; //The Collective player number.
const COLLECTIVE_RES = [
		"R-Defense-WallUpgrade04", "R-Struc-Materials05",
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
		"R-Wpn-Howitzer-Accuracy01", "R-Wpn-RocketSlow-ROF03",
		"R-Wpn-Howitzer-Damage01",
];

camAreaEvent("vtolRemoveZone", function(droid)
{
	if((droid.player === CO) && (isVTOL(droid)))
	{
		camSafeRemoveObject(droid, false);
	}

	resetLabel("vtolRemoveZone");
});

//Order the truck to build some defenses.
function truckDefense()
{
	var truck = enumDroid(CO, DROID_CONSTRUCT);
	if(enumDroid(CO, DROID_CONSTRUCT).length > 0)
		queue("truckDefense", 160000);

	var list = ["AASite-QuadBof", "WallTower04", "GuardTower-RotMg", "WallTower-Projector"];
	camQueueBuilding(CO, list[camRand(list.length)], camMakePos("uplinkPos"));
}

//VTOL units stop coming when the Collective HQ is destroyed.
function checkCollectiveHQ()
{
	if(getObject("COCommandCenter") === null)
	{
		camToggleVtolSpawn();
	}
	else
	{
		queue("checkCollectiveHQ", 8000);
	}
}

//Triggers after 1 minute. Then attacks every 2 minutes until HQ is destroyed.
function vtolAttack()
{
	var list; with (camTemplates) list = [colatv];
	camSetVtolData(CO, "vtolAppearPos", "vtolRemovePos", list, 120000);
}

//The project captured the uplink.
function captureUplink()
{
	const GOODSND = "pcv621.ogg";	//"Objective captured"
	playSound(GOODSND);
	hackRemoveMessage("C2D_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);
	//setAlliance(UPLINK, CO, false);
}

//Extra win condition callback. Failure code works, however the uplink is far too
//fragile for this to work properly. Thus it is unused for now.
function checkNASDACentral()
{
	/*
	if(getObject("uplink") == null)
	{
		const BADSND = "pcv622.ogg"	//"Objective destroyed"
		playSound(BADSND);
		return false;
	}
	*/

	var enemyStuff = enumArea("uplinkClearArea", CO, false);
	if(enemyStuff.length === 0)
	{
		camCallOnce("captureUplink");
		return true;
	}
}

function enableReinforcements()
{
	playSound("pcv440.ogg"); // Reinforcements are available.
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2_6S", {
		area: "RTLZ",
		message: "C2D_LZ",
		reinforcements: 300, //5 min
		callback: "checkNASDACentral"
	});
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2_6S",{
		area: "RTLZ",
		message: "C2D_LZ",
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
		"COCommandCenter": { tech: "R-Struc-VTOLPad-Upgrade01" },
		"COResearchLab": { tech: "R-Struc-Research-Upgrade04" },
		"COCommandRelay": { tech: "R-Wpn-Bomb02" },
		"COHeavyFactory": { tech: "R-Wpn-Howitzer-Accuracy01" },
	});

	setAlliance(CAM_HUMAN_PLAYER, UPLINK, true);
	setAlliance(CO, UPLINK, true);

	setPower(20000, CO);
	camCompleteRequiredResearch(COLLECTIVE_RES, CO);

	camSetEnemyBases({
		"COSouthEastBase": {
			cleanup: "baseCleanup",
			detectMsg: "C2D_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv393.ogg",
		},
	});

	with (camTemplates) camSetFactories({
		"COHeavyFactory": {
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: 50000,
			regroup: false,
			repair: 40,
			templates: [cohhpv, comhltat, cohct]
		},
		"COSouthCyborgFactory": {
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: 20000,
			regroup: true,
			repair: 40,
			templates: [npcybc, npcybf, npcybr, cocybag]
		},
	});

	camManageTrucks(CO);
	truckDefense();
	hackAddMessage("C2D_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);

	camEnableFactory("COHeavyFactory");
	camEnableFactory("COSouthCyborgFactory");

	queue("checkCollectiveHQ", 8000);
	queue("enableReinforcements", 20000);
	queue("vtolAttack", 60000);
}
