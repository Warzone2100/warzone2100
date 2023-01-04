include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const Y_SCROLL_LIMIT = 137;
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
const VTOL_POSITIONS = [
	"vtolAppearPosW", "vtolAppearPosE",
];
var winFlag;
var mapLimit;
var videoInfo; //holds some info about when to play a video.

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

//Return a random assortment of droids with the given templates.
function randomTemplates(list)
{
	var i = 0;
	var extras = [cTempl.nxmsens, cTempl.nxmsamh];
	var droids = [];
	var size = 12 + camRand(4); //Max of 15.

	for (i = 0; i < size; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	//Sensor and vindicator hovers.
	for (i = 0; i < 4; ++i)
	{
		droids.push(extras[camRand(extras.length)]);
	}

	return droids;
}

//Chose a random spawn point for the VTOLs.
function vtolAttack()
{
	var list = [cTempl.nxmheapv, cTempl.nxlpulsev];
	camSetVtolData(NEXUS, VTOL_POSITIONS, "vtolRemovePos", list, camChangeOnDiff(camMinutesToMilliseconds(3)));
}

//Chose a random spawn point to send ground reinforcements.
function phantomFactorySpawn()
{
	var list;
	var chosenFactory;

	switch (camRand(3))
	{
		case 0:
			list = [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmlinkh];
			chosenFactory = "phantomFacWest";
			break;
		case 1:
			list = [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmlinkh];
			chosenFactory = "phantomFacEast";
			break;
		case 2:
			list = [cTempl.nxcylas, cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmlinkh];
			chosenFactory = "phantomFacMiddle";
			break;
		default:
			list = [cTempl.nxhgauss, cTempl.nxmpulseh, cTempl.nxmlinkh];
			chosenFactory = "phantomFacWest";
	}

	if (countDroid(DROID_ANY, NEXUS) < 80)
	{
		camSendReinforcement(NEXUS, camMakePos(chosenFactory), randomTemplates(list), CAM_REINFORCE_GROUND, {
			data: { regroup: false, count: -1, },
		});
	}
}

//Choose a target to fire the LasSat at. Automatically increases the limits
//when no target is found in the area.
function vaporizeTarget()
{
	var target;
	var targets = enumArea(0, Y_SCROLL_LIMIT, mapWidth, Math.floor(mapLimit), CAM_HUMAN_PLAYER, false).filter((obj) => (
		obj.type === DROID || obj.type === STRUCTURE
	));

	if (!targets.length)
	{
		//Choose random coordinate within the limits.
		target = {
			x: camRand(mapWidth),
			y: Y_SCROLL_LIMIT + camRand(mapHeight - Math.floor(mapLimit)),
		};

		if (target.y > Math.floor(mapLimit))
		{
			target.y = Math.floor(mapLimit);
		}
	}
	else
	{
		var dr = targets.filter((obj) => (obj.type === DROID && !isVTOL(obj)));
		var vt = targets.filter((obj) => (obj.type === DROID && isVTOL(obj)));
		var st = targets.filter((obj) => (obj.type === STRUCTURE));

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
			//Need to travel about 119 tiles in ~1 hour so:
			//119 tiles / 60 minutes = 1.983 tiles per minute
			//1.983 tile per minute / 60 seconds = 0.03305 tiles per second
			//0.03305 * 10 sec = ~0.33 tiles per blast at 10 second intervals.
			mapLimit += 0.33; //sector clear; move closer
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
	const LOC = camMakePos(obj);
	//Initially lock onto target
	var xCoord = LOC.x;
	var yCoord = LOC.y;

	//Introduce some randomness. More accurate than last mission.
	if (camRand(101) < 33)
	{
		var xRand = camRand(2);
		var yRand = camRand(2);
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

	if (winFlag === false)
	{
		if (camRand(101) < 40)
		{
			playSound(LASSAT_FIRING, xCoord, yCoord);
		}

		//Missed it so hit close to target
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
			if (videoInfo[i].res === "R-Sys-Resistance")
			{
				enableResearch("R-Comp-MissileCodes01", CAM_HUMAN_PLAYER);
			}
			else if (videoInfo[i].res === "R-Comp-MissileCodes03")
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
		setMissionTime(camHoursToSeconds(1));

		phantomFactorySpawn();
		queue("vaporizeTarget", camSecondsToMilliseconds(2));
		setTimer("vaporizeTarget", camSecondsToMilliseconds(10));
		setTimer("phantomFactorySpawn", camChangeOnDiff(camMinutesToMilliseconds(5)));
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

	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	mapLimit = 137.0;
	winFlag = false;
	videoInfo = [
		{played: false, video: "MB3_AD2_MSG3", type: MISS_MSG, res: "R-Sys-Resistance"},
		{played: false, video: "MB3_AD2_MSG4", type: CAMP_MSG, res: "R-Comp-MissileCodes01"},
		{played: false, video: "MB3_AD2_MSG5", type: CAMP_MSG, res: "R-Comp-MissileCodes02"},
		{played: false, video: "MB3_AD2_MSG6", type: CAMP_MSG, res: "R-Comp-MissileCodes03"},
	];

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "CAM_3_4S", {
		callback: "checkMissileSilos"
	});

	setScrollLimits(0, Y_SCROLL_LIMIT, 64, 256);

	//Destroy everything above limits
	var destroyZone = enumArea(0, 0, 64, Y_SCROLL_LIMIT, CAM_HUMAN_PLAYER, false);
	for (let i = 0, l = destroyZone.length; i < l; ++i)
	{
		camSafeRemoveObject(destroyZone[i], false);
	}

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	setMissionTime(camMinutesToSeconds(5));
	enableResearch("R-Sys-Resistance", CAM_HUMAN_PLAYER);

	camCompleteRequiredResearch(NEXUS_RES, NEXUS);
	camPlayVideos({video: "MB3_AD2_MSG", type: MISS_MSG});

	setTimer("checkTime", camSecondsToMilliseconds(0.2));
	queue("vtolAttack", camChangeOnDiff(camMinutesToMilliseconds(3)));
}
