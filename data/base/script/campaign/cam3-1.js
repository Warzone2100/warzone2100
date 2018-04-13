include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEXUS_RES = [
	"R-Sys-Engineering03", "R-Defense-WallUpgrade07",
	"R-Struc-Materials07", "R-Struc-Factory-Upgrade06",
	"R-Struc-Factory-Cyborg-Upgrade06", "R-Struc-VTOLFactory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals06",
	"R-Cyborg-Metals07", "R-Vehicle-Armor-Heat05", "R-Cyborg-Armor-Heat05",
	"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
	"R-Wpn-Bomb-Accuracy03", "R-Wpn-Missile-Damage01", "R-Wpn-Missile-ROF01",
	"R-Sys-Sensor-Upgrade01", "R-Sys-NEXUSrepair", "R-Wpn-Rail-Damage01",
	"R-Wpn-Rail-ROF01", "R-Wpn-Rail-Accuracy01", "R-Wpn-Flamer-Damage06",
];
var launchTimes;
var detTimes;
var launchSounds;
var detSounds;

//Remove Nexus VTOL droids.
camAreaEvent("vtolRemoveZone", function(droid)
{
	if (droid.player !== CAM_HUMAN_PLAYER)
	{
		if (isVTOL(droid))
		{
			camSafeRemoveObject(droid, false);
		}
	}

	resetLabel("vtolRemoveZone", NEXUS);
});

camAreaEvent("hillTriggerZone", function(droid)
{
	camManageGroup(camMakeGroup("hillGroupHovers"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("hillPos1"),
			camMakePos("hillPos2"),
			camMakePos("hillPos3"),
		],
		interval: 25000,
		regroup: true,
		count: -1
		//morale: 25,
		//fallback: camMakePos("hillRetreat")
	});

	camManageGroup(camMakeGroup("hillGroupCyborgs"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("hillPos1"),
			camMakePos("hillPos2"),
			camMakePos("hillPos3"),
		],
		interval: 15000,
		regroup: true,
		count: -1
		//morale: 25,
		//fallback: camMakePos("hillRetreat")
	});
});

//Setup Nexus VTOL hit and runners.
function vtolAttack()
{
	var list; with (camTemplates) list = [nxlscouv, nxmtherv];
	camSetVtolData(NEXUS, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(300000), "NXCommandCenter"); //5 min
}

//These groups are active immediately.
function cyborgAttack()
{
	camManageGroup(camMakeGroup("lzAttackCyborgs"), CAM_ORDER_ATTACK, {
		pos: [
			camMakePos("swRetreat"),
			camMakePos("hillPos1"),
			camMakePos("hillPos2"),
			camMakePos("hillPos3"),
		],
		regroup: true,
		count: -1,
		morale: 90,
		fallback: camMakePos("swRetreat")
	});
}

function hoverAttack()
{
	camManageGroup(camMakeGroup("lzAttackHovers"), CAM_ORDER_ATTACK, {
		pos: [
			camMakePos("swRetreat"),
			camMakePos("hillPos1"),
			camMakePos("hillPos2"),
			camMakePos("hillPos3"),
		],
		regroup: true,
		count: -1,
		morale: 90,
		fallback: camMakePos("swRetreat")
	});
}

//Setup next mission part if all missile silos are destroyed (setupNextMission()).
function missileSilosDestroyed()
{
	const SILO_COUNT = 4;
	const SILO_ALIAS = "NXMissileSilo";
	var destroyed = 0;

	for (var i = 0; i < SILO_COUNT; ++i)
	{
		destroyed += (getObject(SILO_ALIAS + (i + 1)) === null) ? 1 : 0;
	}

	return destroyed === SILO_COUNT;
}

//Nuclear missile destroys everything not in safe zone.
function nukeAndCountSurvivors()
{
	var nuked = enumArea(0, 0, mapWidth, mapHeight, ALL_PLAYERS, false);
	var safeZone = enumArea("valleySafeZone", CAM_HUMAN_PLAYER, false);
	var safeLen = safeZone.length;

	//Make em' explode!
	for (var i = 0, t = nuked.length; i < t; ++i)
	{
		var nukeIt = true;
		for (var s = 0; s < safeLen; ++s)
		{
			if (nuked[i].id === safeZone[s].id)
			{
				nukeIt = false;
				break;
			}
		}

		if (nukeIt && camDef(nuked[i]) && (nuked[i].id !== 0))
		{
			camSafeRemoveObject(nuked[i], true);
		}
	}

	return safeLen; //Must be at least 1 to win.
}

//Expand the map and play video and prevent transporter reentry.
function setupNextMission()
{
	if (missileSilosDestroyed())
	{
		camPlayVideos(["labort.ogg", "MB3_1B_MSG", "MB3_1B_MSG2"]);

		setScrollLimits(0, 0, 64, 64); //Reveal the whole map.
		setMissionTime(camChangeOnDiff(1800)); // 30 min

		hackRemoveMessage("CM31_TAR_UPLINK", PROX_MSG, CAM_HUMAN_PLAYER);
		hackAddMessage("CM31_HIDE_LOC", PROX_MSG, CAM_HUMAN_PLAYER);

		setReinforcementTime(-1);
	}
	else
	{
		queue("setupNextMission", 2000);
	}
}


