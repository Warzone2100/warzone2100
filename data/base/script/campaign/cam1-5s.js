include("script/campaign/libcampaign.js");

function eventStartLevel()
{
	camSetupTransporter(11, 52, 100, 126);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, 0);
	setMissionTime(3600); //60 min
	hackAddMessage("SB1_5_MSG", MISS_MSG, 0, true);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_5");
}
