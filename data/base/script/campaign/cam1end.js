include("script/campaign/libcampaign.js");

var index;

function eventVideoDone()
{
	const VIDEOS = ["CAM1_OUT", "CAM1_OUT2", "CAM2_BRIEF"];
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
	camSetupTransporter(11, 52, 40, 1);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(900); //15 min
	eventVideoDone();
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "CAM_2A");
}
