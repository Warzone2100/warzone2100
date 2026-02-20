include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const MIS_Y_SCROLL_LIMIT = 137;
const MIS_UNIT_LIMIT_FOR_SPAWN = 40;
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
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals09",
	"R-Cyborg-Metals09", "R-Vehicle-Armor-Heat06", "R-Cyborg-Armor-Heat06",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Damage03", "R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage03",
	"R-Wpn-Energy-ROF03", "R-Wpn-Missile-Accuracy01", "R-Wpn-Missile-Damage02",
	"R-Wpn-Rail-Accuracy01", "R-Wpn-Rail-Damage03", "R-Wpn-Rail-ROF03",
	"R-Sys-Sensor-Upgrade01", "R-Sys-NEXUSrepair", "R-Wpn-Flamer-Damage06",
	"R-Sys-NEXUSsensor",
];
const mis_vtolPositions = [
	"vtolAppearPosW", "vtolAppearPosE",
];
const mis_researchTargets = {
	missileCode1: "R-Comp-MissileCodes01",
	missileCode2: "R-Comp-MissileCodes02",
	missileCode3: "R-Comp-MissileCodes03",
	resistance: "R-Sys-Resistance"
};
var winFlag;
var mapLimit;
var videoInfo; //holds some info about when to play a video.

