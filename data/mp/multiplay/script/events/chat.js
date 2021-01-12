function eventChat(from, to, message)
{
	if (message == "bettertogether" && cheatmode)
	{
		for (var i in Upgrades[from].Brain)
		{
			if (Upgrades[from].Brain[i].BaseCommandLimit > 0) // is commander
			{
				Upgrades[from].Brain[i].BaseCommandLimit += 4;
				Upgrades[from].Brain[i].CommandLimitByLevel += 2;
				// you must set the thresholds this way, as an array, because of the clunky
				// way that this is implemented behind the scenes
				Upgrades[from].Brain[i].RankThresholds = [ 0, 2, 4, 8, 16, 24, 32, 48, 64 ];
			}
		}
		console("Made player " + from + "'s commanders SUPERIOR!");
	}
	if (message == "makesuperior" && cheatmode)
	{
		for (var i in Upgrades[from].Body)
		{
			if (Upgrades[from].Body[i].bodyClass === 'Droids' || Upgrades[from].Body[i].bodyClass === 'Cyborgs')
			{
				Upgrades[from].Body[i].HitPoints += 500;
				Upgrades[from].Body[i].HitPointPct += 100;
				Upgrades[from].Body[i].Armour += 500;
				Upgrades[from].Body[i].Thermal += 500;
				Upgrades[from].Body[i].Power += 500;
			}
		}
		console("Made player " + from + "'s units SUPERIOR!");
	}
}
