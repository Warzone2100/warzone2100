// General rules for the campaign
//
// * Enable unit design and minimap only when an HQ exists

function setupGame()
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

function eventGameLoaded()
{
	setupGame();
}

function eventGameInit()
{
	setupGame();
}

function eventStartLevel()
{
	// Disable by default
	setMiniMap(false);
	setDesign(false);
	setDroidLimit(0, 100, DROID_ANY);
	setDroidLimit(0, 10, DROID_COMMAND);
	setDroidLimit(0, 15, DROID_CONSTRUCT);

	setStructureLimits("A0PowerGenerator", 5, 0);
	setStructureLimits("A0ResourceExtractor", 200, 0);
	setStructureLimits("A0ResearchFacility", 5, 0);
	setStructureLimits("A0LightFactory", 5, 0);
	setStructureLimits("A0CommandCentre", 1, 0);
	setStructureLimits("A0ComDroidControl", 1, 0);
	setStructureLimits("A0CyborgFactory", 5, 0);
	setStructureLimits("A0VTolFactory1", 5, 0);

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
	//debug("RESEARCH : " + research.fullname + "(" + research.name + ") for " + player);
	// iterate over all results
	for (var i = 0; i < research.results.length; i++)
	{
		var v = research.results[i];
		//debug("    RESULT : class=" + v['class'] + " parameter=" + v['parameter'] + " value=" + v['value'] + " filter=" + v['filterParameter'] + " filterparam=" + v['filterParameter']);
		for (var cname in Upgrades[player][v['class']]) // iterate over all components of this type
		{
			var parameter = v['parameter'];
			var ctype = v['class'];
			var filterparam = v['filterParameter'];
			if ('filterParameter' in v && Stats[ctype][cname][filterparam] != v['filterValue']) // more specific filter
			{
				continue;
			}
			if (Stats[ctype][cname][parameter] > 0) // only applies if stat has above zero value already
			{
				Upgrades[player][ctype][cname][parameter] += Math.ceil(Stats[ctype][cname][parameter] * v['value'] / 100);
				//debug("      upgraded " + cname + " to " + Upgrades[player][ctype][cname][parameter] + " by " + Math.ceil(Stats[ctype][cname][parameter] * v['value'] / 100));
			}
		}
	}
}

var lastHitTime = 0;
function eventAttacked(victim, attacker) {
	if (gameTime > lastHitTime + 5000)
	{
		lastHitTime = gameTime;
		if (victim.type === STRUCTURE)
			playSound("pcv337.ogg", victim.x, victim.y, victim.z);
		else
			playSound("pcv399.ogg", victim.x, victim.y, victim.z);
	}
}
