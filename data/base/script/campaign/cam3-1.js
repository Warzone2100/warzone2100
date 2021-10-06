include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEXUS_RES = [
	"R-Sys-Engineering03", "R-Defense-WallUpgrade07", "R-Struc-Materials07",
	"R-Struc-VTOLPad-Upgrade06", "R-Wpn-Bomb-Damage03", "R-Sys-NEXUSrepair",
	"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
	"R-Wpn-Mortar-Acc03", "R-Wpn-MG-Damage09", "R-Wpn-Mortar-ROF04",
	"R-Vehicle-Engine07", "R-Vehicle-Metals07", "R-Vehicle-Armor-Heat04",
	"R-Cyborg-Metals07", "R-Cyborg-Armor-Heat04", "R-Wpn-RocketSlow-ROF05",
	"R-Wpn-AAGun-Damage06", "R-Wpn-AAGun-ROF05", "R-Wpn-Howitzer-Damage09",
	"R-Wpn-Howitzer-ROF04", "R-Wpn-Cannon-Damage08", "R-Wpn-Cannon-ROF04",
	"R-Wpn-Missile-Damage01", "R-Wpn-Missile-ROF01", "R-Wpn-Missile-Accuracy01",
	"R-Wpn-Rail-Damage01", "R-Wpn-Rail-ROF01", "R-Wpn-Rail-Accuracy01",
	"R-Wpn-Energy-Damage02", "R-Wpn-Energy-ROF01", "R-Wpn-Energy-Accuracy01",
];
var launchInfo;
var detonateInfo;

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
		interval: camSecondsToMilliseconds(25),
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
		interval: camSecondsToMilliseconds(15),
		regroup: true,
		count: -1
		//morale: 25,
		//fallback: camMakePos("hillRetreat")
	});
});

//Setup Nexus VTOL hit and runners.
function vtolAttack()
{
	var list = [cTempl.nxlscouv, cTempl.nxmtherv];
	camSetVtolData(NEXUS, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(camMinutesToMilliseconds(5)), "NXCommandCenter");
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
	//Avoid destroying the one base if the player opted not to destroy it themselves.
	var nuked = enumArea(0, 0, mapWidth, mapHeight, ALL_PLAYERS, false).filter(function(obj) {
		return obj.type !== STRUCTURE || (obj.type === STRUCTURE && obj.group === null);
	});
	var safeZone = enumArea("valleySafeZone", CAM_HUMAN_PLAYER, false);
	var foundUnit = false;

	//Make em' explode!
	for (var i = 0, len = nuked.length; i < len; ++i)
	{
		var nukeIt = true;
		var obj1 = nuked[i];

		//Check if it's in the safe area.
		for (var j = 0, len2 = safeZone.length; j < len2; ++j)
		{
			var obj2 = safeZone[j];

			if (obj1.id === obj2.id)
			{
				if (obj1.type === DROID && obj1.player === CAM_HUMAN_PLAYER)
				{
					foundUnit = true;
				}

				nukeIt = false;
				break;
			}
		}

		if (nukeIt && obj1 !== null && obj1.id !== 0)
		{
			camSafeRemoveObject(obj1, true);
		}
	}

	return foundUnit; //Must have saved at least one unit to win.
}

//Expand the map and play video and prevent transporter reentry.
function setupNextMission()
{
	if (missileSilosDestroyed())
	{
		camSetExtraObjectiveMessage(_("Move all units into the valley"));

		camPlayVideos(["labort.ogg", {video: "MB3_1B_MSG", type: CAMP_MSG}, {video: "MB3_1B_MSG2", type: MISS_MSG}]);

		setScrollLimits(0, 0, 64, 64); //Reveal the whole map.
		setMissionTime(camChangeOnDiff(camMinutesToSeconds(30)));

		hackRemoveMessage("CM31_TAR_UPLINK", PROX_MSG, CAM_HUMAN_PLAYER);
		hackAddMessage("CM31_HIDE_LOC", PROX_MSG, CAM_HUMAN_PLAYER);

		setReinforcementTime(-1);
		removeTimer("setupNextMission");
	}
}


