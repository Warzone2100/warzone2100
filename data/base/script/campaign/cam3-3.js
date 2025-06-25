include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");
include("script/campaign/transitionTech.js");

const mis_nexusRes = [
	"R-Sys-Engineering03", "R-Defense-WallUpgrade09", "R-Struc-Materials09",
	"R-Struc-VTOLPad-Upgrade06", "R-Wpn-Bomb-Damage03", "R-Sys-NEXUSrepair",
	"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
	"R-Wpn-Mortar-Acc03", "R-Wpn-MG-Damage10", "R-Wpn-Mortar-ROF04",
	"R-Vehicle-Engine08", "R-Vehicle-Metals09", "R-Vehicle-Armor-Heat06",
	"R-Cyborg-Metals09", "R-Cyborg-Armor-Heat06", "R-Wpn-RocketSlow-ROF06",
	"R-Wpn-AAGun-Damage06", "R-Wpn-AAGun-ROF06", "R-Wpn-Howitzer-Damage09",
	"R-Wpn-Howitzer-ROF04", "R-Wpn-Cannon-Damage09", "R-Wpn-Cannon-ROF06",
	"R-Wpn-Missile-Damage02", "R-Wpn-Missile-ROF02", "R-Wpn-Missile-Accuracy02",
	"R-Wpn-Rail-Damage02", "R-Wpn-Rail-ROF02", "R-Wpn-Rail-Accuracy01",
	"R-Wpn-Energy-Damage03", "R-Wpn-Energy-ROF03", "R-Wpn-Energy-Accuracy01",
	"R-Wpn-AAGun-Accuracy03", "R-Wpn-Howitzer-Accuracy03", "R-Sys-NEXUSsensor",
];
const mis_nexusResClassic = [
	"R-Defense-WallUpgrade09", "R-Struc-Materials09", "R-Struc-Factory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals07",
	"R-Cyborg-Metals08", "R-Vehicle-Armor-Heat05", "R-Cyborg-Armor-Heat05",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Damage03", "R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage03",
	"R-Wpn-Energy-ROF03", "R-Wpn-Missile-Accuracy01", "R-Wpn-Missile-Damage02",
	"R-Wpn-Rail-Damage02", "R-Wpn-Rail-ROF02", "R-Sys-Sensor-Upgrade01",
	"R-Sys-NEXUSrepair", "R-Wpn-Flamer-Damage06", "R-Sys-NEXUSsensor",
];
const mis_spawnPatrol = {
	first: ["spawnPos1", "spawnPos2", "spawnPos3"],
	second: ["spawnPos4", "spawnPos5", "spawnPos6", "spawnPos7"],
	third: ["spawnPos8", "spawnPos9", "spawnPos10"],
	fourth: ["spawnPos11", "spawnPos12", "spawnPos13", "spawnPos14"]
};
var currWave;
var loseFlag;
var stopWaves;

