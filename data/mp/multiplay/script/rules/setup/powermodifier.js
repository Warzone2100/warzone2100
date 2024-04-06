function setupPowerModifier(player)
{
	if (powerType === 0)
	{
		setPowerModifier(85, player);
	}
	else if (powerType === 1)
	{
		setPowerModifier(105, player);
	}
	else if (powerType === 2)
	{
		setPowerModifier(125, player);
	}

	// insane difficulty is meant to be insane...
	if (playerData[player].difficulty === INSANE)
	{
		setPowerModifier((85 + 20 * powerType) * 2, player);
	}
	else if (playerData[player].difficulty === HARD)
	{
		setPowerModifier((85 + 20 * powerType) * 1.5, player);
	}
	else if (playerData[player].difficulty === MEDIUM)
	{
		setPowerModifier((85 + 20 * powerType) * 1.1, player);
	}
	else if (playerData[player].difficulty === EASY)
	{
		setPowerModifier((85 + 20 * powerType) * 0.75, player);
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
