include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const MIS_SILO_PLAYER = 1;
const mis_nexusRes = [
	"R-Sys-Engineering03", "R-Defense-WallUpgrade12", "R-Struc-Materials10",
	"R-Struc-VTOLPad-Upgrade06", "R-Wpn-Bomb-Damage03", "R-Sys-NEXUSrepair",
	"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
	"R-Wpn-Mortar-Acc03", "R-Wpn-MG-Damage10", "R-Wpn-Mortar-ROF04",
	"R-Vehicle-Engine09", "R-Vehicle-Metals10", "R-Vehicle-Armor-Heat07",
	"R-Cyborg-Metals10", "R-Cyborg-Armor-Heat07", "R-Wpn-RocketSlow-ROF06",
	"R-Wpn-AAGun-Damage06", "R-Wpn-AAGun-ROF06", "R-Wpn-Howitzer-Damage09",
	"R-Wpn-Howitzer-ROF04", "R-Wpn-Cannon-Damage09", "R-Wpn-Cannon-ROF06",
	"R-Wpn-Missile-Damage03", "R-Wpn-Missile-ROF03", "R-Wpn-Missile-Accuracy02",
	"R-Wpn-Rail-Damage03", "R-Wpn-Rail-ROF03", "R-Wpn-Rail-Accuracy01",
	"R-Wpn-Energy-Damage03", "R-Wpn-Energy-ROF03", "R-Wpn-Energy-Accuracy01",
	"R-Wpn-AAGun-Accuracy03", "R-Wpn-Howitzer-Accuracy03", "R-Sys-NEXUSsensor",
];
const mis_nexusResClassic = [
	"R-Defense-WallUpgrade09", "R-Struc-Materials09", "R-Struc-Factory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals08",
	"R-Cyborg-Metals08", "R-Vehicle-Armor-Heat06", "R-Cyborg-Armor-Heat06",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Damage03", "R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage03",
	"R-Wpn-Energy-ROF03", "R-Wpn-Missile-Accuracy01", "R-Wpn-Missile-Damage02",
	"R-Wpn-Rail-Accuracy01", "R-Wpn-Rail-Damage03", "R-Wpn-Rail-ROF03",
	"R-Sys-Sensor-Upgrade01", "R-Sys-NEXUSrepair", "R-Wpn-Flamer-Damage06",
	"R-Sys-NEXUSsensor",
];
const mis_vtolSpawnPositions = ["vtolSpawnPos1", "vtolSpawnPos2"];
var capturedSilos; // victory flag letting us know if we captured any silos.
var mapLimit; //LasSat slowly creeps toward missile silos.
var truckLocCounter;

camAreaEvent("NEDefenseZone", function(droid) {
	camEnableFactory("NXcyborgFac2Arti");

	camManageGroup(camMakeGroup("NEDefenseZone"), CAM_ORDER_DEFEND, {
		pos: [
			camMakePos("northMainEntrance"),
			camMakePos("samSiteHillMiddle"),
		],
		radius: 14,
		fallback: camMakePos("missileSiloRetreat"),
		morale: 90,
		regroup: true
	});
});

camAreaEvent("NWDefenseZone", function(droid) {
	camEnableFactory("NXbase2HeavyFac");

	camManageGroup(camMakeGroup("NWDefenseZone"), CAM_ORDER_DEFEND, {
		pos: [
			camMakePos("samSiteHillMiddle"),
			camMakePos("southOfGammaBase"),
		],
		radius: 14,
		fallback: camMakePos("missileSiloRetreat"),
		morale: 90,
		regroup: true
	});
});

