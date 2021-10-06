include("script/campaign/libcampaign.js");

function eventStartLevel()
{
	camSetupTransporter(11, 52, 40, 1);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(camMinutesToSeconds(15));
	camPlayVideos([{video: "CAM1_OUT", type: CAMP_MSG}, {video: "CAM1_OUT2", type: CAMP_MSG}, {video: "CAM2_BRIEF", type: CAMP_MSG}]);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "CAM_2A");
}
