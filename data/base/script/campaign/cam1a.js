
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var timerComing = 0; // start mission timer after first power gen
                     // and derrick is built

function gameLost()
{
	gameOverMessage(false);
}

// player zero's droid enteres this area
function eventAreaLaunchScavAttack(droid)
{
	var spos = getObject("scav1soundpos");
	playSound("pcv375.ogg", spos.x, spos.y, 0);
	playSound("pcv456.ogg");
	hackAddMessage("MB1A_MSG", MISS_MSG, 0, true);
	hackAddMessage("C1A_OBJ1", PROX_MSG, 0, false);
	// send scavengers on war path if triggered above
	camManageGroup(
		camMakeGroup("ScavAttack1", ENEMIES), CAM_ORDER_ATTACK,
		{ pos: camMakePos("playerBase") }
	);
	camMarkTiles();
	camMarkTiles("ScavAttack1");
}

function runAway()
{
	var oilPatch = getObject("oilPatch");
	var droids = enumRange(oilPatch.x, oilPatch.y, 7, 7, false);
	camManageGroup(camMakeGroup(droids), CAM_ORDER_ATTACK, {
		pos: camMakePos("ScavAttack1"),
		fallback: camMakePos("retreat1"),
		morale: 20 // will run away after losing a few people
	});
}

function doAmbush()
{
	camManageGroup(camMakeGroup("RoadblockArea"), CAM_ORDER_ATTACK, {
		pos: camMakePos("oilPatch"),
		fallback: camMakePos("retreat2"),
		morale: 50 // will mostly die
	});
}

// player zero's droid enteres this area
function eventAreaScavAttack1(droid)
{
	hackRemoveMessage("C1A_OBJ1", PROX_MSG, 0);
	camMarkTiles();
	hackAddMessage("C1A_BASE0", PROX_MSG, 0, false);
	camMarkTiles();
	camMarkTiles("RoadblockArea");
	queue("runAway", 1000);
	queue("doAmbush", 5000);
}

function eventAreaRoadblockArea(droid)
{
	camEnableFactory("base1factory");
	camMarkTiles();
	camMarkTiles("raidTrigger");
}

function eventArearaidTrigger(droid)
{
	camManageGroup(
		camMakeGroup("raidTrigger", ENEMIES),
		CAM_ORDER_ATTACK, { pos: camMakePos("scavbase3area") }
	);
	camManageGroup(
		camMakeGroup("raidGroup", ENEMIES),
		CAM_ORDER_ATTACK, { pos: camMakePos("scavbase3area") }
	);
	camManageGroup(
		camMakeGroup("scavbase3area", ENEMIES),
		CAM_ORDER_DEFEND, { pos: camMakePos("scavbase3area") }
	);
	camEnableFactory("base2factory1");
	camMarkTiles();
	camMarkTiles("scavbase3area");
}

function eventAreascavbase3area(droid)
{
	camEnableFactory("base2factory2");
	camMarkTiles();
}

// proceed to next level
function gameWon()
{
	camNextLevel("CAM_1B");
}

// things that need checking every second
function tick()
{
	// check if game is lost
	var factories = countStruct("A0LightFactory") + countStruct("A0CyborgFactory");
	var droids = countDroid(DROID_CONSTRUCT);
	if (droids == 0 && factories == 0)
		queue("gameLost", 4000); // wait 4 secs before throwing the game
	// check if game is won
	if (camAllArtifactsPickedUp() && camAllEnemyBasesEliminated())
		queue("gameWon", 6000); // wait 6 secs before giving it
}

function playDelayed374(where)
{
	var spos = getObject(where);
	playSound("pcv374.ogg", spos.x, spos.y, 0);
}

function camEnemyBaseEliminated_scavgroup1()
{
	hackRemoveMessage("C1A_BASE0", PROX_MSG, 0);
	hackAddMessage("C1A_BASE1", PROX_MSG, 0, false);
	var spos = getObject("scav1soundpos");
	playSound("pcv391.ogg", spos.x, spos.y, 0);
	queue("playDelayed374", 2000, "scav2soundpos");
}

