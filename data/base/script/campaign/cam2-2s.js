include("script/campaign/libcampaign.js");

var index;

function eventVideoDone()
{
	const VIDEOS = ["MB2_2_MSG", "MB2_2_MSG2", "MB2_2_MSG3"];
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
    camSetupTransporter(87, 100, 70, 1);
    centreView(88, 101);
    setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
    setMissionTime(camChangeOnDiff(4200)); // 1 hour. 10 minutes
	eventVideoDone();
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_2");
}
