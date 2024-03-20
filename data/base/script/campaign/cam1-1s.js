include("script/campaign/libcampaign.js");

var cheat;
var powModVideoPlayed;
const mis_Labels = {
	startPos: {x: 13, y: 52},
	lz: {x: 10, y: 51, x2: 12, y2: 53},
	trPlace: {x: 11, y: 52},
	trExit: {x: 1, y: 32}
};

function eventChat(from, to, message)
{
	if (camIsCheating() && message === "let me win")
	{
		cheat = true;
	}
}

//Video if player does not yet have power module built
function resPowModVideo()
{
	powModVideoPlayed = true;
	camPlayVideos({video: "MB1_B2_MSG", type: MISS_MSG});
}

//Sector clear commander!
function secondVideo()
{
	camPlayVideos({video: "SB1_1_MSG", type: MISS_MSG});
}

//Has player built the power module?
function powerModuleBuilt()
{
	const gens = enumStruct(CAM_HUMAN_PLAYER, POWER_GEN, false);
	for (let x = 0, l = gens.length; x < l; ++x)
	{
		if ((gens[x].modules > 0) && (gens[x].status === BUILT))
		{
			return true;
		}
	}
	return false;
}

//Only way to pass this mission is to build a power module or be in cheat mode.
function checkForPowerModule()
{
	if (cheat || powerModuleBuilt())
	{
		camSetupTransporter(mis_Labels.trPlace.x, mis_Labels.trPlace.y, mis_Labels.trExit.x, mis_Labels.trExit.y);
		setMissionTime(camChangeOnDiff(camMinutesToSeconds(25))); // 25 min for offworld
		secondVideo();

		if (powModVideoPlayed)
		{
			removeTimer("checkForPowerModule");
		}
	}
}

function eventStartLevel()
{
	centreView(mis_Labels.startPos.x, mis_Labels.startPos.y);
	setNoGoArea(mis_Labels.lz.x, mis_Labels.lz.y, mis_Labels.lz.x2, mis_Labels.lz.y2, CAM_HUMAN_PLAYER);
	setMissionTime(camChangeOnDiff(camMinutesToSeconds(10))); // 10 min for building module.
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_1");
	cheat = false;
	powModVideoPlayed = false;

	if (!powerModuleBuilt())
	{
		resPowModVideo();
		setTimer("checkForPowerModule", camSecondsToMilliseconds(3));
	}

	checkForPowerModule();
}
