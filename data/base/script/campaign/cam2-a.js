include("script/campaign/transitionTech.js");
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const MIS_TRANSPORT_LIMIT = 4;
var transporterIndex; //Number of transport loads sent into the level
var startedFromMenu;

camAreaEvent("vtolRemoveZone", function(droid)
{
	camSafeRemoveObject(droid, false);
	resetLabel("vtolRemoveZone", CAM_THE_COLLECTIVE);
});

//Attack and destroy all those who resist the Machine! -The Collective
function secondVideo()
{
	camPlayVideos({video: "MB2A_MSG2", type: CAMP_MSG});
}

//Damage the base and droids for the player
function preDamageStuff()
{
	const droids = enumDroid(CAM_HUMAN_PLAYER);
	const structures = enumStruct(CAM_HUMAN_PLAYER);

	for (let x = 0; x < droids.length; ++x)
	{
		const droid = droids[x];
		if (!camIsTransporter(droid))
		{
			setHealth(droid, 45 + camRand(20));
		}
	}

	for (let x = 0; x < structures.length; ++x)
	{
		const struc = structures[x];
		setHealth(struc, 45 + camRand(45));
	}
}

function getDroidsForCOLZ()
{
	const droids = [];
	const sensors = [cTempl.comsens, cTempl.comsens];
	const COUNT = 6 + camRand(5);
	let templates;
	let usingHeavy = false;

	if (camRand(100) < 50)
	{
		templates = [cTempl.npcybm, cTempl.commgt, cTempl.npcybc, cTempl.npcybr];
	}
	else
	{
		templates = (!camClassicMode()) ? [cTempl.cohct, cTempl.commrl, cTempl.comorb] : [cTempl.cohct, cTempl.commc, cTempl.comorb];
		usingHeavy = true;
	}

	for (let i = 0; i < COUNT; ++i)
	{
		if (!i && usingHeavy)
		{
			droids.push(sensors[camRand(sensors.length)]); //bring a sensor
		}
		else
		{
			droids.push(templates[camRand(templates.length)]);
		}
	}

	return droids;
}

//Send Collective transport units
function sendCOTransporter()
{
	const tPos = getObject("COTransportPos");
	const nearbyDefense = enumRange(tPos.x, tPos.y, 15, CAM_THE_COLLECTIVE, false);

	if (nearbyDefense.length > 0)
	{
		const list = getDroidsForCOLZ();
		camSendReinforcement(CAM_THE_COLLECTIVE, camMakePos("COTransportPos"), list,
			CAM_REINFORCE_TRANSPORT, {
				entry: { x: 125, y: 100 },
				exit: { x: 125, y: 70 }
			}
		);
	}
	else
	{
		removeTimer("sendCOTransporter");
	}
}

//Extra transport units are only awarded to those who start Beta campaign
//from the main menu. Otherwise a player can just bring in there Alpha units
function sendPlayerTransporter()
{
	if (!camDef(transporterIndex))
	{
		transporterIndex = 0;
	}

	if (transporterIndex === MIS_TRANSPORT_LIMIT)
	{
		downTransporter();
		return;
	}

	const droids = [];
	const bodyList = [tBody.tank.mantis, tBody.tank.python];
	const propulsionList = [tProp.tank.tracks, tProp.tank.tracks, tProp.tank.hover];
	const weaponList = [
		tWeap.tank.heavyCannon, tWeap.tank.heavyCannon, tWeap.tank.heavyCannon,
		tWeap.tank.lancer, tWeap.tank.lancer, tWeap.tank.bombard, tWeap.tank.miniRocketArray
	];
	const specialList = [tSensor.sensor, tCommand.commander];
	const BODY = bodyList[camRand(bodyList.length)];
	const PROP = propulsionList[camRand(propulsionList.length)];

	for (let i = 0; i < 10; ++i)
	{
		const WEAP = (!transporterIndex && (i < specialList.length)) ? specialList[i] : weaponList[camRand(weaponList.length)];
		droids.push({ body: BODY, prop: PROP, weap: WEAP });
	}

	camSendReinforcement(CAM_HUMAN_PLAYER, camMakePos("landingZone"), droids,
		CAM_REINFORCE_TRANSPORT, {
			entry: { x: 87, y: 126 },
			exit: { x: 87, y: 126 }
		}
	);
}

//Continuously spawns heavy units on the north part of the map every 7 minutes
function mapEdgeDroids()
{
	const TANK_NUM = 8 + camRand(6);
	const list = (!camClassicMode()) ? [cTempl.npcybm, cTempl.npcybr, cTempl.commrp, cTempl.cohct] : [cTempl.npcybm, cTempl.npcybr, cTempl.commc, cTempl.cohct];

	const droids = [];
	for (let i = 0; i < TANK_NUM; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	camSendReinforcement(CAM_THE_COLLECTIVE, camMakePos("groundUnitPos"), droids, CAM_REINFORCE_GROUND);
}

function wave2()
{
	const list = [cTempl.colatv, cTempl.colatv];
	const ext = {
		limit: [3, 3], //paired with list array
		alternate: true,
		altIdx: 0
	};
	camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(4)), "COCommandCenter", ext);
}

