include("script/campaign/libcampaign.js");

function eventStartLevel()
{
	camSetupTransporter(11, 52, 126, 112);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(camChangeOnDiff(camHoursToSeconds(2)));
	camPlayVideos([{video: "MB1D_MSG", type: CAMP_MSG}, {video: "MB1D_MSG2", type: MISS_MSG}]);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_D");
}