//Remove Nexus VTOL droids.
camAreaEvent("vtolRemoveZone", function(droid)
{
	if (droid.player !== CAM_HUMAN_PLAYER && camVtolCanDisappear(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	resetLabel("vtolRemoveZone", CAM_NEXUS);
});

function wave2()
{
	const list = [cTempl.nxlpulsev, cTempl.nxlpulsev];
	const ext = {limit: [4, 4], alternate: true, altIdx: 0};
	camSetVtolData(CAM_NEXUS, mis_vtolPositions, "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(3)), undefined, ext);
}

function wave3()
{
	const list = [cTempl.nxlpulsev, cTempl.nxmheapv];
	const ext = {limit: [4, 4], alternate: true, altIdx: 0};
	camSetVtolData(CAM_NEXUS, mis_vtolPositions, "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(3)), undefined, ext);
}

//Setup Nexus VTOL hit and runners. Choose a random spawn point for the VTOLs.
function vtolAttack()
{
	if (camClassicMode())
	{
		const list = [cTempl.nxlpulsev, cTempl.nxmheapv, cTempl.nxmheapv, cTempl.nxlpulsev];
		const ext = {limit: [2, 5, 5, 2], alternate: true, altIdx: 0};
		camSetVtolData(CAM_NEXUS, mis_vtolPositions, "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(3)), undefined, ext);
	}
	else
	{
		const list = [cTempl.nxmheapv, cTempl.nxmtherv];
		const ext = {limit: [4, 4], alternate: true, altIdx: 0};
		camSetVtolData(CAM_NEXUS, mis_vtolPositions, "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(3)), undefined, ext);
		queue("wave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("wave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

//Chose a random spawn point to send ground reinforcements.
function phantomFactorySpawn()
{
	if (winFlag)
	{
		return;
	}

	let units;
	let location;
	const extraUnits = [cTempl.nxmsens, cTempl.nxmsens, cTempl.nxmsamh, cTempl.nxmsamh];

	switch (camRand(3))
	{
		case 0:
			units = {units: [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmlinkh], appended: extraUnits};
			location = "phantomFacWest";
			break;
		case 1:
			units = {units: [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmlinkh], appended: extraUnits};
			location = "phantomFacEast";
			break;
		case 2:
			units = {units: [cTempl.nxcylas, cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmlinkh], appended: extraUnits};
			location = "phantomFacMiddle";
			break;
		default:
			units = {units: [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmlinkh], appended: extraUnits};
			location = "phantomFacWest";
	}
	if ((difficulty >= INSANE) || camAllowInsaneSpawns())
	{
		units.units.push(cTempl.nxmangel); //Insane adds Angel units as a possibility.
	}

	if (countDroid(DROID_ANY, CAM_NEXUS) < MIS_UNIT_LIMIT_FOR_SPAWN)
	{
		const limits = {minimum: 12, maxRandom: 3};
		camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_NEXUS, CAM_REINFORCE_CONDITION_NONE, location, units, limits.minimum, limits.maxRandom);
	}
}

function insaneReinforcementSpawn()
{
	if (winFlag)
	{
		return;
	}
	const extraUnits = [cTempl.nxmsens, cTempl.nxmsens, cTempl.nxmsamh, cTempl.nxmsamh];
	const units = {units: [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmscouh, cTempl.nxmangel], appended: extraUnits};
	const limits = {minimum: 12, maxRandom: 3};
	const location = camMakePos(getObject("phantomFacSouth"));
	if (countDroid(DROID_ANY, CAM_NEXUS) < MIS_UNIT_LIMIT_FOR_SPAWN)
	{
		camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_NEXUS, CAM_REINFORCE_CONDITION_NONE, location, units, limits.minimum, limits.maxRandom);
	}
}

function insaneTransporterAttack()
{
	if (winFlag)
	{
		return;
	}
	const DISTANCE_FROM_POS = 25;
	const SCAN_RADIUS = 2;
	const units = [cTempl.nxhgauss, cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmpulseh, cTempl.nxmscouh];
	const limits = {minimum: 10, maxRandom: 0};
	const location = camGenerateRandomMapCoordinate(getObject("startPosition"), CAM_GENERIC_WATER_STAT, DISTANCE_FROM_POS, SCAN_RADIUS);
	camSendGenericSpawn(CAM_REINFORCE_TRANSPORT, CAM_NEXUS, CAM_REINFORCE_CONDITION_NONE, location, units, limits.minimum, limits.maxRandom);
}

// Explode trucks to significantly reduce chances of gaming the lassat.
function destroyTrucksInBlastZone()
{
	const IGNORE_LIMIT = 240;
	if (Math.floor(mapLimit) > IGNORE_LIMIT)
	{
		// Lassat is only a few tiles away from the silos and player doesn't have much room left.
		removeTimer("destroyTrucksInBlastZone");
		return;
	}
	const objects = enumArea(0, MIS_Y_SCROLL_LIMIT, mapWidth, Math.floor(mapLimit), CAM_HUMAN_PLAYER, false);
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
	const targets = enumArea(0, MIS_Y_SCROLL_LIMIT, mapWidth, Math.floor(mapLimit), CAM_HUMAN_PLAYER, false).filter((obj) => (
		(obj.type === DROID && obj.droidType !== DROID_CONSTRUCT) || obj.type === STRUCTURE
	));

	if (!targets.length)
	{
		//Choose random coordinate within the limits.
		target = {
			x: camRand(mapWidth),
			y: MIS_Y_SCROLL_LIMIT + camRand(mapHeight - Math.floor(mapLimit)),
		};

		if (target.y > Math.floor(mapLimit))
		{
			target.y = Math.floor(mapLimit);
		}
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

	//Stop firing LasSat if the third missile unlock code was researched.
	if (winFlag === false)
	{
		//Droid or structure was destroyed before firing so pick a new one.
		if (!camDef(target))
		{
			queue("vaporizeTarget", camSecondsToMilliseconds(0.1));
			return;
		}
		if (Math.floor(mapLimit) < mapHeight)
		{
			if (camClassicMode() && tweakOptions.camClassic_balance32)
			{
				//Need to travel about 119 tiles in 80 minutes so:
				//119 tiles / 80 minutes = 1.4875 tiles per minute
				//1.4875 tile per minute / 60 seconds = 0.02479 tiles per second
				//0.02479 * 10 sec = ~0.24 tiles per blast at 10 second intervals.
				mapLimit += 0.24; //sector clear; move closer
			}
			else
			{
				//Need to travel about 119 tiles in ~1 hour so:
				//119 tiles / 60 minutes = 1.9833 tiles per minute
				//1.9833 tile per minute / 60 seconds = 0.03305 tiles per second
				//0.03305 * 10 sec = ~0.33 tiles per blast at 10 second intervals.
				mapLimit += 0.33; //sector clear; move closer
			}
		}
		laserSatFuzzyStrike(target);
	}
	else
	{
		removeTimer("vaporizeTarget");
	}
}

//A simple way to fire the LasSat with a chance of missing.
function laserSatFuzzyStrike(obj)
{
	const loc = camMakePos(obj);
	//Initially lock onto target
	let xCoord = loc.x;
	let yCoord = loc.y;

	//Introduce some randomness. More accurate than last mission.
	if (camRand(101) < 33)
	{
		const X_RAND = camRand(2);
		const Y_RAND = camRand(2);
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

	if (winFlag === false)
	{
		if (camRand(101) < 40)
		{
			playSound(cam_sounds.laserSatelliteFiring, xCoord, yCoord);
		}

		//Missed it so hit close to target
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
}

//Play videos and allow winning once the final one is researched.
function eventResearched(research, structure, player)
{
	for (let i = 0, l = videoInfo.length; i < l; ++i)
	{
		if (research.name === videoInfo[i].res && !videoInfo[i].played)
		{
			videoInfo[i].played = true;
			camPlayVideos({video: videoInfo[i].video, type: videoInfo[i].type});
			if (videoInfo[i].res === mis_researchTargets.resistance)
			{
				enableResearch(mis_researchTargets.missileCode1, CAM_HUMAN_PLAYER);
			}
			else if (videoInfo[i].res === mis_researchTargets.missileCode3)
			{
				winFlag = true;
			}
		}
	}
}

//For checking when the five minute delay is over.
function checkTime()
{
	if (getMissionTime() <= 2)
	{
		camPlayVideos({video: "MB3_AD2_MSG2", type: CAMP_MSG});
		if (camClassicMode() && tweakOptions.camClassic_balance32)
		{
			camSetMissionTimer(camMinutesToSeconds(80)); // To accommodate the research bug of 3.2 balance.
		}
		else
		{
			camSetMissionTimer(camHoursToSeconds(1));
		}

		phantomFactorySpawn();
		queue("vaporizeTarget", camSecondsToMilliseconds(2));
		setTimer("destroyTrucksInBlastZone", camSecondsToMilliseconds(9));
		setTimer("vaporizeTarget", camSecondsToMilliseconds(10));
		setTimer("phantomFactorySpawn", camChangeOnDiff(camMinutesToMilliseconds(5)));
		if (camAllowInsaneSpawns())
		{
			setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(3));
			setTimer("insaneTransporterAttack", camMinutesToMilliseconds(3));
		}
		removeTimer("checkTime");
	}
}

//Check if the silos still exist and only allow winning if the player captured them.
//NOTE: Being in cheat mode disables the extra failure condition.
function checkMissileSilos()
{
	if (winFlag)
	{
		return true;
	}

	if (!camIsCheating() && !countStruct("NX-ANTI-SATSite", CAM_HUMAN_PLAYER))
	{
		return false;
	}
}

function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Protect the missile silos and research for the missile codes"));

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone");
	mapLimit = 137.0;
	winFlag = false;
	videoInfo = [
		{played: false, video: "MB3_AD2_MSG3", type: MISS_MSG, res: mis_researchTargets.resistance},
		{played: false, video: "MB3_AD2_MSG4", type: CAMP_MSG, res: mis_researchTargets.missileCode1},
		{played: false, video: "MB3_AD2_MSG5", type: CAMP_MSG, res: mis_researchTargets.missileCode2},
		{played: false, video: "MB3_AD2_MSG6", type: CAMP_MSG, res: mis_researchTargets.missileCode3},
	];

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, cam_levels.gammaEnd.pre, {
		callback: "checkMissileSilos"
	});

	setScrollLimits(0, MIS_Y_SCROLL_LIMIT, 64, 256);

	//Destroy everything above limits
	const destroyZone = enumArea(0, 0, 64, MIS_Y_SCROLL_LIMIT, CAM_HUMAN_PLAYER, false);
	for (let i = 0, l = destroyZone.length; i < l; ++i)
	{
		camSafeRemoveObject(destroyZone[i], false);
	}

	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	setMissionTime(camMinutesToSeconds(5));
	enableResearch(mis_researchTargets.resistance, CAM_HUMAN_PLAYER);

	if (camClassicMode())
	{
		camClassicResearch(mis_nexusResClassic, CAM_NEXUS);
	}
	else
	{
		camCompleteRequiredResearch(mis_nexusRes, CAM_NEXUS);
	}

	camPlayVideos({video: "MB3_AD2_MSG", type: MISS_MSG});

	setTimer("checkTime", camSecondsToMilliseconds(0.2));
	queue("vtolAttack", camChangeOnDiff(camMinutesToMilliseconds(3)));
}
