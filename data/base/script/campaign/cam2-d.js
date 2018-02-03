include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const UPLINK = 1; //The satellite uplink player number.
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
	if ((droid.player === THE_COLLECTIVE) && isVTOL(droid))
	{
		camSafeRemoveObject(droid, false);
	}

	resetLabel("vtolRemoveZone");
});

//Order the truck to build some defenses.
function truckDefense()
{
	if (enumDroid(THE_COLLECTIVE, DROID_CONSTRUCT).length > 0)
	{
		queue("truckDefense", 160000);
	}

	var list = ["AASite-QuadBof", "WallTower04", "GuardTower-RotMg", "WallTower-Projector"];
	camQueueBuilding(THE_COLLECTIVE, list[camRand(list.length)], camMakePos("uplinkPos"));
}

//Attacks every 2 minutes until HQ is destroyed.
function vtolAttack()
{
	var list; with (camTemplates) list = [colatv];
	camSetVtolData(THE_COLLECTIVE, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(120000), "COCommandCenter");
}

//The project captured the uplink.
function captureUplink()
{
	const GOODSND = "pcv621.ogg";	//"Objective captured"
	playSound(GOODSND);
	hackRemoveMessage("C2D_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
}

//Extra win condition callback.
function checkNASDACentral()
{
	if (getObject("uplink") === null)
	{
		return false; //It was destroyed
	}

	if (camCountStructuresInArea("uplinkClearArea", THE_COLLECTIVE) === 0)
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
		callback: "checkNASDACentral",
		annihilate: true,
		retlz: true
	});
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2_6S", {
		area: "RTLZ",
		message: "C2D_LZ",
		reinforcements: -1,
		annihilate: true,
		retlz: true
	});

	var startpos = getObject("startPosition");
	var lz = getObject("landingZone"); //player lz
	var tent = getObject("transporterEntry");
	var text = getObject("transporterExit");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	camSetArtifacts({
		"COCommandCenter": { tech: "R-Struc-VTOLPad-Upgrade01" },
		"COResearchLab": { tech: "R-Struc-Research-Upgrade04" },
		"COCommandRelay": { tech: "R-Wpn-Bomb02" },
		"COHeavyFactory": { tech: "R-Wpn-Howitzer-Accuracy01" },
	});

	setAlliance(CAM_HUMAN_PLAYER, UPLINK, true);
	setAlliance(THE_COLLECTIVE, UPLINK, true);

	camCompleteRequiredResearch(COLLECTIVE_RES, THE_COLLECTIVE);

	camSetEnemyBases({
		"COSouthEastBase": {
			cleanup: "baseCleanup",
			detectMsg: "C2D_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
	});

	with (camTemplates) camSetFactories({
		"COHeavyFactory": {
			assembly: "COHeavyFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(60000),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: [cohhpv, comhltat, cohct]
		},
		"COSouthCyborgFactory": {
			assembly: "COSouthCyborgFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(40000),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [npcybc, npcybf, npcybr, cocybag]
		},
	});

	camManageTrucks(THE_COLLECTIVE);
	truckDefense();
	hackAddMessage("C2D_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);

	camEnableFactory("COHeavyFactory");
	camEnableFactory("COSouthCyborgFactory");

	queue("enableReinforcements", 22000);
	queue("vtolAttack", 120000); // 2 min
}
