
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

function eventAreaAttackArea1(droid)
{
	camManageGroup(camMakeGroup("enemy1Force1"), CAM_ORDER_ATTACK, {
		pos: camMakePos("enemy1Force1Pos"),
		fallback: camMakePos("enemy1Force1Fallback"),
		morale: 50
	});
	var t = camTemplates;
	camEnableFactory("base1factory"); // pink factory
	camSetFactoryData("base2factory", { // sic! hill factory
 		assembly: "assembly2",
		order: CAM_ORDER_ATTACK,  // changes
		data: { pos: "playerBase" }, // changes
		groupSize: 10, // changes
		maxSize: 10,
		throttle: 40000,
		templates: [ t.trike, t.bloke, t.buggy, t.bloke, ] // changes
	});
	camMarkTiles();
	camMarkTiles("AttackArea2");
}

function eventAreaAttackArea2(droid)
{
	camEnableFactory("base4factory");
	camMarkTiles();
}

function camEnemyBaseEliminated_base4group()
{
	hackRemoveMessage("C1B_BASE2", PROX_MSG, 0);
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_1_1S");
	var startpos = getObject("startPosition");
	var lz = label("landingZone");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, 0);

	setReinforcementTime(-1);
	setMissionTime(3600);
	setAlliance(6, 7, true);

	setPower(200, 6);
	setPower(200, 7);

	hackAddMessage("MB1B_MSG", MISS_MSG, 0, true);
	hackAddMessage("C1B_BASE2", PROX_MSG, 0, false);

	camMarkTiles("AttackArea1");

	camSetArtifacts({
		"base1factory": { tech: "R-Wpn-Flamer-Damage01" },
		"base2factory": { tech: "R-Wpn-MG2Mk1" },
		"base3sensor": { tech: "R-Sys-Sensor-Turret01" },
		"base4gen": { tech: "R-Struc-PowerModuleMk1" },
	});

	camSetEnemyBases({
		"base1group": {
			cleanup: "enemybase1",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg",
		},
		"base2group": {
			cleanup: "enemybase2",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg",
		},
		"base3group": {
			// sensor tower; no cleanup
			detectSnd: "pcv375.ogg",
		},
		"base4group": {
			cleanup: "enemybase4",
			eliminateSnd: "pcv392.ogg",
		},
	});

	var t = camTemplates;
	camSetFactories({
		"base1factory": {
			assembly: "assembly1",
			order: CAM_ORDER_ATTACK,
			data: { pos: "playerBase" },
			groupSize: 6,
			maxSize: 6,
			throttle: 40000,
			templates: [ t.trike, t.bloke, t.buggy, t.bloke ]
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
			templates: [ t.bloke ] // will override later
		},
		"base4factory": {
			assembly: "assembly4",
			order: CAM_ORDER_ATTACK,
			data: { pos: "playerBase" },
 			groupSize: 8,
			maxSize: 8,
			throttle: 40000,
			templates: [ t.trike, t.bloke, t.buggy, t.bjeep ]
		},
	});
	camEnableFactory("base2factory");
	//Timed attacks if player dawdles
	queue("eventAreaAttackArea2", 360000);
}
