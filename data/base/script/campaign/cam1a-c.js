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
const MIS_TOTAL_TRANSPORTS = 15; // Total transporter loads that can be dropped off.
const MIS_TOTAL_PER_INDEX = 3; // How many times transporter can drop at an LZ.
var index; // Current LZ (SE, N, canyon, south hill, road north of base)
var switchLZ; // Counter for incrementing index every third landing
var sentTransports; // Keeps track of how much the transporter dropped off at a specific LZ, and the overall total.

// Remove this function some time far after 4.6.4 since this just serves to allow old saves on this mission to function without victory problems.
function eventGameLoaded()
{
	if (!camDef(sentTransports))
	{
		dump("Old Alpha 10 internal data format detected"); // Save to WZ log file.
		sentTransports = {
			timesSentAtIndex: [MIS_TOTAL_PER_INDEX, MIS_TOTAL_PER_INDEX, MIS_TOTAL_PER_INDEX, MIS_TOTAL_PER_INDEX, MIS_TOTAL_PER_INDEX],
			total: MIS_TOTAL_TRANSPORTS
		};
		// Simulates last transport load.
		index = sentTransports.timesSentAtIndex.length - 1;
		switchLZ = 2;
	}
}

//Check if all enemies are gone and win after 15 transports
function extraVictoryCondition()
{
	const enemies = enumArea(0, 0, mapWidth, mapHeight, ENEMIES, false);
	if (sentTransports.total >= MIS_TOTAL_TRANSPORTS && enemies.length === 0)
	{
		return true;
	}
}

//Makes a large group of ground units appear on map
function checkForGroundForces()
{
	const __REGULAR_SPAWN_CONDITION = ((index < 2) && (switchLZ === MIS_TOTAL_PER_INDEX));
	const __INSANE_SPAWN_CONDITION = (camAllowInsaneSpawns() && (sentTransports.total % 2 === 0));

	if (__REGULAR_SPAWN_CONDITION || __INSANE_SPAWN_CONDITION)
	{
		//Amounts for the ground force
		const MAX_CANNON_TANKS = 10;
		const MAX_ARTILLERY_TANKS = 6;
		const MAX_INSANE_ADDITIONAL_UNITS = 2 + camRand(3);

		const droids = [];
		const insaneTemplates = [ cTempl.npcybr, cTempl.nphct, cTempl.nphmgt ];
		if (sentTransports.total > (MIS_TOTAL_PER_INDEX * 3))
		{
			insaneTemplates.push(cTempl.npsbb); //Bring in BBs
		}

		for (let i = 0; i < MAX_CANNON_TANKS; ++i)
		{
			droids.push(cTempl.nphct);
		}
		for (let i = 0; i < MAX_ARTILLERY_TANKS; ++i)
		{
			droids.push(cTempl.npmorb);
		}
		if (camAllowInsaneSpawns())
		{
			for (let i = 0; i < MAX_INSANE_ADDITIONAL_UNITS; ++i)
			{
				droids.push(insaneTemplates[camRand(insaneTemplates.length)]);
			}
		}
		droids.push(cTempl.npsens);

		//What part of the map to appear at
		let pos;
		if (camAllowInsaneSpawns())
		{
			const positions = ["reinforceSouthEast", "reinforceNorth", "reinforceNorthEast"];
			pos = positions[camRand(positions.length)];
		}
		else if (index === 0)
		{
			pos = camMakePos("reinforceSouthEast");
		}
		else if (index === 1)
		{
			pos = camMakePos("reinforceNorth");
		}

		camSendReinforcement(CAM_NEW_PARADIGM, pos, droids, CAM_REINFORCE_GROUND);
	}
}

//Sends a transport with cyborgs to an LZ three times before going to another
//New Paradigm transport appears fifteen times before mission win
function sendTransport()
{
	if (camAllowInsaneSpawns())
	{
		const availableLZs = [];
		for (let i = 0, len = mis_landingZoneList.length; i < len; ++i)
		{
			if (sentTransports.timesSentAtIndex[i] < MIS_TOTAL_PER_INDEX)
			{
				availableLZs.push(mis_landingZoneList[i]);
			}
		}
		if (!availableLZs.length)
		{
			return; // Sending transporters is over with.
		}
		const RND_LZ_STR = availableLZs[camRand(availableLZs.length)];
		index = mis_landingZoneList.indexOf(RND_LZ_STR);
	}

	const position = camMakePos(mis_landingZoneList[index]);
	switchLZ += 1;
	sentTransports.total += 1;
	sentTransports.timesSentAtIndex[index] += 1;

	// (2, 3, 4, or 5) pairs of each droid template.
	// This emulates wzcam's droid count distribution.
	const unitDistribution = ((camClassicMode()) ? [2, 3, 4, 4, 4, 4, 4, 4, 4] : [4, 4, 4, 5, 5]);
	const COUNT = unitDistribution[camRand(unitDistribution.length)];

	const templates = [ cTempl.npcybc, cTempl.npcybf, cTempl.npcybm ];
	if (camAllowInsaneSpawns())
	{
		templates.push(cTempl.npcybr);
	}

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

	// Switch to a different LZ every third landing -- Insane+ randomizes it so skip this.
	if (!camAllowInsaneSpawns() && (switchLZ === MIS_TOTAL_PER_INDEX))
	{
		index += 1;
		switchLZ = 0;
	}

	if (sentTransports.total >= MIS_TOTAL_TRANSPORTS)
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

	camSetStandardWinLossConditions(CAM_VICTORY_STANDARD, cam_levels.alpha11.pre, {
		callback: "extraVictoryCondition"
	});

	const startPos = getObject("startPosition");
	const lz = getObject("landingZone");
	centreView(startPos.x, startPos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, CAM_HUMAN_PLAYER);

	camSetMissionTimer(camChangeOnDiff(camHoursToSeconds(1)));

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
	sentTransports = {timesSentAtIndex: [0, 0, 0, 0, 0], total: 0};

	queue("startTransporterAttack", camSecondsToMilliseconds(10));
}
