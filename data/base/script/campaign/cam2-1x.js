include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");
include("script/campaign/transitionTech.js");

var victoryFlag;

const MIS_TRANSPORT_TEAM_PLAYER = 1;
const mis_collectiveRes = [
	"R-Defense-WallUpgrade04", "R-Struc-Materials04", "R-Sys-Engineering02",
	"R-Vehicle-Engine03", "R-Vehicle-Metals03", "R-Cyborg-Metals03",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage04",
	"R-Wpn-Cannon-ROF01", "R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage05", "R-Wpn-MG-ROF02", "R-Wpn-Mortar-Acc01",
	"R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-ROF01",
	"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage04",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage04", "R-Sys-Sensor-Upgrade01"
];
const mis_collectiveResClassic = [
	"R-Defense-WallUpgrade03", "R-Struc-Materials03", "R-Vehicle-Engine04",
	"R-Vehicle-Metals03", "R-Cyborg-Metals03", "R-Vehicle-Armor-Heat01",
	"R-Cyborg-Armor-Heat01", "R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage04",
	"R-Wpn-Cannon-ROF01", "R-Wpn-Flamer-Damage04", "R-Wpn-Flamer-ROF01",
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF02", "R-Sys-Sensor-Upgrade01",
	"R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-Damage04", "R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage03", "R-Wpn-RocketSlow-ROF03"
];

//trigger event when droid reaches the downed transport.
camAreaEvent("crashSite", function(droid)
{
	//Unlikely to happen.
	if (!enumDroid(MIS_TRANSPORT_TEAM_PLAYER).length)
	{
		gameOverMessage(false);
		return;
	}

	playSound(cam_sounds.rescue.unitsRescued);

	hackRemoveMessage("C21_OBJECTIVE", PROX_MSG, CAM_HUMAN_PLAYER);

	const droids = enumDroid(MIS_TRANSPORT_TEAM_PLAYER);
	for (let i = 0; i < droids.length; ++i)
	{
		donateObject(droids[i], CAM_HUMAN_PLAYER);
	}

	//Give the donation enough time to transfer them to the player. Otherwise
	//the level will end too fast and will trigger asserts in the next level.
	queue("triggerWin", camSecondsToMilliseconds(2));
});

function insaneReinforcementSpawn()
{
	const SCAN_DISTANCE = 2; // Skyscrapers are close to some edges.
	const DISTANCE_FROM_POS = 25;
	const units = [cTempl.commrl, cTempl.commrp, cTempl.npcybc];
	const limits = {minimum: 2, maxRandom: 2};
	const location = camGenerateRandomMapEdgeCoordinate(getObject("startingPosition"), CAM_GENERIC_LAND_STAT, DISTANCE_FROM_POS, SCAN_DISTANCE);
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_BASES, location, units, limits.minimum, limits.maxRandom);
}

//function that applies damage to units in the downed transport transport team.
function preDamageUnits()
{
	setHealth(getObject("transporter"), 40);
	const droids = enumDroid(MIS_TRANSPORT_TEAM_PLAYER);
	for (let j = 0; j < droids.length; ++j)
	{
		setHealth(droids[j], 40 + camRand(20));
	}
}

//victory callback will thus complete the level.
function triggerWin()
{
	victoryFlag = true;
}

function setupCyborgGroups()
{
	//create group of cyborgs and send them on war path
	camManageGroup(camMakeGroup("cyborgPositionNorth"), CAM_ORDER_ATTACK, {
		regroup: false
	});

	//create group of cyborgs and send them on war path
	camManageGroup(camMakeGroup("cyborgPositionEast"), CAM_ORDER_ATTACK, {
		regroup: false
	});
}

function setCrashedTeamExp()
{
	const DROID_EXP = camGetRankThreshold("professional");
	const droids = enumDroid(MIS_TRANSPORT_TEAM_PLAYER).filter((dr) => (
		!camIsSystemDroid(dr) && !camIsTransporter(dr)
	));

	for (let i = 0; i < droids.length; ++i)
	{
		const droid = droids[i];
		setDroidExperience(droid, DROID_EXP);
	}

	preDamageUnits();
}

//Checks if the downed transport has been destroyed and issue a game lose.
function checkCrashedTeam()
{
	if (getObject("transporter") === null)
	{
		return false;
	}

	if (camDef(victoryFlag) && victoryFlag)
	{
		if (camAllowInsaneSpawns())
		{
			if (!camAllEnemyBasesEliminated())
			{
				return;
			}
		}

		return true;
	}
}

