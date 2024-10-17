include("script/campaign/transitionTech.js");
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var transporterIndex; //Number of bonus transports that have flown in.
var startedFromMenu;
var truckLocCounter;

//Remove Nexus VTOL droids.
camAreaEvent("vtolRemoveZone", function(droid)
{
	if (droid.player !== CAM_HUMAN_PLAYER && camVtolCanDisappear(droid))
	{
		camSafeRemoveObject(droid, false);
	}
	resetLabel("vtolRemoveZone", CAM_NEXUS);
});

//Order base three groups to do stuff and enable cyborg factories in the north
camAreaEvent("northFactoryTrigger", function(droid)
{
	improveNexusAlloys();
	camCallOnce("insaneSetupSpawns");
	camEnableFactory("NXcybFac-b3");
	camEnableFactory("NXcybFac-b4");

	camManageGroup(camMakeGroup("NEAttackerGroup"), CAM_ORDER_ATTACK, {
		regroup: true,
		morale: 90,
		fallback: camMakePos("SWBaseRetreat")
	});

	camManageGroup(camMakeGroup("NEDefenderGroup"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("genericasAssembly"),
			camMakePos("northFacAssembly"),
		],
		interval: camMinutesToMilliseconds(1),
		regroup: true,
	});
});

//Enable factories in the SW base
camAreaEvent("westFactoryTrigger", function(droid)
{
	camCallOnce("enableWestFactories");
});

//Prevents VTOLs from flying over the west trigger
camAreaEvent("westFactoryVTOLTrigger", function(droid)
{
	camCallOnce("enableWestFactories");
});

//Enable all factories if the player tries to bypass a trigger area
camAreaEvent ("middleTrigger", function(droid)
{
	improveNexusAlloys();
	enableAllFactories();
	camCallOnce("insaneSetupSpawns");
});

function insaneSetupSpawns()
{
	if (difficulty >= INSANE)
	{
		setTimer("insaneTransporterAttack", camMinutesToMilliseconds(4.5));
		setTimer("insaneReinforcementSpawn", camMinutesToMilliseconds(5.5));
	}
}

function enableWestFactories()
{
	improveNexusAlloys();
	camCallOnce("insaneSetupSpawns");
	camEnableFactory("NXcybFac-b2-1");
	camEnableFactory("NXcybFac-b2-2");
	camEnableFactory("NXHvyFac-b2");
}

function setUnitRank(transport)
{
	const droidExp = [
		camGetRankThreshold("hero", true), //Can make Hero Commanders if recycled.
		camGetRankThreshold("special"),
		camGetRankThreshold("elite"),
		camGetRankThreshold("veteran")
	];
	const droids = enumCargo(transport);

	for (let i = 0, len = droids.length; i < len; ++i)
	{
		const droid = droids[i];
		if (droid.droidType !== DROID_CONSTRUCT && droid.droidType !== DROID_REPAIR)
		{
			setDroidExperience(droid, droidExp[transporterIndex - 1]);
		}
	}
}

function eventTransporterLanded(transport)
{
	if (startedFromMenu)
	{
		setUnitRank(transport);
	}
}

//Enable all factories.
function enableAllFactories()
{
	const factoryNames = [
		"NXcybFac-b3", "NXcybFac-b2-1", "NXcybFac-b2-2", "NXHvyFac-b2", "NXcybFac-b4",
	];

	camCallOnce("insaneSetupSpawns");
	for (let j = 0, i = factoryNames.length; j < i; ++j)
	{
		camEnableFactory(factoryNames[j]);
	}
}

function truckDefense()
{
	if (enumDroid(CAM_NEXUS, DROID_CONSTRUCT).length === 0)
	{
		removeTimer("truckDefense");
		return;
	}

	const list = ["Emplacement-Howitzer150", "NX-Emp-MedArtMiss-Pit", "Emplacement-RotHow"];
	let position;

	if (truckLocCounter === 0)
	{
		position = camMakePos("buildPos1");
		truckLocCounter += 1;
	}
	else
	{
		position = camMakePos("buildPos2");
		truckLocCounter = 0;
	}

	camQueueBuilding(CAM_NEXUS, list[camRand(list.length)], position);
}

