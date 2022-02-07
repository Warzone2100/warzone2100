
include("script/campaign/libcampaign.js");

function eventStartLevel()
{
	camSetupTransporter(11, 52, 39, 126);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(camChangeOnDiff(camHoursToSeconds(1)));
	camPlayVideos({video: "SB1_3_UPDATE", type: CAMP_MSG});
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_3");
}
