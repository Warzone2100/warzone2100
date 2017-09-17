include("script/campaign/libcampaign.js");

function eventStartLevel()
{
	var ti = camChangeOnDiff(3600); //60 min
	camSetupTransporter(11, 52, 100, 126);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(ti);
	hackAddMessage("SB1_5_MSG", MISS_MSG, CAM_HUMAN_PLAYER, true);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_5");
}
