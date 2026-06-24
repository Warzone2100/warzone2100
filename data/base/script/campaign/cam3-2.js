include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");
include("script/campaign/transitionTech.js");

const MIS_ALPHA_PLAYER = 1; //Team alpha units belong to player 1.
const mis_nexusRes = [
	"R-Sys-Engineering03", "R-Defense-WallUpgrade10", "R-Struc-Materials09",
	"R-Struc-VTOLPad-Upgrade06", "R-Wpn-Bomb-Damage03", "R-Sys-NEXUSrepair",
	"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
	"R-Wpn-Mortar-Acc03", "R-Wpn-MG-Damage10", "R-Wpn-Mortar-ROF04",
	"R-Vehicle-Engine08", "R-Vehicle-Metals08", "R-Vehicle-Armor-Heat05",
	"R-Cyborg-Metals08", "R-Cyborg-Armor-Heat05", "R-Wpn-RocketSlow-ROF06",
	"R-Wpn-AAGun-Damage06", "R-Wpn-AAGun-ROF06", "R-Wpn-Howitzer-Damage09",
	"R-Wpn-Howitzer-ROF04", "R-Wpn-Cannon-Damage09", "R-Wpn-Cannon-ROF06",
	"R-Wpn-Missile-Damage01", "R-Wpn-Missile-ROF01", "R-Wpn-Missile-Accuracy01",
	"R-Wpn-Rail-Damage01", "R-Wpn-Rail-ROF01", "R-Wpn-Rail-Accuracy01",
	"R-Wpn-Energy-Damage03", "R-Wpn-Energy-ROF03", "R-Wpn-Energy-Accuracy01",
	"R-Wpn-AAGun-Accuracy03", "R-Wpn-Howitzer-Accuracy03", "R-Sys-NEXUSsensor",
];
const mis_nexusResClassic = [
	"R-Defense-WallUpgrade08", "R-Struc-Materials08", "R-Struc-Factory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals07",
	"R-Cyborg-Metals07", "R-Vehicle-Armor-Heat05", "R-Cyborg-Armor-Heat05",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Damage03", "R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage02",
	"R-Wpn-Energy-ROF02", "R-Wpn-Missile-Accuracy01", "R-Wpn-Missile-Damage01",
	"R-Wpn-Rail-Damage02", "R-Wpn-Rail-ROF02", "R-Sys-Sensor-Upgrade01",
	"R-Sys-NEXUSrepair", "R-Wpn-Flamer-Damage06", "R-Sys-NEXUSsensor",
];
var alphaUnitIDs;
var startExtraLoss;

