include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_collectiveRes = [
	"R-Defense-WallUpgrade06", "R-Struc-Materials06", "R-Sys-Engineering02",
	"R-Vehicle-Engine04", "R-Vehicle-Metals05", "R-Cyborg-Metals05",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage05", "R-Wpn-Cannon-ROF02",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage07",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage05",
	"R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF02", "R-Wpn-Howitzer-ROF02",
	"R-Wpn-Howitzer-Damage08", "R-Cyborg-Armor-Heat01", "R-Vehicle-Armor-Heat01",
	"R-Wpn-Bomb-Damage02", "R-Wpn-AAGun-Damage03", "R-Wpn-AAGun-ROF03",
	"R-Wpn-AAGun-Accuracy02", "R-Wpn-Howitzer-Accuracy01", "R-Struc-VTOLPad-Upgrade03",
];

function camEnemyBaseDetected_COMainBase()
{
	camManageGroup(camMakeGroup("mediumBaseGroup"), CAM_ORDER_DEFEND, {
		pos: camMakePos("uplinkBaseCorner"),
		radius: 20,
		regroup: false,
	});

	camEnableFactory("COCyborgFactory-b1");
	camEnableFactory("COCyborgFactory-b2");
	camEnableFactory("COHeavyFactory-b2R");
}

camAreaEvent("factoryTriggerWest", function(droid)
{
	enableTimeBasedFactories();
});

camAreaEvent("factoryTriggerEast", function(droid)
{
	enableTimeBasedFactories();
});

function camEnemyBaseEliminated_COUplinkBase()
{
	hackRemoveMessage("C26_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
}

//Group together attack droids in this base that are not already in a group
function camEnemyBaseDetected_COMediumBase()
{
	const droids = enumArea("mediumBaseCleanup", CAM_THE_COLLECTIVE, false).filter((obj) => (
		obj.type === DROID && obj.group === null && obj.canHitGround
	));

	camManageGroup(camMakeGroup(droids), CAM_ORDER_ATTACK, {
		regroup: false,
	});
}

function truckDefense()
{
	if (enumDroid(CAM_THE_COLLECTIVE, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	const list = ["Emplacement-Howitzer105", "Emplacement-Rocket06-IDF", "Sys-CB-Tower01", "Emplacement-Howitzer105", "Emplacement-Rocket06-IDF", "Sys-SensoTower02"];
	camQueueBuilding(CAM_THE_COLLECTIVE, list[camRand(list.length)], camMakePos("buildPos1"));
	camQueueBuilding(CAM_THE_COLLECTIVE, list[camRand(list.length)], camMakePos("buildPos2"));
}

function southEastAttack()
{
	camManageGroup(camMakeGroup("southEastGroup"), CAM_ORDER_COMPROMISE, {
		pos: [
			camMakePos("playerLZ"),
		],
		repair: 40,
		regroup: false,
	});
}

function northWestAttack()
{
	camManageGroup(camMakeGroup("uplinkBaseGroup"), CAM_ORDER_ATTACK, {
		//morale: 10,
		//fallback: camMakePos("base1CybAssembly"),
		regroup: false,
	});
}

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

function enableTimeBasedFactories()
{
	camEnableFactory("COMediumFactory");
	camEnableFactory("COCyborgFactory-Arti");
	camEnableFactory("COHeavyFactory-b2L");
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "SUB_2_7S", {
		area: "RTLZ",
		message: "C26_LZ",
		reinforcements: camMinutesToSeconds(3)
	});

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone"); //player lz
	const tEnt = getObject("transporterEntry");
	const tExt = getObject("transporterExit");
	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);

	const enemyLz = getObject("COLandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, CAM_THE_COLLECTIVE);

	camSetArtifacts({
		"COCyborgFactory-Arti": { tech: "R-Wpn-Rocket07-Tank-Killer" },
		"COCommandCenter": { tech: "R-Wpn-Mortar3" },
		"uplink": { tech: "R-Sys-VTOLCBS-Tower01" },
		"COMediumFactory": { tech: "R-Wpn-Cannon4AMk1" },
		"COWhirlwindSite": { tech: "R-Wpn-AAGun04" },
	});

	camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);

	camSetEnemyBases({
		"COUplinkBase": {
			cleanup: "uplinkBaseCleanup",
			detectMsg: "C26_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COMainBase": {
			cleanup: "mainBaseCleanup",
			detectMsg: "C26_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COMediumBase": {
			cleanup: "mediumBaseCleanup",
			detectMsg: "C26_BASE3",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"COCyborgFactory-Arti": {
			assembly: "COCyborgFactory-ArtiAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 36 : 40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.npcybc, cTempl.npcybf, cTempl.cocybag, cTempl.npcybr]
		},
		"COCyborgFactory-b1": {
			assembly: "COCyborgFactory-b1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 6,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 45 : 50)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.cocybag, cTempl.npcybr]
		},
		"COCyborgFactory-b2": {
			assembly: "COCyborgFactory-b2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 45 : 50)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.npcybc, cTempl.npcybf]
		},
		"COHeavyFactory-b2L": {
			assembly: "COHeavyFactory-b2LAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 72 : 80)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: [cTempl.cohact, cTempl.comhpv, cTempl.comrotm]
		},
		"COHeavyFactory-b2R": {
			assembly: "COHeavyFactory-b2RAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 54 : 60)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: [cTempl.comrotm, cTempl.comhltat, cTempl.cohact, cTempl.comsensh]
		},
		"COMediumFactory": {
			assembly: "COMediumFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds((difficulty <= MEDIUM) ? 40 : 45)),
			data: {
				regroup: false,
				repair: 30,
				count: -1,
			},
			templates: [cTempl.comhpv, cTempl.comagt, cTempl.comrotm]
		},
	});

	hackAddMessage("C26_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);

	if (difficulty >= HARD)
	{
		addDroid(CAM_THE_COLLECTIVE, 26, 27, "Truck Panther Tracks", tBody.tank.panther, tProp.tank.tracks, "", "", tConstruct.truck);
		addDroid(CAM_THE_COLLECTIVE, 42, 4, "Truck Panther Tracks", tBody.tank.panther, tProp.tank.tracks, "", "", tConstruct.truck);
		camManageTrucks(CAM_THE_COLLECTIVE);
		setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(6)));
	}

	queue("northWestAttack", camChangeOnDiff(camMinutesToMilliseconds(3)));
	queue("mainBaseAttackGroup", camChangeOnDiff(camMinutesToMilliseconds((difficulty <= MEDIUM) ? 4 : 4.5)));
	queue("southEastAttack", camChangeOnDiff(camMinutesToMilliseconds((difficulty <= MEDIUM) ? 4.5 : 5)));
	queue("enableTimeBasedFactories", camChangeOnDiff(camMinutesToMilliseconds((difficulty <= MEDIUM) ? 5 : 6)));
}
