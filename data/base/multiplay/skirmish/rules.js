//
// Skirmish Base Script.
//
// contains the rules for starting and ending a game.
// as well as warning messages.
//

// TODO set tech level as a global, and set that up here as well
// TODO use gameTime instead of t second ticker...

// /////////////////////////////////////////////////////////////////

function eventGameInit()
{
	//set up the reticule buttons
	addReticuleButton(COMMAND);
	addReticuleButton(CANCEL);
	addReticuleButton(BUILD);
	addReticuleButton(MANUFACTURE);
	addReticuleButton(RESEARCH);
	addReticuleButton(INTELMAP);
	addReticuleButton(DESIGN);

	for (var playnum = 0; playnum < MAX_PLAYERS; playnum++)
	{
		enableStructure("A0CommandCentre", playnum);		// make structures available to build
		enableStructure("A0LightFactory", playnum);
		enableStructure("A0ResourceExtractor", playnum);
		enableStructure("A0PowerGenerator", playnum);
		enableStructure("A0ResearchFacility", playnum);
		
		setStructureLimits("A0LightFactory", 5, playnum);	// set structure limits
		setStructureLimits("A0PowerGenerator", 8, playnum);
		setStructureLimits("A0ResearchFacility", 5, playnum);
		setStructureLimits("A0CommandCentre", 1, playnum);
		setStructureLimits("A0ComDroidControl", 1, playnum);
		setStructureLimits("A0CyborgFactory", 5, playnum);
		setStructureLimits("A0VTolFactory1", 5, playnum);
	}
	applyLimitSet();	// set limit options

	const numCleanTech = 5;	// do x for clean	
	const numBaseTech = 20; // do x for base
	var techlist = new ARRAY(
		"R-Vehicle-Prop-Wheels",
		"R-Sys-Spade1Mk1",
		"R-Vehicle-Body01",
		"R-Comp-SynapticLink",
		"R-Cyborg-Legs01",
		"R-Wpn-MG1Mk1",
		"R-Defense-HardcreteWall",
		"R-Vehicle-Prop-Wheels",
		"R-Sys-Spade1Mk1",
		"R-Struc-Factory-Cyborg",
		"R-Cyborg-Wpn-MG",
		"R-Defense-Pillbox01",
		"R-Defense-Tower01",
		"R-Vehicle-Body01",
		"R-Sys-Engineering01",
		"R-Struc-CommandRelay",
		"R-Vehicle-Prop-Halftracks",
		"R-Comp-CommandTurret01",
		"R-Sys-Sensor-Turret01",
		"R-Wpn-Flamer01Mk1",
		"R-Vehicle-Body05",
		"R-Struc-Research-Module",
		"R-Struc-PowerModuleMk1",
		"R-Struc-Factory-Module",
		"R-Struc-RepairFacility",
		"R-Sys-MobileRepairTurret01",
		"R-Vehicle-Engine01",
		"R-Cyborg-Wpn-Cannon",
		"R-Cyborg-Wpn-Flamer",
		"R-Wpn-MG3Mk1",
		"R-Wpn-Cannon1Mk1",
		"R-Wpn-Mortar01Lt",
		"R-Defense-Pillbox05",
		"R-Defense-TankTrap01",
		"R-Defense-WallTower02",
		"R-Sys-Sensor-Tower01",
		"R-Defense-Pillbox04",
		"R-Wpn-MG2Mk1",
		"R-Wpn-Rocket05-MiniPod",
		"R-Wpn-MG-Damage01",
		"R-Wpn-Rocket-Damage01",
		"R-Defense-WallTower01");

	for (var playnum = 0; playnum < MAX_PLAYERS; playnum++)
	{
		enableResearch("R-Sys-Sensor-Turret01", playnum);
		enableResearch("R-Wpn-MG1Mk1", playnum);
		enableResearch("R-Sys-Engineering01", playnum);

		// enable cyborgs components that can't be enabled with research
		makeComponentAvailable("CyborgSpade", playnum);
		makeComponentAvailable("CyborgRepair", playnum);

		if (baseType == CAMP_CLEAN)
		{
			setPowerLevel(1300, playnum);
			for (var count = 0; count < numCleanTech; count++)
			{
				completeResearch(techlist[count], playnum);
			}
		}
		else if (baseType == CAMP_BASE)
		{
			setPowerLevel(2500, playnum);
			for (var count = 0; count < numBaseTech; count++)
			{
				completeResearch(techlist[count], playnum);
			}
		}
		else // CAMP_WALLS
		{
			setPowerLevel(2500, playnum);
			for (var count = 0; count < techlist.length; count++)
			{
				completeResearch(techlist[count], playnum);
			}
		}
	}
	setGlobalTimer("checkEndConditions", 100);
}

