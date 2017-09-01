include("script/campaign/libcampaign.js");

var index;

function eventVideoDone()
{
	const VIDEOS = ["SB1_7_MSG", "SB1_7_MSG2"];
	if (!camDef(index))
	{
		index = 0;
	}

	if (index < VIDEOS.length)
	{
		hackAddMessage(VIDEOS[index], MISS_MSG, CAM_HUMAN_PLAYER, true);
		index = index + 1;
	}
}

function eventStartLevel()
{
	camSetupTransporter(11, 52, 55, 1);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(camChangeOnDiff(1800)); //30 min
	eventVideoDone();
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_7");
}