//Remove Nexus VTOL droids.
camAreaEvent("vtolRemoveZone", function(droid)
{
	if (droid.player !== CAM_HUMAN_PLAYER && camVtolCanDisappear(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	resetLabel("vtolRemoveZone", CAM_NEXUS);
});

function insaneWave2()
{
	const list = [cTempl.nxlscouv, cTempl.nxlscouv];
	const ext = {limit: [3, 3], alternate: true, altIdx: 0};
	camSetVtolData(CAM_NEXUS, mis_vtolSpawnPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
}

function insaneWave3()
{
	const list = [cTempl.nxmtherv, cTempl.nxmtherv];
	const ext = {limit: [2, 2], alternate: true, altIdx: 0};
	camSetVtolData(CAM_NEXUS, mis_vtolSpawnPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
}

//Setup Nexus VTOL hit and runners. Choose a random spawn point for the VTOLs.
function insaneVtolAttack()
{
	if (camClassicMode())
	{
		const list = [cTempl.nxmtherv, cTempl.nxmheapv];
		const ext = {limit: [5, 5], alternate: true, altIdx: 0};
		camSetVtolData(CAM_NEXUS, mis_vtolSpawnPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
	}
	else
	{
		const list = [cTempl.nxmheapv, cTempl.nxmheapv];
		const ext = {limit: [2, 2], alternate: true, altIdx: 0};
		camSetVtolData(CAM_NEXUS, mis_vtolSpawnPositions, "vtolRemoveZone", list, camMinutesToMilliseconds(4), CAM_REINFORCE_CONDITION_ARTIFACTS, ext);
		queue("insaneWave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("insaneWave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

function setupGroups()
{
	camManageGroup(camMakeGroup("NXVtolBaseCleanup"), CAM_ORDER_ATTACK, {
		count: -1,
		regroup: false
	});

	camManageGroup(camMakeGroup("mainBaseCleanup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("missileSiloRetreat"),
			camMakePos("westMainEntrance"),
			camMakePos("vtolBaseEntrance"),
			camMakePos("northMainEntrance"),
		],
		interval: camSecondsToMilliseconds(45),
		regroup: false
	});

	camManageGroup(camMakeGroup("NXsuicideSquad"), CAM_ORDER_ATTACK, {
		regroup: false,
		count: -1
	});
}

//Activate all trigger factories if not done already.
function enableAllFactories()
{
	camEnableFactory("NXbase2HeavyFac");
	camEnableFactory("NXcyborgFac2Arti");
}

function truckDefense()
{
	if (enumDroid(CAM_NEXUS, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	const list = [
		"Emplacement-Howitzer150", "Emplacement-Howitzer150",
		"NX-Emp-MedArtMiss-Pit", "NX-Emp-MedArtMiss-Pit", "NX-Emp-MedArtMiss-Pit",
		"Sys-NX-CBTower"
	];
	let position;

	if (truckLocCounter === 0)
	{
		position = camMakePos("buildPos1");
		truckLocCounter += 1;
	}
	else
	{
		position = camMakePos("buildPos2");
		truckLocCounter = 0;
	}

	camQueueBuilding(CAM_NEXUS, list[camRand(list.length)], position);
}

function insaneReinforcementSpawn()
{
	const units = {units: [cTempl.nxmpulseh, cTempl.nxmscouh, cTempl.nxmrailh, cTempl.nxmangel], appended: cTempl.nxmsens};
	const limits = {minimum: 10, maxRandom: 5};
	const location = camMakePos("southSpawnPos");
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_NEXUS, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function startInsaneReinforcementAttack()
{
    setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(4));
}

function insaneTransporterAttack()
{
	const units = {units: cTempl.nxmangel, appended: cTempl.nxmsens};
	const limits = {minimum: 9, maxRandom: 0};
	const location = camMakePos("lzReinforcementPos");
	camSendGenericSpawn(CAM_REINFORCE_TRANSPORT, CAM_NEXUS, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

// Explode trucks to significantly reduce chances of gaming the lassat.
function destroyTrucksInBlastZone()
{
	const objects = enumArea(0, 0, mapWidth, Math.floor(mapLimit), CAM_HUMAN_PLAYER, false);
	for (let i = 0, len = objects.length; i < len; ++i)
	{
		const obj = objects[i];
		if (obj.type === DROID && obj.droidType === DROID_CONSTRUCT)
		{
			camSafeRemoveObject(obj, true);
		}
	}
}

//Choose a target to fire the LasSat at. Automatically increases the limits
//when no target is found in the area.
function vaporizeTarget()
{
	let target;
	const targets = enumArea(0, 0, mapWidth, Math.floor(mapLimit), CAM_HUMAN_PLAYER, false).filter((obj) => (
		(obj.type === DROID && obj.droidType !== DROID_CONSTRUCT) || obj.type === STRUCTURE
	));

	if (!targets.length)
	{
		//Choose random coordinate within the limits.
		target = {
			x: camRand(mapWidth),
			y: camRand(Math.floor(mapLimit)),
		};
	}
	else
	{
		const MIN_DROID_COST = 150;
		const dr = targets.filter((obj) => (obj.type === DROID && !isVTOL(obj) && (obj.cost >= MIN_DROID_COST || obj.experience > 0)));
		const vt = targets.filter((obj) => (obj.type === DROID && isVTOL(obj) && (obj.cost >= MIN_DROID_COST || obj.experience > 0)));
		const st = targets.filter((obj) => (obj.type === STRUCTURE && obj.stattype !== WALL && obj.status === BUILT));

		if (dr.length)
		{
			target = dr[0];
		}
		if (vt.length && (camRand(100) < 15))
		{
			target = vt[0]; //don't care about VTOLs as much
		}
		if (st.length && (!camDef(target) || !camRand(2))) //chance to focus on a structure
		{
			target = st[0];
		}
		// Choose something less specific if the above rules can't be satisfied.
		if (!camDef(target))
		{
			target = targets[camRand(targets.length)];
		}
	}

	//Droid or structure was destroyed before firing so pick a new one.
	if (!camDef(target))
	{
		queue("vaporizeTarget", camSecondsToMilliseconds(0.1));
		return;
	}
	if (Math.floor(mapLimit) < Math.floor(mapHeight / 2))
	{
		//total tiles = 256. 256 / 2 = 128 tiles. 128 / 60 = 2.13 tiles per minute.
		//2.13 / 60 = 0.0355 tiles per second. 0.0355 * 10 = ~0.36 tiles every 10 seconds.
		//This assumes an hour to completely cover the upper half of the home map.
		mapLimit += 0.36; //sector clear; move closer
	}
	laserSatFuzzyStrike(target);
}

//A simple way to fire the LasSat with a chance of missing.
function laserSatFuzzyStrike(obj)
{
	const loc = camMakePos(obj);
	//Initially lock onto target
	let xCoord = loc.x;
	let yCoord = loc.y;

	//Introduce some randomness
	if (camRand(101) < 67)
	{
		const X_RAND = camRand(3);
		const Y_RAND = camRand(3);
		xCoord = camRand(2) ? loc.x - X_RAND : loc.x + X_RAND;
		yCoord = camRand(2) ? loc.y - Y_RAND : loc.y + Y_RAND;
	}

	if (xCoord < 0)
	{
		xCoord = 0;
	}
	else if (xCoord > mapWidth)
	{
		xCoord = mapWidth;
	}

	if (yCoord < 0)
	{
		yCoord = 0;
	}
	else if (yCoord > Math.floor(mapLimit))
	{
		yCoord = Math.floor(mapLimit);
	}

	if (camRand(101) < 40)
	{
		playSound(cam_sounds.laserSatelliteFiring, xCoord, yCoord);
	}

	//Missed it so hit close to target.
	if (loc.x !== xCoord || loc.y !== yCoord || !camDef(obj.id))
	{
		fireWeaponAtLoc("LasSat", xCoord, yCoord, CAM_HUMAN_PLAYER);
	}
	else
	{
		//Hit it directly
		fireWeaponAtObj("LasSat", obj, CAM_HUMAN_PLAYER);
	}
}

//Donate the silos to the player. Allow capturedSilos victory flag to be true.
function allySiloWithPlayer()
{
	playSound(cam_sounds.objectiveCaptured);
	hackRemoveMessage("CM3D1_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
	camAbsorbPlayer(MIS_SILO_PLAYER, CAM_HUMAN_PLAYER);
	capturedSilos = true;
}

//Check if the silos still exist and only allow winning if the player captured them.
function checkMissileSilos()
{
	if (!countStruct("NX-ANTI-SATSite", CAM_HUMAN_PLAYER) && !countStruct("NX-ANTI-SATSite", MIS_SILO_PLAYER))
	{
		return false;
	}

	if (capturedSilos)
	{
		const Y_SCROLL_LIMIT = 140; // About the same number as the one in the Gamma 8 script.
		const safeToWinObjs = enumArea(0, Y_SCROLL_LIMIT, mapWidth, mapHeight, CAM_HUMAN_PLAYER, false).filter((obj) => (
			((obj.type === DROID && obj.droidType === DROID_CONSTRUCT) || (obj.type === STRUCTURE && obj.stattype === FACTORY && obj.status === BUILT))
		));

		if (safeToWinObjs.length > 0)
		{
			return true;
		}
	}

	const siloArea = camMakePos(getObject("missileSilos"));
	const safe = enumRange(siloArea.x, siloArea.y, 10, ALL_PLAYERS, false);
	const enemies = safe.filter((obj) => (obj.player === CAM_NEXUS));
	const player = safe.filter((obj) => (obj.player === CAM_HUMAN_PLAYER));
	if (!enemies.length && player.length)
	{
		camCallOnce("allySiloWithPlayer");
	}
}

function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Build a forward base at the silos"));

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone");
	const lz2 = getObject("landingZone2"); //LZ for cam3-4s.
	mapLimit = 1.0;

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, cam_levels.gamma8, {
		callback: "checkMissileSilos"
	});

	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	setNoGoArea(lz2.x, lz2.y, lz2.x2, lz2.y2, CAM_NEXUS);
	camSetMissionTimer(camChangeOnDiff(camHoursToSeconds(2)));

	if (camClassicMode())
	{
		camClassicResearch(mis_nexusResClassic, CAM_NEXUS);

		camSetArtifacts({
			"NXbase2HeavyFac": { tech: "R-Wpn-MdArtMissile" },
			"NXcommandCenter": { tech: "R-Wpn-Laser02" },
			"NXcyborgFac2Arti": { tech: "R-Wpn-RailGun02" },
		});
	}
	else
	{
		camCompleteRequiredResearch(mis_nexusRes, CAM_NEXUS);

		camSetArtifacts({
			"NXbase1VtolFacArti": { tech: "R-Wpn-MdArtMissile" },
			"NXcommandCenter": { tech: ["R-Wpn-Laser02", "R-Defense-WallUpgrade11"] },
			"NXcyborgFac2Arti": { tech: "R-Wpn-RailGun02" },
		});
	}

	setAlliance(CAM_HUMAN_PLAYER, MIS_SILO_PLAYER, true);
	setAlliance(CAM_NEXUS, MIS_SILO_PLAYER, true);

	camSetEnemyBases({
		"NXMainBase": {
			cleanup: "mainBaseCleanup",
			detectMsg: "CM3D1_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NXVtolBase": {
			cleanup: "NXVtolBaseCleanup",
			detectMsg: "CM3D1_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"NXbase1VtolFacArti": {
			assembly: "NxVtolAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			data: {
				regroup: false,
				repair: 67,
				count: -1,
			},
			templates: [cTempl.nxmheapv, cTempl.nxlscouv]
		},
		"NXbase2HeavyFac": {
			assembly: "NxHeavyAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
			data: {
				regroup: true,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.nxmrailh, cTempl.nxmscouh, cTempl.nxmpulseh, cTempl.nxmlinkh]
		},
		"NXcyborgFac1": {
			assembly: "NXcyborgFac1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(45)),
			data: {
				regroup: true,
				repair: 45,
				count: -1,
			},
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
		},
		"NXcyborgFac2Arti": {
			assembly: "NXcyborgFac2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(50)),
			data: {
				regroup: true,
				repair: 50,
				count: -1,
			},
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
		},
	});

	if (difficulty >= HARD)
	{
		addDroid(CAM_NEXUS, 15, 234, "Truck Retribution Hover", tBody.tank.retribution, tProp.tank.hover2, "", "", tConstruct.truck);
		if (camAllowInsaneSpawns())
		{
			addDroid(CAM_NEXUS, 8, 226, "Truck Retribution Hover", tBody.tank.retribution, tProp.tank.hover2, "", "", tConstruct.truck);
		}
		camManageTrucks(CAM_NEXUS, false);
		setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(2.5)));
	}

	camPlayVideos([{video: "MB3_AD1_MSG", type: CAMP_MSG}, {video: "MB3_AD1_MSG2", type: CAMP_MSG}, {video: "MB3_AD1_MSG3", type: MISS_MSG}]);
	hackAddMessage("CM3D1_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
	camEnableFactory("NXbase1VtolFacArti");
	camEnableFactory("NXcyborgFac1");
	truckLocCounter = 0;

	queue("vaporizeTarget", camSecondsToMilliseconds(2));
	queue("setupGroups", camSecondsToMilliseconds(5));
	queue("enableAllFactories", camChangeOnDiff(camMinutesToMilliseconds(5)));

	setTimer("destroyTrucksInBlastZone", camSecondsToMilliseconds(9));
	setTimer("vaporizeTarget", camSecondsToMilliseconds(10));
	if (camAllowInsaneSpawns())
	{
		queue("insaneVtolAttack", camMinutesToMilliseconds(3));
		queue("startInsaneReinforcementAttack", camMinutesToMilliseconds(1.5));
		setTimer("insaneTransporterAttack", camMinutesToMilliseconds(4));
	}
}
