include("script/campaign/libcampaign.js");

function eventStartLevel()
{
	camSetupTransporter(56, 120, 25, 87);
	centreView(57, 119);
	setNoGoArea(55, 119, 57, 121, CAM_HUMAN_PLAYER);
	setMissionTime(camChangeOnDiff(7200)); // 120 minutes
	camPlayVideos(["MB3_1A_MSG", "MB3_1A_MSG2"]);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_3_1");
}
