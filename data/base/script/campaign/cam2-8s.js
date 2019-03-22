include("script/campaign/libcampaign.js");

function eventStartLevel()
{
    camSetupTransporter(87, 100, 126, 60);
    centreView(88, 101);
    setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
    setNoGoArea(49, 83, 51, 85, THE_COLLECTIVE);
    setMissionTime(camChangeOnDiff(camHoursToSeconds(1)));
    camPlayVideos(["MB2_8_MSG", "MB2_8_MSG2"]);
    camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_8");
}