function eventStartLevel()
{
	const VIC_MSG = _("Locate and rescue your units from the shot down transporter");
	const VIC_MSG_SPAWNS = _("Destroy all enemy bases");
	const vicMessages = (!camAllowInsaneSpawns()) ? VIC_MSG : [VIC_MSG, VIC_MSG_SPAWNS];
	camSetExtraObjectiveMessage(vicMessages);

	camSetStandardWinLossConditions(CAM_VICTORY_OFFWORLD, cam_levels.beta3, {
		area: "RTLZ",
		message: "C21_LZ",
		reinforcements: -1,
		callback: "checkCrashedTeam"
	});

	const subLandingZone = getObject("landingZone");
	const startPos = getObject("startingPosition");
	const tEnt = getObject("transporterEntry");
	const tExt = getObject("transporterExit");
	centreView(startPos.x, startPos.y);
	setNoGoArea(subLandingZone.x, subLandingZone.y, subLandingZone.x2, subLandingZone.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);

	//Add crash site blip and from an alliance with the crashed team.
	hackAddMessage("C21_OBJECTIVE", PROX_MSG, CAM_HUMAN_PLAYER, false);
	setAlliance(CAM_HUMAN_PLAYER, MIS_TRANSPORT_TEAM_PLAYER, true);
	setAlliance(MIS_TRANSPORT_TEAM_PLAYER, CAM_THE_COLLECTIVE, true);

	//set downed transport team colour to be Project Green.
	changePlayerColour(MIS_TRANSPORT_TEAM_PLAYER, 0);

	if (camClassicMode())
	{
		camClassicResearch(mis_collectiveResClassic, CAM_THE_COLLECTIVE);
		camClassicResearch(mis_alphaResearchNewClassic, MIS_TRANSPORT_TEAM_PLAYER);
		camClassicResearch(mis_playerResBetaClassic, MIS_TRANSPORT_TEAM_PLAYER);
	}
	else
	{
		camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);
		camCompleteRequiredResearch(mis_alphaResearchNew, MIS_TRANSPORT_TEAM_PLAYER);
		camCompleteRequiredResearch(mis_playerResBeta, MIS_TRANSPORT_TEAM_PLAYER);

		if (difficulty >= HARD)
		{
			camUpgradeOnMapTemplates(cTempl.commc, cTempl.commrp, CAM_THE_COLLECTIVE);
		}
	}

	if (camAllowInsaneSpawns())
	{
		addDroid(CAM_THE_COLLECTIVE, 35,22, "MRP Panther Tracks", tBody.tank.panther, tProp.tank.tracks, "", "", tWeap.tank.miniRocketPod);
		addDroid(CAM_THE_COLLECTIVE, 35,23, "MRP Panther Tracks", tBody.tank.panther, tProp.tank.tracks, "", "", tWeap.tank.miniRocketPod);
		addDroid(CAM_THE_COLLECTIVE, 35,24, "Heavy Cannon Tiger Tracks", tBody.tank.tiger, tProp.tank.tracks, "", "", tWeap.tank.heavyCannon);
		addDroid(CAM_THE_COLLECTIVE, 35,25, "Heavy Cannon Tiger Tracks", tBody.tank.tiger, tProp.tank.tracks, "", "", tWeap.tank.heavyCannon);
		addDroid(CAM_THE_COLLECTIVE, 35,26, "Heavy Cannon Tiger Tracks", tBody.tank.tiger, tProp.tank.tracks, "", "", tWeap.tank.heavyCannon);
		addDroid(CAM_THE_COLLECTIVE, 36,26, "Heavy Cannon Tiger Tracks", tBody.tank.tiger, tProp.tank.tracks, "", "", tWeap.tank.heavyCannon);
		addDroid(CAM_THE_COLLECTIVE, 37,26, "Heavy Cannon Tiger Tracks", tBody.tank.tiger, tProp.tank.tracks, "", "", tWeap.tank.heavyCannon);
		addDroid(CAM_THE_COLLECTIVE, 38,26, "MRP Panther Tracks", tBody.tank.panther, tProp.tank.tracks, "", "", tWeap.tank.miniRocketPod);
		addDroid(CAM_THE_COLLECTIVE, 39,26, "MRP Panther Tracks", tBody.tank.panther, tProp.tank.tracks, "", "", tWeap.tank.miniRocketPod);
	}

	camSetEnemyBases({
		"COHardpointBase": {
			cleanup: "hardpointBaseCleanup",
			detectMsg: "C21_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COBombardBase": {
			cleanup: "bombardBaseCleanup",
			detectMsg: "C21_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"COBunkerBase": {
			cleanup: "bunkerBaseCleanup",
			detectMsg: "C21_BASE3",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	setCrashedTeamExp();
	victoryFlag = false;
	queue("setupCyborgGroups", camSecondsToMilliseconds(5));
	if (camAllowInsaneSpawns())
	{
		setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(3));
	}
}
