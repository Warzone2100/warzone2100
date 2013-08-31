//
// Skirmish Base Script.
//
// contains the rules for starting and ending a game.
// as well as warning messages.
//
// /////////////////////////////////////////////////////////////////

var lastHitTime = 0;
var cheatmode = false;

function eventGameInit()
{
	if (tilesetType == "URBAN")
	{
		replaceTexture("page-8-player-buildings-bases.png", "page-8-player-buildings-bases-urban.png");
		replaceTexture("page-9-player-buildings-bases.png", "page-9-player-buildings-bases-urban.png");
	}
	else if (tilesetType == "ROCKIES")
	{
		replaceTexture("page-8-player-buildings-bases.png", "page-8-player-buildings-bases-rockies.png");
		replaceTexture("page-9-player-buildings-bases.png", "page-9-player-buildings-bases-rockies.png");
	}

	receiveAllEvents(true);

	setReticuleButton(0, _("Close"), "image_cancel_up.png", "image_cancel_down.png");
	setReticuleButton(1, _("Manufacture (F1)"), "image_manufacture_up.png", "image_manufacture_down.png");
	setReticuleButton(2, _("Research (F2)"), "image_research_up.png", "image_research_down.png");
	setReticuleButton(3, _("Build (F3)"), "image_build_up.png", "image_build_down.png");
	setReticuleButton(4, _("Design (F4)"), "image_design_up.png", "image_design_down.png");
	setReticuleButton(5, _("Intelligence Display (F5)"), "image_intelmap_up.png", "image_intelmap_down.png");
	setReticuleButton(6, _("Commanders (F6)"), "image_commanddroid_up.png", "image_commanddroid_down.png");
	showInterface();

	if (tilesetType != "ARIZONA")
	{
		setSky("texpages/page-25-sky-urban.png", 0.5, 10000.0);
	}

	hackNetOff();
	for (var playnum = 0; playnum < maxPlayers; playnum++)
	{
		if (powerType == 0)
		{
			setPowerModifier(85, playnum);
		}
		else if (powerType == 2)
		{
			setPowerModifier(125, playnum);
		}

		// insane difficulty is meant to be insane...
		if (playerData[playnum].difficulty == INSANE)
		{
			setPowerModifier(200 + 15 * powerType, playnum);
		}
		else if (playerData[playnum].difficulty == EASY)
		{
			setPowerModifier(70 + 5 * powerType, playnum);
		}

		setDroidLimit(playnum, 150, DROID_ANY);
		setDroidLimit(playnum, 10, DROID_COMMAND);
		setDroidLimit(playnum, 15, DROID_CONSTRUCT);

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

		// set starting power for players
		setPower(1000, playnum);

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

	var structlist = enumStruct(selectedPlayer, HQ);
	for (var i = 0; i < structlist.length; i++)
	{
		// Simulate build events to enable minimap/unit design when an HQ exists
		eventStructureBuilt(structlist[i]);
	}

	hackNetOn();
	setTimer("checkEndConditions", 3000);
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
		if (alliancesType == ALLIANCES_TEAMS || alliancesType == ALLIANCES_UNSHARED)
		{
			for (var playnum = 0; playnum < maxPlayers; playnum++)
			{
				if (playnum != selectedPlayer && allianceExistsBetween(selectedPlayer, playnum))
				{
					factories = countStruct("A0LightFactory", playnum) + countStruct("A0CyborgFactory", playnum);
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
		if (playnum != selectedPlayer && !allianceExistsBetween(selectedPlayer, playnum))	// checking enemy player
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
	if (gameTime > lastHitTime + 5000 && victimObj.player == selectedPlayer)
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

function eventResearched(research, structure, player)
{
	//debug("RESEARCH : " + research.fullname + "(" + research.name + ") for " + player + " results=" + research.results);
	for (var v = 0; v < research.results.length; v++)
	{
		var s = research.results[v].split(":");
		if (['Droids', 'Cyborgs'].indexOf(s[0]) >= 0) // research result applies to droids and cyborgs
		{
			for (var i in Upgrades[player].Body) // loop over all bodies
			{
				if (Stats.Body[i].BodyClass === s[0]) // if match against hint in ini file, change it
				{
					// eg Upgrades[0].Body['Tiger']['Armour']
					Upgrades[player].Body[i][s[1]] += Math.ceil(Stats.Body[i][s[1]] * s[2] / 100);
					//debug("  upgraded " + i + " :: " + s[1] + " to " + Upgrades[player].Body[i][s[1]] + " by " + Math.ceil(Stats.Body[i][s[1]] * s[2] / 100));
				}
			}
		}
		else if (['ResearchPoints', 'ProductionPoints', 'PowerPoints', 'RepairPoints', 'RearmPoints'].indexOf(s[0]) >= 0)
		{
			for (var i in Upgrades[player].Building)
			{
				if (Stats.Building[i][s[0]] > 0) // only applies if building has this stat already
				{
					Upgrades[player].Building[i][s[0]] += Math.ceil(Stats.Building[i][s[0]] * s[1] / 100);
					//debug("  upgraded " + i + " to " + Upgrades[player].Building[i][s[0]] + " by " + Math.ceil(Stats.Building[i][s[0]] * s[1] / 100));
				}
			}
		}
		else if (['Wall', 'Structure'].indexOf(s[0]) >= 0)
		{
			for (var i in Upgrades[player].Building)
			{
				if (Stats.Building[i].Type === s[0]) // applies to specific building type
				{
					Upgrades[player].Building[i][s[1]] += Math.ceil(Stats.Building[i][s[1]] * s[2] / 100);
					//debug("  upgraded " + i + " to " + Upgrades[player].Building[i][s[1]] + " by " + Math.ceil(Stats.Building[i][s[1]] * s[2] / 100));
				}
			}
		}
		else if (['ECM', 'Sensor', 'Repair', 'Construct'].indexOf(s[0]) >= 0)
		{
			for (var i in Upgrades[player][s[0]])
			{
				// Upgrades.player.type.buildingName.parameter ... hard to read but short and flexible
				Upgrades[player][s[0]][i][s[1]] += Math.ceil(Stats[s[0]][i][s[1]] * s[2] / 100);
				//debug("  upgraded " + i + " to " + Upgrades[player][s[0]][i][s[1]] + " by " + Math.ceil(Stats[s[0]][i][s[1]] * s[2] / 100));
			}
		}
		else if (Stats.WeaponClass.indexOf(s[0]) >= 0) // if first field is a weapon class
		{
			for (var i in Upgrades[player].Weapon)
			{
				if (Stats.Weapon[i][s[1]] > 0 && Stats.Weapon[i].ImpactClass === s[0])
				{
					Upgrades[player].Weapon[i][s[1]] += Math.ceil(Stats.Weapon[i][s[1]] * s[2] / 100);
					//debug("  upgraded " + i + " to " + Upgrades[player].Weapon[i][s[1]] + " by " + Math.ceil(Stats.Weapon[i][s[1]] * s[2] / 100));
				}
			}
		}
		else
		{
			debug("(error) Unrecognized research hint=" + s[0]);
		}
	}
}

function eventCheatMode(entered)
{
	cheatmode = entered; // remember this setting
}

function eventChat(from, to, message)
{
	if (message == "makesuperior" && cheatmode)
	{
		for (var i in Upgrades[from].Body)
		{
			if (Upgrades[from].Body[i].bodyClass === 'Droids' || Upgrades[from].Body[i].bodyClass === 'Cyborgs')
			{
				Upgrades[from].Body[i].HitPoints += 500;
				Upgrades[from].Body[i].Armour += 500;
				Upgrades[from].Body[i].Thermal += 500;
				Upgrades[from].Body[i].Power += 500;
			}
		}
		console("Made player " + from + "'s units SUPERIOR!");
	}
}
