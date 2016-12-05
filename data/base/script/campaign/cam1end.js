include("script/campaign/libcampaign.js");

const extraVideos = ["CAM1_OUT2", "CAM2_BRIEF"];
var index;

function eventVideoDone()
{
	if(index < extraVideos.length)
	{
		hackAddMessage(extraVideos[index], MISS_MSG, CAM_HUMAN_PLAYER, true);
		index = index + 1;
	}
}

function eventStartLevel()
{
	index = 0;
	camSetupTransporter(11, 52, 40, 1);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(600); //10 min
	hackAddMessage("CAM1_OUT", MISS_MSG, CAM_HUMAN_PLAYER, true);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "CAM_2A");
}
