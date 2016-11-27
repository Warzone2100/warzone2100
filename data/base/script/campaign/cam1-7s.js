include("script/campaign/libcampaign.js");

function secondVideo()
{
	hackAddMessage("SB1_7_MSG2", MISS_MSG, CAM_HUMAN_PLAYER, true);
}

function eventStartLevel()
{
	camSetupTransporter(11, 52, 55, 1);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(1800); //30 min
	hackAddMessage("SB1_7_MSG", MISS_MSG, CAM_HUMAN_PLAYER, true);
	secondVideo();
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_7");
}