// /////////////////////////////////////////////////////////////////
// END CONDITIONS
function checkEndConditions()
{
	// Only read in data for players we check (speed optimization)
	var factories = Array(maxPlayers);
	var droids = Array(maxPlayers);
	for (var playnum = 0; playnum < maxPlayers; playernum++)
	{
		factories[playnum] = -1;
		droids[playnum] = -1;
	}
	factories[me] = enumStruct(me, factory).length;
	droids[me] = enumDroid(me).length;

	// Losing Conditions
	if (droids[me] == 0 && factories[me] == 0)
	{
		var gameLost = TRUE;

		/* If teams enabled check if all team members have lost  */
		if (alliancesType == ALLIANCES_TEAMS)
		{
			for (var playnum = 0; playnum < maxPlayers; playernum++)
			{
				if (playnum != me && allianceExistsBetween(me, playnum))
				{
					factories[playnum] = enumStruct(playnum, factory).length;
					droids[playnum] = enumDroid(playnum).length;
					if (droids[playnum] > 0 || factories[playnum] > 0)
					{
						gameLost = FALSE;	// someone from our team still alive
						break;
					}
				}
			}
		}

		if (gameLost)
		{
			gameOverMessage(endMsg, MISS_MSG, 0, FALSE);
			removeTimer("checkEndConditions");
			return;
		}
	}
	
	// Winning Conditions
	var gamewon = TRUE;

	// check if all enemies defeated
	for (var playnum = 0; playnum < maxPlayers; playernum++)
	{
		if (playnum != me && !allianceExistsBetween(me, playnum))	// checking enemy player
		{
			if (factories[playnum] == -1) // cached count?
			{
				factories[playnum] = enumStruct(playnum, factory).length; // nope
				droids[playnum] = enumDroid(playnum).length;
			}
			if (droids[playnum] > 0 || factories[playnum] > 0)
			{
				gamewon = FALSE;	//one of the enemies still alive
				break;
			}
		}
	}

	if (gamewon)
	{
		gameOverMessage(winMsg, MISS_MSG, 0, TRUE);
		removeTimer("checkEndConditions");
	}	
}

// /////////////////////////////////////////////////////////////////
// WARNING MESSAGES
// Base Under Attack
function eventBaseHit(CALL_STRUCT_ATTACKED, selectedPlayer, ref hitStruc, ref attackerObj)
{
	if (t >= 10)
	{
		t=0;
		if (hitStruc != NULLOBJECT)
		{
			playSoundPos("pcv337.ogg", selectedPlayer, hitStruc.x, hitStruc.y, hitStruc.z);	//show position if still alive
		}
		else
		{
			playSound("pcv337.ogg", selectedPlayer);
		}
	}
}

event everySec(every, 10)
{
	t=t+1;
}

//go to where the structure being attacked is on CTRL B
event seeBaseHit(CALL_MISSION_END)
{
	if (hitStruc!=NULLOBJECT)
	{
		centreView(hitStruc);
		t=0;							//flag known about!
	}
}

