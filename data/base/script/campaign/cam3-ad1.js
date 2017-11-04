include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const SILO_PLAYER = 1;
const LASSAT_FIRING = "pcv650.ogg"; // LASER SATELLITE FIRING!!!
const NEXUS_RES = [
	"R-Defense-WallUpgrade09", "R-Struc-Materials09", "R-Struc-Factory-Upgrade06",
	"R-Struc-Factory-Cyborg-Upgrade06", "R-Struc-VTOLFactory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals08",
	"R-Cyborg-Metals08", "R-Vehicle-Armor-Heat06", "R-Cyborg-Armor-Heat06",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Accuracy03", "R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage03",
	"R-Wpn-Energy-ROF03", "R-Wpn-Missile-Accuracy01", "R-Wpn-Missile-Damage02",
	"R-Wpn-Rail-Accuracy01", "R-Wpn-Rail-Damage03", "R-Wpn-Rail-ROF03",
	"R-Sys-Sensor-Upgrade01", "R-Sys-NEXUSrepair", "R-Wpn-Flamer-Damage06",
];
var capturedSilos; // victory flag letting us know if we captured any silos.
var mapLimit; //LasSat slowly creeps toward missile silos.
var retryTime; //Delay before next Lassat strike.

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

function setupPatrolGroups()
{
	camManageGroup(camMakeGroup("NXVtolBaseCleanup"), CAM_ORDER_DEFEND, {
		pos: [
			camMakePos("westMainEntrance"),
			camMakePos("vtolBaseEntrance"),
		],
		radius: 18,
		fallback: camMakePos("missileSiloRetreat"),
		morale: 90,
		regroup: false
	});

	camManageGroup(camMakeGroup("mainBaseCleanup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("missileSiloRetreat"),
			camMakePos("westMainEntrance"),
			camMakePos("vtolBaseEntrance"),
			camMakePos("northMainEntrance"),
		],
		interval: 45000,
		regroup: false
	});

	camManageGroup(camMakeGroup("NXsuicideSquad"), CAM_ORDER_ATTACK, {
		regroup: false,
		count: -1
	});
}

//Activate all trigger factories after 10 minutes if not done already.
function enableAllFactories()
{
	camEnableFactory("NXbase2HeavyFac");
	camEnableFactory("NXcyborgFac2Arti");
}

//Choose a target to fire the LasSat at. Automatically increases the limits
//when no target is found in the area.
function vaporizeTarget()
{
	var targets = enumArea(0, 0, mapWidth, mapLimit, CAM_HUMAN_PLAYER, false);
	var target;

	if (!targets.length)
	{
		const ONE_THIRD = Math.floor(mapHeight / 3);
		//Choose random coordinate within the limits.
		target = {
			"x": camRand(mapWidth),
			"y": camRand(mapLimit),
		};

		if (mapLimit < mapHeight)
		{
			mapLimit = mapLimit + 2; //sector clear; move closer.
		}

		if ((mapLimit >= ONE_THIRD) && (mapLimit < (2 * ONE_THIRD)))
		{
			retryTime = 10000; //Slow down cover cam1-a area.
		}
		else if (mapLimit >= (2 * ONE_THIRD))
		{
			retryTime = 20000; //Slow down even more on cam3-c area.
		}
	}
	else
	{
		target = camMakePos(targets[0]);
	}

	laserSatFuzzyStrike(target);
	queue("vaporizeTarget", retryTime);
}

//A simple way to fire the LasSat with a chance of missing.
function laserSatFuzzyStrike(obj)
{
	const LOC = camMakePos(obj);

	//Introduce some randomness
	var xRand = camRand(2);
	var yRand = camRand(2);
	var xCoord = camRand(2) ? LOC.x - xRand : LOC.x + xRand;
	var yCoord = camRand(2) ? LOC.y - yRand : LOC.y + yRand;

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
	else if (yCoord > mapLimit)
	{
		yCoord = mapLimit;
	}

	if (camRand(101) < 40)
	{
		playSound(LASSAT_FIRING, xCoord, yCoord);
	}
	fireWeaponAtLoc(xCoord, yCoord, "LasSat");
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
	if (!countStruct("NX-ANTI-SATSite", CAM_HUMAN_PLAYER)
		&& !countStruct("NX-ANTI-SATSite", SILO_PLAYER))
	{
		playSound("pcv622.ogg"); //Objective failed.
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
	var siloZone = getObject("missileSilos");
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	var lz2 = getObject("landingZone2");
	mapLimit = 1;
	retryTime = 5000;

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "CAM3A-D2", {
		callback: "checkMissileSilos"
	});

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x1, lz.y1, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	setNoGoArea(lz2.x1, lz2.y1, lz2.x2, lz2.y2, NEXUS); //LZ for cam3-4s.
	setNoGoArea(siloZone.x1, siloZone.y1, siloZone.x2, siloZone.y2, CAM_HUMAN_PLAYER);
	setMissionTime(7200); //2 hr

	setPower(AI_POWER, NEXUS);
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

	with (camTemplates) camSetFactories({
		"NXbase1VtolFacArti": {
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(40000),
			data: {
				regroup: false,
				repair: 67,
				count: -1,
			},
			templates: [nxmheapv, nxlscouv] //nxlpulsev, nxmtherv
		},
		"NXbase2HeavyFac": {
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(50000),
			data: {
				regroup: true,
				repair: 40,
				count: -1,
			},
			templates: [nxmrailh, nxmscouh, nxmpulseh, nxmlinkh] //nxmsamh
		},
		"NXcyborgFac1": {
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(30000),
			data: {
				regroup: true,
				repair: 45,
				count: -1,
			},
			templates: [nxcyrail, nxcyscou, nxcylas]
		},
		"NXcyborgFac2Arti": {
			order: CAM_ORDER_ATTACK,
			groupSize: 5,
			throttle: camChangeOnDiff(30000),
			data: {
				regroup: true,
				repair: 50,
				count: -1,
			},
			templates: [nxcyrail, nxcyscou, nxcylas]
		},
	});

	camPlayVideos(["MB3_AD1_MSG", "MB3_AD1_MSG2", "MB3_AD1_MSG3"]);
	hackAddMessage("CM3D1_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
	camEnableFactory("NXbase1VtolFacArti");
	camEnableFactory("NXcyborgFac1");

	queue("vaporizeTarget", 2000);
	queue("setupPatrolGroups", 10000); // 10 sec.
	queue("enableAllFactories", camChangeOnDiff(600000)); // 10 min.
}