//Extra transport units are only awarded to those who start Gamma campaign
//from the main menu.
function sendPlayerTransporter()
{
	const transportLimit = 4; //Max of four transport loads if starting from menu.
	if (!camDef(transporterIndex))
	{
		transporterIndex = 0;
	}

	if (transporterIndex === transportLimit)
	{
		removeTimer("sendPlayerTransporter");
		return;
	}

	const droids = [];
	const bodyList = [tBody.tank.tiger, tBody.tank.tiger, tBody.tank.python, tBody.tank.mantis];
	const propulsionList = [tProp.tank.hover, tProp.tank.hover, tProp.tank.tracks];
	const weaponList = [
		tWeap.tank.assaultCannon, tWeap.tank.assaultCannon, tWeap.tank.inferno,
		tWeap.tank.inferno, tWeap.tank.assaultGun, tWeap.tank.assaultGun,
		tWeap.tank.hyperVelocityCannon, tWeap.tank.tankKiller
	];
	const specialList = [tConstruct.truck, tConstruct.truck, tCommand.commander, tCommand.commander];
	const BODY = bodyList[camRand(bodyList.length)];
	const PROP = propulsionList[camRand(propulsionList.length)];

	for (let i = 0; i < 10; ++i)
	{
		let prop = PROP;
		let weap = (!transporterIndex && (i < specialList.length)) ? specialList[i] : weaponList[camRand(weaponList.length)];
		if (transporterIndex === 1 && i < 4)
		{
			weap = tWeap.tank.whirlwind; //Bring 4 Whirlwinds on the 2nd transport.
		}
		if (BODY === tBody.tank.mantis)
		{
			prop = tProp.tank.tracks; //Force Mantis to use Tracks.
		}
		if (weap === tConstruct.truck)
		{
			prop = tProp.tank.hover; //Force trucks to use Hover.
		}
		droids.push({ body: BODY, prop: prop, weap: weap });
	}

	camSendReinforcement(CAM_HUMAN_PLAYER, camMakePos("landingZone"), droids,
		CAM_REINFORCE_TRANSPORT, {
			entry: { x: 63, y: 118 },
			exit: { x: 63, y: 118 }
		}
	);

	transporterIndex += 1;
}

function wave2()
{
	const CONDITION = ((difficulty >= INSANE) ? CAM_REINFORCE_CONDITION_ARTIFACTS : "NXCommandCenter");
	const list = [cTempl.nxlscouv, cTempl.nxlscouv];
	const ext = {limit: [2, 2], alternate: true, altIdx: 0};
	camSetVtolData(CAM_NEXUS, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(5)), CONDITION, ext);
}

function wave3()
{
	const CONDITION = ((difficulty >= INSANE) ? CAM_REINFORCE_CONDITION_ARTIFACTS : "NXCommandCenter");
	const list = [cTempl.nxlneedv, cTempl.nxlneedv];
	const ext = {limit: [3, 3], alternate: true, altIdx: 0};
	camSetVtolData(CAM_NEXUS, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(5)), CONDITION, ext);
}

//Setup Nexus VTOL hit and runners.
function vtolAttack()
{
	const CONDITION = ((difficulty >= INSANE) ? CAM_REINFORCE_CONDITION_ARTIFACTS : "NXCommandCenter");
	if (camClassicMode())
	{
		const list = [cTempl.nxlscouv, cTempl.nxmtherv, cTempl.nxlneedv, cTempl.nxlscouv];
		const ext = {limit: [2, 4, 2, 2], alternate: true, altIdx: 0};
		camSetVtolData(CAM_NEXUS, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(5)), CONDITION, ext);
	}
	else
	{
		const list = [cTempl.nxmtherv, cTempl.nxmtherv];
		const ext = {limit: [2, 2], alternate: true, altIdx: 0};
		camSetVtolData(CAM_NEXUS, "vtolAppearPos", "vtolRemoveZone", list, camChangeOnDiff(camMinutesToMilliseconds(5)), CONDITION, ext);
		queue("wave2", camChangeOnDiff(camSecondsToMilliseconds(30)));
		queue("wave3", camChangeOnDiff(camSecondsToMilliseconds(60)));
	}
}

