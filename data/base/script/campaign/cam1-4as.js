
include("script/campaign/libcampaign.js");

function eventStartLevel()
{
	camSetupTransporter(11, 52, 80, 1);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, 0);
	setMissionTime(1800);
	hackAddMessage("SB1_4_MSG", MISS_MSG, 0, true);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_4A");
}
