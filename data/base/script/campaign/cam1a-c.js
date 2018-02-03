
include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const landingZoneList = [ "npPos1", "npPos2", "npPos3", "npPos4", "npPos5" ];
const landingZoneMessages = [ "C1A-C_LZ1", "C1A-C_LZ2", "C1A-C_LZ3", "C1A-C_LZ4", "C1A-C_LZ5" ];
const cyborgPatrolList = [
	"seCybPos1", "seCybPos2", "seCybPos3", "northCybPos1",
	"northCybPos2", "northCybPos3", "canyonCybPos1",
	"canyonCybPos2", "canyonCybPos3", "hillCybPos1",
	"hillCybPos2", "hillCybPos3", "1aCybPos1",
	"1aCybPos2", "1aCybPos3",
];
const NEW_PARADIGM_RES = [
	"R-Wpn-MG-Damage04", "R-Wpn-MG-ROF01", "R-Defense-WallUpgrade02",
	"R-Struc-Materials02", "R-Struc-Factory-Upgrade02",
	"R-Struc-Factory-Cyborg-Upgrade02", "R-Vehicle-Engine02",
	"R-Vehicle-Metals02", "R-Cyborg-Metals02", "R-Wpn-Cannon-Accuracy01",
	"R-Wpn-Cannon-Damage03", "R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-ROF01",
	"R-Wpn-Mortar-Damage03", "R-Wpn-Mortar-Acc01", "R-Wpn-Rocket-Accuracy01",
	"R-Wpn-Rocket-Damage03", "R-Wpn-Rocket-ROF02", "R-Wpn-RocketSlow-Accuracy01",
	"R-Wpn-RocketSlow-Damage02", "R-Struc-RprFac-Upgrade03",
];
var index; //Current LZ (SE, N, canyon, south hill, road north of base)
var switchLZ; //Counter for incrementing index every third landing

//Check if all enemies are gone and win after 15 transports
function extraVictoryCondition()
{
	var enemies = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false);
	if(index === 5 && enemies.length === 0)
	{
		return true;
	}
}

//Makes a large group of ground units appear on map
function checkForGroundForces()
{
	if (index < 2 && switchLZ === 3)
	{
		//Amounts for the ground force
		const maxTanks = 16;
		const firstAmount = 10;

		var droidGroup1 = []; //Heavy cannon mantis track units
		var droidGroup2 = []; //Sensor and heavy mortar units
		var templates;
		with (camTemplates) templates = [ nphct, npmsens, npmorb ];

		for (var i = 0; i <= maxTanks; ++i)
		{
			if (i <= firstAmount)
			{
				droidGroup1[i] = templates[0];
			}
			if (i === firstAmount + 1)
			{
				droidGroup2[i - 1 - firstAmount] = templates[1];
			}
			else
			{
				droidGroup2[i - 1 - firstAmount] = templates[2];
			}
		}

		//What part of map to appear at
		var pos = (index === 0) ? camMakePos("reinforceSouthEast") : camMakePos("reinforceNorth");
		camSendReinforcement(NEW_PARADIGM, pos, droidGroup1, CAM_REINFORCE_GROUND, {
			data: {regroup: false, count: -1,},
		});
		camSendReinforcement(NEW_PARADIGM, pos, droidGroup2, CAM_REINFORCE_GROUND);
	}
}

//Sends a transport with cyborgs to an LZ three times before going to another
//New Paradigm transport appears fifteen times before mission win
function sendTransport()
{
	var position = camMakePos(landingZoneList[index]);
	switchLZ += 1;

	// (2 or 3 or 4) pairs of each droid template.
	// This emulates wzcam's droid count distribution.
	var count = [ 2, 3, 4, 4, 4, 4, 4, 4, 4 ][camRand(9)];

	var templates;
	with (camTemplates) templates = [ npcybc, npcybf, npcybm ];

	var droids = [];
	for (var i = 0; i < count; ++i)
	{
		var t = templates[camRand(templates.length)];
		// two droids of each template
		droids[droids.length] = t;
		droids[droids.length] = t;
	}


	camSendReinforcement(NEW_PARADIGM, position, droids, CAM_REINFORCE_TRANSPORT, {
		entry: { x: 126, y: 36 },
		exit: { x: 126, y: 76 },
		message: landingZoneMessages[index],
		order: CAM_ORDER_PATROL,
		data: {
			pos:[
				camMakePos( cyborgPatrolList[(3 * index)] ),
				camMakePos( cyborgPatrolList[(3 * index) + 1] ),
				camMakePos( cyborgPatrolList[(3 * index) + 2] ),
			],
			radius: 8,
			interval: 60000, //60 sec
			regroup: true,
			count: -1,
		}
	});

	checkForGroundForces();

	//Switch to a different LZ every third landing
	if (switchLZ === 3)
	{
		index += 1;
		switchLZ = 0;
	}

	if (index === 5)
	{
		return;
	}
	else
	{
		queue("sendTransport", camChangeOnDiff(60000)); //1 min
	}
}

function eventStartLevel()
{
	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_1_7S", {
		callback: "extraVictoryCondition"
	});

	var startpos = getObject("startPosition");
	var lz = getObject("landingZone");
	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	setMissionTime(camChangeOnDiff(3600)); //1 hour

	// make sure player doesn't build on enemy LZs
	for (var i = 6; i <= 10; ++i)
	{
		var ph = getObject("NPLZ" + i);
		setNoGoArea(ph.x, ph.y, ph.x2, ph.y2, NEW_PARADIGM);
	}

	camCompleteRequiredResearch(NEW_PARADIGM_RES, NEW_PARADIGM);
	camPlayVideos(["MB1A-C_MSG", "MB1A-C_MSG2"]);

	index = 0;
	switchLZ = 0;

	queue("sendTransport", 10000);
}
