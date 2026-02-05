include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var allowWin;
const mis_collectiveRes = [
	"R-Defense-WallUpgrade06", "R-Struc-Materials06", "R-Sys-Engineering02",
	"R-Vehicle-Engine06", "R-Vehicle-Metals06", "R-Cyborg-Metals06",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage06", "R-Wpn-Cannon-ROF03",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage08",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc03", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF04", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage06",
	"R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF03", "R-Wpn-Howitzer-ROF03",
	"R-Wpn-Howitzer-Damage09", "R-Cyborg-Armor-Heat03", "R-Vehicle-Armor-Heat03",
	"R-Wpn-Bomb-Damage02", "R-Wpn-AAGun-Damage03", "R-Wpn-AAGun-ROF03",
	"R-Wpn-AAGun-Accuracy02", "R-Wpn-Howitzer-Accuracy02", "R-Struc-VTOLPad-Upgrade03",
];
const mis_collectiveResClassic = [
	"R-Defense-WallUpgrade06", "R-Struc-Materials06", "R-Struc-Factory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade03", "R-Vehicle-Engine06", "R-Vehicle-Metals06",
	"R-Cyborg-Metals06", "R-Sys-Engineering02", "R-Wpn-AAGun-Accuracy01",
	"R-Wpn-AAGun-Damage01", "R-Wpn-AAGun-ROF01", "R-Wpn-Bomb-Damage01",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage06", "R-Wpn-Cannon-ROF03",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-Howitzer-Accuracy02",
	"R-Wpn-Howitzer-Damage03", "R-Sys-Sensor-Upgrade01", "R-Wpn-MG-Damage07",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage06",
	"R-Wpn-RocketSlow-ROF03"
];
const mis_Labels = {
	startPos: {x: 92, y: 99},
	lz: {x: 86, y: 99, x2: 88, y2: 101},
	trPlace: {x: 87, y: 100},
	trExit: {x: 0, y: 55},
	northTankAssembly: {x: 95, y: 3},
	southCyborgAssembly: {x: 123, y: 125},
	westTankAssembly: {x: 3, y: 112}, //This was unused. Now part of Hard/Insane playthroughs.
	vtolRemovePos: {x: 127, y: 64},
	vtolSpawnPos: {x: 99, y: 1},
	vtolSpawnPos2: {x: 127, y: 65},
	vtolSpawnPos3: {x: 127, y: 28},
	vtolSpawnPos4: {x: 36, y: 1},
	vtolSpawnPos5: {x: 1, y: 28},
};

//Remove enemy vtols when in the remove zone area.
function checkEnemyVtolArea()
{
	const vtols = enumRange(mis_Labels.vtolRemovePos.x, mis_Labels.vtolRemovePos.y, 2, CAM_THE_COLLECTIVE, false).filter((obj) => (isVTOL(obj)));

	for (let i = 0, l = vtols.length; i < l; ++i)
	{
		if (camVtolCanDisappear(vtols[i]))
		{
			camSafeRemoveObject(vtols[i], false);
		}
	}
}

//Play last video sequence.
function playLastVideo()
{
	camPlayVideos({video: "CAM2_OUT", type: CAMP_MSG});
}

//Allow a win if a transporter was launched with a construction droid.
function eventTransporterLaunch(transporter)
{
	if (!allowWin && transporter.player === CAM_HUMAN_PLAYER)
	{
		const cargoDroids = enumCargo(transporter);

		for (let i = 0, len = cargoDroids.length; i < len; ++i)
		{
			const virDroid = cargoDroids[i];

			if (virDroid && virDroid.droidType === DROID_CONSTRUCT)
			{
				allowWin = true;
				break;
			}
		}
	}
}

//Attack every 30 seconds.
function vtolAttack()
{
	let vtolPositions = [
		mis_Labels.vtolSpawnPos,
		mis_Labels.vtolSpawnPos2,
		mis_Labels.vtolSpawnPos3,
		mis_Labels.vtolSpawnPos4,
		mis_Labels.vtolSpawnPos5
	];

	if (difficulty >= INSANE)
	{
		vtolPositions = undefined; //to randomize the spawns each time
	}

	let list = [];
	if (camClassicMode())
	{
		list = [ cTempl.commorv, cTempl.colcbv, cTempl.colagv, cTempl.comhvat ];
	}
	else
	{
		list = [ cTempl.commorv, cTempl.commorv, cTempl.comhvat, cTempl.commorvt, cTempl.comacv ];
	}

	const extras = {
		minVTOLs: 4,
		maxRandomVTOLs: (difficulty >= HARD) ? 2 : 1
	};

	camSetVtolData(CAM_THE_COLLECTIVE, vtolPositions, mis_Labels.vtolRemovePos, list, camSecondsToMilliseconds(30), undefined, extras);
}

//SouthEast attackers which are mostly cyborgs.
function cyborgAttack()
{
	let list = [];
	if (camClassicMode())
	{
		list = [cTempl.npcybr, cTempl.cocybag, cTempl.npcybc, cTempl.comhltat, cTempl.cohhpv];
	}
	else
	{
		list = [cTempl.cocybtk, cTempl.cocybag, cTempl.cocybsn, cTempl.comhltat, cTempl.cohhpv];
	}
	const extraUnits = [cTempl.cowwt, cTempl.cowwt];
	const units = {units: list, appended: extraUnits};
	const limits = {minimum: (difficulty >= INSANE) ? 15 : 18, maxRandom: (difficulty >= INSANE) ? 2 : 7};
	const location = camMakePos(mis_Labels.southCyborgAssembly);
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_NONE, location, units, limits.minimum, limits.maxRandom);
}

