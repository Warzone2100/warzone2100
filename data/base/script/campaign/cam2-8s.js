include("script/campaign/libcampaign.js");

const VIDEOS = ["MB2_8_MSG", "MB2_8_MSG2"];
var index;

function eventVideoDone()
{
	if(index < VIDEOS.length)
	{
		hackAddMessage(VIDEOS[index], MISS_MSG, CAM_HUMAN_PLAYER, true);
		index = index + 1;
	}
}

function eventStartLevel()
{
	index = 0;
    camSetupTransporter(87, 100, 126, 60);
    centreView(88, 101);
    setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
    setMissionTime(3600); // 1 hour.
	eventVideoDone();
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_8");
}
