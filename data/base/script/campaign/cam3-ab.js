include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const NEXUS_RES = [
	"R-Defense-WallUpgrade09", "R-Struc-Materials09", "R-Struc-Factory-Upgrade06",
	"R-Struc-Factory-Cyborg-Upgrade06", "R-Struc-VTOLFactory-Upgrade06",
	"R-Struc-VTOLPad-Upgrade06", "R-Vehicle-Engine09", "R-Vehicle-Metals07",
	"R-Cyborg-Metals07", "R-Vehicle-Armor-Heat05", "R-Cyborg-Armor-Heat05",
	"R-Sys-Engineering03", "R-Vehicle-Prop-Hover02", "R-Vehicle-Prop-VTOL02",
	"R-Wpn-Bomb-Accuracy03", "R-Wpn-Energy-Accuracy01", "R-Wpn-Energy-Damage02",
	"R-Wpn-Energy-ROF02", "R-Wpn-Missile-Accuracy01", "R-Wpn-Missile-Damage02",
	"R-Wpn-Rail-Damage02", "R-Wpn-Rail-ROF02", "R-Sys-Sensor-Upgrade01",
	"R-Sys-NEXUSrepair", "R-Wpn-Flamer-Damage06",
];
var videoIndex;
var edgeMapIndex;
var edgeMapCounter;
var winFlag;

//Remove Nexus VTOL droids.
camAreaEvent("vtolRemoveZone", function(droid)
{
	if (droid.player !== CAM_HUMAN_PLAYER)
	{
		if (isVTOL(droid))
		{
			camSafeRemoveObject(droid, false);
		}
	}

	resetLabel("vtolRemoveZone", NEXUS);
});

function sendEdgeMapDroids()
{
	const COUNT = 13 + camRand(8); // 13 - 20.
	const EDGE = ["SWPhantomFactory", "NWPhantomFactory"];
	var list; with (camTemplates) list = [
		nxcyrail, nxcyscou, nxcylas, nxlflash, nxmrailh, nxmlinkh, nxmscouh,
		nxmsamh, nxmstrike,
	];

	if (!camDef(edgeMapIndex) || !camDef(edgeMapCounter))
	{
		edgeMapIndex = 0;
		edgeMapCounter = 0;
	}

	var droids = [];
	for (var i = 0; i < COUNT; ++i)
	{
		droids.push(list[camRand(list.length)]);
	}

	camSendReinforcement(NEXUS, camMakePos(EDGE[edgeMapIndex]), list,
		CAM_REINFORCE_GROUND, {
			data: {regroup: true, count: -1}
		}
	);

	edgeMapIndex += 1;
	if (edgeMapIndex === EDGE.length)
	{
		edgeMapIndex = 0;
	}

	edgeMapCounter += 1;
	if (edgeMapCounter === EDGE.length)
	{
		edgeMapCounter = 0;
	}

	//If the counter is zero, then a whole pass has completed. Then wait longer to send more.
	if (!edgeMapCounter)
	{
		queue("sendEdgeMapDroids", camChangeOnDiff(240000)); // ~4 min.
	}
	else
	{
		queue("sendEdgeMapDroids", camChangeOnDiff(5000)); // 5 sec.
	}
}

//Play videos.
function eventVideoDone()
{
	const VIDEOS = ["MB3_AB_MSG", "MB3_AB_MSG2", "MB3_AB_MSG3"];
	if (!camDef(videoIndex))
	{
		videoIndex = 0;
	}

	if (videoIndex < VIDEOS.length)
	{
		hackAddMessage(VIDEOS[videoIndex], MISS_MSG, CAM_HUMAN_PLAYER, true);
		videoIndex += 1;
	}
}

//Setup Nexus VTOL hit and runners. NOTE: These do not go away in this mission.
function vtolAttack()
{
	var list; with (camTemplates) list = [nxlscouv, nxmtherv, nxmheapv];
	camSetVtolData(NEXUS, "vtolAppearPos", "vtolRemovePos", list, camChangeOnDiff(240000)); //4 min
}

function powerTransfer()
{
	setPower(playerPower(me) + 5000);
	playSound("power-transferred.ogg");
}

function eventResearched(research, structure, player)
{
	if (research.name === "R-Sys-Resistance-Upgrade01")
	{
		winFlag = true;
	}
}

function hackPlayer()
{
	camHackIntoPlayer(CAM_HUMAN_PLAYER, NEXUS);
	if (camGetNexusState())
	{
		queue("hackPlayer", 6000);
	}
}

function synapticsSound()
{
	playSound(SYNAPTICS_ACTIVATED);
}

function resistanceResearched()
{
	if (winFlag)
	{
		camSetNexusState(false);
		return true; //Set in eventResearched().
	}
}

function eventStartLevel()
{
	const NEXUS_POWER = 999999;
	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "CAM3C", {
		callback: "resistanceResearched"
	});

	camSetNexusState(true);
	eventVideoDone();

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);
	setMissionTime(camChangeOnDiff(3600)); //1 hour.

	setPower(NEXUS_POWER, NEXUS);
	camCompleteRequiredResearch(NEXUS_RES, NEXUS);

	enableResearch("R-Sys-Resistance-Upgrade01", CAM_HUMAN_PLAYER);
	winFlag = false;

	vtolAttack();
	sendEdgeMapDroids();
	camHackIntoPlayer(CAM_HUMAN_PLAYER, NEXUS, true);

	queue("synapticsSound", 2000);
	queue("powerTransfer", 5000);
	queue("hackPlayer", 8000);
}
