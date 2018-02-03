include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const PLAYER_RES = [
	"R-Wpn-MG1Mk1", "R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels",
];

// player zero's droid enteres this area
camAreaEvent("LaunchScavAttack", function(droid)
{
	var spos = getObject("scav1soundpos");
	playSound("pcv375.ogg", spos.x, spos.y, 0);
	playSound("pcv456.ogg");
	camPlayVideos("MB1A_MSG");
	hackAddMessage("C1A_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER, false);
	// send scavengers on war path if triggered above
	camManageGroup(camMakeGroup("ScavAttack1", ENEMIES), CAM_ORDER_ATTACK, {
		pos: camMakePos("playerBase"),
		fallback: camMakePos("retreat1"),
		morale: 50
	});
	// activate mission timer, unlike the original campaign.
	setMissionTime(camChangeOnDiff(3600));
});

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
camAreaEvent("ScavAttack1", function(droid)
{
	hackRemoveMessage("C1A_OBJ1", PROX_MSG, CAM_HUMAN_PLAYER);
	queue("runAway", 1000);
	queue("doAmbush", 5000);
});

camAreaEvent("RoadblockArea", function(droid)
{
	camEnableFactory("base1factory");
});

camAreaEvent("raidTrigger", function(droid)
{
	camManageGroup( camMakeGroup("raidTrigger", ENEMIES), CAM_ORDER_ATTACK, {
		pos: camMakePos("scavbase3area")
	});
	camManageGroup(camMakeGroup("raidGroup", ENEMIES), CAM_ORDER_ATTACK, {
		pos: camMakePos("scavbase3area")
	});
	camManageGroup(camMakeGroup("scavbase3area", ENEMIES), CAM_ORDER_DEFEND, {
		pos: camMakePos("scavbase3area")
	});
	camEnableFactory("base2factory1");
});

camAreaEvent("scavbase3area", function(droid)
{
	camEnableFactory("base2factory2");
});

function camEnemyBaseEliminated_scavgroup1()
{
	camEnableFactory("base1factory");
}

function camEnemyBaseEliminated_scavgroup2()
{
	queue("camDetectEnemyBase", 2000, "scavgroup3");
}

function enableStartingBuildings()
{
	const STRUCTS = [
		"A0CommandCentre", "A0PowerGenerator", "A0ResourceExtractor",
		"A0ResearchFacility", "A0LightFactory",
	];

	for (var i = 0; i < STRUCTS.length; ++i)
	{
		enableStructure(STRUCTS[i], CAM_HUMAN_PLAYER);
	}
}

function eventStartLevel()
{
	const PLAYER_POWER = 1300;
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "CAM_1B");

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	setPower(PLAYER_POWER, CAM_HUMAN_PLAYER);
	setAlliance(6, 7, true);

	enableStartingBuildings();
	camCompleteRequiredResearch(PLAYER_RES, CAM_HUMAN_PLAYER);
	//These are available without needing to build a HQ.
	enableTemplate("ConstructionDroid");
	enableTemplate("ViperLtMGWheels");

	// give player briefing
	hackAddMessage("CMB1_MSG", CAMP_MSG, CAM_HUMAN_PLAYER, false);
	setMissionTime(-1);

	// feed libcampaign.js with data to do the rest
	camSetEnemyBases({
		"scavgroup1": {
			cleanup: "scavbase1area",
			detectMsg: "C1A_BASE0",
			detectSnd: "pcv375.ogg",
			eliminateSnd: "pcv391.ogg"
		},
		"scavgroup2": {
			cleanup: "scavbase2area",
			detectMsg: "C1A_BASE1",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"scavgroup3": {
			cleanup: "scavbase3area",
			detectMsg: "C1A_BASE2",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
		"scavgroup4": {
			cleanup: "scavbase4area",
			detectMsg: "C1A_BASE3",
			detectSnd: "pcv374.ogg",
			eliminateSnd: "pcv392.ogg"
		},
	});

	camSetArtifacts({
		"base1factory": { tech: "R-Wpn-Flamer01Mk1" },
		"base2factory2": { tech: "R-Sys-Engineering01" },
		"base2factory1": { tech: "R-Defense-Tower01" },
		"artifact4pos": { tech: "R-Wpn-MG-Damage01" },
	});

	with (camTemplates) camSetFactories({
		"base1factory": {
			assembly: "assembly1",
			order: CAM_ORDER_ATTACK,
			data: { pos: "playerBase" },
			groupSize: 3,
			maxSize: 3,
			throttle: camChangeOnDiff(20000),
			templates: [ trike, bloke ]
		},
		"base2factory1": {
			assembly: "assembly2",
			order: CAM_ORDER_ATTACK,
			data: { pos: "playerBase" },
			groupSize: 4,
			maxSize: 4,
			throttle: camChangeOnDiff(16000),
			templates: [ bloke, buggy, bloke ]
		},
		"base2factory2": {
			assembly: "assembly3",
			order: CAM_ORDER_ATTACK,
			data: { pos: "playerBase" },
			groupSize: 4,
			maxSize: 4,
			throttle: camChangeOnDiff(13000),
			templates: [ bjeep, bloke, trike, bloke ]
		},
	});
}