function camEnemyBaseEliminated_scavgroup2()
{
	hackRemoveMessage("C1A_BASE1", PROX_MSG, 0);
	hackAddMessage("C1A_BASE2", PROX_MSG, 0, false);
	var spos = getObject("scav2soundpos");
	playSound("pcv392.ogg", spos.x, spos.y, 0);
	queue("playDelayed374", 2000, "scav3soundpos");
}

function camEnemyBaseEliminated_scavgroup3()
{
	leftovers = enumArea("scavbase3area");
	hackRemoveMessage("C1A_BASE2", PROX_MSG, 0);
	hackAddMessage("C1A_BASE3", PROX_MSG, 0, false);
	var spos = getObject("scav3soundpos");
	playSound("pcv392.ogg", spos.x, spos.y, 0);
	queue("playDelayed374", 2000, "retreat4");
	camEnableFactory("base2factory2");
}

function camEnemyBaseEliminated_scavgroup4()
{
	leftovers = enumArea("scavbase4area");
	hackRemoveMessage("C1A_BASE3", PROX_MSG, 0);
	var spos = getObject("retreat4");
	playSound("pcv392.ogg", spos.x, spos.y, 0);
}

function eventStructureBuilt(structure, droid)
{
	if (structure.stattype == POWER_GEN)
		timerComing++;
	else if (structure.stattype == RESOURCE_EXTRACTOR)
		timerComing++;
	if (timerComing == 2)
		setMissionTime(3600);
}

function eventStartLevel()
{
	var startpos = getObject("startPosition");
	var lz = label("landingZone");

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, 0);
	setPower(1300);
	setPower(200, 6);
	setPower(200, 7);

	// allow to build stuff
	enableStructure("A0CommandCentre", 0);
	enableStructure("A0PowerGenerator", 0);
	enableStructure("A0ResourceExtractor", 0);
	enableStructure("A0ResearchFacility", 0);
	enableStructure("A0LightFactory", 0);

	// needs to be done this way so doesn't enable rest of tree!
	makeComponentAvailable("MG1Mk1", me);

	completeResearch("R-Vehicle-Body01", me);
	completeResearch("R-Sys-Spade1Mk1", me);
	completeResearch("R-Vehicle-Prop-Wheels", me);

	// give player briefing
	hackAddMessage("CMB1_MSG", CAMP_MSG, 0, false);

	setReinforcementTime(-1);
	setMissionTime(-1);

	setTimer("tick", 1000);

	setStructureLimits("A0PowerGenerator", 5, 0);
	setStructureLimits("A0ResourceExtractor", 200, 0);
	setStructureLimits("A0ResearchFacility", 5, 0);
	setStructureLimits("A0LightFactory", 5, 0);
	setStructureLimits("A0CommandCentre", 1, 0);

	// feed libcampaign.js with data to do the rest
	camSetEnemyBases({
		"scavgroup1": { area: "scavbase1area" },
		"scavgroup2": { area: "scavbase2area" },
		"scavgroup3": { area: "scavbase3area" },
		"scavgroup4": { area: "scavbase4area" },
	});
	camSetArtifacts({
		"base1factory": { tech: "R-Wpn-Flamer01Mk1" },
		"base2factory2": { tech: "R-Sys-Engineering01" },
		"base2factory1": { tech: "R-Defense-Tower01" },
		"artifact4pos": { tech: "R-Wpn-MG-Damage01" },
	});
	var t = camTemplates;
	camSetFactories({
		"base1factory": {
			assembly: "assembly1",
			groupSize: 4,
			order: CAM_ORDER_ATTACK,
			data: "playerBase",
			templates: [ t.trike, t.bloke ]
		},
		"base2factory1": {
			assembly: "assembly2",
			groupSize: 4,
			order: CAM_ORDER_ATTACK,
			data: "playerBase",
			templates: [ t.bloke, t.buggy, t.bloke ]
		},
		"base2factory2": {
			assembly: "assembly3",
			groupSize: 4,
			order: CAM_ORDER_ATTACK,
			data: "playerBase",
			templates: [ t.bjeep, t.bloke, t.trike, t.bloke ]
		},
	});
	camMarkTiles("LaunchScavAttack");
}
