/* 
SUB_2_1 Script
Authors: Cristian Odorico (Alpha93) / KJeff01
 */
//libraries initialization
include ("script/campaign/libcampaign.js");
include ("script/campaign/templates.js");

//constants initialization
const downedTransportTeam = 1;

//function that applies damage to units in the downed transport transport team
function preDamageUnits()
{
    // fill the array with the objects defining the allied units in the crash site area
    var downedTransportUnits = enumArea ("crashSite", ALLIES, false);
    
    for (j = 0; j < downedTransportUnits.length; j++)
    {
        setHealth(downedTransportUnits, 40 + camRand(20));
    }
}

//create group of cyborgs and send them on war path
camManageGroup ( camMakeGroup("cyborgPosition"), CAM_ORDER_ATTACK, {
                            pos: camMakePos ("cyborgAttack"),
                            morale: 50,
                            regroup: true
                            });
//all other tanks are to defend their positions
                          
//trigger event when droid reaches the downed transport
camAreaEvent("crashSite", function(droid)
{
    //initialize function specific variables
    const successSound = "pcv615.ogg";
    const failureSound = "pcv622.ogg";
    const downedTransportTeam = 1;
    //count structures in the crash area
    const transport = enumStruct(downedTransportTeam).length;
    var customVictoryFlag = 0;
    var remainingTime = 0;
    //remove blip
    hackRemoveMessage("C21_OBJECTIVE", PROX_MSG, CAM_HUMAN_PLAYER, true);
    //victory condition: transport must be alive
    if (transport === 1)
    {
            playSound(successSound);
            customVictoryFlag = 1;
    }
    else
    {
            //if transport died, the game is over
            playSound(failureSound);
            gameOverMessage(false);
    }             

    //store the time in the variable to give power later
    remainingTime = getMissionTime();

    //if transport is alive, transfer units, turn time into power and load next level
    if (customVictoryFlag === 1)
    {
        //get a list of droids in the downed transport team array
        var downedTransportUnits = enumDroid(downedTransportTeam);
        for(var i = 0; i < downedTransportUnits.length; i++)
        {
            //transfer the units
            donateObject(downedTransportUnits[i], CAM_HUMAN_PLAYER);
        }
        //turn time into power
        extraPowerTime(remainingTime, CAM_HUMAN_PLAYER);
        //load next level
        camNextLevel("CAM_2B");
    }
});

function eventStartLevel()
{
    //variables initialization for LZ setup
    var subLandingZone = getObject("landingZone");
    //set landing zone
    setNoGoArea(subLandingZone.x, subLandingZone.y, subLandingZone.x2, subLandingZone.y2);
    //set alliance between player and AI downed transport team
    setAlliance(CAM_HUMAN_PLAYER, downedTransportTeam, true);
    //set downed transport team colour to be Project Green
    changePlayerColour(downedTransportTeam, 0);
    //disable reinforcements
    setReinforcementTime(-1);
    //centre view on starting position
    var startpos = getObject("startingPosition");
    centreView(startpos.x, startpos.y);
    //Setup transporter entry/exit points
    var tent = getObject("transporterEntry");
    startTransporterEntry(tent.x, tent.y, CAM_HUMAN_PLAYER);
    var text = getObject("transporterExit");
    setTransporterExit(text.x, text.y, CAM_HUMAN_PLAYER);
    //add crash site blip
    hackAddMessage("C21_OBJECTIVE", PROX_MSG, CAM_HUMAN_PLAYER, true);
};
