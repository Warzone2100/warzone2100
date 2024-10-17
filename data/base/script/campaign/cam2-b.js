include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_collectiveRes = [
	"R-Defense-WallUpgrade05", "R-Struc-Materials05", "R-Sys-Engineering02",
	"R-Vehicle-Engine04", "R-Vehicle-Metals04", "R-Cyborg-Metals04",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage05",
	"R-Wpn-Cannon-ROF01", "R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage06", "R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc01",
	"R-Wpn-Mortar-Damage04", "R-Wpn-Mortar-ROF01",
	"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage05",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage05", "R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF01"
];
const mis_collectiveResClassic = [
	"R-Defense-WallUpgrade03", "R-Struc-Materials04", "R-Struc-Factory-Upgrade04",
	"R-Struc-VTOLPad-Upgrade01", "R-Vehicle-Engine04", "R-Vehicle-Metals03",
	"R-Cyborg-Metals03", "R-Vehicle-Armor-Heat02", "R-Cyborg-Armor-Heat02",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage04", "R-Wpn-Cannon-ROF02",
	"R-Wpn-Flamer-Damage05", "R-Wpn-Flamer-ROF02", "R-Wpn-MG-Damage05",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage04",
	"R-Wpn-Mortar-ROF02", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage05",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage04",
	"R-Wpn-RocketSlow-ROF03", "R-Sys-Sensor-Upgrade01"
];

camAreaEvent("vtolRemoveZone", function(droid)
{
	if ((droid.player !== CAM_HUMAN_PLAYER) && camVtolCanDisappear(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	resetLabel("vtolRemoveZone", CAM_THE_COLLECTIVE);
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
	hackRemoveMessage("C2B_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);

	const droids = enumArea("base4Cleanup", CAM_THE_COLLECTIVE, false).filter((obj) => (
		obj.type === DROID && obj.group === null
	));

	camManageGroup(camMakeGroup(droids), CAM_ORDER_ATTACK, {
		count: -1,
		regroup: false,
		repair: 67
	});
}

function activateBase1Defenders()
{
	camManageGroup(camMakeGroup("NBaseGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("leftSideAmbushPos1"),
			camMakePos("leftSideAmbushPos2"),
			camMakePos("leftSideAmbushPos3"),
		],
		interval: camSecondsToMilliseconds(60),
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
		interval: camSecondsToMilliseconds(60),
		regroup: false,
	});
}

function ambushPlayer()
{
	camManageGroup(camMakeGroup("centralBaseGroup"), CAM_ORDER_ATTACK, {
		count: -1,
		regroup: false,
		repair: 67
	});
}

function wave2()
{
	const CONDITION = ((difficulty >= INSANE) ? CAM_REINFORCE_CONDITION_ARTIFACTS : "COCommandCenter");
	const list = [cTempl.colatv, cTempl.colatv];
	const ext = {limit: [4, 4], alternate: true, altIdx: 0};
	camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(5)), CONDITION, ext);
}

function wave3()
{
	const CONDITION = ((difficulty >= INSANE) ? CAM_REINFORCE_CONDITION_ARTIFACTS : "COCommandCenter");
	const list = [cTempl.colcbv, cTempl.colcbv];
	const ext = {limit: [4, 4], alternate: true, altIdx: 0};
	camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(5)), CONDITION, ext);
}

