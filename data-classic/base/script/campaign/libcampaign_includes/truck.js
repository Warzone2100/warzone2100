
////////////////////////////////////////////////////////////////////////////////
// Truck management.
////////////////////////////////////////////////////////////////////////////////

//;; ## camManageTrucks(player)
//;;
//;; Manage trucks for an AI player. This assumes recapturing oils and
//;; rebuilding destroyed trucks in factories, the latter is implemented
//;; via ```camQueueDroidProduction()``` mechanism.
//;;
function camManageTrucks(player)
{
	__camTruckInfo[player] = { enabled: 1, queue: [] };
}

//;; ## camQueueBuilding(player, stat[, pos])
//;;
//;; Assuming truck management is enabled for the player, issue an order
//;; to build a specific building near a certain position. The order
//;; would be issued once as soon as a free truck becomes available. It will
//;; not be re-issued in case the truck is destroyed before the building
//;; is finished. If position is unspecified, the building would be built
//;; near the first available truck. Otherwise, position may be a label
//;; or a POSITION-like object.
//;;
function camQueueBuilding(player, stat, pos)
{
	var ti = __camTruckInfo[player];
	ti.queue.push({ stat: stat, pos: camMakePos(pos) });
}


//////////// privates

function __camEnumFreeTrucks(player)
{
	var rawDroids = enumDroid(player, DROID_CONSTRUCT);
	var droids = [];
	for (var i = 0, l = rawDroids.length; i < l; ++i)
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
	for (var i = 1, l = droids.length; i < l; ++i)
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
	for (var player in __camTruckInfo)
	{
		var ti = __camTruckInfo[player];
		var truck;

		// First, build things that were explicitly ordered.
		while (ti.queue.length > 0)
		{
			var qi = ti.queue[0];
			var pos = qi.pos;
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
			}

			enableStructure(qi.stat, player);
			var loc = pickStructLocation(truck, qi.stat, pos.x, pos.y);
			if (camDef(loc) && camDef(truck))
			{
				if (orderDroidBuild(truck, DORDER_BUILD, qi.stat, loc.x, loc.y))
				{
					ti.queue.shift(); // consider it handled
				}
			}
		}

		// Then, capture free oils.
		var oils = enumFeature(-1, "OilResource");
		if (oils.length === 0)
		{
			continue;
		}
		var oil = oils[0];
		truck = __camGetClosestTruck(player, oil);
		if (camDef(truck))
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
