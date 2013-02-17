// General rules for the campaign
//
// * Enable unit design and minimap only when an HQ exists

function eventGameInit()
{
	if (tilesetType != "ARIZONA")
	{
		setSky("texpages/page-25-sky-urban.png", 0.5, 10000.0);
	}
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
