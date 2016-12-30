/* 
cam2bx.vlo/slo conversion to JS
Author: Cristian Odorico (Alpha93)
 */

function eventStartLevel()
{
    //variable initialization and player power acquisition
    var myPower = playerPower(me);
    var powerTransfer = 4000;
    var powerTransferSound = "power-transferred.ogg";
    //increase player power level and play sound
    myPower = myPower + powerTransfer;
    playSound(powerTransferSound);
}