function vtolAttack()
{
	const CONDITION = ((difficulty >= INSANE) ? CAM_REINFORCE_CONDITION_ARTIFACTS : "COCommandCenter");
	if (camClassicMode())
	{
		const list = [cTempl.colcbv, cTempl.colatv];
		const ext = {limit: [4, 4], alternate: true, altIdx: 0};
		camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(5)), CONDITION, ext);
	}
	else
	{
		const list = [cTempl.colpbv, cTempl.colpbv];
		const ext = {limit: [4, 4], alternate: true, altIdx: 0};
		camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(5)), CONDITION, ext);
		queue("wave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("wave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

function insaneReinforcementSpawn()
{
	const units = [cTempl.comatt, cTempl.comit, cTempl.cohct];
	const limits = {minimum: 7, maxRandom: 3};
	const location = ["insaneSpawnPos1", "insaneSpawnPos2", "insaneSpawnPos3", "insaneSpawnPos4"];
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function insaneTransporterAttack()
{
	const DISTANCE_FROM_POS = 40; // Inaccessible SouthWestern areas are still within limits hence the big distance check.
	const units = {units: [cTempl.cohct, cTempl.commrl, cTempl.comorb], appended: cTempl.comsens};
	const limits = {minimum: 9, maxRandom: 0};
	const location = camGenerateRandomMapCoordinate(getObject("startPosition"), CAM_GENERIC_LAND_STAT, DISTANCE_FROM_POS);
	camSendGenericSpawn(CAM_REINFORCE_TRANSPORT, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function truckDefense()
{
	if (enumDroid(CAM_THE_COLLECTIVE, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	const list = ["CO-Tower-MG3", "CO-Tower-LtATRkt", "CO-WallTower-HvCan", "CO-Tower-LtATRkt"];
	camQueueBuilding(CAM_THE_COLLECTIVE, list[camRand(list.length)]);
}

function transferPower()
{
	//increase player power level and play sound
	setPower(playerPower(CAM_HUMAN_PLAYER) + 4000);
	playSound(cam_sounds.powerTransferred);
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, cam_levels.beta4.pre);

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone"); //player lz
	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	setMissionTime(camChangeOnDiff(camHoursToSeconds(2)));
	camPlayVideos([{video: "MB2_B_MSG", type: CAMP_MSG}, {video: "MB2_B_MSG2", type: MISS_MSG}]);

	if (camClassicMode())
	{
		camClassicResearch(mis_collectiveResClassic, CAM_THE_COLLECTIVE);

		camSetArtifacts({
			"COResearchLab": { tech: "R-Wpn-Flame2" },
			"COHeavyFac-b4": { tech: "R-Wpn-RocketSlow-ROF01" },
			"COHeavyFacL-b1": { tech: "R-Wpn-MG-ROF03" },
			"COCommandCenter": { tech: "R-Vehicle-Body06" }, //Panther
		});
	}
	else
	{
		camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);

		if (difficulty >= MEDIUM)
		{
			camUpgradeOnMapTemplates(cTempl.commc, cTempl.commrp, CAM_THE_COLLECTIVE);
		}
		// New HMG Tiger Tracks units in first attack group
		if (difficulty >= HARD)
		{
			addDroid(CAM_THE_COLLECTIVE, 92, 59, "Heavy Machinegun Tiger Tracks", tBody.tank.tiger, tProp.tank.tracks, "", "", tWeap.tank.heavyMachinegun);
			addDroid(CAM_THE_COLLECTIVE, 96, 59, "Heavy Machinegun Tiger Tracks", tBody.tank.tiger, tProp.tank.tracks, "", "", tWeap.tank.heavyMachinegun);
			addDroid(CAM_THE_COLLECTIVE, 97, 59, "Heavy Machinegun Tiger Tracks", tBody.tank.tiger, tProp.tank.tracks, "", "", tWeap.tank.heavyMachinegun);
		}
		camUpgradeOnMapTemplates(cTempl.npcybf, cTempl.cocybth, CAM_THE_COLLECTIVE);

		camSetArtifacts({
			"COResearchLab": { tech: ["R-Wpn-Flame2", "R-Defense-WallUpgrade05"] },
			"COHeavyFac-b4": { tech: "R-Wpn-RocketSlow-ROF01" },
			"COHeavyFacL-b1": { tech: "R-Wpn-MG-ROF03" },
			"COCommandCenter": { tech: "R-Vehicle-Body02" }, //Leopard
			"COCybFac-b4": { tech: "R-Wpn-Cannon-ROF01" },
			"COBombardPit": { tech: "R-Wpn-Mortar-Damage04" },
		});
	}

	camSetEnemyBases({
		"CONorthBase": {
			cleanup: "base1Cleanup",
			detectMsg: "C2B_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COCentralBase": {
			cleanup: "base2Cleanup",
			detectMsg: "C2B_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COMiddleBase": {
			cleanup: "base4Cleanup",
			detectMsg: "C2B_BASE4",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"COHeavyFacL-b1": {
			assembly: "COHeavyFacL-b1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(70)),
			data: {
				regroup: false,
				repair: 30,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.comatt, cTempl.cohct, cTempl.commrp] : [cTempl.comatt, cTempl.cohct, cTempl.commc]
		},
		"COHeavyFacR-b1": {
			assembly: "COHeavyFacR-b1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
			data: {
				regroup: false,
				repair: 30,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.comatt, cTempl.cohct, cTempl.commrp] : [cTempl.comatt, cTempl.cohct, cTempl.commc]
		},
		"COCybFacL-b2": {
			assembly: "COCybFacL-b2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(30)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.npcybc, cTempl.npcybr]
		},
		"COCybFacR-b2": {
			assembly: "COCybFacR-b2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.npcybc, cTempl.npcybr, cTempl.cocybth, cTempl.npcybm] : [cTempl.npcybc, cTempl.npcybr, cTempl.npcybf, cTempl.npcybm]
		},
		"COHeavyFac-b4": {
			assembly: "COHeavyFac-b4Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(50)),
			data: {
				regroup: false,
				repair: 30,
				count: -1,
			},
			templates: [cTempl.comatt, cTempl.comit]
		},
		"COCybFac-b4": {
			assembly: "COCybFac-b4Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: (!camClassicMode()) ? [cTempl.npcybc, cTempl.npcybr, cTempl.cocybth] : [cTempl.npcybc, cTempl.npcybr, cTempl.npcybf]
		},
	});

	camManageTrucks(CAM_THE_COLLECTIVE);
	hackAddMessage("C2B_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);

	camEnableFactory("COHeavyFac-b4");
	camEnableFactory("COCybFac-b4");

	queue("transferPower", camSecondsToMilliseconds(2));
	queue("ambushPlayer", camSecondsToMilliseconds(3));
	queue("vtolAttack", camChangeOnDiff(camMinutesToMilliseconds(4)));
	queue("activateBase1Defenders2", camChangeOnDiff(camMinutesToMilliseconds(20)));
	queue("activateBase1Defenders", camChangeOnDiff(camMinutesToMilliseconds(30)));
	setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(3)));
	if (difficulty >= INSANE)
	{
		setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(3.5));
		setTimer("insaneTransporterAttack", camMinutesToMilliseconds(4.5));
	}

	truckDefense();
}
