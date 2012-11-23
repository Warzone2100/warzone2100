//
// Skirmish Base Script.
//
// contains the rules for starting and ending a game.
// as well as warning messages.
//
// /////////////////////////////////////////////////////////////////

var lastHitTime = 0;

function eventGameInit()
{
	hackNetOff();
	for (var playnum = 0; playnum < maxPlayers; playnum++)
	{
		setDroidLimit(playnum, 150);

		enableStructure("A0CommandCentre", playnum);		// make structures available to build
		enableStructure("A0LightFactory", playnum);
		enableStructure("A0ResourceExtractor", playnum);
		enableStructure("A0PowerGenerator", playnum);
		enableStructure("A0ResearchFacility", playnum);

		// We need to enable these in order for scripts to be able to generate their templates.
		makeComponentAvailable("CyborgLegs", playnum);
		makeComponentAvailable("Cyb-Wpn-Atmiss", playnum);
		makeComponentAvailable("CyborgCannon", playnum);
		makeComponentAvailable("CyborgChaingun", playnum);
		makeComponentAvailable("CyborgFlamer01", playnum);
		makeComponentAvailable("Cyb-Wpn-Grenade", playnum);
		makeComponentAvailable("Cyb-Hvywpn-A-T", playnum);
		makeComponentAvailable("Cyb-Hvywpn-Acannon", playnum);
		makeComponentAvailable("Cyb-Hvywpn-HPV", playnum);
		makeComponentAvailable("Cyb-Hvywpn-Mcannon", playnum);
		makeComponentAvailable("Cyb-Hvywpn-PulseLsr", playnum);
		makeComponentAvailable("Cyb-Hvywpn-RailGunner", playnum);
		makeComponentAvailable("Cyb-Hvywpn-TK", playnum);
		makeComponentAvailable("Cyb-Wpn-Laser", playnum);
		makeComponentAvailable("Cyb-Wpn-Rail1", playnum);
		makeComponentAvailable("CyborgRocket", playnum);
		makeComponentAvailable("CyborgRotMG", playnum);
		makeComponentAvailable("Cyb-Wpn-Thermite", playnum);
		makeComponentAvailable("CyborgFlamer01", playnum);
		
		setStructureLimits("A0LightFactory", 5, playnum);	// set structure limits
		setStructureLimits("A0PowerGenerator", 8, playnum);
		setStructureLimits("A0ResearchFacility", 5, playnum);
		setStructureLimits("A0CommandCentre", 1, playnum);
		setStructureLimits("A0ComDroidControl", 1, playnum);
		setStructureLimits("A0CyborgFactory", 5, playnum);
		setStructureLimits("A0VTolFactory1", 5, playnum);
	}
	applyLimitSet();	// set limit options

	const numCleanTech = 4;	// do x for clean	
	const numBaseTech = 18; // do x for base
	var techlist = new Array(
		"R-Vehicle-Prop-Wheels",
		"R-Sys-Spade1Mk1",
		"R-Vehicle-Body01",
		"R-Comp-SynapticLink",
		"R-Wpn-MG1Mk1",
		"R-Defense-HardcreteWall",
		"R-Vehicle-Prop-Wheels",
		"R-Sys-Spade1Mk1",
		"R-Struc-Factory-Cyborg",
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
		"R-Defense-WallTower01",
		"R-Defense-Tower06");

	for (var playnum = 0; playnum < maxPlayers; playnum++)
	{
		enableResearch("R-Sys-Sensor-Turret01", playnum);
		enableResearch("R-Wpn-MG1Mk1", playnum);
		enableResearch("R-Sys-Engineering01", playnum);

		// enable cyborgs components that can't be enabled with research
		makeComponentAvailable("CyborgSpade", playnum);
		makeComponentAvailable("CyborgRepair", playnum);

		setPower(2000, playnum);

		if (baseType == CAMP_CLEAN)
		{
			for (var count = 0; count < numCleanTech; count++)
			{
				completeResearch(techlist[count], playnum);
			}
			// Keep only some structures for insane AI
			var structs = enumStruct(playnum);
			for (var i = 0; i < structs.length; i++)
			{
				var s = structs[i];
				if (playerData[playnum].difficulty != INSANE
				    || (s.stattype != WALL && s.stattype != DEFENSE && s.stattype != GATE
				        && s.stattype != RESOURCE_EXTRACTOR))
				{
					removeObject(s, false);
				}
			}
		}
		else if (baseType == CAMP_BASE)
		{
			for (var count = 0; count < numBaseTech; count++)
			{
				completeResearch(techlist[count], playnum);
			}
			// Keep only some structures
			var structs = enumStruct(playnum);
			for (var i = 0; i < structs.length; i++)
			{
				var s = structs[i];
				if ((playerData[playnum].difficulty != INSANE && (s.stattype == WALL || s.stattype == DEFENSE))
				    || s.stattype == GATE || s.stattype == CYBORG_FACTORY || s.stattype == COMMAND_CONTROL)
				{
					removeObject(s, false);
				}
			}
		}
		else // CAMP_WALLS
		{
			for (var count = 0; count < techlist.length; count++)
			{
				completeResearch(techlist[count], playnum);
			}
		}
	}

	// Disabled by default
	setMiniMap(false);
	setDesign(false);
	// This is the only template that should be enabled before design is allowed
	enableTemplate("ConstructionDroid");

	var structlist = enumStruct(me, HQ);
	for (var i = 0; i < structlist.length; i++)
	{
		// Simulate build events to enable minimap/unit design when an HQ exists
		eventStructureBuilt(structlist[i]);
	}

	hackNetOn();
	setTimer("checkEndConditions", 100);
}

