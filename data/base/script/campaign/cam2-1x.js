/* 
SUB_2_1 Script
Author: Cristian Odorico (Alpha93)
 */
//libraries initialization
include ("script/campaign/libcampaign.js");
include ("script/campaign/templates.js");

//variables initialization
var player = 0;
var downedTransportTeam = 1;
var downedTransportUnits = [];
var successSound = "pcv615.ogg";
var failureSound = "pcv622.ogg";
var customVictoryFlag = 0;
var remainingTime = 0;

// generate a random number between a min and max range (both ends included)
function preDamage(min, max)
{
    min = Math.ceil(min);
    max = Math.floor(max);
    return Math.floor(Math.random () * ( max - min + 1) + min);
}

// function that applies damage to units in the downed transport transport team
function preDamageUnits() 
{
    // fill the array with the objects defining the allied units in the crash site area
    downedTransportUnits = enumArea ("crashSite", ALLIES, false); 
    var j = 0; // index initialization
    for ( j = 0; j < downedTransportUnits.length; j++)
    {
        // if Object type is DROID run this
        if ( downedTransportUnits.type[j] === DROID ) 
        {
            //reduce unit HPs between 50% and 70%
            downedTransportUnits.health[j] = downedTransportUnits.health[j] * preDamage(0.3, 0.5);
        }
         // if Object type is STRUCTURE (i.e. j index points to the element containing the transport)
        if ( downedTransportUnits.type[j] === STRUCTURE)
        {
            //reduce transport HPs anywhere between 30% and 50%
            downedTransportUnits.health[j] = downedTransportUnits.health[j] * preDamage(0.5, 0.7); 
        }
    }
    return downedTransportUnits;
}

//trigger event when droid reaches the downed transport
camAreaEvent("crashSite", function(droid)
{
    // initialize index variable to transfer droid
    var i = 0;
    // victory conditions check and unit transfer
    for ( i = 0; i < downedTransportUnits.length; i++)
    {
        if (downedTransportUnits[i].type === STRUCTURE) //condition to
        {
            if (downedTransportUnits[i].health !== NULLOBJECT)
            {
                playSound(successSound);
                customVictoryFlag = 1;
            }
            else
            {
                playSound(failureSound);
                gameOverMessage(false);
            }
        }
        //transfer the units
        eventObjectTransfer (downedTransportUnits[i].me, downedTransportTeam); 
    }
    remainingTime = getMissionTime();
    if (customVictoryFlag === 1)
    {
        extraPowerTime(remainingTime, player);
        camNextLevel("CAM_2B");
    }
});

function eventStartLevel()
{
    //variables initialization for LZ setup
    var subLandingZone = getObject("subLandingZone");
    //set landing zone
    setNoGoArea(subLandingZone.x, subLandingZone.y, subLandingZone.x2, subLandingZone.y2);
    //set alliance between player and AI downed transport team
    setAlliance(player, downedTransportTeam, true);
    //disable reinforcements
    setReinforcementTime(-1);
};
