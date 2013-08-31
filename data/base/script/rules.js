// General rules for the campaign
//
// * Enable unit design and minimap only when an HQ exists

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
	else if (tilesetType != "ARIZONA")
	{
		setSky("texpages/page-25-sky-urban.png", 0.5, 10000.0);
	}

	setReticuleButton(0, _("Close"), "image_cancel_up.png", "image_cancel_down.png");
	setReticuleButton(1, _("Manufacture (F1)"), "image_manufacture_up.png", "image_manufacture_down.png");
	setReticuleButton(2, _("Research (F2)"), "image_research_up.png", "image_research_down.png");
	setReticuleButton(3, _("Build (F3)"), "image_build_up.png", "image_build_down.png");
	setReticuleButton(4, _("Design (F4)"), "image_design_up.png", "image_design_down.png");
	setReticuleButton(5, _("Intelligence Display (F5)"), "image_intelmap_up.png", "image_intelmap_down.png");
	setReticuleButton(6, _("Commanders (F6)"), "image_commanddroid_up.png", "image_commanddroid_down.png");
	showInterface();
}

function eventStartLevel()
{
	// Disable by default
	setMiniMap(false);
	setDesign(false);
	setDroidLimit(0, 100, DROID_ANY);
	setDroidLimit(0, 10, DROID_COMMAND);
	setDroidLimit(0, 15, DROID_CONSTRUCT);

	var structlist = enumStruct(me, HQ);
	for (var i = 0; i < structlist.length; i++)
	{
		// Simulate build events to enable minimap/unit design when an HQ exists
		eventStructureBuilt(structlist[i]);
	}
	structlist = enumStructOffWorld(me, HQ);
	for (var i = 0; i < structlist.length; i++)
	{
		eventStructureBuilt(structlist[i]);
	}

	// set income modifier for player 0 (human)
	if (difficulty == EASY)
	{
		setPowerModifier(110);
	}
	else if (difficulty == HARD)
	{
		setPowerModifier(90);
	}
}

function eventStructureBuilt(struct)
{
	if (struct.player == selectedPlayer && struct.type == STRUCTURE && struct.stattype == HQ)
	{
		// Enable unit design and minimap when an HQ gets built
		setMiniMap(true);
		setDesign(true);
	}
}

function eventDestroyed(victim)
{
	if (victim.player == selectedPlayer && victim.type == STRUCTURE && victim.stattype == HQ)
	{
		// Disable unit design and minimap when the HQ gets destroyed
		setMiniMap(false);
		setDesign(false);
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
