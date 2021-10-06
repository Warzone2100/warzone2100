include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var allowWin;
const COLLECTIVE_RES = [
	"R-Defense-WallUpgrade06", "R-Struc-Materials06", "R-Sys-Engineering02",
	"R-Vehicle-Engine06", "R-Vehicle-Metals06", "R-Cyborg-Metals06",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage06","R-Wpn-Cannon-ROF03",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage07",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage06",
	"R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF03", "R-Wpn-Howitzer-ROF03",
	"R-Wpn-Howitzer-Damage09", "R-Cyborg-Armor-Heat03", "R-Vehicle-Armor-Heat03",
	"R-Wpn-Bomb-Damage02", "R-Wpn-AAGun-Damage03", "R-Wpn-AAGun-ROF03",
	"R-Wpn-AAGun-Accuracy02", "R-Wpn-Howitzer-Accuracy02", "R-Struc-VTOLPad-Upgrade03",
];
const startpos = {x: 88, y: 101};

//Remove enemy vtols when in the remove zone area.
function checkEnemyVtolArea()
{
	var pos = {x: 127, y: 64};
	var vtols = enumRange(pos.x, pos.y, 2, THE_COLLECTIVE, false).filter(function(obj) {
		return isVTOL(obj);
	});

	for (var i = 0, l = vtols.length; i < l; ++i)
	{
		if ((vtols[i].weapons[0].armed < 20) || (vtols[i].health < 60))
		{
			camSafeRemoveObject(vtols[i], false);
		}
	}
}

//Play last video sequence and destroy all droids on map.
function playLastVideo()
{
	var droids = enumArea(0, 0, mapWidth, mapHeight, CAM_HUMAN_PLAYER, false).filter(function(obj) {
		return (obj.type === DROID && !camIsTransporter(obj));
	});

	for (var i = 0, l = droids.length; i < l; ++i)
	{
		camSafeRemoveObject(droids[i], false);
	}
	camPlayVideos({video: "CAM2_OUT", type: CAMP_MSG});
}

//Allow a win if a transporter was launched with a construction droid.
function eventTransporterLaunch(transporter)
{
	if (!allowWin && transporter.player === CAM_HUMAN_PLAYER)
	{
		var cargoDroids = enumCargo(transporter);

		for (var i = 0, len = cargoDroids.length; i < len; ++i)
		{
			var virDroid = cargoDroids[i];

			if (virDroid && virDroid.droidType === DROID_CONSTRUCT)
			{
				allowWin = true;
				break;
			}
		}
	}
}

