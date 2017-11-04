include("script/campaign/libcampaign.js");

function eventStartLevel()
{
    camSetupTransporter(87, 100, 100, 1);
    centreView(88, 101);
    setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
    setMissionTime(camChangeOnDiff(5400)); // 1 hour - 30 minutes.
    camPlayVideos(["MB2_7_MSG", "MB2_7_MSG2"]);
    camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_7");
}