//Remove Nexus VTOL droids.
camAreaEvent("vtolRemoveZone", function(droid)
{
	if (droid.player !== CAM_HUMAN_PLAYER && camVtolCanDisappear(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	resetLabel("vtolRemoveZone", CAM_NEXUS);
});

camAreaEvent("nxEscapeTrigger", function(droid)
{
	if (droid.player === CAM_NEXUS && !isVTOL(droid) && droid.droidType !== DROID_CYBORG)
	{
		camSafeRemoveObject(droid, false);
		loseFlag = true;
	}
	resetLabel("nxEscapeTrigger", CAM_NEXUS);
});

// Discourage very early assaults on the base as it will likely be futile and cause an early loss.
function earlyAssault()
{
	const units = {units: [cTempl.nxmrailh, cTempl.nxmscouh], appended: cTempl.nxmsens};
	const limits = {minimum: 10, maxRandom: 5};
	const location = camMakePos("NE-PhantomFactory");
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_NEXUS, CAM_REINFORCE_CONDITION_NONE, location, units, limits.minimum, limits.maxRandom);
}

function camEnemyBaseDetected_nxCyborgBase()
{
	if (currWave > 0)
	{
		return;
	}

	enableCybFactory();
	setupConvoy();
	earlyAssault();
}

function enableCybFactory()
{
	camEnableFactory("nxCybFactory");
}

function removeBlip()
{
	hackRemoveMessage("C3-3_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
}

function setupConvoy()
{
	setTimer("phantomFactory", camMinutesToMilliseconds(2.8));
	if (camAllowInsaneSpawns())
	{
		setTimer("insaneTransporterAttack", camMinutesToMilliseconds(4.5));
	}
}

function sendCyborgBackup()
{
	const listNECyb = [cTempl.nxcylas, cTempl.nxcylas, cTempl.nxcyscou];
	const listNWCyb = [cTempl.nxcyscou, cTempl.nxcyscou, cTempl.nxcylas, cTempl.nxcyrail];
	sendEdgeMapDroids("NE-PhantomFactory", listNECyb, true);
	sendEdgeMapDroids("NW-PhantomFactory", listNWCyb, true);
}

function phantomFactory()
{
	if (stopWaves)
	{
		removeTimer("phantomFactory");
		return;
	}

	++currWave;
	playSound(cam_sounds.enemyEscaping); // Alert player that spawns are coming.
	hackAddMessage("C3-3_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);

	const CYB_WAVE_START = 8;
	const listNE = [cTempl.nxmrailh, cTempl.nxmscouh, cTempl.nxlflash, cTempl.nxmlinkh];
	const listNW = [cTempl.nxmrailh, cTempl.nxmrailh, cTempl.nxmscouh, cTempl.nxmscouh];
	sendEdgeMapDroids("NE-PhantomFactory", listNE, false);
	sendEdgeMapDroids("NW-PhantomFactory", listNW, false);
	if (currWave >= CYB_WAVE_START)
	{
		sendCyborgBackup();
	}

	queue("removeBlip", camSecondsToMilliseconds(10));
}

function sendEdgeMapDroids(location, list, cyborgWave)
{
	const DROID_COUNT = ((!cyborgWave) ? (Math.floor(currWave / 2) + 2) : Math.floor(currWave / 3));
	const droids = [];
	const positions = [];
	positions.push(mis_spawnPatrol.first[camRand(mis_spawnPatrol.first.length)]);
	positions.push(mis_spawnPatrol.second[camRand(mis_spawnPatrol.second.length)]);
	positions.push(mis_spawnPatrol.third[camRand(mis_spawnPatrol.third.length)]);
	positions.push(mis_spawnPatrol.fourth[camRand(mis_spawnPatrol.fourth.length)]);
	positions.push("spawnPosFinal");

	for (let i = 0; i < DROID_COUNT; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	camSendReinforcement(CAM_NEXUS, location, droids, CAM_REINFORCE_GROUND, {
		order: CAM_ORDER_PATROL,
		data: {
			pos: positions,
			patrolType: CAM_PATROL_CYCLE,
			interval: camSecondsToMilliseconds(90),
			regroup: false,
			count: -1,
		}
	});
}

function setupPatrolGroups()
{
	camManageGroup(camMakeGroup("centralCyborgGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("centralPatrolPos1"),
			camMakePos("centralPatrolPos2"),
			camMakePos("centralPatrolPos3"),
		],
		interval: camSecondsToMilliseconds(40),
		regroup: true,
		count: -1
	});

	camManageGroup(camMakeGroup("eastCyborgGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("eastPatrolPos1"),
			camMakePos("eastPatrolPos2"),
			camMakePos("eastPatrolPos3"),
		],
		interval: camSecondsToMilliseconds(40),
		regroup: true,
		count: -1
	});
}

function wave2()
{
	const LOC = ((camAllowInsaneSpawns()) ? undefined : "vtolAppearPos");
	const list = [cTempl.nxlscouv, cTempl.nxmpulsev];
	const ext = {limit: [2, 2], alternate: true, altIdx: 0};
	camSetVtolData(CAM_NEXUS, LOC, "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(4)), CAM_REINFORCE_CONDITION_BASES, ext);
}

function wave3()
{
	const LOC = ((camAllowInsaneSpawns()) ? undefined : "vtolAppearPos");
	const list = [cTempl.nxmpulsev, cTempl.nxmtherv];
	const ext = {limit: [2, 2], alternate: true, altIdx: 0};
	camSetVtolData(CAM_NEXUS, LOC, "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(4)), CAM_REINFORCE_CONDITION_BASES, ext);
}

//Setup Nexus VTOL hit and runners.
function vtolAttack()
{
	const LOC = ((camAllowInsaneSpawns()) ? undefined : "vtolAppearPos");
	if (camClassicMode())
	{
		const list = [cTempl.nxlscouv, cTempl.nxlscouv, cTempl.nxmheapv];
		const ext = {limit: [2, 2, 2], alternate: true, altIdx: 0};
		camSetVtolData(CAM_NEXUS, LOC, "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(4)), CAM_REINFORCE_CONDITION_BASES, ext);
	}
	else
	{
		const list = [cTempl.nxmheapv, cTempl.nxmheapv];
		const ext = {limit: [2, 2], alternate: true, altIdx: 0};
		camSetVtolData(CAM_NEXUS, LOC, "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(4)), CAM_REINFORCE_CONDITION_BASES, ext);
		queue("wave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("wave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

function insaneTransporterAttack()
{
	const DISTANCE_FROM_POS = 10;
	const units = [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas];
	const limits = {minimum: 5, maxRandom: 5};
	const location = camGenerateRandomMapCoordinate(getObject("startPosition"), CAM_GENERIC_LAND_STAT, DISTANCE_FROM_POS);
	camSendGenericSpawn(CAM_REINFORCE_TRANSPORT, CAM_NEXUS, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function defeatedWaves()
{
	if (stopWaves)
	{
		return true;
	}
	if (loseFlag)
	{
		return false;
	}
	if (camAllEnemyBasesEliminated())
	{
		const tanks = enumDroid(CAM_NEXUS).filter((obj) => (
			obj.player === CAM_NEXUS && !isVTOL(obj) && obj.droidType !== DROID_CYBORG
		)).length;

		if (!tanks.length)
		{
			stopWaves = true;
			return true; // You can only win once all tanks are gone.
		}
	}
}

function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Prevent any Nexus hover tanks from escaping the area"));

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, cam_levels.gamma6, {
		area: "RTLZ",
		message: "C33_LZ",
		reinforcements: camMinutesToSeconds(15),
		annihilate: true,
		retlz: true,
		callback: "defeatedWaves"
	});

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone");
	const tEnt = getObject("transporterEntry");
	const tExt = getObject("transporterExit");
	currWave = 0;
	loseFlag = false;
	stopWaves = false;

	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);

	if (camClassicMode())
	{
		camClassicResearch(mis_nexusResClassic, CAM_NEXUS);
	}
	else
	{
		camCompleteRequiredResearch(mis_nexusRes, CAM_NEXUS);
	}

	camSetEnemyBases({
		"nxCyborgBase": {
			cleanup: "nxNorthBase",
			detectMsg: "CM33_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"nxCybFactory": {
			assembly: "nxCybAssembly",
			order: CAM_ORDER_ATTACK,
			groupSize: 3,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(45)),
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			templates: [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas]
		},
	});

	hackAddMessage("C3-3_OBJ2", PROX_MSG, CAM_HUMAN_PLAYER);
	camPlayVideos([{video: "MB3_3_MSG2", type: MISS_MSG}]);

	queue("setupPatrolGroups", camSecondsToMilliseconds(2));
	queue("enableCybFactory", camMinutesToSeconds(3.5));
	queue("setupConvoy", camMinutesToMilliseconds(5.5));
	queue("vtolAttack", camMinutesToMilliseconds(7));
}
