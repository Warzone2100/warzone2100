include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var allowWin;
var launchedBeforeExit;
var launchCount;
const COLLECTIVE_RES = [
	"R-Defense-WallUpgrade06", "R-Struc-Materials06",
	"R-Struc-Factory-Upgrade06", "R-Struc-Factory-Cyborg-Upgrade06",
	"R-Struc-VTOLFactory-Upgrade03", "R-Struc-VTOLPad-Upgrade03",
	"R-Vehicle-Engine06", "R-Vehicle-Metals06", "R-Cyborg-Metals06",
	"R-Vehicle-Armor-Heat02", "R-Cyborg-Armor-Heat02",
	"R-Sys-Engineering02", "R-Wpn-Cannon-Accuracy02", "R-Wpn-Cannon-Damage06",
	"R-Wpn-Cannon-ROF03", "R-Wpn-Flamer-Damage06", "R-Wpn-Flamer-ROF03",
	"R-Wpn-MG-Damage07", "R-Wpn-MG-ROF03", "R-Wpn-Mortar-Acc02",
	"R-Wpn-Mortar-Damage06", "R-Wpn-Mortar-ROF03",
	"R-Wpn-Rocket-Accuracy02", "R-Wpn-Rocket-Damage06",
	"R-Wpn-Rocket-ROF03", "R-Wpn-RocketSlow-Accuracy03",
	"R-Wpn-RocketSlow-Damage06", "R-Sys-Sensor-Upgrade01",
	"R-Wpn-Howitzer-Accuracy02", "R-Wpn-RocketSlow-ROF03",
	"R-Wpn-Howitzer-Damage03", "R-Wpn-AAGun-Accuracy01", "R-Wpn-AAGun-Damage01",
	"R-Wpn-AAGun-ROF01", "R-Wpn-Bomb-Accuracy01",
];

//Remove enemy vtols when in the remove zone area.
function checkEnemyVtolArea()
{
	var pos = {"x": 127, "y": 64};
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

	queue("checkEnemyVtolArea", 1000);
}

//Play last video sequence and destroy all droids on map.
function playLastVideo()
{
	var droids = enumArea(0, 0, mapWidth, mapHeight, false).filter(function(obj) {
		return obj.type === DROID;
	});

	for (var i = 0, l = droids.length; i < l; ++i)
	{
		camSafeRemoveObject(droids[i], false);
	}

	camPlayVideos("CAM2_OUT");
}

function eventMissionTimeout()
{
	camCallOnce("playLastVideo");
}

// Allow win if the transporter was launched at least three times.
function eventTransporterLaunch(transport)
{
	if (transport.player === CAM_HUMAN_PLAYER)
	{
		launchedBeforeExit = true;
		launchCount = launchCount + 1;
		if (launchCount > 2)
		{
			allowWin = true;
		}
	}
}

//This is triggered all over the source to end these type of missions if no
//droids remain on map.
function eventTransporterExit(transport)
{
	var len = enumDroid(CAM_HUMAN_PLAYER).filter(function(dr) {
		return !camIsTransporter(dr);
	}).length;
	if ((!launchedBeforeExit || (!len && launchedBeforeExit)) && transport.player === CAM_HUMAN_PLAYER)
	{
		camCallOnce("fastMissionEnd");
	}

	launchedBeforeExit = false;
}

function fastMissionEnd()
{
	setMissionTime(1);
}

//Return randomly selected droid templates.
function randomTemplates(list, isTransport)
{
	var droids = [];
	var size = camDef(isTransport) ? 8 + camRand(3) : 18 + camRand(8);

	for (var i = 0; i < size; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	return droids;
}

//Attack every 30 seconds.
function vtolAttack()
{
	const VTOL_POSITIONS = [
		{"x": 99, "y": 1},
		{"x": 127, "y": 65},
		{"x": 127, "y": 28},
		{"x": 36, "y": 1},
		{"x": 1, "y": 28},
	];
	var vtolRemovePos = {"x": 127, "y": 64};

	var list; with (camTemplates) list = [commorv, colcbv, colagv, comhvat];
	camSetVtolData(THE_COLLECTIVE, VTOL_POSITIONS, vtolRemovePos, list, camChangeOnDiff(30000));
}

//SouthEast attackers which are mostly cyborgs.
function cyborgAttack()
{
	var southCyborgAssembly = {"x": 123, "y": 125};
	var list; with (camTemplates) list = [npcybr, cocybag, npcybc, comhltat, cohhpv];

	camSendReinforcement(THE_COLLECTIVE, camMakePos(southCyborgAssembly), randomTemplates(list), CAM_REINFORCE_GROUND, {
		data: { regroup: false, count: -1 }
	});

	queue("cyborgAttack", camChangeOnDiff(240000));
}

//North road attacker consisting of powerful weaponry.
function tankAttack()
{
	var northTankAssembly = {"x": 95, "y": 3};
	//var westTankAssembly = {"x": 3, "y": 112}; //This was unused.

	var list; with (camTemplates) list = [comhltat, cohact, cohhpv, comagt, cohbbt];
	var pos = [];
	pos.push(northTankAssembly);

	camSendReinforcement(THE_COLLECTIVE, camMakePos(northTankAssembly), randomTemplates(list), CAM_REINFORCE_GROUND, {
		data: { regroup: false, count: -1, },
	});
	queue("tankAttack", camChangeOnDiff(180000));
}

function checkIfLaunched()
{
	if (allowWin)
	{
		return true;
	}

	if ((getMissionTime() <= 1) && !allowWin)
	{
		return false;
	}
}

//Everything in this level mostly just requeues itself until the mission ends.
function eventStartLevel()
{
	var startpos = {"x": 88, "y": 101};
	var lz = {"x": 86, "y": 99, "x2": 88, "y2": 101};
	var tCoords = {"xStart": 87, "yStart": 100, "xOut": 0, "yOut": 55};

	camSetStandardWinLossConditions(CAM_VICTORY_TIMEOUT, "CAM_3A", {
		reinforcements: 420, //Duration the transport "leaves" map.
		callback: "checkIfLaunched"
	});
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	camSetupTransporter(tCoords.xStart, tCoords.yStart, tCoords.xOut, tCoords.yOut);

	setMissionTime(1800); // 30 min.
	camCompleteRequiredResearch(COLLECTIVE_RES, THE_COLLECTIVE);
	setPower(AI_POWER, THE_COLLECTIVE);

	videoIndex = 0;
	allowWin = false;
	launchedBeforeExit = false;
	launchCount = 0;
	camPlayVideos(["MB2_DII_MSG", "MB2_DII_MSG2"]);

	//These requeue themselves every so often.
	vtolAttack();
	cyborgAttack();
	tankAttack();
	//collectiveTransportScouts();
	checkEnemyVtolArea();
}
