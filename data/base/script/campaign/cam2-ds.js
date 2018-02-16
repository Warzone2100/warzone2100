include("script/campaign/libcampaign.js");
var index;

function eventStartLevel()
{
    camSetupTransporter(87, 100, 1, 100);
    centreView(88, 101);
    setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
    setNoGoArea(49, 83, 51, 85, THE_COLLECTIVE);
    setMissionTime(camChangeOnDiff(4500)); // 1 hour, 15 minutes.
    camPlayVideos(["MB2_DI_MSG", "MB2_DI_MSG2"]);
    camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2D");
}
