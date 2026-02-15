include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_collectiveRes = [
	"R-Defense-WallUpgrade05", "R-Struc-Materials06", "R-Sys-Engineering02",
	"R-Vehicle-Engine05", "R-Vehicle-Metals05", "R-Cyborg-Metals05",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage05", "R-Wpn-Cannon-ROF02",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage07",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc03", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF04", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage05",
	"R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF02", "R-Wpn-Howitzer-ROF02",
	"R-Wpn-Howitzer-Damage08", "R-Cyborg-Armor-Heat01", "R-Vehicle-Armor-Heat01",
	"R-Wpn-Bomb-Damage02", "R-Wpn-AAGun-Damage03", "R-Wpn-AAGun-ROF03",
	"R-Wpn-AAGun-Accuracy02", "R-Wpn-Howitzer-Accuracy01", "R-Struc-VTOLPad-Upgrade03",
];
const mis_collectiveResClassic = [
	"R-Defense-WallUpgrade05", "R-Struc-Materials05", "R-Struc-Factory-Upgrade05",
	"R-Struc-VTOLPad-Upgrade03", "R-Vehicle-Engine05", "R-Vehicle-Metals05",
	"R-Cyborg-Metals05", "R-Vehicle-Armor-Heat02", "R-Cyborg-Armor-Heat02",
	"R-Sys-Engineering02", "R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage05",
	"R-Wpn-Cannon-ROF03", "R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03",
	"R-Wpn-Howitzer-Accuracy02", "R-Wpn-Howitzer-Damage02", "R-Sys-Sensor-Upgrade01",
	"R-Wpn-MG-Damage07", "R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage06",
	"R-Wpn-RocketSlow-ROF03"
];
const mis_vtolAppearPositions = ["insaneVtolAppearPos1", "insaneVtolAppearPos2", "insaneVtolAppearPos3"];

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

camAreaEvent("vtolRemoveZone", function(droid)
{
	if ((droid.player !== CAM_HUMAN_PLAYER) && camVtolCanDisappear(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	resetLabel("vtolRemoveZone", CAM_THE_COLLECTIVE);
});

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

function insaneWave2()
{
	const list = [cTempl.colhvat, cTempl.colhvat];
	const ext = {limit: [2, 2], alternate: true, altIdx: 0};
	camSetVtolData(CAM_THE_COLLECTIVE, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(3.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
}

function insaneWave3()
{
	const list = [cTempl.commorv, cTempl.comhvcv];
	const ext = {limit: [2, 4], alternate: true, altIdx: 0};
	camSetVtolData(CAM_THE_COLLECTIVE, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(3.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
}

function insaneVtolAttack()
{
	if (camClassicMode())
	{
		const list = [cTempl.commorvt, cTempl.commorv, cTempl.colhvat];
		const ext = {limit: [5, 5, 4], alternate: true, altIdx: 0};
		camSetVtolData(CAM_THE_COLLECTIVE, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(3.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
	}
	else
	{
		const list = [cTempl.commorvt, cTempl.commorvt];
		const ext = {limit: [2, 2], alternate: true, altIdx: 0};
		camSetVtolData(CAM_THE_COLLECTIVE, mis_vtolAppearPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(3.5), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
		queue("insaneWave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("insaneWave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

function insaneReinforcementSpawn()
{
	const units = {units: [cTempl.comltath, cTempl.cohact, cTempl.comrotm, cTempl.comrotm], appended: cTempl.comsensh};
	const limits = {minimum: 6, maxRandom: 4};
	const location = ["insaneSouthEastSpawn", "insaneWestSpawn", "insaneNorthSpawn"];
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
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
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, cam_levels.beta9.pre, {
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

	if (camClassicMode())
	{
		camClassicResearch(mis_collectiveResClassic, CAM_THE_COLLECTIVE);

		camSetArtifacts({
			"COCyborgFactory-Arti": { tech: "R-Wpn-Rocket07-Tank-Killer" },
			"COCommandCenter": { tech: "R-Wpn-Mortar3" },
			"uplink": { tech: "R-Sys-VTOLCBS-Tower01" },
		});
	}
	else
	{
		camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);

		camUpgradeOnMapTemplates(cTempl.npcybf, cTempl.cocybth, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybc, cTempl.cocybsn, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybr, cTempl.cocybtk, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.npcybm, cTempl.cocybag, CAM_THE_COLLECTIVE);
		camUpgradeOnMapTemplates(cTempl.colagv, cTempl.colhvat, CAM_THE_COLLECTIVE);

		camSetArtifacts({
			"COCyborgFactory-Arti": { tech: "R-Wpn-Rocket07-Tank-Killer" },
			"COCommandCenter": { tech: "R-Wpn-Mortar-ROF04" },
			"uplink": { tech: "R-Sys-VTOLCBS-Tower01" },
			"COMediumFactory": { tech: "R-Wpn-Cannon4AMk1" },
			"COWhirlwindSite": { tech: "R-Wpn-AAGun04" },
		});
	}

	if (!camAllowInsaneSpawns())
	{
		insaneRemoveExtraVtolPads();
	}

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
			templates: (!camClassicMode()) ? [cTempl.cocybsn, cTempl.cocybth, cTempl.cocybag, cTempl.cocybtk] : [cTempl.npcybc, cTempl.npcybf, cTempl.cocybag, cTempl.npcybr]
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
			templates: (!camClassicMode()) ? [cTempl.cocybag, cTempl.cocybtk] : [cTempl.cocybag, cTempl.npcybr]
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
			templates: (!camClassicMode()) ? [cTempl.cocybsn, cTempl.cocybth] : [cTempl.npcybc, cTempl.npcybf]
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
	if (camAllowInsaneSpawns())
	{
		queue("insaneVtolAttack", camMinutesToMilliseconds(7));
		setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(4.5));
	}

	queue("northWestAttack", camChangeOnDiff(camMinutesToMilliseconds(3)));
	queue("mainBaseAttackGroup", camChangeOnDiff(camMinutesToMilliseconds((difficulty <= MEDIUM) ? 4 : 4.5)));
	queue("southEastAttack", camChangeOnDiff(camMinutesToMilliseconds((difficulty <= MEDIUM) ? 4.5 : 5)));
	queue("enableTimeBasedFactories", camChangeOnDiff(camMinutesToMilliseconds((difficulty <= MEDIUM) ? 5 : 6)));
}
