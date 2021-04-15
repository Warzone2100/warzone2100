// /////////////////////////////////////////////////////////////////
// END CONDITIONS
function checkEndConditions()
{
	let factories = countStruct("A0LightFactory") + countStruct("A0CyborgFactory");
	let droids = countDroid(DROID_ANY);

	// Losing Conditions
	if (droids == 0 && factories == 0)
	{
		let gameLost = true;

		/* If teams enabled check if all team members have lost  */
		if (alliancesType == ALLIANCES_TEAMS || alliancesType == ALLIANCES_UNSHARED)
		{
			for (let playnum = 0; playnum < maxPlayers; playnum++)
			{
				if (playnum != selectedPlayer && allianceExistsBetween(selectedPlayer, playnum))
				{
					factories = countStruct("A0LightFactory", playnum) + countStruct("A0CyborgFactory", playnum);
					droids = countDroid(DROID_ANY, playnum);
					if (droids > 0 || factories > 0)
					{
						gameLost = false;	// someone from our team still alive
						break;
					}
				}
			}
		}

		if (gameLost)
		{
			gameOverMessage(false);
			removeTimer("checkEndConditions");
			return;
		}
	}

	// Winning Conditions
	let gamewon = true;

	// check if all enemies defeated
	for (let playnum = 0; playnum < maxPlayers; playnum++)
	{
		if (playnum != selectedPlayer && !allianceExistsBetween(selectedPlayer, playnum))	// checking enemy player
		{
			factories = countStruct("A0LightFactory", playnum) + countStruct("A0CyborgFactory", playnum); // nope
			droids = countDroid(DROID_ANY, playnum);
			if (droids > 0 || factories > 0)
			{
				gamewon = false;	//one of the enemies still alive
				break;
			}
		}
	}

	if (gamewon)
	{
		gameOverMessage(true);
		removeTimer("checkEndConditions");
	}
}