// /////////////////////////////////////////////////////////////////
// END CONDITIONS
function checkEndConditions()
{
	var factories = countStruct("A0LightFactory") + countStruct("A0CyborgFactory");
	var droids = countDroid(DROID_ANY);

	// Losing Conditions
	if (droids == 0 && factories == 0)
	{
		var gameLost = true;

		/* If teams enabled check if all team members have lost  */
		if (alliancesType == ALLIANCES_TEAMS)
		{
			for (var playnum = 0; playnum < maxPlayers; playnum++)
			{
				if (playnum != me && allianceExistsBetween(me, playnum))
				{
					factories = countStruct("A0LightFactory", playnum) + enumStruct("A0CyborgFactory", playnum);
					droids = countDroid(DROID_ANY, playnum);
					if (droids > 0 || factories > 0)
					{
						gameLost = false;	// someone from our team still alive
						break;
					}
				}
			}
		}

		if (gameLost)
		{
			gameOverMessage(false);
			removeTimer("checkEndConditions");
			return;
		}
	}
	
	// Winning Conditions
	var gamewon = true;

	// check if all enemies defeated
	for (var playnum = 0; playnum < maxPlayers; playnum++)
	{
		if (playnum != me && !allianceExistsBetween(me, playnum))	// checking enemy player
		{
			factories = countStruct("A0LightFactory", playnum) + countStruct("A0CyborgFactory", playnum); // nope
			droids = countDroid(DROID_ANY, playnum);
			if (droids > 0 || factories > 0)
			{
				gamewon = false;	//one of the enemies still alive
				break;
			}
		}
	}

	if (gamewon)
	{
		gameOverMessage(true);
		removeTimer("checkEndConditions");
	}
}

// /////////////////////////////////////////////////////////////////
// WARNING MESSAGES
// Base Under Attack
function eventAttacked(victimObj, attackerObj)
{
	if (gameTime > lastHitTime + 5000)
	{
		lastHitTime = gameTime;
		if (victimObj.type == STRUCTURE)
		{
			playSound("pcv337.ogg", victimObj.x, victimObj.y, victimObj.z);	// show position if still alive
		}
		else
		{
			playSound("pcv399.ogg", victimObj.x, victimObj.y, victimObj.z);
		}
	}
}

function eventStructureBuilt(struct)
{
	if (struct.player == selectedPlayer && struct.type == STRUCTURE && struct.stattype == HQ)
	{
		setMiniMap(true); // show minimap
		setDesign(true); // permit designs
	}
}

function eventDestroyed(victim)
{
	if (victim.player == selectedPlayer && victim.type == STRUCTURE && victim.stattype == HQ)
	{
		setMiniMap(false); // hide minimap if HQ is destroyed
		setDesign(false); // and disallow design
	}
}
