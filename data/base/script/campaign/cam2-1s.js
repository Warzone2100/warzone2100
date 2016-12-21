/* 
SUB_2_1S campaign script
 */
include("script/campaign/libcampaign.js");

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
    //Give player briefings
    hackAddMessage("MB2_1MSG", MISS_MSG, 0, true);
    hackAddMessage("MB2_1MSG2", MISS_MSG, 0, false);
    //Set Victory Conditions
    camSetStandardWinLossConditions(CAM_VICTORY_PRE_OFFWORLD, "SUB_2_1");
    
}