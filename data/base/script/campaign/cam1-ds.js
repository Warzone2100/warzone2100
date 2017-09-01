include("script/campaign/libcampaign.js");

var index;

function eventVideoDone()
{
	const VIDEOS = ["MB1D_MSG", "MB1D_MSG2"];
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
	camSetupTransporter(11, 52, 126, 112);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(camChangeOnDiff(7200)); //2 hours
	eventVideoDone();
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_D");
}
