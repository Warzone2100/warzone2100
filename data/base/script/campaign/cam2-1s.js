/* 
SUB_2_1S campaign script
Authors: Cristian Odorico (Alpha93) / KJeff01
 */
include("script/campaign/libcampaign.js");

function secondVideo()
{
    hackAddMessage("MB2_1_MSG2", MISS_MSG, CAM_HUMAN_PLAYER, true);
}

function eventVideoDone()
{
    hackRemoveMessage("MB2_1_MSG", MISS_MSG, CAM_HUMAN_PLAYER);
    camCallOnce("secondVideo");
}

function eventStartLevel()
{
    //Setup Transport
    camSetupTransporter(87, 100, 70, 126);
    //Centre View on Area
    centreView(88, 101);
    //Setup Landing Zone
    setNoGoArea(86, 99, 88, 101, CAM_HUMAN_PLAYER);
    //Set Mission Time
    setMissionTime(1800);
    //Give player briefings, remove the first message after it is played
    hackAddMessage("MB2_1_MSG", MISS_MSG, CAM_HUMAN_PLAYER, true);    
    //Set Victory Conditions
    camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_1");   
}
