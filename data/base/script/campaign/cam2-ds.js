include("script/campaign/libcampaign.js");

//"Commander, the only reactor located in this sector"...
function secondVideo()
{
	hackAddMessage("MB2_DI_MSG2", MISS_MSG, CAM_HUMAN_PLAYER, true);
}

function eventVideoDone()
{
	camCallOnce("secondVideo");
}

function eventStartLevel()
{
	index = 0;
    camSetupTransporter(87, 100, 1, 100);
    centreView(88, 101);
    setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
    setMissionTime(4500); // 1 hour, 15 minutes.
	//"Commander, we are now able to research and build VTOLs"...
	hackAddMessage("MB2_DI_MSG", MISS_MSG, CAM_HUMAN_PLAYER, true);
	camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2D");
}
