include("script/campaign/libcampaign.js");

function eventStartLevel()
{
	camSetupTransporter(11, 52, 40, 1);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(900); //15 min
	camPlayVideos(["CAM1_OUT", "CAM1_OUT2", "CAM2_BRIEF"]);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "CAM_2A");
}
