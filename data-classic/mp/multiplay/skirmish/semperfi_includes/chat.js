
function eventChat(from, to, message)
{
	if (to !== me || to === from)
	{
		return; // not for me
	}
	if (message === "donatetruck" && allianceExistsBetween(from, to))
	{
		// donate first truck
		var droids = enumDroid(me, DROID_CONSTRUCT);
		if (droids.length > 0)
		{
			donateObject(droids[0], from);
		}
	}
	else if (message === "donatepower" && allianceExistsBetween(from, to))
	{
		donatePower(playerPower(me) / 2, from);
	}
	else if (message === "antiair" && allianceExistsBetween(from, to))
	{
		enemyHasVtol = true;
	}
	else if (message === "crazycolours")
	{
		setSunIntensity(0.6, 0.4, 0.3,  1.0, 0.8, 0.7,  1.2, 0.9, 0.8);
	}
}
