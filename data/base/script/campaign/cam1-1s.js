include("script/campaign/libcampaign.js");

var cheat;
var powModVideoPlayed;

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
	var gens = enumStruct(CAM_HUMAN_PLAYER, POWER_GEN, false);
	for (var x = 0, l = gens.length; x < l; ++x)
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
		camSetupTransporter(11, 52, 1, 32);
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
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
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