//Remove Nexus VTOL droids.
camAreaEvent("vtolRemoveZone", function(droid)
{
	if (droid.player !== CAM_HUMAN_PLAYER && camVtolCanDisappear(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	resetLabel("vtolRemoveZone", CAM_NEXUS);
});

//This is an area just below the "doorway" into the alpha team pit. Activates
//groups that are hidden farther south.
camAreaEvent("rescueTrigger", function(droid)
{
	hackRemoveMessage("C3-2_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
	camManageGroup(camMakeGroup("laserTankGroup"), CAM_ORDER_ATTACK, {
		regroup: true,
		count: -1,
		morale: 90,
		fallback: camMakePos("healthRetreatPos")
	});
	//Activate edge map queue and donate all of alpha to the player.
	phantomFactorySE();
	setAlliance(MIS_ALPHA_PLAYER, CAM_NEXUS, false);
	camAbsorbPlayer(MIS_ALPHA_PLAYER, CAM_HUMAN_PLAYER);

	queue("getAlphaUnitIDs", camSecondsToMilliseconds(0.5));
	setTimer("phantomFactorySE", camChangeOnDiff(camMinutesToMilliseconds(5)));

	camPlayVideos({video: "MB3_2_MSG4", type: MISS_MSG});
});

//Play videos, donate alpha to the player and setup reinforcements.
camAreaEvent("phantomFacTrigger", function(droid)
{
	camPlayVideos([cam_sounds.incoming.incomingIntelligenceReport, {video: "MB3_2_MSG3", type: CAMP_MSG}]); //Warn about VTOLs.
	queue("enableReinforcements", camSecondsToMilliseconds(5));
	queue("vtolAttack", camChangeOnDiff(camMinutesToMilliseconds(2)));
	if (camAllowInsaneSpawns())
	{
		queue("insaneTransporterAttack", camSecondsToMilliseconds(20));
		setTimer("insaneTransporterAttack", camMinutesToMilliseconds(4.5));
	}
});

function setAlphaExp()
{
	const DROID_EXP = camGetRankThreshold("hero", true); //Hero Commander rank.
	const alphaDroids = enumArea("alphaPit", MIS_ALPHA_PLAYER, false).filter((obj) => (
		obj.type === DROID
	));

	for (let i = 0, l = alphaDroids.length; i < l; ++i)
	{
		const dr = alphaDroids[i];
		if (!camIsSystemDroid(dr))
		{
			setDroidExperience(dr, DROID_EXP);
		}
	}
}

//Get the IDs of Alpha units after they were donated to the player.
function getAlphaUnitIDs()
{
	alphaUnitIDs = [];
	const alphaDroids = enumArea("alphaPit", CAM_HUMAN_PLAYER, false).filter((obj) => (
		obj.type === DROID && obj.experience > 0
	));

	for (let i = 0, l = alphaDroids.length; i < l; ++i)
	{
		const dr = alphaDroids[i];
		alphaUnitIDs.push(dr.id);
	}
	startExtraLoss = true;
}

function phantomFactoryNE()
{
	const list = [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas];
	sendEdgeMapDroids(6, "NE-PhantomFactory", list);
}

function phantomFactorySW()
{
	const list = [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas];
	sendEdgeMapDroids(8, "SW-PhantomFactory", list);
}

function phantomFactorySE()
{
	const list = [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas, cTempl.nxlflash, cTempl.nxmrailh, cTempl.nxmlinkh];
	sendEdgeMapDroids(10 + camRand(6), "SE-PhantomFactory", list); //10-15 units
}

function sendEdgeMapDroids(droidCount, location, list)
{
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_NEXUS, CAM_REINFORCE_CONDITION_NONE, location, list, droidCount);
}

function insaneTransporterAttack()
{
	const DISTANCE_FROM_POS = 30;
	const OBJ_SCAN_RADIUS = 1;
	const units = [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxcylas, cTempl.nxmscouh, cTempl.nxlflash, cTempl.nxmrailh];
	const limits = {minimum: 8, maxRandom: 2};
	const location = camGenerateRandomMapCoordinate(getObject("startPosition"), CAM_GENERIC_LAND_STAT, DISTANCE_FROM_POS, OBJ_SCAN_RADIUS, false);
	camSendGenericSpawn(CAM_REINFORCE_TRANSPORT, CAM_NEXUS, CAM_REINFORCE_CONDITION_NONE, location, units, limits.minimum, limits.maxRandom);
}

function setupPatrolGroups()
{
	camManageGroup(camMakeGroup("cyborgGroup1"), CAM_ORDER_ATTACK, {
		regroup: true,
		count: -1,
		morale: 90,
		fallback: camMakePos("healthRetreatPos")
	});

	camManageGroup(camMakeGroup("cyborgGroup2"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("upperMiddlePos"),
			camMakePos("upperMiddleEastPos"),
			camMakePos("playerLZ"),
			camMakePos("upperMiddleWest"),
			camMakePos("upperMiddleHill"),
		],
		interval: camSecondsToMilliseconds(20),
		regroup: true,
		count: -1
	});

	camManageGroup(camMakeGroup("cyborgGroup3"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("upperMiddleWest"),
			camMakePos("upperMiddleHill"),
			camMakePos("lowerMiddleEast"),
			camMakePos("lowerMiddleHill"),
		],
		interval: camSecondsToMilliseconds(20),
		regroup: true,
		count: -1
	});

	camManageGroup(camMakeGroup("cyborgGroup4"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("lowerMiddleEast"),
			camMakePos("lowerMiddleHill"),
			camMakePos("lowerMiddleWest"),
			camMakePos("SWCorner"),
			camMakePos("alphaDoorway"),
		],
		interval: camSecondsToMilliseconds(25),
		regroup: true,
		count: -1
	});

	camManageGroup(camMakeGroup("cyborgGroup5"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("upperMiddlePos"),
			camMakePos("upperMiddleEastPos"),
			camMakePos("playerLZ"),
			camMakePos("upperMiddleWest"),
			camMakePos("upperMiddleHill"),
			camMakePos("lowerMiddleEast"),
			camMakePos("lowerMiddleHill"),
			camMakePos("lowerMiddleWest"),
			camMakePos("SWCorner"),
			camMakePos("alphaDoorway"),
			camMakePos("NE-PhantomFactory"),
			camMakePos("SW-PhantomFactory"),
			camMakePos("SE-PhantomFactory"),
		],
		interval: camSecondsToMilliseconds(35),
		regroup: true,
		count: -1
	});
}

