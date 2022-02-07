
include("script/campaign/libcampaign.js");

function eventStartLevel()
{
	camSetupTransporter(11, 52, 39, 1);
	centreView(13, 52);
	setNoGoArea(10, 51, 12, 53, CAM_HUMAN_PLAYER);
	setMissionTime(camChangeOnDiff(camMinutesToSeconds(30)));
	camPlayVideos({video: "SB1_2_MSG", type: CAMP_MSG});
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_1_2");
}
