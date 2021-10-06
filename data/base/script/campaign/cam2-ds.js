include("script/campaign/libcampaign.js");
var index;

function eventStartLevel()
{
    camSetupTransporter(87, 100, 1, 100);
    centreView(88, 101);
    setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
    setNoGoArea(49, 83, 51, 85, THE_COLLECTIVE);
    setMissionTime(camChangeOnDiff(camMinutesToSeconds(75)));
    camPlayVideos([{video: "MB2_DI_MSG", type: MISS_MSG}, {video: "MB2_DI_MSG2", type: CAMP_MSG}]);
    camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2D");
}
