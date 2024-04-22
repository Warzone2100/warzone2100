include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_nexusRes = [
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
	"R-Sys-NEXUSsensor",
];
const mis_nexusResClassic = [
	"R-Defense-WallUpgrade07", "R-Struc-Materials07", "R-Struc-Factory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals06",
	"R-Cyborg-Metals07", "R-Vehicle-Armor-Heat05", "R-Cyborg-Armor-Heat05",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Damage03", "R-Wpn-Missile-Damage01", "R-Wpn-Missile-ROF01",
	"R-Wpn-Rail-Damage01", "R-Wpn-Rail-ROF01", "R-Sys-Sensor-Upgrade01",
	"R-Sys-NEXUSrepair", "R-Wpn-Flamer-Damage06", "R-Sys-NEXUSsensor",
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

	resetLabel("vtolRemoveZone", CAM_NEXUS);
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

function wave2()
{
	const list = [cTempl.nxlscouv, cTempl.nxlscouv];
	const ext = {
		limit: [2, 2], //paired with list array
		alternate: true,
		altIdx: 0
	};
	camSetVtolData(CAM_NEXUS, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(camMinutesToMilliseconds(5)), "NXCommandCenter", ext);
}

function wave3()
{
	const list = [cTempl.nxlneedv, cTempl.nxlneedv];
	const ext = {
		limit: [3, 3], //paired with list array
		alternate: true,
		altIdx: 0
	};
	camSetVtolData(CAM_NEXUS, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(camMinutesToMilliseconds(5)), "NXCommandCenter", ext);
}

//Setup Nexus VTOL hit and runners.
function vtolAttack()
{
	if (camClassicMode())
	{
		const list = [cTempl.nxlscouv, cTempl.nxmtherv];
		camSetVtolData(CAM_NEXUS, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(camMinutesToMilliseconds(5)), "NXCommandCenter");
	}
	else
	{
		const list = [cTempl.nxmtherv, cTempl.nxmtherv];
		const ext = {
			limit: [2, 2], //paired with list array
			alternate: true,
			altIdx: 0
		};
		camSetVtolData(CAM_NEXUS, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(camMinutesToMilliseconds(5)), "NXCommandCenter", ext);
		queue("wave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("wave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
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
	let destroyed = 0;

	for (let i = 0; i < SILO_COUNT; ++i)
	{
		destroyed += (getObject(SILO_ALIAS + (i + 1)) === null) ? 1 : 0;
	}

	return destroyed === SILO_COUNT;
}

//Nuclear missile destroys everything not in safe zone.
function nukeAndCountSurvivors()
{
	// Remove the base in the rare event an auto-explosion triggers as we nuke the base here.
	camSetEnemyBases({});
	const nuked = enumArea(0, 0, mapWidth, mapHeight, ALL_PLAYERS, false);
	const safeZone = enumArea("valleySafeZone", CAM_HUMAN_PLAYER, false);
	let foundUnit = false;

	//Make em' explode!
	for (let i = 0, len = nuked.length; i < len; ++i)
	{
		let nukeIt = true;
		const obj1 = nuked[i];

		//Check if it's in the safe area.
		for (let j = 0, len2 = safeZone.length; j < len2; ++j)
		{
			const obj2 = safeZone[j];

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

		camPlayVideos([cam_sounds.missile.launch.missileLaunchAborted, {video: "MB3_1B_MSG", type: CAMP_MSG}, {video: "MB3_1B_MSG2", type: MISS_MSG}]);

		setScrollLimits(0, 0, 64, 64); //Reveal the whole map.
		setMissionTime(camChangeOnDiff(camMinutesToSeconds((tweakOptions.classicTimers) ? 25 : 30)));

		hackRemoveMessage("CM31_TAR_UPLINK", PROX_MSG, CAM_HUMAN_PLAYER);
		hackAddMessage("CM31_HIDE_LOC", PROX_MSG, CAM_HUMAN_PLAYER);

		setReinforcementTime(-1);
		removeTimer("setupNextMission");
	}
}

//Play countdown sounds. Elements are shifted out of the missile launch/detonation arrays as they play.
function getCountdown()
{
	const ACCEPTABLE_TIME_DIFF = 2;
	const SILOS_DESTROYED = missileSilosDestroyed();
	const countdownObject = SILOS_DESTROYED ? detonateInfo : launchInfo;
	let skip = false;

	for (let i = 0, len = countdownObject.length; i < len; ++i)
	{
		const CURRENT_TIME = getMissionTime();
		if (CURRENT_TIME <= countdownObject[0].time)
		{
			if (CURRENT_TIME < (countdownObject[0].time - ACCEPTABLE_TIME_DIFF))
			{
				skip = true; //Huge time jump?
			}
			if (!skip)
			{
				playSound(countdownObject[0].sound, CAM_HUMAN_PLAYER);
			}

			if (SILOS_DESTROYED)
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
	if (!camAllArtifactsPickedUp())
	{
		return;
	}

	const safeZone = enumArea("valleySafeZone", CAM_HUMAN_PLAYER, false).filter((obj) => (
		obj.type === DROID
	));
	const allDroids = enumArea(0, 0, mapWidth, mapHeight, CAM_HUMAN_PLAYER, false).filter((obj) => (
		obj.type === DROID
	));

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

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone");
	const tEnt = getObject("transporterEntry");
	const tExt = getObject("transporterExit");

	//Time is in seconds.
	launchInfo = [
		{sound: cam_sounds.missile.launch.missileLaunchIn60Minutes, time: camMinutesToSeconds(60)},
		{sound: cam_sounds.missile.launch.missileLaunchIn50Minutes, time: camMinutesToSeconds(50)},
		{sound: cam_sounds.missile.launch.missileLaunchIn40Minutes, time: camMinutesToSeconds(40)},
		{sound: cam_sounds.missile.launch.missileLaunchIn30Minutes, time: camMinutesToSeconds(30)},
		{sound: cam_sounds.missile.launch.missileLaunchIn20Minutes, time: camMinutesToSeconds(20)},
		{sound: cam_sounds.missile.launch.missileLaunchIn10Minutes, time: camMinutesToSeconds(10)},
		{sound: cam_sounds.missile.launch.missileEnteringFinalLaunchPeriod, time: camMinutesToSeconds(5) + 10},
		{sound: cam_sounds.missile.launch.missileLaunchIn5Minutes, time: camMinutesToSeconds(5)},
		{sound: cam_sounds.missile.launch.missileLaunchIn4Minutes, time: camMinutesToSeconds(4)},
		{sound: cam_sounds.missile.launch.missileLaunchIn3Minutes, time: camMinutesToSeconds(3)},
		{sound: cam_sounds.missile.launch.missileLaunchIn2Minutes, time: camMinutesToSeconds(2)},
		{sound: cam_sounds.missile.launch.missileLaunchIn1Minute, time: camMinutesToSeconds(1)},
		{sound: cam_sounds.missile.launch.finalMissileLaunchSequenceInitiated, time: 25},
		{sound: cam_sounds.missile.countdown, time: 11},
		{sound: cam_sounds.missile.launch.missileLaunched, time: 2},
	];
	detonateInfo = [
		{sound: cam_sounds.missile.detonate.warheadActivatedCountdownBegins, time: camMinutesToSeconds(60) - 9},
		{sound: cam_sounds.missile.detonate.detonationIn60Minutes, time: camMinutesToSeconds(60) - 10},
		{sound: cam_sounds.missile.detonate.detonationIn50Minutes, time: camMinutesToSeconds(50)},
		{sound: cam_sounds.missile.detonate.detonationIn40Minutes, time: camMinutesToSeconds(40)},
		{sound: cam_sounds.missile.detonate.detonationIn30Minutes, time: camMinutesToSeconds(30)},
		{sound: cam_sounds.missile.detonate.detonationIn20Minutes, time: camMinutesToSeconds(20)},
		{sound: cam_sounds.missile.detonate.detonationIn10Minutes, time: camMinutesToSeconds(10)},
		{sound: cam_sounds.missile.detonate.detonationIn5Minutes, time: camMinutesToSeconds(5)},
		{sound: cam_sounds.missile.detonate.detonationIn4Minutes, time: camMinutesToSeconds(4)},
		{sound: cam_sounds.missile.detonate.detonationIn3Minutes, time: camMinutesToSeconds(3)},
		{sound: cam_sounds.missile.detonate.detonationIn2Minutes, time: camMinutesToSeconds(2)},
		{sound: cam_sounds.missile.detonate.detonationIn1Minute, time: camMinutesToSeconds(1)},
		{sound: cam_sounds.missile.detonate.finalDetonationSequenceInitiated, time: 20},
		{sound: cam_sounds.missile.countdown, time: 10},
	];

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, "CAM_3B", {
		area: "RTLZ",
		reinforcements: camMinutesToSeconds(3),
		callback: "unitsInValley"
	});

	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);
	setScrollLimits(0, 32, 64, 64);

	if (camClassicMode())
	{
		camClassicResearch(mis_nexusResClassic, CAM_NEXUS);
	}
	else
	{
		camCompleteRequiredResearch(mis_nexusRes, CAM_NEXUS);

		camSetArtifacts({
			"NXMediumFac": { tech: "R-Wpn-Cannon-Damage08" },
			"NXCommandCenter": { tech: "R-Defense-WallUpgrade08" },
		});
	}

	camSetEnemyBases({
		"NX-SWBase": {
			cleanup: "baseCleanupArea",
			detectMsg: "CM31_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"NXCybFac1": {
			assembly: "NXCybFac1Assembly",
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
		"NXCybFac2": {
			assembly: "NXCybFac2Assembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(50)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
		},
		"NXMediumFac": {
			assembly: "NXMediumFacAssembly",
			order: CAM_ORDER_PATROL,
			data: {
				pos: [
					camMakePos("patrolPos1"),
					camMakePos("patrolPos2"),
					camMakePos("patrolPos3"),
					camMakePos("patrolPos4"),
					camMakePos("patrolPos5"),
					camMakePos("patrolPos6"),
					camMakePos("patrolPos7"),
					camMakePos("patrolPos8"),
				],
				interval: camSecondsToMilliseconds(30),
				regroup: false,
				repair: 45,
				count: -1,
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
