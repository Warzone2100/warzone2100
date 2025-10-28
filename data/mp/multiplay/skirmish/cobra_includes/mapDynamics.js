
//Figure out if we are on a hover map. This is determined by checking if a
//ground only propulsion fails to reach a target (checking if it is a vtol only player
//or map spotter pits) and doing similar checks for hover propulsion.
//Furthermore it can discover if it is sharing land with an enemy and disable/enable
//unit production depending on the case until it reaches hover propulsion.
function checkIfSeaMap()
{
	function uncached()
	{
		let hoverMap = false;
		seaMapWithLandEnemy = false;

		for (let i = 0; i < maxPlayers; ++i)
		{
			if (!propulsionCanReach("wheeled01", _MY_BASE.x, _MY_BASE.y, startPositions[i].x, startPositions[i].y))
			{

				//Check if it is a map 'spotter' pit
				//Cyborgs will turn off in divided maps with a physical barrier still
				let temp = 0;

				for (let t = 0; t < maxPlayers; ++t)
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
			for (let i = 0; i < maxPlayers; ++i)
			{
				if ((i !== me) && !allianceExistsBetween(i, me) && propulsionCanReach("wheeled01", _MY_BASE.x, _MY_BASE.y, startPositions[i].x, startPositions[i].y))
				{
					//Check to see if it is a closed player slot
					if (countDroid(DROID_ANY, i) > 0)
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

	return cacheThis(uncached, [], "checkIfSeaMap" + me, Infinity);
}

//All derricks and all oil resources to find the map total.
function countAllResources()
{
	function uncached()
	{
		let amount = enumFeature(ALL_PLAYERS, __OIL_RES).length;

		for (let i = 0; i < maxPlayers; ++i)
		{
			amount += enumStruct(i, _STRUCTURES.derrick).length;
		}

		if (isDefined(scavengerPlayer))
		{
			amount += enumStruct(scavengerPlayer, _STRUCTURES.derrick).length;
		}

		return amount;
	}

	return cacheThis(uncached, [], "countAllResources" + me, Infinity);
}

// The amount of oil each player should hold.
function averageOilPerPlayer()
{
	function uncached()
	{
		let players = 0;
		//maxPlayers is useless here in case there are some empty slots.
		for (let i = 0; i < maxPlayers; ++i)
		{
			const _data = playerData[i];

			players += ((_data.isHuman || _data.isAI) ? 1 : 0);
		}

		return Math.floor(countAllResources() / players);
	}

	return cacheThis(uncached, [],  "averageOilPerPlayer" + me, Infinity);
}

//Is the map a low/medium/high power level. Returns a string of LOW/MEDIUM/HIGH.
function mapOilLevel()
{
	function uncached()
	{
		let str;
		const __perPlayer = averageOilPerPlayer();

		if (__perPlayer <= 10)
		{
			str = "LOW";
		}
		else if ((__perPlayer > 10) && (__perPlayer < 20))
		{
			str = "MEDIUM";
		}
		else if ((__perPlayer >= 20) && (__perPlayer < 30))
		{
			str = "HIGH";
		}
		else
		{
			str = "NTW";
		}

		return str;
	}

	return cacheThis(uncached, [], "mapOilLevel" + me, Infinity);
}

function highOilMap()
{
	function uncached()
	{
		const __oil = mapOilLevel();

		if (__oil === "HIGH" || __oil === "NTW")
		{
			return true;
		}

		return false;
	}

	return cacheThis(uncached, [], "highOilMap" + me, Infinity);
}

//Determine the base area that Cobra claims.
function cobraBaseArea()
{
	function uncached()
	{
		const __extraTiles = 20;
		const __clipOffset = 0;
		const _area = {x1: mapWidth, y1: mapHeight, x2: 0, y2: 0};
		const _baseStructures = [
			_STRUCTURES.factory,
			_STRUCTURES.cyborgFactory,
			_STRUCTURES.vtolFactory,
			_STRUCTURES.lab,
			_STRUCTURES.gen,
			_STRUCTURES.hq,
			_STRUCTURES.repair,
			_STRUCTURES.uplink,
			_STRUCTURES.lassat,
		];

		for (let i = 0, len = _baseStructures.length; i < len; ++i)
		{
			const __structureType = _baseStructures[i];
			const _objects = enumStruct(me, __structureType);

			for (let j = 0, len2 = _objects.length; j < len2; ++j)
			{
				const _structure = _objects[j];

				if (_structure.x < _area.x1)
				{
					_area.x1 = _structure.x;
				}
				if (_structure.x > _area.x2)
				{
					_area.x2 = _structure.x;
				}
				if (_structure.y < _area.y1)
				{
					_area.y1 = _structure.y;
				}
				if (_structure.y > _area.y2)
				{
					_area.y2 = _structure.y;
				}
			}
		}

		_area.x1 = _area.x1 - __extraTiles;
		_area.y1 = _area.y1 - __extraTiles;
		_area.x2 = _area.x2 + __extraTiles;
		_area.y2 = _area.y2 + __extraTiles;

		const _clipFirstCoords = clipToMapBounds({x: _area.x1, y: _area.y1}, __clipOffset);
		const _clipSecondCoords = clipToMapBounds({x: _area.x2, y: _area.y2}, __clipOffset);

		_area.x1 = _clipFirstCoords.x;
		_area.y1 = _clipFirstCoords.y;
		_area.x2 = _clipSecondCoords.x;
		_area.y2 = _clipSecondCoords.y;

		return _area;
	}

	return cacheThis(uncached, [], "cobraBaseArea" + me, 70000);
}