function hardCyborgAttackRandom()
{
	let list = []; //favor cannon cyborg
	if (camClassicMode())
	{
		list = [cTempl.npcybr, cTempl.cocybag, cTempl.npcybc, cTempl.npcybc, cTempl.comrotm];
	}
	else
	{
		list = [cTempl.cocybtk, cTempl.cocybag, cTempl.cocybsn, cTempl.cocybsn, cTempl.comrotm];
	}
	const extraUnits = [cTempl.cowwt, cTempl.cowwt, cTempl.comsens];
	const units = {units: list, appended: extraUnits};
	const limits = {minimum: (difficulty >= INSANE) ? 15 : 18, maxRandom: (difficulty >= INSANE) ? 2 : 7};
	const location = camMakePos(camGenerateRandomMapEdgeCoordinate(mis_Labels.startPos));
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_NONE, location, units, limits.minimum, limits.maxRandom);
}

//North road attacker consisting of powerful weaponry.
function tankAttack()
{
	const extraUnits = [cTempl.cowwt, cTempl.cowwt];
	const list = [cTempl.comhltat, cTempl.cohact, cTempl.cohhpv, cTempl.comagt];
	if (getMissionTime() < (60 * 22))
	{
		list.push(cTempl.cohbbt);
	}
	const units = {units: list, appended: extraUnits};
	const limits = {minimum: (difficulty >= INSANE) ? 15 : 18, maxRandom: (difficulty >= INSANE) ? 2 : 7};
	const location = camMakePos(mis_Labels.northTankAssembly);
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_NONE, location, units, limits.minimum, limits.maxRandom);
}

function hardTankAttackWest()
{
	const extraUnits = [cTempl.cowwt, cTempl.cowwt];
	const list = [cTempl.comhltat, cTempl.cohact, cTempl.cohhpv, cTempl.comagt];
	if (getMissionTime() < (60 * 22))
	{
		list.push(cTempl.cohbbt);
	}
	const units = {units: list, appended: extraUnits};
	const limits = {minimum: 8, maxRandom: 2};
	const location = camMakePos(mis_Labels.westTankAssembly);
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_NONE, location, units, limits.minimum, limits.maxRandom);
}

function insaneTransporterAttack()
{
	const DISTANCE_FROM_POS = 10;
	const units = [cTempl.cohact, cTempl.comhltat, cTempl.cohhpv];
	if (getMissionTime() < (60 * 22))
	{
		units.push(cTempl.cohbbt);
	}
	const limits = {minimum: 8, maxRandom: 2};
	const location = camMakePos(camGenerateRandomMapCoordinate(mis_Labels.startPos, CAM_GENERIC_LAND_STAT, DISTANCE_FROM_POS));
	camSendGenericSpawn(CAM_REINFORCE_TRANSPORT, CAM_THE_COLLECTIVE, CAM_REINFORCE_CONDITION_NONE, location, units, limits.minimum, limits.maxRandom);
}

//NOTE: this is only called once from the library's eventMissionTimeout().
function checkIfLaunched()
{
	const transporters = enumArea(0, 0, mapWidth, mapHeight, CAM_HUMAN_PLAYER, false).filter((obj) => (
		obj.type === DROID && camIsTransporter(obj)
	));
	if (transporters.length > 0)
	{
		allowWin = false;
	}

	if (allowWin)
	{
		camCallOnce("playLastVideo");
		return true;
	}
	return false;
}

//Everything in this level mostly just requeues itself until the mission ends.
function eventStartLevel()
{
	if (difficulty >= INSANE)
	{
		camSetExtraObjectiveMessage(_("Send off at least one transporter with a truck and survive The Collective assault until the timer ends"));
	}
	else
	{
		camSetExtraObjectiveMessage(_("Send off as many transporters as you can and bring at least one truck"));
	}

	camSetStandardWinLossConditions(CAM_VICTORY_TIMEOUT, cam_levels.gamma1, {
		reinforcements: camMinutesToSeconds(7), //Duration the transport "leaves" map.
		callback: "checkIfLaunched"
	});

	camSetupTransporter(mis_Labels.trPlace.x, mis_Labels.trPlace.y, mis_Labels.trExit.x, mis_Labels.trExit.y);
	centreView(mis_Labels.startPos.x, mis_Labels.startPos.y);
	setNoGoArea(mis_Labels.lz.x, mis_Labels.lz.y, mis_Labels.lz.x2, mis_Labels.lz.y2, CAM_HUMAN_PLAYER);

	setMissionTime(camMinutesToSeconds(30));

	if (camClassicMode())
	{
		camClassicResearch(mis_collectiveResClassic, CAM_THE_COLLECTIVE);
	}
	else
	{
		camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);
	}

	allowWin = false;
	camPlayVideos([{video: "MB2_DII_MSG", type: CAMP_MSG}, {video: "MB2_DII_MSG2", type: MISS_MSG}]);

	queue("vtolAttack", camSecondsToMilliseconds(30));
	if (difficulty >= INSANE)
	{
		setPower(playerPower(CAM_HUMAN_PLAYER) + 12000);
		setTimer("insaneTransporterAttack", camMinutesToMilliseconds(5));
	}
	if (difficulty >= HARD)
	{
		setTimer("hardTankAttackWest", camChangeOnDiff(camMinutesToMilliseconds(7)));
		setTimer("hardCyborgAttackRandom", camChangeOnDiff(camMinutesToMilliseconds(6)));
	}
	setTimer("cyborgAttack", camChangeOnDiff(camMinutesToMilliseconds(4)));
	setTimer("tankAttack", camChangeOnDiff(camMinutesToMilliseconds(3)));
	setTimer("checkEnemyVtolArea", camSecondsToMilliseconds(1));
}
