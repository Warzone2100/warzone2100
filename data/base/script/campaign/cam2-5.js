include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_collectiveRes = [
	"R-Defense-WallUpgrade05", "R-Struc-Materials05", "R-Sys-Engineering02",
	"R-Vehicle-Engine04", "R-Vehicle-Metals05", "R-Cyborg-Metals05",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage05", "R-Wpn-Cannon-ROF02",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage07",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage05",
	"R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF02", "R-Wpn-Howitzer-ROF01",
	"R-Wpn-Howitzer-Damage07", "R-Cyborg-Armor-Heat01", "R-Vehicle-Armor-Heat01",
	"R-Wpn-Bomb-Damage01", "R-Wpn-AAGun-Damage02", "R-Wpn-AAGun-ROF03",
	"R-Wpn-AAGun-Accuracy01", "R-Struc-VTOLPad-Upgrade02"
];
const mis_collectiveResClassic = [
	"R-Defense-WallUpgrade03", "R-Struc-Materials05", "R-Struc-Factory-Upgrade05",
	"R-Struc-VTOLPad-Upgrade01", "R-Vehicle-Engine04", "R-Vehicle-Metals03",
	"R-Cyborg-Metals04", "R-Vehicle-Armor-Heat02", "R-Cyborg-Armor-Heat02",
	"R-Sys-Engineering02", "R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage04",
	"R-Wpn-Cannon-ROF02", "R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03",
	"R-Wpn-Howitzer-Accuracy01", "R-Wpn-Howitzer-Damage01", "R-Sys-Sensor-Upgrade01",
	"R-Wpn-MG-Damage07", "R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage05",
	"R-Wpn-RocketSlow-ROF03"
];
const mis_vtolAppearPositions = ["vtolAppearPos1", "vtolAppearPos2"];

camAreaEvent("factoryTrigger", function(droid)
{
	enableFactories();
	camManageGroup(camMakeGroup("canalGuards"), CAM_ORDER_ATTACK, {
		morale: 60,
		fallback: camMakePos("COMediumFactoryAssembly"),
		repair: 67,
		regroup: false,
	});
});

