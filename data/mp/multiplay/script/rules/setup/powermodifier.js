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
		if (powerType === 0)
		{
			setPowerModifier(170, player);
		}
		else if (powerType === 1)
		{
			setPowerModifier(210, player);
		}
		else if (powerType === 2)
		{
			setPowerModifier(250, player);
		}
	}
	else if (playerData[player].difficulty === HARD)
	{
		if (powerType === 0)
		{
			setPowerModifier(128, player);
		}
		else if (powerType === 1)
		{
			setPowerModifier(158, player);
		}
		else if (powerType === 2)
		{
			setPowerModifier(188, player);
		}
	}
	else if (playerData[player].difficulty === MEDIUM)
	{
		if (powerType === 0)
		{
			setPowerModifier(94, player);
		}
		else if (powerType === 1)
		{
			setPowerModifier(116, player);
		}
		else if (powerType === 2)
		{
			setPowerModifier(138, player);
		}
	}
	else if (playerData[player].difficulty === EASY)
	{
		if (powerType === 0)
		{
			setPowerModifier(64, player);
		}
		else if (powerType === 1)
		{
			setPowerModifier(79, player);
		}
		else if (powerType === 2)
		{
			setPowerModifier(94, player);
		}
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
