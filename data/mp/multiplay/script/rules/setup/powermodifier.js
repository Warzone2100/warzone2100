/*
To change how much power each Power Level setting gives,
adjust the values 85 and 20. 85 is the base value, while 20 is
how much it changes between Power Level settings.

The math `85 + 20 * powerType`,
is interpreted in this way:
Low Power = 85,
Medium Power = 105,
High Power = 125.

The variable powerType is as follows:
powerType = 0 for Low Power,
powerType = 1 for Medium Power,
powerType = 2 for High Power,
powerType is a variable from the source code,
and is set when a Power Level setting is selected by the player,
please do not attempt to modify it.

To change the difficulty factor of bots,
adjust the variable diffMod under each difficulty check,
Do not touch 'let diffMod = 1;'.

The difficulty factor is interpreted in this way:
Easy = -25% power per second (0.75x),
Medium = =0% power per second (1x), no change,
Hard = +50% power per second (1.5x),
Insane = +100% power per second (2x).
*/
function setupPowerModifier(player)
{
	// This sets the player(s) power modifier,
	// it also sets the bots power modifier briefly,
	// but that is quickly overwritten in the bot section below.
	setPowerModifier(85 + 20 * powerType, player);

	// Set difficulty factor variable.
	let diffMod = 1;

	// The following sets the power modifier for bots.
	// Insane difficulty is meant to be insane...
	if (playerData[player].isAI)
	{
		if (playerData[player].difficulty === INSANE)
		{
			diffMod = 2;
		}
		else if (playerData[player].difficulty === HARD)
		{
			diffMod = 1.5;
		}
		else if (playerData[player].difficulty === MEDIUM)
		{
			diffMod = 0; // No change, but is present here for easy modding.
		}
		else if (playerData[player].difficulty === EASY)
		{
			diffMod = 0.75;
		}
		setPowerModifier((85 + 20 * powerType) * diffMod, player);
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
