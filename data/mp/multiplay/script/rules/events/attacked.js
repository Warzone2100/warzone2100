// /////////////////////////////////////////////////////////////////
// WARNING MESSAGES
// Base Under Attack
function eventAttacked(victimObj, attackerObj)
{
	if ((victimObj.player === selectedPlayer) && (attackerObj.player !== selectedPlayer) && (gameTime > (lastHitTime + 5000)))
	{
		lastHitTime = gameTime;
		if (victimObj.type === STRUCTURE)
		{
			playSound("pcv337.ogg", victimObj.x, victimObj.y, victimObj.z);	// show position if still alive
		}
		else
		{
			playSound("pcv399.ogg", victimObj.x, victimObj.y, victimObj.z);
		}
	}
}
