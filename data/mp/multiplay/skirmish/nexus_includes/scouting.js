
function chooseScoutArea()
{
	const SCAN_RANGE = 6;
	var x = 0;
	var y = 0;
	var r = 0;
	var theta = (2 * Math.PI);
	var xAb = 0;
	var yAb = 0;
	var badStuff;

	if (defined(scoutInfo.location.x) && defined(scoutInfo.location.y) && groupCanReach(groups.scouts, scoutInfo.location.x, scoutInfo.location.y))
	{
		badStuff = enumRange(scoutInfo.location.x, scoutInfo.location.y, SCAN_RANGE, ENEMIES, false).length;

		if (!groupNearCoordinate(groups.scouts, scoutInfo.location, 5))
		{
			orderGroupLoc(groups.scouts, scoutInfo.location.x, scoutInfo.location.y, DORDER_SCOUT);
			return;
		}
	}

	if (defined(badStuff) && badStuff > 0)
	{
		//Some enemy object is around so stay near it and maybe find it.
		const TILE_LENGTH = 3; //about how many tiles around we should can go.
		r = Math.floor(TILE_LENGTH * Math.sqrt(Math.random()));
		xAb = Math.abs(Math.floor(r * Math.cos(theta)));
		yAb = Math.abs(Math.floor(r * Math.sin(theta)));
		x = (random(2) > 0) ? scoutInfo.location.x + xAb : scoutInfo.location.x - xAb;
		y = (random(2) > 0) ? scoutInfo.location.y + yAb : scoutInfo.location.y - yAb;
	}
	else
	{
		//Choose a random area around the circumference of the circle.
		r = Math.floor(scoutInfo.currentRadius * Math.sqrt(Math.random()));
		xAb = Math.abs(Math.floor(r * Math.cos(theta)));
		yAb = Math.abs(Math.floor(r * Math.sin(theta)));
		x = (random(2) > 0) ? scoutInfo.base.x + xAb : scoutInfo.base.x - xAb;
		y = (random(2) > 0) ? scoutInfo.base.y + yAb : scoutInfo.base.y - yAb;
	}

	//Don't go off map
	if (x < 2)
	{
		x = 2;
	}
	else if (x > mapWidth - 2)
	{
		x = mapWidth - 2;
	}

	if (y < 2)
	{
		y = 2;
	}
	else if (y > mapHeight - 2)
	{
		y = mapHeight - 2;
	}

	if (groupCanReach(groups.scouts, x, y))
	{
		scoutInfo.location.x = x;
		scoutInfo.location.y = y;
	}
}

function expandScoutRadius()
{
	var badStuff = enumRange(scoutInfo.base.x, scoutInfo.base.y, scoutInfo.currentRadius, ENEMIES, true).length;

	if (badStuff === 0)
	{
		const MAP_LIMIT = 250; //About the longest of any map side length

		if (scoutInfo.currentRadius >= MAP_LIMIT)
		{
			return;
		}

		scoutInfo.currentRadius = scoutInfo.currentRadius + scoutInfo.expandTileRate;
	}
	else
	{
		const MAX_FEARED_OBJECTS = 20;

		//See if we should lower the radius in case of intense enemy assault
		if (badStuff >= MAX_FEARED_OBJECTS)
		{
			var diff = (scoutInfo.currentRadius - scoutInfo.expandTileRate);

			if (diff <= 2)
			{
				scoutInfo.currentRadius = 2;
			}
			else
			{
				scoutInfo.currentRadius = (scoutInfo.currentRadius - scoutInfo.expandTileRate);
			}
		}
	}
}

//Spread out over 3 ticks
function scoutMain()
{
	if (throttleThis("scout_throttle", secondsToMilliseconds(8)))
	{
		return;
	}

	if (scoutInfo.expandTileRate < 1)
	{
		debugMessage("scoutMain. Expand rate less than 1 (setting to 1).");
		scoutInfo.expandTileRate = 1;
	}

	if (scoutInfo.currentRadius < 0)
	{
		debugMessage("scoutMain. Current radius less than 0 (setting to 0)");
		scoutInfo.currentRadius = 0;
	}

	expandScoutRadius();
	queue("chooseScoutArea", TICK * 2);
}
