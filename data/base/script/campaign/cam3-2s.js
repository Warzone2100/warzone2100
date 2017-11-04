include("script/campaign/libcampaign.js");

function eventStartLevel()
{
	camSetupTransporter(56, 120, 25, 87);
	centreView(57, 119);
	setNoGoArea(55, 119, 57, 121, CAM_HUMAN_PLAYER);
	setMissionTime(camChangeOnDiff(3600)); // 60 minutes
	camPlayVideos(["MB3_2_MSG", "MB3_2_MSG2"]);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_3_2");
}
