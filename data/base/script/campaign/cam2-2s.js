include("script/campaign/libcampaign.js");

const extraVideos = ["MB2_2_MSG2", "MB2_2_MSG3"];
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
    camSetupTransporter(87, 100, 70, 1);
    centreView(88, 101);
    setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
    setMissionTime(4200); // 1 hour. 10 minutes
	hackAddMessage("MB2_2_MSG", MISS_MSG, CAM_HUMAN_PLAYER, true);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_2");
}
