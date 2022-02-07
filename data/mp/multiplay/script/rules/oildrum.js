function placeOilDrum()
{
	var drums = enumFeature(ALL_PLAYERS, "OilDrum").length;
	if (drums >= oilDrumData.maxOilDrums)
	{
		return;
	}

	var x = syncRandom(mapWidth - 20) + 10;
	var y = syncRandom(mapHeight - 20) + 10;

	// Don't allow placement of structures onto a potential drum location if a truck
	// could suddenly built something near it.
	var nearbyTruck = false;
	const SCAN_RANGE_TRUCKS = 6;
	var nearbyObjects = enumRange(x, y, SCAN_RANGE_TRUCKS, ALL_PLAYERS, false);
	for (var i = 0, len = nearbyObjects.length; i < len; ++i)
	{
		var object = nearbyObjects[i];
		if (object.type === DROID && object.droidType === DROID_CONSTRUCT)
		{
			nearbyTruck = true;
			break;
		}
	}

	// scan about the same radius as the biggest game objects in the case a drum
	// wants to be placed near once. This way the scan should touch the center
	// tile of the object.
	const SCAN_RANGE_OCCUPIED = 3;
	// see if the random position is valid
	var occupied = (enumRange(x, y, SCAN_RANGE_OCCUPIED, ALL_PLAYERS, false).length > 0);
	var unreachable = true;
	for (var i = 0; i < maxPlayers; ++i)
	{
		if (propulsionCanReach("hover01", x, y, startPositions[i].x, startPositions[i].y))
		{
			unreachable = false;
			break;
		}
	}

	var terrain = terrainType(x, y);
	if (terrain == TER_WATER || terrain == TER_CLIFFFACE)
	{
		unreachable = true;
	}

	if (occupied || unreachable || nearbyTruck || (gameTime - oilDrumData.lastSpawn <= 200))
	{
		// try again in a different position after 1 second
		queue("placeOilDrum", 1000);
		return;
	}

	oilDrumData.lastSpawn = gameTime;
	addFeature("OilDrum", x, y);
}

function eventPickup(feature, droid)
{
	if (feature.stattype == OIL_DRUM)
	{
		var delay;
		// generate Geom(1/6) distribution for oil drum respawn delay
		for (delay = 0; ; ++delay)
		{
			if (syncRandom(6) == 0)
			{
				break;
			}
		}
		if (oilDrumData.delay > 120000)
		{
			oilDrumData.delay = 0;
		}
		oilDrumData.delay = oilDrumData.delay + 100;
		// amounts to 10 minutes average respawn time
		queue("placeOilDrum", (delay * 120000) + oilDrumData.delay);
	}
}

function oilDrumInit()
{
	// always at least one oil drum, and one more for every 64x64 tiles of map area
	oilDrumData.maxOilDrums = (mapWidth * mapHeight) >> 12; // replace float division with shift for sync-safety
	for (var i = 0; i < oilDrumData.maxOilDrums; ++i)
	{
		queue("placeOilDrum", 10000 * i);
	}
}
