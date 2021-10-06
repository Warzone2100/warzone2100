include("script/campaign/libcampaign.js");

function eventStartLevel()
{
     camSetupTransporter(50, 245, 63, 118);
     centreView(50, 245);
     setNoGoArea(49, 244, 51, 246, CAM_HUMAN_PLAYER);
     setNoGoArea(7, 52, 9, 54, NEXUS);
     setScrollLimits(0, 137, 64, 256);
     setMissionTime(camMinutesToSeconds(30));
     setPower(playerPower(CAM_HUMAN_PLAYER) + 50000, CAM_HUMAN_PLAYER);
     camPlayVideos([{video: "MB3_4_MSG", type: CAMP_MSG}, {video: "MB3_4_MSG2", type: MISS_MSG}]);
     camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "CAM_3_4");
}
