function setupBase(player)	// inside hackNetOff()
{
	if (baseType === CAMP_CLEAN)
	{
		setPower(1300, player);
		completeResearchOnTime(cleanTech, player);
		removeStructs(player);
	}
	else if (baseType === CAMP_BASE)
	{
		setPower(2500, player);
		completeResearchOnTime(timeBaseTech, player);
		removeStructs(player);
	}
	else // CAMP_WALLS
	{
		setPower(2500, player);
		completeResearchOnTime(timeAdvancedBaseTech, player);
	}
}

function removeStructs(player)
{
	for (const structure of enumStruct(player))
	{
		if (shouldRemove(structure, player))
		{
			removeObject(structure);
		}
	}
}

function shouldRemove(structure, player)
{
	if (baseType === CAMP_CLEAN && playerData[player].difficulty !== INSANE)
	{
		return true; // remove everything
	}
	if (baseType === CAMP_CLEAN && playerData[player].difficulty === INSANE)
	{
		return structure.stattype !== WALL
			&& structure.stattype !== DEFENSE
			&& structure.stattype !== GATE
			&& structure.stattype !== RESOURCE_EXTRACTOR;
	}
	else if (baseType === CAMP_BASE && playerData[player].difficulty !== INSANE)
	{
		return structure.stattype === WALL
			|| structure.stattype === DEFENSE
			|| structure.stattype === GATE
			|| structure.stattype === CYBORG_FACTORY
			|| structure.stattype === COMMAND_CONTROL;
	}
	else if (baseType === CAMP_BASE && playerData[player].difficulty === INSANE)
	{
		return structure.stattype === CYBORG_FACTORY
			|| structure.stattype === COMMAND_CONTROL;
	}
	else
	{
		return false; // don't remove anything
	}
}