camAreaEvent("vtolRemoveZone", function(droid)
{
	if ((droid.player !== CAM_HUMAN_PLAYER) && camVtolCanDisappear(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	resetLabel("vtolRemoveZone", CAM_THE_COLLECTIVE);
});

function camEnemyBaseEliminated_COEastBase()
{
	hackRemoveMessage("C25_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
}

//Tell everything not grouped on map to attack
function camEnemyBaseDetected_COEastBase()
{
	const droids = enumArea(0, 0, mapWidth, mapHeight, CAM_THE_COLLECTIVE, false).filter((obj) => (
		obj.type === DROID && obj.group === null && obj.canHitGround
	));

	camManageGroup(camMakeGroup(droids), CAM_ORDER_ATTACK, {
		count: -1,
		regroup: false,
		repair: 80
	});
}

function setupDamHovers()
{
	camManageGroup(camMakeGroup("damGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("damWaypoint1"),
			camMakePos("damWaypoint2"),
			camMakePos("damWaypoint3"),
		],
		//morale: 10,
		//fallback: camMakePos("damWaypoint1"),
		repair: 67,
		regroup: true,
	});
}

function setupCyborgsNorth()
{
	camManageGroup(camMakeGroup("northCyborgs"), CAM_ORDER_ATTACK, {
		morale: 70,
		fallback: camMakePos("COMediumFactoryAssembly"),
		repair: 67,
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

function enableFactories()
{
	camEnableFactory("COMediumFactory");
	camEnableFactory("COCyborgFactoryL");
	camEnableFactory("COCyborgFactoryR");
	if (camAllowInsaneSpawns())
	{
		queue("insaneVtolAttack", camMinutesToMilliseconds(2.5));
		setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(6));
		setTimer("insaneTransporterAttack", camMinutesToMilliseconds(7));
	}
}

function insaneWave2()
{
	const list = [cTempl.colhvat, cTempl.colhvat];
	const ext = {limit: [2, 2], alternate: true, altIdx: 0};
	camSetVtolData(CAM_THE_COLLECTIVE, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
}

function insaneWave3()
{
	const list = [cTempl.commorv, cTempl.comhvcv];
	const ext = {limit: [2, 2], alternate: true, altIdx: 0};
	camSetVtolData(CAM_THE_COLLECTIVE, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
}

function insaneVtolAttack()
{
	if (camClassicMode())
	{
		const list = [cTempl.colpbv, cTempl.colhvat, cTempl.commorv];
		const ext = {limit: [4, 3, 4], alternate: true, altIdx: 0};
		camSetVtolData(CAM_THE_COLLECTIVE, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
	}
	else
	{
		const list = [cTempl.colpbv, cTempl.colpbv];
		const ext = {limit: [2, 2], alternate: true, altIdx: 0};
		camSetVtolData(CAM_THE_COLLECTIVE, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
		queue("insaneWave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("insaneWave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

function insaneReinforcementSpawn()
{
	const SCAN_DISTANCE = 2;
	const DISTANCE_FROM_POS = 30;
	const units = [cTempl.comltath, cTempl.cohhvch, cTempl.comagh];
	const limits = {minimum: 4, maxRandom: 2};
	const location = camGenerateRandomMapEdgeCoordinate(getObject("startPosition"), CAM_GENERIC_WATER_STAT, DISTANCE_FROM_POS, SCAN_DISTANCE);
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function insaneTransporterAttack()
{
	const DISTANCE_FROM_POS = 30;
	const units = [cTempl.comhltat, cTempl.comhpv, cTempl.comagt];
	const limits = {minimum: 4, maxRandom: 3};
	const location = camGenerateRandomMapCoordinate(getObject("startPosition"), CAM_GENERIC_LAND_STAT, DISTANCE_FROM_POS);
	camSendGenericSpawn(CAM_REINFORCE_TRANSPORT, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function insaneRemoveExtraVtolPads()
{
	const objects = enumArea("insaneExtraVtolPadArea", CAM_THE_COLLECTIVE, false);

	for (let i = 0, len = objects.length; i < len; ++i)
	{
		const obj = objects[i];

		if (obj.type === STRUCTURE && obj.stattype === REARM_PAD)
		{
			camSafeRemoveObject(obj, false);
		}
	}
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, cam_levels.beta7.pre,{
		area: "RTLZ",
		message: "C25_LZ",
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

	if (camClassicMode())
	{
		camClassicResearch(mis_collectiveResClassic, CAM_THE_COLLECTIVE);

		camSetArtifacts({
			"NuclearReactor": { tech: "R-Struc-Power-Upgrade01" },
			"COMediumFactory": { tech: "R-Wpn-Cannon4AMk1" },
			"COCyborgFactoryL": { tech: "R-Wpn-MG4" },
		});
	}
	else
	{
		camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);

		if (difficulty >= MEDIUM)
		{
			camUpgradeOnMapTemplates(cTempl.commc, cTempl.commrp, CAM_THE_COLLECTIVE);
		}
		camUpgradeOnMapTemplates(cTempl.npcybf, cTempl.cocybth, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybc, cTempl.cocybsn, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybr, cTempl.cocybtk, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybm, cTempl.cocybag, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.colatv, cTempl.colhvat, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.comtath, cTempl.comltath, CAM_THE_COLLECTIVE);

		camSetArtifacts({
			"NuclearReactor": { tech: "R-Struc-Power-Upgrade01" },
			"COMediumFactory": { tech: "R-Wpn-Cannon-ROF02" },
			"COCyborgFactoryL": { tech: "R-Wpn-MG4" },
			"COTankKillerHardpoint": { tech: "R-Wpn-RocketSlow-ROF02" },
		});
	}

	if (!camAllowInsaneSpawns())
	{
		insaneRemoveExtraVtolPads();
	}

	camSetEnemyBases({
		"COEastBase": {
			cleanup: "baseCleanup",
			detectMsg: "C25_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"CODamBase": {
			cleanup: "damBaseCleanup",
			detectMsg: "C25_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"COMediumFactory": {
			assembly: "COMediumFactoryAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(55)),
			data: {
				regroup: false,
				repair: 20,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.commrp, cTempl.comhltat, cTempl.comhpv] : [cTempl.commc, cTempl.comatt, cTempl.comhpv]
		},
		"COCyborgFactoryL": {
			assembly: "COCyborgFactoryLAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(35)),
			data: {
				regroup: false,
				repair: 30,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.cocybag, cTempl.cocybth, cTempl.cocybtk] : [cTempl.cocybag, cTempl.npcybf, cTempl.npcybr]
		},
		"COCyborgFactoryR": {
			assembly: "COCyborgFactoryRAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(45)),
			data: {
				regroup: false,
				repair: 30,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.cocybtk, cTempl.cocybsn] : [cTempl.npcybr, cTempl.npcybc]
		},
	});

	hackAddMessage("C25_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);

	queue("setupDamHovers", camSecondsToMilliseconds(3));
	queue("setupCyborgsEast", camChangeOnDiff(camMinutesToMilliseconds(3)));
	queue("enableFactories", camChangeOnDiff(camMinutesToMilliseconds(8)));
	queue("setupCyborgsNorth", camChangeOnDiff(camMinutesToMilliseconds(10)));
}
