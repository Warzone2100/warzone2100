include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

var videoIndex;
var allowWin;
const VIDEOS = ["MB2_DII_MSG", "MB2_DII_MSG2", "CAM2_OUT"];
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
	var vtols = enumRange(pos.x, pos.y, 10, THE_COLLECTIVE, false);
	vtols.filter(function(obj) { return (obj.player === THE_COLLECTIVE) && isVTOL(obj); });

	for(var i = 0; i < vtols.length; ++i)
	{
		if((vtols[i].weapons[0].armed < 20) || (vtols[i].health < 60))
		{
			camSafeRemoveObject(vtols[i], false);
		}
	}

	queue("checkEnemyVtolArea", 1000);
}

//Play first two immediately...
function eventVideoDone(playLastVideo)
{
	if(camDef(playLastVideo))
	{
		hackAddMessage(VIDEOS[videoIndex], MISS_MSG, CAM_HUMAN_PLAYER, true);
	}

	if(videoIndex < VIDEOS.length - 1)
	{
		hackAddMessage(VIDEOS[videoIndex], MISS_MSG, CAM_HUMAN_PLAYER, true);
		videoIndex = videoIndex + 1;
	}
}

//Play last video sequence and destroy all droids on map.
function playLastVideo()
{
	var droids = enumArea(0, 0, mapWidth, mapHeight, false).filter(function(obj) {
		return obj.type === DROID;
	});

	for(var i = 0; i < droids.length; ++i)
	{
		camSafeRemoveObject(droids[i], false);
	}

	eventVideoDone(true);
}

function eventMissionTimeout()
{
	camCallOnce("playLastVideo");
}


//Return 8 - 15 droids.
function randomTemplates(list, isTransport)
{
	var droids = [];
	var size = 8 + camRand(8);

	if(camDef(isTransport))
	{
		size = 8 + camRand(3);
	}

	for(var i = 0; i < size; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	return droids;
}

//Attack every 30 seconds.
function vtolAttack()
{
	var vtolPosNorthRoad = {"x": 99, "y": 1};
	var vtolPosNorthEast = {"x": 127, "y": 65};
	var vtolPosNorthEast2 = {"x": 127, "y": 28};
	var vtolPosNorthWest = {"x": 36, "y": 1};
	var vtolPosNorthWestLeft = {"x": 1, "y": 28};
	var vtolRemovePos = {"x": 127, "y": 64};

	var startPos = [];
	startPos.push(vtolPosNorthRoad);
	startPos.push(vtolPosNorthEast);
	startPos.push(vtolPosNorthEast2);
	startPos.push(vtolPosNorthWest);
	startPos.push(vtolPosNorthWestLeft);

	var list; with (camTemplates) list = [commorv, colcbv, colagv, comhvat];
	camSetVtolData(THE_COLLECTIVE, startPos, vtolRemovePos, list, camChangeOnDiff(30000));
}

//Every 10 minutes.
/*
function collectiveTransportScouts()
{
	var COTransportPos = {"x": 117, "y": 116};

	var list; with (camTemplates) list = [commgt, npcybc, comhltat, npcybr, cocybag];
	camSendReinforcement(THE_COLLECTIVE, camMakePos(COTransportPos), randomTemplates(list, true),
		CAM_REINFORCE_TRANSPORT, {
			entry: { x: 126, y: 100 },
			exit: { x: 126, y: 70 }
		}
	);

	queue("collectiveTransportScouts", camChangeOnDiff(600000));
}
*/

//every 5 min
function cyborgAttack()
{
	var southCyborgAssembly = {"x": 123, "y": 125};
	var list; with (camTemplates) list = [npcybr, cocybag, npcybc, comhltat, cohhpv];

	camSendReinforcement(THE_COLLECTIVE, camMakePos(southCyborgAssembly), randomTemplates(list), CAM_REINFORCE_GROUND, {
		data: { regroup: false, count: -1 }
	});

	queue("cyborgAttack", camChangeOnDiff(300000));
}

//Every 4 min
function tankAttack()
{
	var northTankAssembly = {"x": 95, "y": 3};
	//var westTankAssembly = {"x": 3, "y": 112}; //This was unused.

	var list; with (camTemplates) list = [comhltat, cohact, cohhpv, comagt];
	var pos = [];
	pos.push(northTankAssembly);

	camSendReinforcement(THE_COLLECTIVE, camMakePos(northTankAssembly), randomTemplates(list), CAM_REINFORCE_GROUND, {
		data: { regroup: false, count: -1, },
	});
	queue("tankAttack", camChangeOnDiff(240000));
}

function checkIfLaunched()
{
	var launched = enumRange(88, 101, 10,CAM_HUMAN_PLAYER, false).filter(function(obj) {
		return (obj.type === DROID &&
		(obj.droidType === DROID_TRANSPORTER || obj.droidType === DROID_SUPERTRANSPORTER));
	});

	if (!launched.length)
	{
		allowWin = true;
	}

	if ((getMissionTime() === 0) && !allowWin)
	{
		return false;
	}

	if (allowWin)
	{
		return true;
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
		callback: "chechIfLaunched"
	});
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	camSetupTransporter(tCoords.xStart, tCoords.yStart, tCoords.xOut, tCoords.yOut);

	setMissionTime(1800); // 30 min.
	camCompleteRequiredResearch(COLLECTIVE_RES, THE_COLLECTIVE);

	videoIndex = 0;
	allowWin = false;
	eventVideoDone();

	//These requeue themselves every so often.
	vtolAttack();
	cyborgAttack();
	tankAttack();
	//collectiveTransportScouts();
	checkEnemyVtolArea();
}