//Play countdown sounds. Elements are shifted out of the sound arrays as they play.
function getCountdown()
{
	var missilesDead = missileSilosDestroyed();
	var times = missilesDead ? detTimes : launchTimes;
	var sounds = missilesDead ? detSounds : launchSounds;
	var skip = false;

	for (var i = 0, t = times.length; i < t; ++i)
	{
		var currentTime = getMissionTime();
		if (currentTime <= times[0])
		{
			if (camDef(times[1]) && (currentTime <= times[1]))
			{
				skip = true; //Huge time jump?
			}

			if (!skip)
			{
				playSound(sounds[0], CAM_HUMAN_PLAYER);
			}

			if (missilesDead)
			{
				detTimes.shift();
				detSounds.shift();
			}
			else
			{
				launchSounds.shift();
				launchTimes.shift();
			}

			break;
		}
	}

	queue("getCountdown", 120);
}

function enableAllFactories()
{
	camEnableFactory("NXCybFac1");
	camEnableFactory("NXCybFac2");
	camEnableFactory("NXMediumFac");
}

//For now just make sure we have all the droids in the canyon.
function unitsInValley()
{
	var safeZone = enumArea("valleySafeZone", CAM_HUMAN_PLAYER, false);
	var allDroids = enumArea(0, 0, mapWidth, mapHeight, CAM_HUMAN_PLAYER, false).filter(function(obj) {
		return (obj.type === DROID);
	});

	if (safeZone.length === allDroids.length)
	{
		if (nukeAndCountSurvivors())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

function enableReinforcements()
{
	const REINFORCEMENT_TIME = 180; //3 minutes.
	playSound("pcv440.ogg"); // Reinforcements are available.
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_3B", {
		area: "RTLZ",
		reinforcements: REINFORCEMENT_TIME,
		callback: "unitsInValley"
	});
}

function eventStartLevel()
{
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	var tent = getObject("transporterEntry");
	var text = getObject("transporterExit");

	launchTimes = [
		3600, 3000, 2400, 1800, 1200, 600, 310, 300,
		240, 180, 120, 60, 25, 11, 2,
	];
	detTimes = [
		3591, 3590, 3000, 2400, 1800, 1200, 600, 300,
		240, 180, 120, 60, 20, 10,
	];
	launchSounds = [
		"60min.ogg", "50min.ogg", "40min.ogg", "30min.ogg", "20min.ogg",
		"10min.ogg", "meflp.ogg", "5min.ogg", "4min.ogg", "3min.ogg",
		"2min.ogg", "1min.ogg", "flseq.ogg", "10to1.ogg", "mlaunch.ogg",
	];
	detSounds = [
		"mlaunch.ogg", "det60min.ogg", "det50min.ogg", "det40min.ogg",
		"det30min.ogg", "det20min.ogg", "det10min.ogg", "det5min.ogg",
		"det4min.ogg", "det3min.ogg", "det2min.ogg", "det1min.ogg",
		"fdetseq.ogg", "10to1.ogg",
	];

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_3B", {
		area: "RTLZ",
		reinforcements: -1,
		callback: "unitsInValley"
	});

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);
	setScrollLimits(0, 32, 64, 64);

	var enemyLz = getObject("NXlandingZone");
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, NEXUS);

	camCompleteRequiredResearch(NEXUS_RES, NEXUS);

	camSetEnemyBases({
		"NX-SWBase": {
			cleanup: "baseCleanupArea",
			detectMsg: "CM31_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
	});

	with (camTemplates) camSetFactories({
		"NXCybFac1": {
			assembly: "NXCybFac1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(30000),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [nxcyrail, nxcyscou, nxcylas]
		},
		"NXCybFac2": {
			assembly: "NXCybFac2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(40000),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [nxcyrail, nxcyscou, nxcylas]
		},
		"NXMediumFac": {
			assembly: "NXMediumFacAssembly",
			order: CAM_ORDER_DEFEND,
			data: {
				pos: [
					camMakePos("defenderPos1"),
					camMakePos("defenderPos2"),
					camMakePos("defenderPos3"),
				],
				regroup: false,
				repair: 45,
				count: -1,
				radius: 15,
			},
			group: camMakeGroup("baseDefenderGroup"),
			groupSize: 5,
			throttle: camChangeOnDiff(60000),
			templates: [nxmscouh, nxmrailh]
		},
	});

	hackAddMessage("CM31_TAR_UPLINK", PROX_MSG, CAM_HUMAN_PLAYER);

	cyborgAttack();
	getCountdown();

	queue("setupNextMission", 8000);
	queue("enableReinforcements", 15000);
	queue("hoverAttack", camChangeOnDiff(240000)); // 4 min
	queue("vtolAttack", camChangeOnDiff(300000)); //5 min
	queue("enableAllFactories", camChangeOnDiff(300000)); //5 min
}
