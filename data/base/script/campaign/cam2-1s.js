/* 
SUB_2_1S campaign script
Author: Cristian Odorico / Alpha93
 */
include("script/campaign/libcampaign.js");
var CAM_HUMAN_PLAYER = 0;

function eventLevelStart()
{
    //Variables initialization
    var startpos = getObject("startingPosition");
    var lz = getObject("landingZone");
    var takeOff = getObject("transporterTakeOff");
    var exitPoint = getObject("transporterExit");
    //Setup Transport
    camSetupTransporter(takeOff.x, takeOff.y, exitPoint.x, exitPoint.y);
    //Centre View on Area
    centreView(startpos.x, startpos.y);
    //Setup Landing Zone
    setNoGoArea(lz.x, lz.y, lz.x2, lz.y2);
    //Set Mission Time
    setMissionTime(1800);
    //Give player briefings, remove the first message after it is played
    hackAddMessage("MB2_1MSG", MISS_MSG, CAM_HUMAN_PLAYER, false);
    hackRemoveMessage("MB2_1MSG", MISS_MSG, CAM_HUMAN_PLAYER);
    hackAddMessage("MB2_1MSG2", MISS_MSG, CAM_HUMAN_PLAYER, true);
    //Set Victory Conditions
    camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_1");   
}