function wave2()
{
	const CONDITION = ((camAllowInsaneSpawns()) ? undefined : "NXvtolStrikeTower");
	const list = [cTempl.nxlscouv, cTempl.nxlscouv];
	const ext = {limit: [3, 3], alternate: true, altIdx: 0};
	camSetVtolData(CAM_NEXUS, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(3)), CONDITION, ext);
}

function wave3()
{
	const CONDITION = ((camAllowInsaneSpawns()) ? undefined : "NXvtolStrikeTower");
	const list = [cTempl.nxlneedv, cTempl.nxlneedv];
	const ext = {limit: [3, 3], alternate: true, altIdx: 0};
	camSetVtolData(CAM_NEXUS, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(3)), CONDITION, ext);
}

//Setup Nexus VTOL hit and runners.
function vtolAttack()
{
	const CONDITION = ((camAllowInsaneSpawns()) ? undefined : "NXvtolStrikeTower");
	if (camClassicMode())
	{
		const list = [cTempl.nxlscouv, cTempl.nxlscouv, cTempl.nxmtherv];
		const ext = {limit: [2, 2, 4], alternate: true, altIdx: 0};
		camSetVtolData(CAM_NEXUS, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(3)), CONDITION, ext);
	}
	else
	{
		const list = [cTempl.nxmtherv, cTempl.nxmtherv];
		const ext = {limit: [3, 3], alternate: true, altIdx: 0};
		camSetVtolData(CAM_NEXUS, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(3)), CONDITION, ext);
		queue("wave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("wave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

//Reinforcements not available until team Alpha brief about VTOLS.
function enableReinforcements()
{
	playSound(cam_sounds.reinforcementsAreAvailable);
	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, cam_levels.gamma5, {
		area: "RTLZ",
		message: "C32_LZ",
		reinforcements: camMinutesToSeconds(3),
		callback: "alphaTeamAlive",
		retlz: true
	});
}

function alphaTeamAlive()
{
	if (camDef(alphaUnitIDs) && startExtraLoss)
	{
		let alphaAlive = false;
		const alive = enumArea(0, 0, mapWidth, mapHeight, CAM_HUMAN_PLAYER, false).filter((obj) => (
			obj.type === DROID
		));

		for (let i = 0, l = alive.length; i < l; ++i)
		{
			for (let x = 0, c = alphaUnitIDs.length; x < c; ++x)
			{
				if (alive[i].id === alphaUnitIDs[x])
				{
					alphaAlive = true;
					break;
				}
			}
		}

		if (alphaAlive === false)
		{
			return false;
		}

		if (alphaAlive === true && alive.length > 0)
		{
			return true;
		}
	}
}

function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Rescue Alpha team from Nexus"));

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone");
	const tEnt = getObject("transporterEntry");
	const tExt = getObject("transporterExit");
	startExtraLoss = false;

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, cam_levels.gamma5, {
		area: "RTLZ",
		message: "C32_LZ",
		reinforcements: -1,
		callback: "alphaTeamAlive",
		retlz: true
	});

	if (camClassicMode())
	{
		camClassicResearch(mis_nexusResClassic, CAM_NEXUS);
		camClassicResearch(mis_gammaAllyResClassic, MIS_ALPHA_PLAYER);
	}
	else
	{
		camCompleteRequiredResearch(mis_nexusRes, CAM_NEXUS);
		camCompleteRequiredResearch(mis_gammaAllyRes, MIS_ALPHA_PLAYER);

		camSetArtifacts({
			"NXartiCyborg": { tech: "R-Wpn-Cannon-ROF05" },
		});
	}

	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);

	setAlliance(MIS_ALPHA_PLAYER, CAM_NEXUS, true);
	setAlliance(MIS_ALPHA_PLAYER, CAM_HUMAN_PLAYER, true);
	changePlayerColour(MIS_ALPHA_PLAYER, 0);

	hackAddMessage("C3-2_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
	queue("setAlphaExp", camSecondsToMilliseconds(2));
	queue("setupPatrolGroups", camChangeOnDiff(camMinutesToMilliseconds(2)));

	setTimer("phantomFactoryNE", camChangeOnDiff(camMinutesToMilliseconds(4.5)));
	setTimer("phantomFactorySW", camChangeOnDiff(camMinutesToMilliseconds(6.5)));
}
