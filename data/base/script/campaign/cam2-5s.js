include("script/campaign/libcampaign.js");

//"Commander, the only reactor located in this sector"...
function secondVideo()
{
	hackAddMessage("MB2_5_MSG2", MISS_MSG, CAM_HUMAN_PLAYER, true);
}

function eventVideoDone()
{
	camCallOnce("secondVideo");
}

function eventStartLevel()
{
	index = 0;
    camSetupTransporter(87, 100, 32, 1);
    centreView(88, 101);
    setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
    setMissionTime(3600); // 1 hour
	//"Commander, we are now able to research and build VTOLs"...
	hackAddMessage("MB2_5_MSG", MISS_MSG, CAM_HUMAN_PLAYER, true);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_5");
}