function wave3()
{
	const list = [cTempl.colcbv, cTempl.colcbv];
	const ext = {
		limit: [2, 2], //paired with list array
		alternate: true,
		altIdx: 0
	};
	camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(4)), "COCommandCenter", ext);
}

function vtolAttack()
{
	if (camClassicMode())
	{
		const list = [cTempl.colcbv, cTempl.colcbv];
		camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(3)), "COCommandCenter");
	}
	else
	{
		const list = [cTempl.colpbv, cTempl.colpbv];
		const ext = {
			limit: [2, 2], //paired with list array
			alternate: true,
			altIdx: 0
		};
		camSetVtolData(CAM_THE_COLLECTIVE, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(4)), "COCommandCenter", ext);
		queue("wave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("wave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

function groupPatrol()
{
	camManageGroup(camMakeGroup("edgeGroup"), CAM_ORDER_ATTACK, {
		regroup: true,
		count: -1,
	});

	camManageGroup(camMakeGroup("IDFGroup"), CAM_ORDER_DEFEND, {
		pos: [
			camMakePos("waypoint1"),
			camMakePos("waypoint2")
		]
	});

	camManageGroup(camMakeGroup("sensorGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("waypoint1"),
			camMakePos("waypoint2")
		]
	});
}

//Build defenses around oil resource
function truckDefense()
{
	if (enumDroid(CAM_THE_COLLECTIVE, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	const defenses = ["CO-Tower-LtATRkt", "PillBox1", "CO-WallTower-HvCan"];
	camQueueBuilding(CAM_THE_COLLECTIVE, defenses[camRand(defenses.length)]);
}

//Gives starting tech and research.
function cam2Setup()
{
	const collectiveRes = [
		"R-Wpn-MG1Mk1", "R-Sys-Engineering02",
		"R-Defense-WallUpgrade04", "R-Struc-Materials04",
		"R-Vehicle-Engine03", "R-Vehicle-Metals03", "R-Cyborg-Metals03",
		"R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage04",
		"R-Wpn-Cannon-ROF01", "R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01",
		"R-Wpn-MG-Damage05", "R-Wpn-MG-ROF02", "R-Wpn-Mortar-Acc01",
		"R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-ROF01",
		"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage04",
		"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
		"R-Wpn-RocketSlow-Damage04", "R-Sys-Sensor-Upgrade01"
	];
	const collectiveResClassic = [
		"R-Defense-WallUpgrade03", "R-Struc-Materials03", "R-Struc-Factory-Upgrade03",
		"R-Vehicle-Engine03", "R-Vehicle-Metals03", "R-Cyborg-Metals03",
		"R-Vehicle-Armor-Heat01", "R-Cyborg-Armor-Heat01", "R-Cyborg-Armor-Heat01",
		"R-Wpn-Cannon-Damage03", "R-Wpn-Cannon-ROF01", "R-Wpn-Flamer-Damage03",
		"R-Wpn-Flamer-ROF01", "R-Wpn-MG-Damage04", "R-Wpn-MG-ROF02",
		"R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-ROF01", "R-Wpn-Rocket-Accuracy02",
		"R-Wpn-Rocket-Damage03", "R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
		"R-Wpn-RocketSlow-Damage03", "R-Sys-Sensor-Upgrade01"
	];

	for (let x = 0, l = mis_structsAlpha.length; x < l; ++x)
	{
		enableStructure(mis_structsAlpha[x], CAM_HUMAN_PLAYER);
	}

	if (camClassicMode())
	{
		camCompleteRequiredResearch(mis_alphaResearchNewClassic, CAM_HUMAN_PLAYER);
		camCompleteRequiredResearch(mis_playerResBetaClassic, CAM_HUMAN_PLAYER);

		if (tweakOptions.camClassic_balance32)
		{
			camClassicResearch(mis_betaStartingResearchClassic, CAM_HUMAN_PLAYER);
			completeResearch("CAM2RESEARCH-UNDO", CAM_HUMAN_PLAYER);
			//The Collective have no research in 3.2
		}
		else
		{
			completeResearch("CAM2RESEARCH-UNDO-Rockets", CAM_HUMAN_PLAYER);
			camCompleteRequiredResearch(mis_betaStartingResearchClassic, CAM_HUMAN_PLAYER);
			camCompleteRequiredResearch(mis_alphaResearchNewClassic, CAM_THE_COLLECTIVE);
			camCompleteRequiredResearch(collectiveResClassic, CAM_THE_COLLECTIVE);
		}

		enableResearch("R-Wpn-Cannon-Accuracy02", CAM_HUMAN_PLAYER);
	}
	else
	{
		camCompleteRequiredResearch(mis_playerResBeta, CAM_HUMAN_PLAYER);
		camCompleteRequiredResearch(mis_alphaResearchNew, CAM_THE_COLLECTIVE);
		camCompleteRequiredResearch(collectiveRes, CAM_THE_COLLECTIVE);
		camCompleteRequiredResearch(mis_alphaResearchNew, CAM_HUMAN_PLAYER);

		if (difficulty >= HARD)
		{
			camUpgradeOnMapTemplates(cTempl.commc, cTempl.commrp, CAM_THE_COLLECTIVE);
		}

		enableResearch("R-Wpn-Cannon-Damage04", CAM_HUMAN_PLAYER);
		enableResearch("R-Wpn-Rocket-Damage04", CAM_HUMAN_PLAYER);
	}

	preDamageStuff();
}

//Get some higher rank droids.
function setUnitRank(transport)
{
	const droidExp = [128, 64, 32, 16];
	let droids;
	let mapRun = false;

	if (transport)
	{
		droids = enumCargo(transport);
	}
	else
	{
		mapRun = true;
		//These are the units in the base already at the start.
		droids = enumDroid(CAM_HUMAN_PLAYER).filter((dr) => (!camIsTransporter(dr)));
	}

	for (let i = 0, len = droids.length; i < len; ++i)
	{
		const droid = droids[i];
		if (droid.droidType !== DROID_CONSTRUCT && droid.droidType !== DROID_REPAIR)
		{
			let mod = 1;
			if (droid.droidType === DROID_COMMAND || droid.droidType === DROID_SENSOR)
			{
				if (camClassicMode())
				{
					mod = 4;
				}
				else
				{
					mod = 8;
				}
			}
			setDroidExperience(droid, mod * droidExp[mapRun ? 0 : (transporterIndex - 1)]);
		}
	}
}

//Bump the rank of the first batch of transport droids as a reward.
function eventTransporterLanded(transport)
{
	if (transport.player === CAM_HUMAN_PLAYER)
	{
		if (!camDef(transporterIndex))
		{
			transporterIndex = 0;
		}

		transporterIndex += 1;

		if (startedFromMenu)
		{
			setUnitRank(transport);
		}

		if (transporterIndex >= MIS_TRANSPORT_LIMIT)
		{
			queue("downTransporter", camMinutesToMilliseconds(1));
		}
	}
}

//Warn that something bad happened to the fifth transport
function reallyDownTransporter()
{
	if (startedFromMenu)
	{
		removeTimer("sendPlayerTransporter");
	}
	setReinforcementTime(LZ_COMPROMISED_TIME);
	playSound(cam_sounds.transport.transportUnderAttack);
}

function downTransporter()
{
	camCallOnce("reallyDownTransporter");
}

function eventTransporterLaunch(transport)
{
	if (transporterIndex >= MIS_TRANSPORT_LIMIT)
	{
		queue("downTransporter", camMinutesToMilliseconds(1));
	}
}

function eventGameLoaded()
{
	if (transporterIndex >= MIS_TRANSPORT_LIMIT)
	{
		setReinforcementTime(LZ_COMPROMISED_TIME);
	}
}

function eventStartLevel()
{
	const PLAYER_POWER = 5000;
	const startPos = getObject("startPosition");
	const lz = getObject("landingZone"); //player lz
	const tEnt = getObject("transporterEntry");
	const tExt = getObject("transporterExit");

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_2_1S");
	setReinforcementTime(LZ_COMPROMISED_TIME);

	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);

	camSetArtifacts({
		"COCommandCenter": { tech: "R-Sys-Engineering02" },
		"COArtiPillbox": { tech: "R-Wpn-MG-ROF02" },
		"COArtiCBTower": { tech: "R-Sys-Sensor-Upgrade01" },
	});

	setMissionTime(camChangeOnDiff(camHoursToSeconds(1)));
	setPower(PLAYER_POWER, CAM_HUMAN_PLAYER);
	cam2Setup();

	//C2A_BASE2 is not really a base
	camSetEnemyBases({
		"CONorthBase": {
			cleanup: "CONorth",
			detectMsg: "C2A_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"CONorthWestBase": {
			cleanup: "CONorthWest",
			detectMsg: "C2A_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camManageTrucks(CAM_THE_COLLECTIVE);
	setUnitRank(); //All pre-placed player droids are ranked.
	camPlayVideos({video: "MB2A_MSG", type: MISS_MSG});
	startedFromMenu = false;

	//Only if starting Beta directly rather than going through Alpha
	if (enumDroid(CAM_HUMAN_PLAYER, DROID_SUPERTRANSPORTER).length === 0)
	{
		startedFromMenu = true;
		sendPlayerTransporter();
		setTimer("sendPlayerTransporter", camMinutesToMilliseconds(5));
	}
	else
	{
		setReinforcementTime(camMinutesToSeconds(5)); // 5 min.
	}

	queue("secondVideo", camSecondsToMilliseconds(12));
	queue("groupPatrol", camChangeOnDiff(camMinutesToMilliseconds(1)));
	queue("vtolAttack", camChangeOnDiff(camMinutesToMilliseconds(3)));
	setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(3)));
	setTimer("sendCOTransporter", camChangeOnDiff(camMinutesToMilliseconds(4)));
	setTimer("mapEdgeDroids", camChangeOnDiff(camMinutesToMilliseconds(7)));

	truckDefense();
}
