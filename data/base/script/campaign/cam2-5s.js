include("script/campaign/libcampaign.js");
var index;

function eventVideoDone()
{
	const VIDEOS = ["MB2_5_MSG", "MB2_5_MSG2"];
	if(!camDef(index))
	{
		index = 0;
	}

	if(index < VIDEOS.length)
	{
		hackAddMessage(VIDEOS[index], MISS_MSG, CAM_HUMAN_PLAYER, true);
		index = index + 1;
	}
}

function eventStartLevel()
{
    camSetupTransporter(87, 100, 32, 1);
    centreView(88, 101);
    setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
    setMissionTime(camChangeOnDiff(3600)); // 1 hour
	eventVideoDone();
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_5");
}
