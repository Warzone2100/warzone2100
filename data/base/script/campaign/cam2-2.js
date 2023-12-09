include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_collectiveRes = [
	"R-Defense-WallUpgrade05", "R-Struc-Materials05", "R-Sys-Engineering02",
	"R-Vehicle-Engine04", "R-Vehicle-Metals04", "R-Cyborg-Metals04",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage05",
	"R-Wpn-Cannon-ROF01", "R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage06", "R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02",
	"R-Wpn-Mortar-Damage04", "R-Wpn-Mortar-ROF01",
	"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage05",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage05", "R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF01"
];
var commandGroup;

camAreaEvent("vtolRemoveZone", function(droid)
{
	if (isVTOL(droid) && (droid.weapons[0].armed < 20) || (droid.health < 60))
	{
		camSafeRemoveObject(droid, false);
	}

	resetLabel("vtolRemoveZone", CAM_THE_COLLECTIVE);
});

camAreaEvent("group1Trigger", function(droid)
{
	hackRemoveMessage("C22_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
	camEnableFactory("COFactoryEast");

	camManageGroup(commandGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("wayPoint1"),
		radius: 0,
		regroup: false,
	});
});

camAreaEvent("wayPoint1Rad", function(droid)
{
	if (isVTOL(droid))
	{
		resetLabel("wayPoint1Rad", CAM_THE_COLLECTIVE);
		return;
	}
	camManageGroup(commandGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("wayPoint3"),
		radius: 0,
		regroup: false,
	});
});

//Tell player that Collective Commander is leaving and group all droids
//that can attack together to defend the enemy commander.
camAreaEvent("wayPoint2Rad", function(droid)
{
	if (droid.droidType !== DROID_COMMAND)
	{
		resetLabel("wayPoint2Rad", CAM_THE_COLLECTIVE);
		return;
	}

	const point = getObject("wayPoint3");
	const defGroup = enumRange(point.x, point.y, 10, CAM_THE_COLLECTIVE, false).filter((obj) => (
		obj.droidType === DROID_WEAPON
	));

	camManageGroup(commandGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("wayPoint4"),
		radius: 0,
		regroup: false
	});

	camManageGroup(camMakeGroup(defGroup), CAM_ORDER_DEFEND, {
		pos: camMakePos("defensePos"),
		regroup: false,
		radius: 10,
		repair: 67,
	});

	playSound(cam_sounds.enemyEscaping);
});

camAreaEvent("failZone", function(droid)
{
	if (droid.droidType === DROID_COMMAND)
	{
		camSafeRemoveObject(droid, false);
		failSequence();
	}
	else
	{
		resetLabel("failZone", CAM_THE_COLLECTIVE);
	}
});

function wave2()
{
	const list = [cTempl.colatv, cTempl.colatv];
	const ext = {
		limit: [4, 4], //paired with list array
		alternate: true,
		altIdx: 0
	};
	camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPoint", "vtolRemovePoint", list, camChangeOnDiff(camMinutesToMilliseconds(5)), "COCommandCenter", ext);
}

function wave3()
{
	const list = [cTempl.colcbv, cTempl.colcbv];
	const ext = {
		limit: [4, 4], //paired with list array
		alternate: true,
		altIdx: 0
	};
	camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPoint", "vtolRemovePoint", list, camChangeOnDiff(camMinutesToMilliseconds(5)), "COCommandCenter", ext);
}

function vtolAttack()
{
	const list = [cTempl.colpbv, cTempl.colpbv];
	const ext = {
		limit: [4, 4], //paired with list array
		alternate: true,
		altIdx: 0
	};
	camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPoint", "vtolRemovePoint", list, camChangeOnDiff(camMinutesToMilliseconds(5)), "COCommandCenter", ext);
	queue("wave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
	queue("wave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
}

//Order the truck to build some defenses.
function truckDefense()
{
	if (enumDroid(CAM_THE_COLLECTIVE, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	const list = ["CO-Tower-LtATRkt", "PillBox1", "CO-WallTower-HvCan"];
	camQueueBuilding(CAM_THE_COLLECTIVE, list[camRand(list.length)]);
}

function showGameOver()
{
	const arti = camGetArtifacts();
	camSafeRemoveObject(arti[0], false);
	gameOverMessage(false);
}

function failSequence()
{
	queue("showGameOver", camSecondsToMilliseconds(0.3));
}

function retreatCommander()
{
	camManageGroup(commandGroup, CAM_ORDER_DEFEND, {
		pos: camMakePos("wayPoint3"),
		radius: 6,
		repair: 67,
		regroup: false
	});
}

//Make the enemy commander flee back to the NW base if attacked.
function eventAttacked(victim, attacker)
{
	if (camDef(victim) &&
		victim.player === CAM_THE_COLLECTIVE &&
		victim.y > Math.floor(mapHeight / 3) && //only if the commander is escaping to the south
		victim.group === commandGroup)
	{
		camCallOnce("retreatCommander");
	}
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_2C",{
		area: "RTLZ",
		message: "C22_LZ",
		reinforcements: camMinutesToSeconds(3),
		retlz: true
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
		"COCommander": { tech: "R-Wpn-RocketSlow-Accuracy03" },
	});

	camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);

	if (difficulty >= MEDIUM)
	{
		camUpgradeOnMapTemplates(cTempl.commc, cTempl.commrp, CAM_THE_COLLECTIVE);
	}

	camSetEnemyBases({
		"COEastBase": {
			cleanup: "eastBaseCleanup",
			detectMsg: "C22_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COWestBase": {
			cleanup: "westBaseCleanup",
			detectMsg: "C22_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"COFactoryEast": {
			assembly: camMakePos("eastAssembly"),
			order: CAM_ORDER_ATTACK,
			groupSize: 6,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(70)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.cohct, cTempl.comtathh, cTempl.comorb] //Heavy factory
		},
		"COFactoryWest": {
			assembly: camMakePos("westAssembly"),
			order: CAM_ORDER_DEFEND,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(80)),
			data: {
				pos: camMakePos("westAssembly"),
				regroup: false,
				repair: 67,
				radius: 18,
				count: -1,
			},
			templates: [cTempl.comtath] //Hover lancers
		},
	});

	commandGroup = camMakeGroup("group1NBase");
	camManageTrucks(CAM_THE_COLLECTIVE);
	camEnableFactory("COFactoryWest");

	hackAddMessage("C22_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);

	queue("vtolAttack", camMinutesToMilliseconds(3));
	setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(3)));
	truckDefense();
}
