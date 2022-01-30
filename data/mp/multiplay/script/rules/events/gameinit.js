function eventGameInit()
{
	//From script/rules/setupgame.js
	setupGame();

	//From script/rules/printsettings.js
	printGameSettings();

	//From script/rules/oildrum.js
	oilDrumInit();

	//global function, doc/js-functions
	hackNetOff();

	//From script/setup/setupscavenger.js
	setupScavenger();

	for (var playnum = 0; playnum < maxPlayers; playnum++)
	{
		//From script/setup/powermodifier.js
		setupPowerModifier(playnum);

		//From script/setup/droidlimit.js
		droidLimit(playnum);

		//From script/setup/setupstructure.js
		setupStructure(playnum);

		//From script/setup/setupstructurelimit.js
		setupStructureLimit(playnum);

		//From script/setup/setupresearch.js
		setupResearch(playnum);

		//From script/setup/setupcomponents.js
		setupComponents(playnum);

		//From script/setup/setupbase.js
		setupBase(playnum);

		//From script/setup/setuptechlevels.js
		setupTechLevel(playnum);

	}

	applyLimitSet();	// set limit options

	hackNetOn();

	//Structures might have been removed so we need to update the reticule button states again
	//From script/rules/reticule.js
	setMainReticule();

	if (tilesetType === "URBAN" || tilesetType === "ROCKIES")
	{
		setTimer("weatherCycle", 45000);
	}
	setTimer("autoSave", 10*60*1000);
}
