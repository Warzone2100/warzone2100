function setupTechLevel(player)
{

	//global function, doc/js-functions
	var techLevel = getMultiTechLevel();
	if (techLevel == 2)
	{
		completeResearchOnTime(timeT2, player);
	}
	else if (techLevel == 3)
	{
		completeResearchOnTime(timeT3, player);
	}
	else if (techLevel == 4)
	{
		completeAllResearch(player);
	}

}
