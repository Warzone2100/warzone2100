
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const CO = 2; //The Collective player number
var transportLimit, index; //Player transports limits for this level


camAreaEvent("vtolRemoveZone", function(droid)
{
	camSafeRemoveObject(droid, false);
	resetLabel("vtolRemoveZone", CO);
});

//Attack and destroy all those who resist the Machine! -The Collective
function secondVideo()
{
	hackAddMessage("MB2A_MSG2", MISS_MSG, CAM_HUMAN_PLAYER, true);
}

//Damage the base and droids for the player
function preDamageStuff()
{
	var droids = enumDroid(CAM_HUMAN_PLAYER);
	var structures = enumStruct(CAM_HUMAN_PLAYER);

	for(var x = 0; x < droids.length; ++x)
	{
		var droid = droids[x];
		setHealth(droid, 45 + camRand(20));
	}

	for(var x = 0; x < structures.length; ++x)
	{
		var struc = structures[x];
		setHealth(struc, 45 + camRand(45));
	}
}

function getDroidsForCOLZ()
{
	const count = [3,3,3,4,4,4,4,4,4][camRand(9)];
	var t;
	var scouts;
	var artillery;
	with (camTemplates) scouts = [npcybm, npcybc, commgt, comsens];
	with (camTemplates) artillery = [npcybc, npcybr, cohct, comct, comorb];

	var droids = [];
	for (var i = 0; i < count; ++i)
	{
		if(camRand(3) === 0)
			t = scouts[camRand(scouts.length)];
		else
			t = artillery[camRand(artillery.length)];
		droids[droids.length] = t;
		droids[droids.length] = t;
	}
	return droids;
}

//Send Collective tranport units
function sendCOTransporter()
{
	var list = getDroidsForCOLZ();
	camSendReinforcement(CO, camMakePos("COTransportPos"), list, 
		CAM_REINFORCE_TRANSPORT, {
			entry: { x: 126, y: 100 },
			exit: { x: 126, y: 70 }
		}
	);

	queue("sendCOTransporter", 300000); //5 min
}

//Extra transport units are only awarded to those who start Beta campaign
//from the main menu. Otherwise a player can just bring in there Alpha units
function sendPlayerTransporter()
{
	if(!camDef(transportLimit) || index === transportLimit)
		return;

	var list
	with (camTemplates) list = [prhct, prltat, npcybr, prrept];
	var droids = [];

	for(var i = 0; i < 10; ++i)
		droids[droids.length] = list[camRand(list.length)];
	
	camSendReinforcement(CAM_HUMAN_PLAYER, camMakePos("landingZone"), droids, 
		CAM_REINFORCE_TRANSPORT, {
			entry: { x: 87, y: 126 },
			exit: { x: 87, y: 126 }
		}
	);

	index += 1;
	queue("sendPlayerTransporter", 300000); //5 min
}

//Continuously spawns heavy units on the north part of the map every 7 minutes
function mapEdgeDroids()
{
	const TankNum = 8 + camRand(6);
	var list
	with (camTemplates) list = [npcybm, npcybr, comct, cohct];

	var droids = [];
	for (var i = 0; i <= TankNum; ++i)
	{
		droids[droids.length] = list[camRand(list.length)];
	}

	camSendReinforcement(CO, camMakePos("groundUnitPos"), droids, CAM_REINFORCE_GROUND);
	queue("mapEdgeDroids", 420000) //7 min
}

function removeVtols()
{
	camRetreatVtols(CO, "vtolRemoveZone");
	queue("removeVtols", 2000);
}

function vtolAttack()
{
	var list; with (camTemplates) list = [colcbv, colatv];
	camSpawnVtols(CO, "vtolAppearPos", list, 5, 300000); //5 min
	queue("vtolAttack", 300000);
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
	var truck = enumDroid(CO, DROID_CONSTRUCT);
	if(enumDroid(CO, DROID_CONSTRUCT).length > 0)
		queue("truckDefense", 160000);

	const list = ["WallTower06", "PillBox1", "WallTower03"];
	camQueueBuilding(CO, list[camRand(list.length)]);
}

