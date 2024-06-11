include("script/campaign/libcampaign.js");
include("script/campaign/templates.js");

const mis_landingZoneList = [ "npPos1", "npPos2", "npPos3", "npPos4", "npPos5" ];
const mis_landingZoneMessages = [ "C1A-C_LZ1", "C1A-C_LZ2", "C1A-C_LZ3", "C1A-C_LZ4", "C1A-C_LZ5" ];
const mis_cyborgPatrolList = [
	"seCybPos1", "seCybPos2", "seCybPos3", "northCybPos1",
	"northCybPos2", "northCybPos3", "canyonCybPos1",
	"canyonCybPos2", "canyonCybPos3", "hillCybPos1",
	"hillCybPos2", "hillCybPos3", "1aCybPos1",
	"1aCybPos2", "1aCybPos3",
];
const mis_newParadigmRes = [
	"R-Wpn-MG1Mk1", "R-Vehicle-Body01", "R-Sys-Spade1Mk1", "R-Vehicle-Prop-Wheels",
	"R-Sys-Engineering01", "R-Wpn-MG-Damage04", "R-Wpn-MG-ROF02", "R-Wpn-Cannon-Damage03",
	"R-Wpn-Flamer-Damage03", "R-Wpn-Flamer-Range01", "R-Wpn-Flamer-ROF01",
	"R-Defense-WallUpgrade03","R-Struc-Materials03", "R-Vehicle-Engine03",
	"R-Struc-RprFac-Upgrade03", "R-Wpn-Rocket-Damage03", "R-Wpn-Rocket-ROF03",
	"R-Vehicle-Metals03", "R-Wpn-Mortar-Damage03", "R-Wpn-Rocket-Accuracy02",
	"R-Wpn-RocketSlow-Damage03", "R-Wpn-Mortar-ROF01", "R-Cyborg-Metals03",
	"R-Wpn-Mortar-Acc01", "R-Wpn-RocketSlow-Accuracy01", "R-Wpn-Cannon-Accuracy01",
];
const mis_newParadigmResClassic = [
	"R-Defense-WallUpgrade02", "R-Struc-Materials02", "R-Struc-Factory-Upgrade02",
	"R-Vehicle-Engine02", "R-Vehicle-Metals02", "R-Cyborg-Metals02",
	"R-Wpn-Cannon-Accuracy01", "R-Wpn-Cannon-Damage03", "R-Wpn-Flamer-Damage03",
	"R-Wpn-Flamer-ROF01", "R-Wpn-MG-Damage04", "R-Wpn-MG-ROF01",
	"R-Wpn-Mortar-Acc01", "R-Wpn-Mortar-Damage03", "R-Wpn-Rocket-Accuracy01",
	"R-Wpn-Rocket-Damage03", "R-Wpn-Rocket-ROF02", "R-Wpn-RocketSlow-Accuracy01",
	"R-Wpn-RocketSlow-Damage02"
];
var index; //Current LZ (SE, N, canyon, south hill, road north of base)
var switchLZ; //Counter for incrementing index every third landing

//Check if all enemies are gone and win after 15 transports
function extraVictoryCondition()
{
	const enemies = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false);
	if (index === 5 && enemies.length === 0)
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
		const MAX_TANKS = 16;
		const FIRST_AMOUNT = 10;

		const droidGroup1 = []; //Heavy cannon mantis track units
		const droidGroup2 = []; //Sensor and heavy mortar units
		const templates = [ cTempl.nphct, cTempl.npmsens, cTempl.npmorb ];

		for (let i = 0; i <= MAX_TANKS; ++i)
		{
			if (i <= FIRST_AMOUNT)
			{
				droidGroup1[i] = templates[0];
			}
			if (i === FIRST_AMOUNT + 1)
			{
				droidGroup2[i - 1 - FIRST_AMOUNT] = templates[1];
			}
			else
			{
				droidGroup2[i - 1 - FIRST_AMOUNT] = templates[2];
			}
		}

		//What part of map to appear at
		const pos = (index === 0) ? camMakePos("reinforceSouthEast") : camMakePos("reinforceNorth");
		camSendReinforcement(CAM_NEW_PARADIGM, pos, droidGroup1, CAM_REINFORCE_GROUND, {
			data: {regroup: false, count: -1,},
		});
		camSendReinforcement(CAM_NEW_PARADIGM, pos, droidGroup2, CAM_REINFORCE_GROUND);
	}
}

//Sends a transport with cyborgs to an LZ three times before going to another
//New Paradigm transport appears fifteen times before mission win
function sendTransport()
{
	const position = camMakePos(mis_landingZoneList[index]);
	switchLZ += 1;

	// (2, 3, 4, or 5) pairs of each droid template.
	// This emulates wzcam's droid count distribution.
	const unitDistribution = ((camClassicMode()) ? [2, 3, 4, 4, 4, 4, 4, 4, 4] : [4, 4, 4, 5, 5]);
	const COUNT = unitDistribution[camRand(unitDistribution.length)];

	const templates = [ cTempl.npcybc, cTempl.npcybf, cTempl.npcybm ];

	const droids = [];
	for (let i = 0; i < COUNT; ++i)
	{
		const t = templates[camRand(templates.length)];
		// two droids of each template
		droids.push(t);
		droids.push(t);
	}

	camSendReinforcement(CAM_NEW_PARADIGM, position, droids, CAM_REINFORCE_TRANSPORT, {
		entry: { x: 126, y: 36 },
		exit: { x: 126, y: 76 },
		message: mis_landingZoneMessages[index],
		order: CAM_ORDER_PATROL,
		data: {
			pos:[
				camMakePos(mis_cyborgPatrolList[(3 * index)]),
				camMakePos(mis_cyborgPatrolList[(3 * index) + 1]),
				camMakePos(mis_cyborgPatrolList[(3 * index) + 2]),
			],
			radius: 8,
			interval: camMinutesToMilliseconds(1),
			regroup: false,
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
		removeTimer("sendTransport");
	}
}

function startTransporterAttack()
{
	sendTransport();
	setTimer("sendTransport", camChangeOnDiff(camMinutesToMilliseconds(1)));
}

function eventStartLevel()
{
	camSetExtraObjectiveMessage(_("Destroy all New Paradigm reinforcements"));

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, "SUB_1_7S", {
		callback: "extraVictoryCondition"
	});

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone");
	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	setMissionTime(camChangeOnDiff(camHoursToSeconds(1)));

	if (camClassicMode())
	{
		camClassicResearch(mis_newParadigmResClassic, CAM_NEW_PARADIGM);
	}
	else
	{
		camCompleteRequiredResearch(mis_newParadigmRes, CAM_NEW_PARADIGM);
	}

	camPlayVideos([{video: "MB1A-C_MSG", type: CAMP_MSG}, {video: "MB1A-C_MSG2", type: MISS_MSG}]);

	index = 0;
	switchLZ = 0;

	queue("startTransporterAttack", camSecondsToMilliseconds(10));
}
