
include("script/campaign/libcampaign.js");

function eventStartLevel()
{
	camSetupTransporter(11, 52, 39, 126);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, 0);
	setMissionTime(3600);
	hackAddMessage("SB1_3_UPDATE", MISS_MSG, 0, true);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_3");
}
