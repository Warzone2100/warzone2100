include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var allowWin;
const mis_collectiveRes = [
	"R-Defense-WallUpgrade06", "R-Struc-Materials06", "R-Sys-Engineering02",
	"R-Vehicle-Engine06", "R-Vehicle-Metals06", "R-Cyborg-Metals06",
	"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage06", "R-Wpn-Cannon-ROF03",
	"R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03", "R-Wpn-MG-Damage08",
	"R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02", "R-Wpn-Mortar-Damage06",
	"R-Wpn-Mortar-ROF03", "R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", "R-Wpn-RocketSlow-Damage06",
	"R-Sys-Sensor-Upgrade01", "R-Wpn-RocketSlow-ROF03", "R-Wpn-Howitzer-ROF03",
	"R-Wpn-Howitzer-Damage09", "R-Cyborg-Armor-Heat03", "R-Vehicle-Armor-Heat03",
	"R-Wpn-Bomb-Damage02", "R-Wpn-AAGun-Damage03", "R-Wpn-AAGun-ROF03",
	"R-Wpn-AAGun-Accuracy02", "R-Wpn-Howitzer-Accuracy02", "R-Struc-VTOLPad-Upgrade03",
];
const mis_Labels = {
	startPos: {x: 92, y: 99},
	lz: {x: 86, y: 99, x2: 88, y2: 101},
	lz2: {x: 49, y: 83, x2: 51, y2: 85},
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
		if ((vtols[i].weapons[0].armed < 20) || (vtols[i].health < 60))
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

//Return randomly selected droid templates.
function randomTemplates(list, transporterAmount, useWhirlwinds)
{
	const WHIRLWIND_AMOUNT = 2;
	const droids = [];
	let size;

	if (camDef(transporterAmount) && transporterAmount)
	{
		size = 8 + camRand(3);
	}
	else
	{
		size = (difficulty === INSANE) ? (15 + camRand(3)) : (18 + camRand(8));
	}

	for (let i = 0; i < size; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	if (useWhirlwinds)
	{
		// Include Whirlwinds for ground reinforcements.
		for (let i = 0; i < WHIRLWIND_AMOUNT; ++i)
		{
			droids.push(cTempl.cowwt);
		}
	}

	return droids;
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

	if (difficulty === INSANE)
	{
		vtolPositions = undefined; //to randomize the spawns each time
	}

	const list = [
		cTempl.commorv, cTempl.commorv, cTempl.comhvat, cTempl.commorvt
	];
	const extras = {
		minVTOLs: 4,
		maxRandomVTOLs: (difficulty >= HARD) ? 2 : 1
	};

	camSetVtolData(CAM_THE_COLLECTIVE, vtolPositions, mis_Labels.vtolRemovePos, list, camSecondsToMilliseconds(30), undefined, extras);
}

//SouthEast attackers which are mostly cyborgs.
function cyborgAttack()
{
	const list = [cTempl.npcybr, cTempl.cocybag, cTempl.npcybc, cTempl.comhltat, cTempl.cohhpv];

	camSendReinforcement(CAM_THE_COLLECTIVE, camMakePos(mis_Labels.southCyborgAssembly), randomTemplates(list, false, true), CAM_REINFORCE_GROUND, {
		data: { regroup: false, count: -1 }
	});
}

function cyborgAttackRandom()
{
	const list = [cTempl.npcybr, cTempl.cocybag, cTempl.npcybc, cTempl.npcybc, cTempl.comrotm]; //favor cannon cyborg

	camSendReinforcement(CAM_THE_COLLECTIVE, camMakePos(camGenerateRandomMapEdgeCoordinate(mis_Labels.startPos)), randomTemplates(list, false, true).concat(cTempl.comsens), CAM_REINFORCE_GROUND, {
		data: { regroup: false, count: -1 }
	});
}

//North road attacker consisting of powerful weaponry.
function tankAttack()
{
	const list = [cTempl.comhltat, cTempl.cohact, cTempl.cohhpv, cTempl.comagt];
	if (getMissionTime() < (60 * 22))
	{
		list.push(cTempl.cohbbt);
	}

	camSendReinforcement(CAM_THE_COLLECTIVE, camMakePos(mis_Labels.northTankAssembly), randomTemplates(list, false, true), CAM_REINFORCE_GROUND, {
		data: { regroup: false, count: -1, },
	});
}

function tankAttackWest()
{
	const list = [cTempl.comhltat, cTempl.cohact, cTempl.cohhpv, cTempl.comagt];
	if (getMissionTime() < (60 * 22))
	{
		list.push(cTempl.cohbbt);
	}

	camSendReinforcement(CAM_THE_COLLECTIVE, camMakePos(mis_Labels.westTankAssembly), randomTemplates(list, true, true), CAM_REINFORCE_GROUND, {
		data: { regroup: false, count: -1, },
	});
}

function transporterAttack()
{
	const droids = [cTempl.cohact, cTempl.comhltat, cTempl.cohhpv];
	if (getMissionTime() < (60 * 22))
	{
		droids.push(cTempl.cohbbt);
	}

	camSendReinforcement(CAM_THE_COLLECTIVE, camMakePos(camGenerateRandomMapCoordinate(mis_Labels.startPos, 10, 1)), randomTemplates(droids, true, false),
		CAM_REINFORCE_TRANSPORT, {
			entry: camGenerateRandomMapEdgeCoordinate(),
			exit: camGenerateRandomMapEdgeCoordinate()
		}
	);
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
	if (difficulty === HARD || difficulty === INSANE)
	{
		camSetExtraObjectiveMessage(_("Send off at least one transporter with a truck and survive The Collective assault until the timer ends"));
	}
	else
	{
		camSetExtraObjectiveMessage(_("Send off as many transporters as you can and bring at least one truck"));
	}

	camSetStandardWinLossConditions(CAM_VICTORY_TIMEOUT, "CAM_3A", {
		reinforcements: camMinutesToSeconds(7), //Duration the transport "leaves" map.
		callback: "checkIfLaunched"
	});

	camSetupTransporter(mis_Labels.trPlace.x, mis_Labels.trPlace.y, mis_Labels.trExit.x, mis_Labels.trExit.y);
	centreView(mis_Labels.startPos.x, mis_Labels.startPos.y);
	setNoGoArea(mis_Labels.lz.x, mis_Labels.lz.y, mis_Labels.lz.x2, mis_Labels.lz.y2, CAM_HUMAN_PLAYER);
	setNoGoArea(mis_Labels.lz2.x, mis_Labels.lz2.y, mis_Labels.lz2.x2, mis_Labels.lz2.y2, CAM_THE_COLLECTIVE);

	setMissionTime(camMinutesToSeconds(30));
	camCompleteRequiredResearch(mis_collectiveRes, CAM_THE_COLLECTIVE);

	allowWin = false;
	camPlayVideos([{video: "MB2_DII_MSG", type: CAMP_MSG}, {video: "MB2_DII_MSG2", type: MISS_MSG}]);

	queue("vtolAttack", camSecondsToMilliseconds(30));
	if (difficulty === INSANE)
	{
		setPower(playerPower(CAM_HUMAN_PLAYER) + 12000);
		setTimer("transporterAttack", camMinutesToMilliseconds(5));
	}
	if (difficulty >= HARD)
	{
		setTimer("tankAttackWest", camChangeOnDiff(camMinutesToMilliseconds(7)));
		setTimer("cyborgAttackRandom", camChangeOnDiff(camMinutesToMilliseconds(6)));
	}
	setTimer("cyborgAttack", camChangeOnDiff(camMinutesToMilliseconds(4)));
	setTimer("tankAttack", camChangeOnDiff(camMinutesToMilliseconds(3)));
	setTimer("checkEnemyVtolArea", camSecondsToMilliseconds(1));
}
