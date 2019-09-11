
//Figure out if we are on a hover map. This is determined by checking if a
//ground only propulsion fails to reach a target (checking if it is a vtol only player
//or map spotter pits) and doing similar checks for hover propulsion.
//Furthermore it can discover if it is sharing land with an enemy and disable/enable
//unit production depending on the case until it reaches hover propulsion.
function checkIfSeaMap()
{
	var hoverMap = false;
	seaMapWithLandEnemy = false;

	for (var i = 0; i < maxPlayers; ++i)
	{
		if (!propulsionCanReach("wheeled01", MY_BASE.x, MY_BASE.y, startPositions[i].x, startPositions[i].y))
		{

			//Check if it is a map 'spotter' pit
			//Cyborgs will turn off in divided maps with a physical barrier still
			var temp = 0;
			for (var t = 0; t < maxPlayers; ++t)
			{
				if (!propulsionCanReach("hover01", startPositions[i].x, startPositions[i].y, startPositions[t].x, startPositions[t].y))
				{
					temp = temp + 1;
				}
			}

			if (temp !== maxPlayers - 1)
			{
				hoverMap = true; //And thus forceHover = true
				break;
			}
		}
	}

	//Determine if we are sharing land on a hover map with an enemy that can reach us via non-hover propulsion.
	if (hoverMap === true)
	{
		for (var i = 0; i < maxPlayers; ++i)
		{
			if ((i !== me) && !allianceExistsBetween(i, me) && propulsionCanReach("wheeled01", MY_BASE.x, MY_BASE.y, startPositions[i].x, startPositions[i].y))
			{
				//Check to see if it is a closed player slot
				if (countDroid(DROID_ANY, i).length)
				{
					seaMapWithLandEnemy = true;
					break;
				}
			}
			if (seaMapWithLandEnemy === true)
			{
				break;
			}
		}
	}

	return hoverMap;
}

//Turn off Machine-guns on T2 and T3
//Very cheap analysis done here.
function CheckStartingBases()
{
	if (personality === "AL")
	{
		return true;
	}

	if (getMultiTechLevel() > 1)
	{
		const CACHE_WEAPONS = subPersonalities[personality].primaryWeapon.weapons.length;
		for (var i = 0; i < CACHE_WEAPONS; ++i)
		{
			if (isDesignable(subPersonalities[personality].primaryWeapon.weapons[i].stat))
			{
				return true;
			}
		}
	}

	return false;
}

//All derricks and all oil resources to find the map total.
function countAllResources()
{
	function uncached()
	{
		var amount = enumFeature(-1, OIL_RES).length;
		for (var i = 0; i < maxPlayers; ++i)
		{
			amount += enumStruct(i, structures.derricks).length;
		}

		if (isDefined(scavengerPlayer))
		{
			amount += enumStruct(scavengerPlayer, structures.derricks).length;
		}

		return amount;
	}

	return cacheThis(uncached, [], undefined, Infinity);
}

// The amount of oil each player should hold.
function averageOilPerPlayer()
{
	function uncached()
	{
		return Math.floor(countAllResources() / maxPlayers);
	}

	return cacheThis(uncached, [],  undefined, Infinity);
}

//Is the map a low/medium/high power level. Returns a string of LOW/MEDIUM/HIGH.
function mapOilLevel()
{
	function uncached()
	{
		var str;
		var perPlayer = averageOilPerPlayer();
		if (perPlayer <= 8)
		{
			str = "LOW";
		}
		else if ((perPlayer > 8) && (perPlayer <= 16))
		{
			str = "MEDIUM";
		}
		else if ((perPlayer > 16) && (perPlayer < 34))
		{
			str = "HIGH";
		}
		else
		{
			str = "NTW";
		}

		return str;
	}

	return cacheThis(uncached, [], undefined, Infinity);
}

//Determine the base area that Cobra claims. Pass an object to see if it is in it.
function initialTerritory(object)
{
	const X_AVERAGE = 2 * Math.ceil(mapWidth / maxPlayers);
	const Y_AVERAGE = 2 * Math.ceil(mapHeight / maxPlayers);

	var area =
	{
		"x1": MY_BASE.x - X_AVERAGE, "y1": MY_BASE.y - Y_AVERAGE,
		"x2": MY_BASE.x + X_AVERAGE, "y2": MY_BASE.y + Y_AVERAGE,
	};

	if (area.x1 < 0)
	{
		area.x1 = 0;
	}
	if (area.y1 < 0)
	{
		area.y1 = 0;
	}
	if (area.x2 > mapWidth)
	{
		area.x2 = mapWidth;
	}
	if (area.y2 > mapHeight)
	{
		area.y2 = mapHeight;
	}

	if (isDefined(object))
	{
		var stuff = enumArea(area.x1, area.y1, area.x2, area.y2);
		for(var i = 0, l = stuff.length; i < l; ++i)
		{
			if(object.id === stuff[i].id)
			{
				return true;
			}
		}

		return false;
	}

	return area;
}