//Return randomly selected droid templates.
function randomTemplates(list, transporterAmount)
{
	var droids = [];
	var size;

	if (camDef(transporterAmount) && transporterAmount)
	{
		size = 8 + camRand(3);
	}
	else
	{
		size = (difficulty === INSANE) ? (15 + camRand(3)) : (18 + camRand(8));
	}

	for (var i = 0; i < size; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	return droids;
}

//Attack every 30 seconds.
function vtolAttack()
{
	var vtolPositions = [
		{x: 99, y: 1},
		{x: 127, y: 65},
		{x: 127, y: 28},
		{x: 36, y: 1},
		{x: 1, y: 28},
	];
	var vtolRemovePos = {x: 127, y: 64};

	if (difficulty === INSANE)
	{
		vtolPositions = undefined; //to randomize the spawns each time
	}

	var list = [cTempl.commorv, cTempl.colcbv, cTempl.colagv, cTempl.comhvat, cTempl.commorvt];
	camSetVtolData(THE_COLLECTIVE, vtolPositions, vtolRemovePos, list, camChangeOnDiff(camSecondsToMilliseconds(30)));
}

//SouthEast attackers which are mostly cyborgs.
function cyborgAttack()
{
	var southCyborgAssembly = {x: 123, y: 125};
	var list = [cTempl.npcybr, cTempl.cocybag, cTempl.npcybc, cTempl.comhltat, cTempl.cohhpv];

	camSendReinforcement(THE_COLLECTIVE, camMakePos(southCyborgAssembly), randomTemplates(list), CAM_REINFORCE_GROUND, {
		data: { regroup: false, count: -1 }
	});
}

function cyborgAttackRandom()
{
	var list = [cTempl.npcybr, cTempl.cocybag, cTempl.npcybc, cTempl.npcybc, cTempl.comrotm]; //favor cannon cyborg

	camSendReinforcement(THE_COLLECTIVE, camMakePos(camGenerateRandomMapEdgeCoordinate(startpos)), randomTemplates(list).concat(cTempl.comsens), CAM_REINFORCE_GROUND, {
		data: { regroup: false, count: -1 }
	});
}

//North road attacker consisting of powerful weaponry.
function tankAttack()
{
	var northTankAssembly = {x: 95, y: 3};
	var list = [cTempl.comhltat, cTempl.cohact, cTempl.cohhpv, cTempl.comagt, cTempl.cohbbt];

	camSendReinforcement(THE_COLLECTIVE, camMakePos(northTankAssembly), randomTemplates(list), CAM_REINFORCE_GROUND, {
		data: { regroup: false, count: -1, },
	});
}

function tankAttackWest()
{
	var westTankAssembly = {x: 3, y: 112}; //This was unused. Now part of Hard/Insane playthroughs.
	var list = [cTempl.comhltat, cTempl.cohact, cTempl.cohhpv, cTempl.comagt, cTempl.cohbbt];

	camSendReinforcement(THE_COLLECTIVE, camMakePos(westTankAssembly), randomTemplates(list, true), CAM_REINFORCE_GROUND, {
		data: { regroup: false, count: -1, },
	});
}

function transporterAttack()
{
	var droids = [cTempl.cohact, cTempl.comhltat, cTempl.cohbbt, cTempl.cohhpv];

	camSendReinforcement(THE_COLLECTIVE, camMakePos(camGenerateRandomMapCoordinate(startpos, 10, 1)), randomTemplates(droids, true),
		CAM_REINFORCE_TRANSPORT, {
			entry: camGenerateRandomMapEdgeCoordinate(),
			exit: camGenerateRandomMapEdgeCoordinate()
		}
	);
}

//NOTE: this is only called once from the library's eventMissionTimeout().
function checkIfLaunched()
{
	var transporters = enumArea(0, 0, mapWidth, mapHeight, CAM_HUMAN_PLAYER, false).filter(function(obj) {
		return (obj.type === DROID && camIsTransporter(obj));
	});
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
	camSetExtraObjectiveMessage(_("Send off at least one transporter with a truck and survive The Collective assault"));

	var lz = {x: 86, y: 99, x2: 88, y2: 101};
	var tCoords = {xStart: 87, yStart: 100, xOut: 0, yOut: 55};

	camSetStandardWinLossConditions(CAM_VICTORY_TIMEOUT, "CAM_3A", {
		reinforcements: camMinutesToSeconds(7), //Duration the transport "leaves" map.
		callback: "checkIfLaunched"
	});
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	camSetupTransporter(tCoords.xStart, tCoords.yStart, tCoords.xOut, tCoords.yOut);

	var enemyLz = {x: 49, y: 83, x2: 51, y2: 85};
	setNoGoArea(enemyLz.x, enemyLz.y, enemyLz.x2, enemyLz.y2, THE_COLLECTIVE);

	setMissionTime(camMinutesToSeconds(30));
	camCompleteRequiredResearch(COLLECTIVE_RES, THE_COLLECTIVE);

	allowWin = false;
	camPlayVideos([{video: "MB2_DII_MSG", type: CAMP_MSG}, {video: "MB2_DII_MSG2", type: MISS_MSG}]);

	vtolAttack();
	if (difficulty === INSANE)
	{
		setPower(playerPower(CAM_HUMAN_PLAYER) + 10000);
		setTimer("transporterAttack", camMinutesToMilliseconds(4));
	}
	if (difficulty >= HARD)
	{
		setTimer("tankAttackWest", camChangeOnDiff(camMinutesToMilliseconds(6)));
		setTimer("cyborgAttackRandom", camChangeOnDiff(camMinutesToMilliseconds(5)));
	}
	setTimer("cyborgAttack", camChangeOnDiff(camMinutesToMilliseconds(4)));
	setTimer("tankAttack", camChangeOnDiff(camMinutesToMilliseconds(3)));
	setTimer("checkEnemyVtolArea", camSecondsToMilliseconds(1));
}
