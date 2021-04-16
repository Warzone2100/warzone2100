function setupPowerModifier(player)
{
	if (powerType == 0)
	{
		setPowerModifier(85, player);
	}
	else if (powerType == 2)
	{
		setPowerModifier(125, player);
	}

	// insane difficulty is meant to be insane...
	if (playerData[player].difficulty == INSANE)
	{
		setPowerModifier(200 + 15 * powerType, player);
	}
	else if (playerData[player].difficulty == HARD)
	{
		setPowerModifier(150 + 10 * powerType, player);
	}
	else if (playerData[player].difficulty == EASY)
	{
		setPowerModifier(70 + 5 * powerType, player);
	}
}

// Power modifier data is currently not saved. Reset it during eventGameLoaded for the moment.
function setupPowerModifierGameLoaded()
{
	for (let i = 0; i < maxPlayers; ++i)
	{
		setupPowerModifier(i);
	}
}