//Play countdown sounds. Elements are shifted out of the missile launch/detonation arrays as they play.
function getCountdown()
{
	var silosDestroyed = missileSilosDestroyed();
	var countdownObject = silosDestroyed ? detonateInfo : launchInfo;
	var skip = false;

	for (var i = 0, len = countdownObject.length; i < len; ++i)
	{
		var currentTime = getMissionTime();
		if (currentTime <= countdownObject[0].time)
		{
			if (len > 1 && (currentTime <= countdownObject[1].time))
			{
				skip = true; //Huge time jump?
			}
			if (!skip)
			{
				playSound(countdownObject[0].sound, CAM_HUMAN_PLAYER);
			}

			if (silosDestroyed)
			{
				detonateInfo.shift();
			}
			else
			{
				launchInfo.shift();
			}

			break;
		}
	}
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
	var safeZone = enumArea("valleySafeZone", CAM_HUMAN_PLAYER, false).filter(function(obj) {
		return obj.type === DROID;
	});
	var allDroids = enumArea(0, 0, mapWidth, mapHeight, CAM_HUMAN_PLAYER, false).filter(function(obj) {
		return obj.type === DROID;
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

function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Destroy the missile silos"));

	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	var tent = getObject("transporterEntry");
	var text = getObject("transporterExit");

	//Time is in seconds.
	launchInfo = [
		{sound: "60min.ogg", time: camMinutesToSeconds(60)},
		{sound: "50min.ogg", time: camMinutesToSeconds(50)},
		{sound: "40min.ogg", time: camMinutesToSeconds(40)},
		{sound: "30min.ogg", time: camMinutesToSeconds(30)},
		{sound: "20min.ogg", time: camMinutesToSeconds(20)},
		{sound: "10min.ogg", time: camMinutesToSeconds(10)},
		{sound: "meflp.ogg", time: camMinutesToSeconds(5) + 10},
		{sound: "5min.ogg", time: camMinutesToSeconds(5)},
		{sound: "4min.ogg", time: camMinutesToSeconds(4)},
		{sound: "3min.ogg", time: camMinutesToSeconds(3)},
		{sound: "2min.ogg", time: camMinutesToSeconds(2)},
		{sound: "1min.ogg", time: camMinutesToSeconds(1)},
		{sound: "flseq.ogg", time: 25},
		{sound: "10to1.ogg", time: 11},
		{sound: "mlaunch.ogg", time: 2},
	];
	detonateInfo = [
		{sound: "mlaunch.ogg", time: camMinutesToSeconds(60) - 9},
		{sound: "det60min.ogg", time: camMinutesToSeconds(60) - 10},
		{sound: "det50min.ogg", time: camMinutesToSeconds(50)},
		{sound: "det40min.ogg", time: camMinutesToSeconds(40)},
		{sound: "det30min.ogg", time: camMinutesToSeconds(30)},
		{sound: "det20min.ogg", time: camMinutesToSeconds(20)},
		{sound: "det10min.ogg", time: camMinutesToSeconds(10)},
		{sound: "det5min.ogg", time: camMinutesToSeconds(5)},
		{sound: "det4min.ogg", time: camMinutesToSeconds(4)},
		{sound: "det3min.ogg", time: camMinutesToSeconds(3)},
		{sound: "det2min.ogg", time: camMinutesToSeconds(2)},
		{sound: "det1min.ogg", time: camMinutesToSeconds(1)},
		{sound: "fdetseq.ogg", time: 20},
		{sound: "10to1.ogg", time: 10},
	];

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_3B", {
		area: "RTLZ",
		reinforcements: camMinutesToSeconds(3),
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

	camSetArtifacts({
		"NXMediumFac": { tech: "R-Wpn-MG-Damage09" },
	});

	camSetEnemyBases({
		"NX-SWBase": {
			cleanup: "baseCleanupArea",
			detectMsg: "CM31_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv394.ogg",
		},
	});

	camSetFactories({
		"NXCybFac1": {
			assembly: "NXCybFac1Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(30)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
		},
		"NXCybFac2": {
			assembly: "NXCybFac2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
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
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
			templates: [cTempl.nxmscouh, cTempl.nxmrailh]
		},
	});

	hackAddMessage("CM31_TAR_UPLINK", PROX_MSG, CAM_HUMAN_PLAYER);

	cyborgAttack();
	getCountdown();

	setTimer("getCountdown", camSecondsToMilliseconds(0.4));
	setTimer("setupNextMission", camSecondsToMilliseconds(2));
	queue("hoverAttack", camChangeOnDiff(camMinutesToMilliseconds(4)));
	queue("vtolAttack", camChangeOnDiff(camMinutesToMilliseconds(5)));
	queue("enableAllFactories", camChangeOnDiff(camMinutesToMilliseconds(5)));
}
