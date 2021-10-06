include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const SILO_PLAYER = 1;
const LASSAT_FIRING = "pcv650.ogg"; // LASER SATELLITE FIRING!!!
const NEXUS_RES = [
	"R-Sys-Engineering03", "R-Defense-WallUpgrade10", "R-Struc-Materials10",
	"R-Struc-VTOLPad-Upgrade06", "R-Wpn-Bomb-Damage03", "R-Sys-NEXUSrepair",
	"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
	"R-Wpn-Mortar-Acc03", "R-Wpn-MG-Damage09", "R-Wpn-Mortar-ROF04",
	"R-Vehicle-Engine09", "R-Vehicle-Metals10", "R-Vehicle-Armor-Heat07",
	"R-Cyborg-Metals10", "R-Cyborg-Armor-Heat07", "R-Wpn-RocketSlow-ROF06",
	"R-Wpn-AAGun-Damage06", "R-Wpn-AAGun-ROF06", "R-Wpn-Howitzer-Damage09",
	"R-Wpn-Howitzer-ROF04", "R-Wpn-Cannon-Damage09", "R-Wpn-Cannon-ROF06",
	"R-Wpn-Missile-Damage03", "R-Wpn-Missile-ROF03", "R-Wpn-Missile-Accuracy02",
	"R-Wpn-Rail-Damage03", "R-Wpn-Rail-ROF03", "R-Wpn-Rail-Accuracy01",
	"R-Wpn-Energy-Damage03", "R-Wpn-Energy-ROF03", "R-Wpn-Energy-Accuracy01",
	"R-Wpn-AAGun-Accuracy03", "R-Wpn-Howitzer-Accuracy03",
];
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
	if (enumDroid(NEXUS, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	var list = ["Emplacement-Howitzer150", "Emplacement-MdART-pit"];
	var position;

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

	camQueueBuilding(NEXUS, list[camRand(list.length)], position);
}

//Choose a target to fire the LasSat at. Automatically increases the limits
//when no target is found in the area.
function vaporizeTarget()
{
	var target;
	var targets = enumArea(0, 0, mapWidth, Math.floor(mapLimit), CAM_HUMAN_PLAYER, false).filter(function(obj) {
		return obj.type === DROID || obj.type === STRUCTURE;
	});

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
		var dr = targets.filter(function(obj) { return obj.type === DROID && !isVTOL(obj); });
		var vt = targets.filter(function(obj) { return obj.type === DROID && isVTOL(obj); });
		var st = targets.filter(function(obj) { return obj.type === STRUCTURE; });

		if (dr.length)
		{
			target = dr[0];
		}
		if (vt.length && (camRand(100) < 15))
		{
			target = vt[0]; //don't care about VTOLs as much
		}
		if (st.length && !camRand(2)) //chance to focus on a structure
		{
			target = st[0];
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
		mapLimit = mapLimit + 0.36; //sector clear; move closer
	}
	laserSatFuzzyStrike(target);
}

//A simple way to fire the LasSat with a chance of missing.
function laserSatFuzzyStrike(obj)
{
	const LOC = camMakePos(obj);
	//Initially lock onto target
	var xCoord = LOC.x;
	var yCoord = LOC.y;

	//Introduce some randomness
	if (camRand(101) < 67)
	{
		var xRand = camRand(3);
		var yRand = camRand(3);
		xCoord = camRand(2) ? LOC.x - xRand : LOC.x + xRand;
		yCoord = camRand(2) ? LOC.y - yRand : LOC.y + yRand;
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
		playSound(LASSAT_FIRING, xCoord, yCoord);
	}

	//Missed it so hit close to target.
	if (LOC.x !== xCoord || LOC.y !== yCoord || !camDef(obj.id))
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
	playSound("pcv621.ogg"); //Objective captured
	hackRemoveMessage("CM3D1_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
	camAbsorbPlayer(SILO_PLAYER, CAM_HUMAN_PLAYER);
	capturedSilos = true;
}

//Check if the silos still exist and only allow winning if the player captured them.
function checkMissileSilos()
{
	if (!countStruct("NX-ANTI-SATSite", CAM_HUMAN_PLAYER) && !countStruct("NX-ANTI-SATSite", SILO_PLAYER))
	{
		return false;
	}

	if (capturedSilos)
	{
		return true;
	}

	var siloArea = camMakePos(getObject("missileSilos"));
	var safe = enumRange(siloArea.x, siloArea.y, 10, ALL_PLAYERS, false);
	var enemies = safe.filter(function(obj) { return obj.player === NEXUS; });
	var player = safe.filter(function(obj) { return obj.player === CAM_HUMAN_PLAYER; });
	if (!enemies.length && player.length)
	{
		camCallOnce("allySiloWithPlayer");
	}
}

function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Secure a missile silo"));

	var siloZone = getObject("missileSilos");
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	var lz2 = getObject("landingZone2");
	mapLimit = 1.0;

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "CAM3A-D2", {
		callback: "checkMissileSilos"
	});

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	setNoGoArea(lz2.x, lz2.y, lz2.x2, lz2.y2, NEXUS); //LZ for cam3-4s.
	setNoGoArea(siloZone.x, siloZone.y, siloZone.x2, siloZone.y2, SILO_PLAYER);
	setMissionTime(camChangeOnDiff(camHoursToSeconds(2)));

	var enemyLz = getObject("NXlandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, NEXUS);

	camCompleteRequiredResearch(NEXUS_RES, NEXUS);

	setAlliance(CAM_HUMAN_PLAYER, SILO_PLAYER, true);
	setAlliance(NEXUS, SILO_PLAYER, true);

	camSetArtifacts({
		"NXbase1VtolFacArti": { tech: "R-Wpn-MdArtMissile" },
		"NXcommandCenter": { tech: "R-Wpn-Laser02" },
		"NXcyborgFac2Arti": { tech: "R-Wpn-RailGun02" },
	});

	camSetEnemyBases({
		"NXMainBase": {
			cleanup: "mainBaseCleanup",
			detectMsg: "CM3D1_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
		"NXVtolBase": {
			cleanup: "NXVtolBaseCleanup",
			detectMsg: "CM3D1_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds(50)),
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds(35)),
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
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
		addDroid(NEXUS, 15, 234, "Truck Retribution Hover", "Body7ABT", "hover02", "", "", "Spade1Mk1");

		camManageTrucks(NEXUS);

		setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(4.5)));
	}

	camPlayVideos([{video: "MB3_AD1_MSG", type: CAMP_MSG}, {video: "MB3_AD1_MSG2", type: CAMP_MSG}, {video: "MB3_AD1_MSG3", type: MISS_MSG}]);
	hackAddMessage("CM3D1_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
	camEnableFactory("NXbase1VtolFacArti");
	camEnableFactory("NXcyborgFac1");
	truckLocCounter = 0;

	queue("vaporizeTarget", camSecondsToMilliseconds(2));
	queue("setupGroups", camSecondsToMilliseconds(5));
	queue("enableAllFactories", camChangeOnDiff(camMinutesToMilliseconds(5)));

	setTimer("vaporizeTarget", camSecondsToMilliseconds(10));
}
