/* powerType is as follows:
powerType = 0 for Low Power
powerType = 1 for Medium Power
powerType = 2 for High Power

To change how much power each Power Level setting gives,
adjust the values 85 and 20. 85 is the base value, while 20 is
how much it changes between Power Level settings.

To change the difficulty factor of bots,
adjust the last number in the bot section below.
*/
function setupPowerModifier(player)
{
	setPowerModifier(85 + 20 * powerType, player);

	// the following sets the power modifier for bots.
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
