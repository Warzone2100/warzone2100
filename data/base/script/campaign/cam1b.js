
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var NPScout; // Sensor scout (initialized when droid is created)

camAreaEvent("AttackArea1", function(droid)
{
	queue("camCallOnce", 2000, "doNPRetreat");
	camManageGroup(camMakeGroup("enemy1Force1", 6), CAM_ORDER_ATTACK, {
		pos: camMakePos("enemy1Force1Pos"),
		fallback: camMakePos("enemy1Force1Fallback"),
		morale: 50
	});
	// pink factory
	camEnableFactory("base1factory");
	// sic! hill factory
	with (camTemplates) camSetFactoryData("base2factory", {
 		assembly: "assembly2",
		order: CAM_ORDER_ATTACK,  // changes
		data: { pos: "playerBase" }, // changes
		groupSize: 10, // changes
		maxSize: 10,
		throttle: 40000,
		templates: [ trike, bloke, buggy, bloke, ] // changes
	});
	camEnableFactory("base2factory"); // re-enable
});

camAreaEvent("AttackArea2", function(droid)
{
	camEnableFactory("base4factory");
});

function doNPRetreat()
{
	var pos = camMakePos("NPSensorTurn");
	if (NPScout)
	{
		camTrace("New Paradigm sensor droid is retreating");
		orderDroidLoc(NPScout, DORDER_MOVE, pos.x, pos.y);
	}
	else
	{
		camTrace("Sensor droid died before retreating.");
	}
}

function eventDestroyed(obj)
{
	if (NPScout && (obj.id === NPScout.id))
	{
		NPScout = null;
		camUnmarkTiles("NPSensorTurn");
		camUnmarkTiles("NPSensorRemove");
	}
}

camAreaEvent("NPSensorTurn", function(droid)
{
	var pos = camMakePos("NPSensorRemove");
	orderDroidLoc(NPScout, DORDER_MOVE, pos.x, pos.y);
});

camAreaEvent("NPSensorRemove", function(droid)
{
	removeObject(NPScout, false);
});

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_1_1S");
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, 0);

	setMissionTime(3600);
	setAlliance(1, 6, true);
	setAlliance(1, 7, true);
	setAlliance(6, 7, true);

	setPower(200, 6);
	setPower(200, 7);

	camSetArtifacts({
		"base1factory": { tech: "R-Wpn-Flamer-Damage01" },
		"base2factory": { tech: "R-Wpn-MG2Mk1" },
		"base3sensor": { tech: "R-Sys-Sensor-Turret01" },
		"base4gen": { tech: "R-Struc-PowerModuleMk1" },
	});

	camSetEnemyBases({
		"base1group": {
			cleanup: "enemybase1",
			detectMsg: "C1B_BASE1",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg",
		},
		"base2group": {
			cleanup: "enemybase2",
			detectMsg: "C1B_BASE0",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg",
		},
		"base3group": {
			detectMsg: "C1B_OBJ1",
			detectSnd: "pcv375.ogg",
			eliminateSnd: "pcv391.ogg",
		},
		"base4group": {
			cleanup: "enemybase4",
			detectMsg: "C1B_BASE2",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg",
		},
	});

	hackAddMessage("MB1B_MSG", MISS_MSG, 0, true);
	camDetectEnemyBase("base4group"); // power surge detected

	with (camTemplates) camSetFactories({
		"base1factory": {
			assembly: "assembly1",
			order: CAM_ORDER_ATTACK,
			data: { pos: "playerBase" },
			groupSize: 6,
			maxSize: 6,
			throttle: 40000,
			templates: [ trike, bloke, buggy, bloke ]
		},
		"base2factory": { // the hill harass factory
			assembly: "assembly2",
			order: CAM_ORDER_PATROL, // will override later
			data: { // will override later
				pos: [ "patrol1", "patrol2", "patrol3", "patrol4" ],
				interval: 20000
			},
			group: camMakeGroup("hillForce"), // will override later
			groupSize: 4, // will override later
			maxSize: 10,
			throttle: 40000,
			templates: [ bloke ] // will override later
		},
		"base4factory": {
			assembly: "assembly4",
			order: CAM_ORDER_ATTACK,
			data: { pos: "playerBase" },
 			groupSize: 8,
			maxSize: 8,
			throttle: 40000,
			templates: [ trike, bloke, buggy, bjeep ]
		},
	});
	camEnableFactory("base2factory");

	//Timed attacks if player dawdles
	queue("eventAreaAttackArea2", 360000);

	// New Paradigm sensor scout
	var pos = getObject("NPSensorAppear");
	NPScout = addDroid(1, pos.x, pos.y, "Scout",
	                   "Body4ABT", "wheeled01", 0, 0, "SensorTurret1Mk1");
	pos = getObject("NPSensorWatch");
	orderDroidLoc(NPScout, DORDER_MOVE, pos.x, pos.y);
}
