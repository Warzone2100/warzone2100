function setupBase(player)	// inside hackNetOff()
{
	if (baseType === CAMP_CLEAN)
	{
		setPower(1300, player);
		completeResearchOnTime(cleanTech, player);
		removeBaseStructures(player);
	}
	else if (baseType === CAMP_BASE)
	{
		setPower(2500, player);
		completeResearchOnTime(timeBaseTech, player);
		removeBaseStructures(player);
	}
	else // CAMP_WALLS
	{
		setPower(2500, player);
		completeResearchOnTime(timeAdvancedBaseTech, player);
	}
}

function removeBaseStructures(player)
{
	for (const structure of enumStruct(player))
	{
		if (shouldRemoveBaseStructure(structure.stattype, player))
		{
			removeObject(structure);
		}
	}
}

function shouldRemoveBaseStructure(stattype, player)
{
	if (baseType === CAMP_CLEAN && playerData[player].difficulty !== INSANE)
	{
		return true; // remove everything
	}
	else if (baseType === CAMP_CLEAN && playerData[player].difficulty === INSANE)
	{
		return stattype !== WALL
			&& stattype !== DEFENSE
			&& stattype !== GATE
			&& stattype !== RESOURCE_EXTRACTOR;
	}
	else if (baseType === CAMP_BASE && playerData[player].difficulty !== INSANE)
	{
		return stattype === WALL
			|| stattype === DEFENSE
			|| stattype === GATE
			|| stattype === CYBORG_FACTORY
			|| stattype === COMMAND_CONTROL;
	}
	else if (baseType === CAMP_BASE && playerData[player].difficulty === INSANE)
	{
		return stattype === CYBORG_FACTORY
			|| stattype === COMMAND_CONTROL;
	}
	else // CAMP_WALLS
	{
		return false; // don't remove anything
	}
}
