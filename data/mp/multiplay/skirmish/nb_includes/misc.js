
/*
 * A few globals that didn't have a place for them in any other file
 *
 */

const gateways = enumGateways();
const baseLocation = startPositions[me];

function distanceToBase(loc)
{
	return distance(loc, baseLocation);
}

function canReachFromBase(propulsion, location)
{
	if (defined(propulsion))
	{
		return propulsionCanReach(propulsion, baseLocation.x, baseLocation.y, location.x, location.y);
	}
}

function myPower()
{
	return playerPower(me) - queuedPower(me);
}

function isAvailable(stat)
{
	return isStructureAvailable(stat, me);
}

function isAlly(player)
{
	return allianceExistsBetween(me, player);
}

function isEnemy(player)
{
	return !isAlly(player);
}

function enumLivingPlayers()
{
	function uncached()
	{
		const ret = [];

		for (let i = 0; i < maxPlayers; ++i)
		{
			if (countStructList(targets, i) > 0)
			{
				ret.push(i);
				continue;
			}

			if (enumDroid(i).length > 0)
			{
				ret.push(i);
			}
		}

		return ret;
	}

	return cached(uncached, 30000);
}

function enumTrucks()
{
	return enumDroid(me, DROID_CONSTRUCT);
}

function goEasy()
{
	personality = randomItem(subpersonalities);
}

function areThereGW()
{
	function uncached() //should only need to be evaluated once at the start
	{
		return (gateways.length > 0);
	}

	return cached(uncached, Infinity);
}
