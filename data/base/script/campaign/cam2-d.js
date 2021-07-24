include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const UPLINK = 1; //The satellite uplink player number.
const COLLECTIVE_RES = [
	"R-Defense-WallUpgrade06", "R-Struc-Materials06", "R-Sys-Engineering02",
	"R-Vehicle-Engine04", "R-Vehicle-Metals05", "R-Cyborg-Metals05",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage05","R-Wpn-Cannon-ROF02",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage07",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage05",
	"R-Wpn-Mortar-ROF02", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage05",
	"R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF02", "R-Wpn-Howitzer-ROF02",
	"R-Wpn-Howitzer-Damage08", "R-Cyborg-Armor-Heat01", "R-Vehicle-Armor-Heat01",
	"R-Wpn-Bomb-Damage02", "R-Wpn-AAGun-Damage03", "R-Wpn-AAGun-ROF03",
	"R-Wpn-AAGun-Accuracy02", "R-Wpn-Howitzer-Accuracy01", "R-Struc-VTOLPad-Upgrade03",
];

camAreaEvent("vtolRemoveZone", function(droid)
{
	if ((droid.player === THE_COLLECTIVE) && isVTOL(droid))
	{
		camSafeRemoveObject(droid, false);
	}

	resetLabel("vtolRemoveZone", THE_COLLECTIVE);
});

//Order the truck to build some defenses.
function truckDefense()
{
	if (enumDroid(THE_COLLECTIVE, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	var list = ["AASite-QuadBof", "CO-WallTower-HvCan", "CO-Tower-RotMG", "CO-Tower-HvFlame"];
	camQueueBuilding(THE_COLLECTIVE, list[camRand(list.length)], camMakePos("uplinkPos"));
}

//Attacks every 2 minutes until HQ is destroyed.
function vtolAttack()
{
	var list = [cTempl.colatv, cTempl.commorvt];
	camSetVtolData(THE_COLLECTIVE, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(camMinutesToMilliseconds(2)), "COCommandCenter");
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

function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Secure the Uplink from The Collective"));

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2_6S", {
		area: "RTLZ",
		message: "C2D_LZ",
		reinforcements: camMinutesToSeconds(5),
		callback: "checkNASDACentral",
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

	var enemyLz = getObject("COLandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, THE_COLLECTIVE);

	camSetArtifacts({
		"COCommandCenter": { tech: "R-Struc-VTOLPad-Upgrade01" },
		"COResearchLab": { tech: "R-Struc-Research-Upgrade04" },
		"COCommandRelay": { tech: "R-Wpn-Bomb02" },
		"COHeavyFactory": { tech: "R-Wpn-Howitzer-Accuracy01" },
		"COHowitzerEmplacement": { tech: "R-Wpn-Howitzer-Damage02" },
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

	camSetFactories({
		"COHeavyFactory": {
			assembly: "COHeavyFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: [cTempl.cohhpv, cTempl.comhltat, cTempl.cohct]
		},
		"COSouthCyborgFactory": {
			assembly: "COSouthCyborgFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.npcybc, cTempl.npcybf, cTempl.npcybr, cTempl.cocybag]
		},
	});

	camManageTrucks(THE_COLLECTIVE);
	truckDefense();
	hackAddMessage("C2D_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);

	camEnableFactory("COHeavyFactory");
	camEnableFactory("COSouthCyborgFactory");

	queue("vtolAttack", camMinutesToMilliseconds(2));
	setTimer("truckDefense", camSecondsToMilliseconds(160));
}