//Gives starting tech and research.
function cam2Setup()
{
	const alphaTech = [
		"MG1Mk1", "Cannon1Mk1", "Cannon2A-TMk1", "Cannon375mmMk1", "Flame1Mk1",
		"MG2Mk1", "MG3Mk1", "Mortar1Mk1", "Mortar2Mk1", "Rocket-BB", "Rocket-LtA-T",
		"Rocket-MRL", "Rocket-Pod", "Cannon1-VTOL", "MG1-VTOL", "MG2-VTOL",
		"MG3-VTOL", "Rocket-VTOL-LtA-T", "Rocket-VTOL-Pod", "Rocket-VTOL-BB",
		"CyborgFlamer01", "CyborgCannon", "CyborgChaingun", "Cyb-Wpn-Atmiss",
		"Cyb-Wpn-Laser", "Cyb-Wpn-Rail1", "CyborgRocket", "CyborgRotMG",
		"CommandTurret1", "Body1REC", "Body5REC", "Body11ABT", "Body4ABT",
		"Body8MBT", "Body12SUP", "CyborgCannonGrd", "CyborgFlamerGrd", 
		"CyborgChain1Ground", "CyborgRkt1Ground", "HalfTrack", "hover01",
		"tracked01", "wheeled01", "Spade1Mk1", "SensorTurret1Mk1", 
		"CommandBrain01" ];
	const structs = [
		"A0CommandCentre", "A0PowerGenerator", "A0ResourceExtractor",
		"A0ResearchFacility", "A0LightFactory", "A0ComDroidControl",
		"A0CyborgFactory", "A0FacMod1", "A0HardcreteMk1CWall",
		"A0HardcreteMk1Wall", "A0PowMod1", "A0RepairCentre3", 
		"A0ResearchModule1", "A0TankTrap", "PillBox1", "PillBox4",
		"PillBox5", "PillBox6", "TankTrapC", "WallTower01",
		"WallTower03", "WallTower04", "WallTower06", "AASite-QuadMg1",
		"Emplacement-MortarPit01", "Emplacement-MRL-pit" ];
	const playerRes = [ 
		"R-Cyborg-Legs01", "R-Cyborg-Metals03", "R-Defense-WallUpgrade03",
		"R-Struc-Factory-Upgrade03", "R-Struc-Factory-Cyborg-Upgrade03",
		"R-Struc-Materials03", "R-Struc-Research-Upgrade03",
		"R-Struc-RprFac-Upgrade03", "R-Sys-Engineering01", "R-Wpn-MG-ROF01",
		"R-Sys-MobileRepairTurret01", "R-Wpn-Cannon-Damage03",
		"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01", "R-Wpn-MG-Damage04",
		"R-Wpn-Mortar-Damage03", "R-Wpn-Rocket-Accuracy02",
		"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy02", 
		"R-Wpn-RocketSlow-Damage03", "R-Vehicle-Engine03", 
		"R-Vehicle-Metals03", "R-Wpn-AAGun03", "R-Defense-AASite-QuadMg1",
		"R-Sys-Sensor-Tower02", "R-Defense-MRL" ];
	const collectiveRes = [
		"R-Defense-WallUpgrade03", "R-Struc-Materials03",
		"R-Struc-Factory-Upgrade03", "R-Struc-Factory-Cyborg-Upgrade03",
		"R-Vehicle-Engine03", "R-Vehicle-Metals03", "R-Cyborg-Metals03", 
		"R-Vehicle-Armor-Heat01", "R-Cyborg-Armor-Heat01", 
		"R-Wpn-Cannon-Accuracy01", "R-Wpn-Cannon-Damage03", 
		"R-Wpn-Cannon-ROF01", "R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01",
		"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF02", "R-Wpn-Mortar-Acc01",
		"R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-ROF01", 
		"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage03", 
		"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03", 
		"R-Wpn-RocketSlow-Damage03", "R-Sys-Sensor-Upgrade01" ];

	for( var x = 0; x < alphaTech.length; ++x)
		makeComponentAvailable(alphaTech[x], me);

	for( var x = 0; x < structs.length; ++x)
		enableStructure(structs[x], me);

	camEnableRes(playerRes, CAM_HUMAN_PLAYER);
	camEnableRes(collectiveRes, CO);
	enableResearch("R-Wpn-Cannon-Accuracy02", me);
	preDamageStuff();
}


function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_2_1S");
	setReinforcementTime(LZ_COMPROMISED_TIME);

	var startpos = getObject("startPosition");
	centreView(startpos.x, startpos.y);
	var lz = getObject("landingZone"); //player lz
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	lz = getObject("COLandingZone");
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CO);
	var tent = getObject("transporterEntry");
	startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
	var text = getObject("transporterExit");
	setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);

	camSetArtifacts({
		"COCommandCenter": { tech: "R-Sys-Engineering02" },
		"COArtiPillbox": { tech: "R-Wpn-MG-ROF02" },
		"COArtiCBTower": { tech: "R-Sys-Sensor-Upgrade01" },
	});

	setMissionTime(3600); //60 min
	setPower(10000, CO);
	setPower(5000, CAM_HUMAN_PLAYER);
	cam2Setup();

	//C2A_BASE2 is not really a base
	camSetEnemyBases({
		"CONorthBase": {
			cleanup: "CONorth",
			detectMsg: "C2A_BASE1",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv393.ogg",
		},
		"CONorthWestBase": {
			cleanup: "CONorthWest",
			detectMsg: "C2A_BASE2",
			detectSnd: "pcv379.ogg",
			eliminateSnd: "pcv393.ogg",
		}
	});
	
	camManageTrucks(CO);
	truckDefense();
	hackAddMessage("MB2A_MSG", MISS_MSG, CAM_HUMAN_PLAYER, true);
	transportLimit = 4;
	index = 0;

	//Only if starting Beta directly rather than going through Alpha
	if(enumDroid(CAM_HUMAN_PLAYER, DROID_TRANSPORTER).length === 0) {
		sendPlayerTransporter();
	}
	else {
		setReinforcementTime(300); // 5 min.
	}

	queue("secondVideo", 12000); // 12 sec
	queue("truckDefense", 15000);// 15 sec.
	queue("groupPatrol", 15000); // 15 sec
	queue("sendCOTransporter", 30000); //30 sec
	queue("removeVtols", 300000);//5 min
	queue("vtolAttack", 300000); //5 min
	queue("mapEdgeDroids", 420000) //7 min
}
