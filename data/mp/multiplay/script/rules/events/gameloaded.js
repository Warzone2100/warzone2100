function eventGameLoaded()
{
	setupGame();
	queue("setupPowerModifierGameLoaded", 100);
}
