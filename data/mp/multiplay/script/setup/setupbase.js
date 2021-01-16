function setupBase(player)	// inside hackNetOff()
{
	if (baseType == CAMP_CLEAN)
	{
		setPower(1300, player);
		completeResearchOnTime(cleanTech, player);
		// Keep only some structures for insane AI
		var structs = enumStruct(player);
		for (var i = 0; i < structs.length; i++)
		{
			var s = structs[i];
			if (playerData[player].difficulty != INSANE
				|| (s.stattype != WALL && s.stattype != DEFENSE && s.stattype != GATE
					&& s.stattype != RESOURCE_EXTRACTOR))
			{
				removeObject(s, false);
			}
		}
	}
	else if (baseType == CAMP_BASE)
	{
		setPower(2500, player);
		completeResearchOnTime(timeBaseTech, player);
		// Keep only some structures
		var structs = enumStruct(player);
		for (var i = 0; i < structs.length; i++)
		{
			var s = structs[i];
			if ((playerData[player].difficulty != INSANE && (s.stattype == WALL || s.stattype == DEFENSE))
				|| s.stattype == GATE || s.stattype == CYBORG_FACTORY || s.stattype == COMMAND_CONTROL)
			{
				removeObject(s, false);
			}
		}
	}
	else // CAMP_WALLS
	{
		setPower(2500, player);
		completeResearchOnTime(timeAdvancedBaseTech, player);
	}
}
