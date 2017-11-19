include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const LASSAT_FIRING = "pcv650.ogg"; // LASER SATELLITE FIRING!!!
const NEXUS_RES = [
	"R-Defense-WallUpgrade09", "R-Struc-Materials09", "R-Struc-Factory-Upgrade06",
	"R-Struc-Factory-Cyborg-Upgrade06", "R-Struc-VTOLFactory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals09",
	"R-Cyborg-Metals09", "R-Vehicle-Armor-Heat06", "R-Cyborg-Armor-Heat06",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Accuracy03", "R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage03",
	"R-Wpn-Energy-ROF03", "R-Wpn-Missile-Accuracy01", "R-Wpn-Missile-Damage02",
	"R-Wpn-Rail-Accuracy01", "R-Wpn-Rail-Damage03", "R-Wpn-Rail-ROF03",
	"R-Sys-Sensor-Upgrade01", "R-Sys-NEXUSrepair", "R-Wpn-Flamer-Damage06",
];
const VTOL_POSITIONS = [
	"vtolAppearPosW", "vtolAppearPosE",
];
var winFlag;
var mapLimit;

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
	var extras; with (camTemplates) extras = [nxmstrike, nxmsamh];
	var droids = [];
	var size = 12 + camRand(4); //Max of 15.

	for (var i = 0; i < size; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	//Vtol strike sensor and vindicator hovers.
	for (var i = 0; i < 4; ++i)
	{
		droids.push(extras[camRand(extras.length)]);
	}

	return droids;
}

//Chose a random spawn point for the VTOLs.
function vtolAttack()
{
	var list; with (camTemplates) list = [nxmheapv, nxlpulsev];
	camSetVtolData(NEXUS, VTOL_POSITIONS, "vtolRemovePos", list, camChangeOnDiff(180000)); // 3 min
}

//Chose a random spawn point to send ground reinforcements.
function phantomFactorySpawn()
{
	var list;
	var chosenFactory;

	switch (camRand(4))
	{
		case 0:
			with (camTemplates) list = [nxhgauss, nxmpulseh, nxmlinkh];
			chosenFactory = "phantomFacNorth";
			break;
		case 1:
			with (camTemplates) list = [nxcylas, nxcyrail, nxcyscou];
			chosenFactory = "phantomFacWest";
			break;
		case 2:
			with (camTemplates) list = [nxhgauss, nxmpulseh, nxmlinkh];
			chosenFactory = "phantomFacEast";
			break;
		case 3:
			with (camTemplates) list = [nxcylas, nxcyrail, nxcyscou, nxhgauss, nxmpulseh, nxmlinkh];
			chosenFactory = "phantomFacMiddle";
			break;
		default:
			with (camTemplates) list = [nxhgauss, nxmpulseh, nxmlinkh];
			chosenFactory = "phantomFacNorth";
	}

	if (countDroid(DROID_ANY, NEXUS) < 80)
	{
		camSendReinforcement(NEXUS, camMakePos(chosenFactory), randomTemplates(list), CAM_REINFORCE_GROUND, {
			data: { regroup: false, count: -1, },
		});
	}

	queue("phantomFactorySpawn", camChangeOnDiff(300000)); // 5 min
}

//Choose a target to fire the LasSat at. Automatically increases the limits
//when no target is found in the area.
function vaporizeTarget()
{
	const MAP_THIRD = Math.floor(mapHeight / 3);
	var targets = enumArea(0, 2 * MAP_THIRD, mapWidth, Math.floor(mapLimit), CAM_HUMAN_PLAYER, false);
	var target;

	if (!targets.length)
	{
		//Choose random coordinate within the limits.
		target = {
			"x": camRand(mapWidth),
			"y": Math.floor((2 * MAP_THIRD) + camRand(MAP_THIRD)),
		};

		if (target.y > Math.floor(mapLimit))
		{
			target.y = Math.floor(mapLimit);
		}

		if (Math.floor(mapLimit) < mapHeight)
		{
			mapLimit = mapLimit + 0.275; // sector clear; move closer.
		}
	}
	else
	{
		target = camMakePos(targets[0]);
	}

	//Stop firing LasSat if the third missile unlock code was researched.
	if (winFlag === false)
	{
		laserSatFuzzyStrike(target);
		queue("vaporizeTarget", 12000);
	}
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
		fireWeaponAtLoc(xCoord, yCoord, "LasSat");
	}
}

//Play videos and allow winning once the final one is researched.
function eventResearched(research, structure, player)
{
	if (research.name === "R-Sys-Resistance")
	{
		camPlayVideos("MB3_AD2_MSG3");
		enableResearch("R-Comp-MissileCodes01", CAM_HUMAN_PLAYER);
	}
	else if (research.name === "R-Comp-MissileCodes01")
	{
		camPlayVideos("MB3_AD2_MSG4");
	}
	else if (research.name === "R-Comp-MissileCodes02")
	{
		camPlayVideos("MB3_AD2_MSG5");
	}
	else if (research.name === "R-Comp-MissileCodes03")
	{
		camPlayVideos("MB3_AD2_MSG6");
		winFlag = true;
	}
}

//For checking when the five minute delay is over.
function checkTime()
{
	if (getMissionTime() <= 2)
	{
		camPlayVideos("MB3_AD2_MSG2");
		setMissionTime(3600); //1 hr
		phantomFactorySpawn();
		queue("vaporizeTarget", 2000);
	}
	else
	{
		queue("checkTime", 150);
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

	if (!isCheating() && !countStruct("NX-ANTI-SATSite", CAM_HUMAN_PLAYER))
	{
		playSound("pcv622.ogg"); //Objective failed.
		return false;
	}
}

function eventStartLevel()
{
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	mapLimit = 2 * Math.floor(mapHeight * 0.33);
	winFlag = false;

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "CAM_3_4S", {
		callback: "checkMissileSilos"
	});

	//Destroy all non-defense structures above the last third of the map and
	//prevent the player from building anything there as well
	setNoGoArea(0, 0, 64, Math.floor(mapLimit), NEXUS);
	var destroyZone = enumArea(0, 0, 64, Math.floor(mapLimit), CAM_HUMAN_PLAYER, false).filter(function(obj) {
		return obj.type === STRUCTURE && (obj.stattype !== WALL && obj.stattype !== DEFENSE);
	});
	for (var i = 0, l = destroyZone.length; i < l; ++i)
	{
		camSafeRemoveObject(destroyZone[i], false);
	}

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	setMissionTime(300); //5 min
	enableResearch("R-Sys-Resistance", CAM_HUMAN_PLAYER);

	setPower(AI_POWER, NEXUS);
	camCompleteRequiredResearch(NEXUS_RES, NEXUS);
	camPlayVideos("MB3_AD2_MSG");

	queue("checkTime", 200);
	queue("vtolAttack", camChangeOnDiff(180000)); // 3 min
}
