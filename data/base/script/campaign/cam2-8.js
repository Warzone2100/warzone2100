include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const COLLECTIVE_RES = [
	"R-Defense-WallUpgrade05", "R-Struc-Materials06",
	"R-Struc-Factory-Upgrade06", "R-Struc-Factory-Cyborg-Upgrade06",
	"R-Struc-VTOLFactory-Upgrade03", "R-Struc-VTOLPad-Upgrade03",
	"R-Vehicle-Engine05", "R-Vehicle-Metals05", "R-Cyborg-Metals06",
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

function vtolAttack()
{
	camManageGroup(camMakeGroup("COVtolGroup"), CAM_ORDER_ATTACK, {
		regroup: false,
	});
}

function setupLandGroups()
{
	var hovers = enumArea("NWTankGroup", THE_COLLECTIVE, false).filter(function(obj) {
		return obj.type === DROID && obj.propulsion === "hover01";
	});
	var tanks = enumArea("NWTankGroup", THE_COLLECTIVE, false).filter(function(obj) {
		return obj.type === DROID && obj.propulsion !== "hover01";
	});

	camManageGroup(camMakeGroup(hovers), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("NWTankPos1"),
			camMakePos("NWTankPos2"),
			camMakePos("NWTankPos3"),
		],
		interval: camSecondsToMilliseconds(25),
		regroup: false,
	});

	camManageGroup(camMakeGroup(tanks), CAM_ORDER_DEFEND, {
		pos: [
			camMakePos("NWTankPos3"),
		],
		radius: 15,
		regroup: false,
	});

	camManageGroup(camMakeGroup("WCyborgGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("WCybPos1"),
			camMakePos("WCybPos2"),
			camMakePos("WCybPos3"),
		],
		//fallback: camMakePos("COHeavyFacR-b2Assembly"),
		//morale: 90,
		interval: camSecondsToMilliseconds(30),
		regroup: false,
	});
}

function enableFactories()
{
	camEnableFactory("COCyborgFac-b1");
	camEnableFactory("COHeavyFacL-b2");
	camEnableFactory("COHeavyFacR-b2");
	camEnableFactory("COVtolFac-b3");
}

function truckDefense()
{
	if (enumDroid(THE_COLLECTIVE, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	var list = ["AASite-QuadBof", "CO-WallTower-HvCan", "CO-Tower-RotMG"];
	camQueueBuilding(THE_COLLECTIVE, list[camRand(list.length)]);
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_2END", {
		area: "RTLZ",
		reinforcements: camMinutesToSeconds(3),
		annihilate: true
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
		"COVtolFac-b3": { tech: "R-Vehicle-Body09" }, //Tiger body
		"COHeavyFacL-b2": { tech: "R-Wpn-HvyHowitzer" },
	});

	camCompleteRequiredResearch(COLLECTIVE_RES, THE_COLLECTIVE);

	camSetEnemyBases({
		"COBase1": {
			cleanup: "COBase1Cleanup",
			detectMsg: "C28_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"COBase2": {
			cleanup: "COBase2Cleanup",
			detectMsg: "C28_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"COBase3": {
			cleanup: "COBase3Cleanup",
			detectMsg: "C28_BASE3",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
	});

	camSetFactories({
		"COCyborgFac-b1": {
			assembly: "COCyborgFac-b1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(30)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.cocybag, cTempl.npcybr, cTempl.npcybf, cTempl.npcybc]
		},
		"COHeavyFacL-b2": {
			assembly: "COHeavyFacL-b2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: [cTempl.comhpv, cTempl.cohact]
		},
		"COHeavyFacR-b2": {
			assembly: "COHeavyFacR-b2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: [cTempl.comrotmh, cTempl.cohct]
		},
		"COVtolFac-b3": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(70)),
			data: {
				regroup: false,
				count: -1,
			},
			templates: [cTempl.comhvat]
		},
	});

	camManageTrucks(THE_COLLECTIVE);
	truckDefense();

	queue("setupLandGroups", camSecondsToMilliseconds(50));
	queue("vtolAttack", camMinutesToMilliseconds(1));
	queue("enableFactories", camChangeOnDiff(camMinutesToMilliseconds(1.5)));
	setTimer("truckDefense", camMinutesToMilliseconds(2));
}
