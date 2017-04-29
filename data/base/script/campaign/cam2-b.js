include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var videoIndex;
const VIDEOS = ["MB2_B_MSG", "MB2_B_MSG2"];
const CO = 2; //The Collective player number.
const collectiveRes = [
	"R-Defense-WallUpgrade03", "R-Struc-Materials04", "R-Struc-Factory-Upgrade04",
	"R-Struc-Factory-Cyborg-Upgrade04", "R-Struc-VTOLFactory-Upgrade01",
	"R-Struc-VTOLPad-Upgrade01", "R-Vehicle-Engine04", "R-Vehicle-Metals03",
	"R-Cyborg-Metals03", "R-Vehicle-Armor-Heat02", "R-Cyborg-Armor-Heat02",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage04", "R-Wpn-Cannon-ROF02",
	"R-Wpn-Flamer-Damage05", "R-Wpn-Flamer-ROF02", "R-Wpn-MG-Damage05",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage04",
	"R-Wpn-Mortar-ROF02", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage05",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage04", "R-Wpn-RocketSlow-ROF03", "R-Sys-Sensor-Upgrade01"
];

camAreaEvent("vtolRemoveZone", function(droid)
{
	if((droid.player === CO) && (isVTOL(droid)))
	{
		camSafeRemoveObject(droid, false);
	}

	resetLabel("vtolRemoveZone");
});

camAreaEvent("factoryTrigger", function(droid)
{
	camEnableFactory("COHeavyFacL-b1");
	camEnableFactory("COCybFacL-b2");
	camEnableFactory("COHeavyFacR-b1");
	camEnableFactory("COCybFacR-b2");
});

function camEnemyBaseDetected_COMiddleBase()
{
	hackRemoveMessage("C2B_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);
}

function eventVideoDone()
{
	if(videoIndex < VIDEOS.length)
	{
		hackAddMessage(VIDEOS[videoIndex], MISS_MSG, CAM_HUMAN_PLAYER, true);
		videoIndex = videoIndex + 1;
	}
}

function activateBase1Defenders()
{
	camManageGroup(camMakeGroup("NBaseGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("leftSideAmbushPos1"),
			camMakePos("leftSideAmbushPos2"),
			camMakePos("leftSideAmbushPos3"),
		],
		interval: 60000,
		regroup: false,
	});
}

function activateBase1Defenders2()
{
	camManageGroup(camMakeGroup("NBaseGroup-below"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("grp2Pos1"),
			camMakePos("grp2Pos2"),
			camMakePos("grp2Pos3"),
			camMakePos("grp2Pos4"),
			camMakePos("grp2Pos5"),
		],
		interval: 60000,
		regroup: false,
	});
}

function ambushPlayer()
{
	camManageGroup(camMakeGroup("centralBaseGroup"), CAM_ORDER_ATTACK, {
		fallback: camMakePos("base2Assembly"),
		morale: 20,
		regroup: false,
	});
}

//VTOL units stop coming when the Collective HQ is destroyed.
function checkCollectiveHQ()
{
	if(getObject("COCommandCenter") == null)
	{
		camEnableVtolSpawn(false);
	}
	else
	{
		queue("checkCollectiveHQ", 8000);
	}
}

function vtolAttack()
{
	var list; with (camTemplates) list = [colcbv, colatv];
	camSetVtolData(CO, "vtolAppearPos", "vtolRemove", list, 600000); //10 min
	checkCollectiveHQ();
}

function truckDefense()
{
	var truck = enumDroid(CO, DROID_CONSTRUCT);
	if(enumDroid(CO, DROID_CONSTRUCT).length > 0)
		queue("truckDefense", 160000);

	const list = ["CO-Tower-MG3", "CO-Tower-LtATRkt", "CO-Tower-MdCan", "CO-Tower-LtATRkt"];
	camQueueBuilding(CO, list[camRand(list.length)]);
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_2_2S");

	var startpos = getObject("startPosition");
	centreView(startpos.x, startpos.y);
	var lz = getObject("landingZone"); //player lz
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	setPower(20000, CO);
	camEnableRes(collectiveRes, CO);
	setMissionTime(7200); // 2 hr.
	videoIndex = 0;
	eventVideoDone(); //Play video sequences.

	camSetArtifacts({
		"COResearchLab": { tech: "R-Wpn-Flame2" },
		"COHeavyFac-b4": { tech: "R-Wpn-RocketSlow-ROF01" },
		"COHeavyFacL-b1": { tech: "R-Wpn-MG-ROF03" },
		"COCommandCenter": { tech: "R-Vehicle-Body06" }, //Panther
	});

	setPower(20000, CO);
	camEnableRes(collectiveRes, CO);

	camSetEnemyBases({
		"CONorthBase": {
			cleanup: "base1Cleanup",
			detectMsg: "C2B_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv393.ogg",
		},
		"COCentralBase": {
			cleanup: "base2Cleanup",
			detectMsg: "C2B_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv393.ogg",
		},
		"COMiddleBase": {
			cleanup: "base4Cleanup",
			detectMsg: "C2B_BASE4",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv393.ogg",
		},
	});

	with (camTemplates) camSetFactories({
		"COHeavyFacL-b1": {
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: 80000,
			regroup: false,
			repair: 40,
			templates: [comatt, cohct, comct]
		},
		"COHeavyFacR-b1": {
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: 80000,
			regroup: false,
			repair: 40,
			templates: [comatt, cohct, comct]
		},
		"COCybFacL-b2": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 30000,
			regroup: false,
			repair: 40,
			templates: [npcybc, npcybr]
		},
		"COCybFacR-b2": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 30000,
			regroup: false,
			repair: 40,
			templates: [npcybc, npcybr, npcybf, npcybm]
		},
		"COHeavyFac-b4": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 50000,
			regroup: false,
			repair: 40,
			templates: [comatt, comit]
		},
		"COCybFac-b4": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: 50000,
			regroup: false,
			repair: 40,
			templates: [npcybc, npcybr, npcybf]
		},
	});
	
	camManageTrucks(CO);
	truckDefense();
	hackAddMessage("C2B_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, true);
	camEnableFactory("COHeavyFac-b4");
	camEnableFactory("COCybFac-b4");

	ambushPlayer();

	queue("vtolAttack", 60000); //1 min
	queue("activateBase1Defenders2", 1200000); //20 min.
	queue("activateBase1Defenders", 1800000); //30 min.
}

