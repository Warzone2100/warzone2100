
////////////////////////////////////////////////////////////////////////////////
// Truck management.
////////////////////////////////////////////////////////////////////////////////

/**
 * ## camManageTrucks(playerId)
 *
 * Manage trucks for an AI player. This assumes recapturing oils and rebuilding destroyed trucks
 * in factories, the latter is implemented via `camQueueDroidProduction()` mechanism.
 * @param {number} playerId
 * @returns {void}
 */
function camManageTrucks(playerId)
{
	__camTruckInfo[playerId] = { enabled: 1, queue: [], player: playerId };
}

/**
 * ## camQueueBuilding(playerId, stat[, position])
 *
 * Assuming truck management is enabled for the player, issue an order to build a specific building
 * near a certain position. The order would be issued once as soon as a free truck becomes available.
 * It will not be re-issued in case the truck is destroyed before the building is finished.
 * If position is unspecified, the building would be built near the first available truck.
 * Otherwise, position may be a label or a `POSITION`-like object.
 * @param {number} playerId
 * @param {string} stat
 * @param {string|Object} [position]
 * @returns {void}
 */
function camQueueBuilding(playerId, stat, position)
{
	var ti = __camTruckInfo[playerId];
	ti.queue.push({ stat: stat, pos: camMakePos(position) });
}

//////////// privates

function __camEnumFreeTrucks(player)
{
	var rawDroids = enumDroid(player, DROID_CONSTRUCT);
	var droids = [];
	for (let i = 0, l = rawDroids.length; i < l; ++i)
	{
		var droid = rawDroids[i];
		if (droid.order !== DORDER_BUILD && droid.order !== DORDER_HELPBUILD && droid.order !== DORDER_LINEBUILD)
		{
			droids.push(droid);
		}
	}
	return droids;
}

function __camGetClosestTruck(player, pos)
{
	var droids = __camEnumFreeTrucks(player);
	if (droids.length <= 0)
	{
		return undefined;
	}

	// Find out which one is the closest.
	var minDroid = droids[0];
	var minDist = camDist(minDroid, pos);
	for (let i = 1, l = droids.length; i < l; ++i)
	{
		var droid = droids[i];
		if (!droidCanReach(droid, pos.x, pos.y))
		{
			continue;
		}
		var dist = camDist(droid, pos);
		if (dist < minDist)
		{
			minDist = dist;
			minDroid = droid;
		}
	}
	return minDroid;
}

function __camTruckTick()
{
	// Issue truck orders for each player.
	// See comments inside the loop to understand priority.
	for (const playerObj in __camTruckInfo)
	{
		var ti = __camTruckInfo[playerObj];
		var player = ti.player;
		var truck;

		// First, build things that were explicitly ordered.
		while (ti.queue.length > 0)
		{
			var qi = ti.queue[0];
			var pos = qi.pos;
			var randx = 0;
			var randy = 0;

			if (camDef(pos))
			{
				// Find the truck most suitable for the job.
				truck = __camGetClosestTruck(player, pos);
				if (!camDef(truck))
				{
					break;
				}
			}
			else
			{
				// Build near any truck if pos was not specified.
				var droids = __camEnumFreeTrucks(player);
				if (droids.length <= 0)
				{
					break;
				}
				truck = droids[0];
				pos = truck;
				randx = (camRand(100) < 50) ? -camRand(2) : camRand(2);
				randy = (camRand(100) < 50) ? -camRand(2) : camRand(2);
			}

			enableStructure(qi.stat, player);
			var loc = pickStructLocation(truck, qi.stat, pos.x, pos.y);
			if (camDef(loc) && camDef(truck))
			{
				if (orderDroidBuild(truck, DORDER_BUILD, qi.stat, loc.x + randx, loc.y + randy))
				{
					ti.queue.shift(); // consider it handled
				}
			}
		}

		// Then, capture free oils.
		var oils = enumFeature(ALL_PLAYERS, "OilResource");
		if (oils.length === 0)
		{
			continue;
		}
		var oil = oils[0];
		truck = __camGetClosestTruck(player, oil);
		if (camDef(truck) && player !== NEXUS)
		{
			enableStructure("A0ResourceExtractor", player);
			orderDroidBuild(truck, DORDER_BUILD, "A0ResourceExtractor", oil.x, oil.y);
			continue;
		}
	}
}

// called from eventDestroyed
function __camCheckDeadTruck(obj)
{
	if (camDef(__camTruckInfo[obj.player]))
	{
		// multi-turret templates are not supported yet
		// cyborg engineers are not supported yet
		// cannot use obj.weapons[] because spade is not a weapon
		camQueueDroidProduction(obj.player, {
			body: obj.body,
			prop: obj.propulsion,
			weap: "Spade1Mk1"
		});
	}
}
