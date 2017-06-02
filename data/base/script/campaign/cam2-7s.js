include("script/campaign/libcampaign.js");
var index;

function eventVideoDone()
{
	const VIDEOS = ["MB2_7_MSG", "MB2_7_MSG2"];
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
    camSetupTransporter(87, 100, 100, 1);
    centreView(88, 101);
    setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
    setMissionTime(camChangeOnDiff(5400)); // 1 hour - 30 minutes.
	eventVideoDone();
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_7");
}