//These groups are active immediately.
function groupPatrolNoTrigger()
{
	camManageGroup(camMakeGroup("cybAttackers"), CAM_ORDER_ATTACK, {
		pos: [
			camMakePos("northFacAssembly"),
			camMakePos("ambushPlayerPos"),
		],
		regroup: true,
		count: -1,
		morale: 90,
		fallback: camMakePos("healthRetreatPos")
	});

	camManageGroup(camMakeGroup("hoverPatrolGrp"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("hoverGrpPos1"),
			camMakePos("hoverGrpPos2"),
			camMakePos("hoverGrpPos3"),
		]
	});

	camManageGroup(camMakeGroup("cybValleyPatrol"), CAM_ORDER_PATROL, {
		pos: [
			camMakePos("southWestBasePatrol"),
			camMakePos("westEntrancePatrol"),
			camMakePos("playerLZPatrol"),
		]
	});

	camManageGroup(camMakeGroup("NAmbushCyborgs"), CAM_ORDER_ATTACK);
}

function insaneReinforcementSpawn()
{
	const units = [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxmscouh, cTempl.nxmrailh];
	const limits = {minimum: 4, maxRandom: 2};
	const location = ["northSpawnPos", "southSpawnPos"];
	camSendGenericSpawn(CAM_REINFORCE_GROUND, CAM_NEXUS, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

function insaneTransporterAttack()
{
	const units = [cTempl.nxcyrail, cTempl.nxcyscou, cTempl.nxmscouh, cTempl.nxmrailh];
	const limits = {minimum: 4, maxRandom: 2};
	const location = camMakePos("NexusTransporterLandingPos");
	camSendGenericSpawn(CAM_REINFORCE_TRANSPORT, CAM_NEXUS, CAM_REINFORCE_CONDITION_ARTIFACTS, location, units, limits.minimum, limits.maxRandom);
}

//Gives starting tech and research.
function cam3Setup()
{
	const nexusRes = [
		"R-Sys-Engineering03", "R-Defense-WallUpgrade07", "R-Struc-Materials07",
		"R-Struc-VTOLPad-Upgrade06", "R-Wpn-Bomb-Damage03", "R-Sys-NEXUSrepair",
		"R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02", "R-Cyborg-Legs02",
		"R-Wpn-Mortar-Acc03", "R-Wpn-MG-Damage09", "R-Wpn-Mortar-ROF04",
		"R-Vehicle-Engine07", "R-Vehicle-Metals06", "R-Vehicle-Armor-Heat03",
		"R-Cyborg-Metals06", "R-Cyborg-Armor-Heat03", "R-Wpn-RocketSlow-ROF04",
		"R-Wpn-AAGun-Damage05", "R-Wpn-AAGun-ROF04", "R-Wpn-Howitzer-Damage09",
		"R-Wpn-Cannon-Damage07", "R-Wpn-Cannon-ROF04",
		"R-Wpn-Missile-Damage01", "R-Wpn-Missile-ROF01", "R-Wpn-Missile-Accuracy01",
		"R-Wpn-Rail-Damage01", "R-Wpn-Rail-ROF01", "R-Wpn-Rail-Accuracy01",
		"R-Wpn-Energy-Damage02", "R-Wpn-Energy-ROF01", "R-Wpn-Energy-Accuracy01",
		"R-Sys-NEXUSsensor",
	];
	const nexusResClassic = [
		"R-Defense-WallUpgrade07", "R-Struc-Materials07", "R-Struc-Factory-Upgrade06",
		"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals07",
		"R-Cyborg-Metals07", "R-Vehicle-Armor-Heat03", "R-Cyborg-Armor-Heat03",
		"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
		"R-Wpn-Bomb-Damage03", "R-Wpn-Missile-Damage01", "R-Wpn-Missile-ROF01",
		"R-Sys-Sensor-Upgrade01", "R-Sys-NEXUSrepair", "R-Wpn-Rail-Damage01",
		"R-Wpn-Rail-ROF01", "R-Wpn-Flamer-Damage06", "R-Sys-NEXUSsensor",
	];

	for (let x = 0, l = mis_structsAlpha.length; x < l; ++x)
	{
		enableStructure(mis_structsAlpha[x], CAM_HUMAN_PLAYER);
	}

	if (camClassicMode())
	{
		camCompleteRequiredResearch(mis_alphaResearchNewClassic, CAM_HUMAN_PLAYER);
		camCompleteRequiredResearch(mis_betaResearchNewClassic, CAM_HUMAN_PLAYER);
		camCompleteRequiredResearch(mis_playerResBetaClassic, CAM_HUMAN_PLAYER);
		camCompleteRequiredResearch(mis_playerResGammaClassic, CAM_HUMAN_PLAYER);

		if (tweakOptions.camClassic_balance32)
		{
			camClassicResearch(mis_gammaStartingResearchClassic, CAM_HUMAN_PLAYER);
			completeResearch("CAM2RESEARCH-UNDO", CAM_HUMAN_PLAYER);
			completeResearch("CAM3RESEARCH-UNDO", CAM_HUMAN_PLAYER);
			//Nexus only has auto-repair and the NavGunSensor for 3.2
			camClassicResearch(cam_nexusSpecialResearch, CAM_NEXUS);
		}
		else
		{
			camCompleteRequiredResearch(mis_gammaStartingResearchClassic, CAM_HUMAN_PLAYER);
			camCompleteRequiredResearch(mis_alphaResearchNewClassic, CAM_NEXUS);
			camCompleteRequiredResearch(mis_betaResearchNewClassic, CAM_NEXUS);
			camCompleteRequiredResearch(nexusResClassic, CAM_NEXUS);
		}

		enableResearch("R-Wpn-Howitzer03-Rot", CAM_HUMAN_PLAYER);
		enableResearch("R-Wpn-MG-Damage08", CAM_HUMAN_PLAYER);
	}
	else
	{
		camCompleteRequiredResearch(mis_gammaAllyRes, CAM_HUMAN_PLAYER);
		camCompleteRequiredResearch(mis_gammaAllyRes, CAM_NEXUS);
		camCompleteRequiredResearch(nexusRes, CAM_NEXUS);

		if (difficulty >= HARD)
		{
			improveNexusAlloys();
		}

		enableResearch("R-Wpn-Howitzer03-Rot", CAM_HUMAN_PLAYER);
		enableResearch("R-Wpn-MG-Damage09", CAM_HUMAN_PLAYER);
		enableResearch("R-Wpn-Flamer-ROF04", CAM_HUMAN_PLAYER);
		enableResearch("R-Defense-WallUpgrade07", CAM_HUMAN_PLAYER);
	}
}

//Normal and lower difficulties has Nexus start off a little bit weaker
function improveNexusAlloys()
{
	if (camClassicMode())
	{
		return;
	}
	const alloys = [
		"R-Vehicle-Metals07", "R-Cyborg-Metals07",
		"R-Vehicle-Armor-Heat04", "R-Cyborg-Armor-Heat04"
	];
	camCompleteRequiredResearch(alloys, CAM_NEXUS);
}

function eventStartLevel()
{
	const PLAYER_POWER = 16000;
	const startPos = getObject("startPosition");
	const lz = getObject("landingZone");
	const tEnt = getObject("transporterEntry");
	const tExt = getObject("transporterExit");

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, cam_levels.gamma2.pre);
	setMissionTime(camChangeOnDiff(camHoursToSeconds(2)));

	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	startTransporterEntry(tEnt.x, tEnt.y, CAM_HUMAN_PLAYER);
	setTransporterExit(tExt.x, tExt.y, CAM_HUMAN_PLAYER);

	camSetArtifacts({
		"NXPowerGenArti": { tech: "R-Struc-Power-Upgrade02" },
		"NXResearchLabArti": { tech: "R-Sys-Engineering03" },
	});

	setPower(PLAYER_POWER, CAM_HUMAN_PLAYER);
	cam3Setup();

	camSetEnemyBases({
		"NEXUS-WBase": {
			cleanup: "westBaseCleanup",
			detectMsg: "CM3A_BASE1",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NEXUS-SWBase": {
			cleanup: "southWestBaseCleanup",
			detectMsg: "CM3A_BASE2",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NEXUS-NEBase": {
			cleanup: "northEastBaseCleanup",
			detectMsg: "CM3A_BASE3",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
		"NEXUS-NWBase": {
			cleanup: "northWestBaseCleanup",
			detectMsg: "CM3A_BASE4",
			detectSnd: cam_sounds.baseDetection.enemyBaseDetected,
			eliminateSnd: cam_sounds.baseElimination.enemyBaseEradicated,
		},
	});

	camSetFactories({
		"NXcybFac-b3": {
			assembly: "NXcybFac-b3Assembly",
			order: CAM_ORDER_ATTACK,
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(50)),
			group: camMakeGroup("NEAttackerGroup"),
			templates: [cTempl.nxcyrail, cTempl.nxcyscou]
		},
		"NXcybFac-b2-1": {
			assembly: "NXcybFac-b2-1Assembly",
			order: CAM_ORDER_ATTACK,
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(40)),
			group: camMakeGroup("cybAttackers"),
			templates: [cTempl.nxcyrail, cTempl.nxcyscou]
		},
		"NXcybFac-b2-2": {
			assembly: "NXcybFac-b2-2Assembly",
			order: CAM_ORDER_PATROL,
			data: {
				pos: [
					camMakePos("southWestBasePatrol"),
					camMakePos("westEntrancePatrol"),
					camMakePos("playerLZPatrol"),
				],
				regroup: false,
				repair: 40,
				count: -1,
			},
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(60)),
			group: camMakeGroup("cybValleyPatrol"),
			templates: [cTempl.nxcyrail, cTempl.nxcyscou]
		},
		"NXHvyFac-b2": {
			assembly: "NXHvyFac-b2Assembly",
			order: CAM_ORDER_PATROL,
			data: {
				pos: [
					camMakePos("hoverGrpPos1"),
					camMakePos("hoverGrpPos2"),
					camMakePos("hoverGrpPos3"),
				],
				regroup: false,
				repair: 45,
				count: -1,
			},
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(70)),
			group: camMakeGroup("hoverPatrolGrp"),
			templates: [cTempl.nxmscouh]
		},
		"NXcybFac-b4": {
			assembly: "NXcybFac-b4Assembly",
			order: CAM_ORDER_ATTACK,
			data: {
				regroup: false,
				repair: 40,
				count: -1,
			},
			groupSize: 4,
			throttle: camChangeOnDiff(camSecondsToMilliseconds(55)),
			templates: [cTempl.nxcyrail, cTempl.nxcyscou]
		},
	});

	if (difficulty >= HARD)
	{
		addDroid(CAM_NEXUS, 8, 112, "Truck Retribution Hover", tBody.tank.retribution, tProp.tank.hover2, "", "", tConstruct.truck);
		camManageTrucks(CAM_NEXUS, false);
		setTimer("truckDefense", camChangeOnDiff(camMinutesToMilliseconds(4.5)));
	}

	camPlayVideos([{video: "MB3A_MSG", type: CAMP_MSG}, {video: "MB3A_MSG2", type: MISS_MSG}]);
	startedFromMenu = false;
	truckLocCounter = 0;

	//Only if starting Gamma directly rather than going through Beta
	if (enumDroid(CAM_HUMAN_PLAYER, DROID_SUPERTRANSPORTER).length === 0)
	{
		startedFromMenu = true;
		setReinforcementTime(LZ_COMPROMISED_TIME);
		sendPlayerTransporter();
		setTimer("sendPlayerTransporter", camMinutesToMilliseconds(5));
	}
	else
	{
		setReinforcementTime(camMinutesToSeconds(5));
	}

	groupPatrolNoTrigger();
	queue("vtolAttack", camChangeOnDiff(camMinutesToMilliseconds(8)));
	queue("enableAllFactories", camChangeOnDiff(camMinutesToMilliseconds(20)));
	queue("improveNexusAlloys", camChangeOnDiff(camMinutesToMilliseconds(25)));
}
