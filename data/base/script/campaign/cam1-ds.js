include("script/campaign/libcampaign.js");

function secondVideo()
{
	hackAddMessage("MB1D_MSG2", MISS_MSG, CAM_HUMAN_PLAYER, true);
}

function eventVideoDone()
{
	camCallOnce("secondVideo");
}

function eventStartLevel()
{
	camSetupTransporter(11, 52, 126, 112);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(7200); //2 hours
	hackAddMessage("MB1D_MSG", MISS_MSG, CAM_HUMAN_PLAYER, true);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_D");
}
