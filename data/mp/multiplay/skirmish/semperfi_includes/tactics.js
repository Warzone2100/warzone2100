
//Check if this vtol is free to do something.
function vtolReady(vtolID)
{
	var vtol = getObject(DROID, me, vtolID);
	var armedVal = Math.floor(vtol.weapons[0].armed);
	if (vtol.order === DORDER_REARM && armedVal < 100)
	{
		return false;
	}

	if (vtol.order !== DORDER_REARM && !armedVal)
	{
		orderDroid(vtol, DORDER_REARM);
		return false;
	}

	return true;
}

//Does a droid need to repair.
function droidNeedsRepair(droidID, percent)
{
	var dr = getObject(DROID, me, droidID);
	if (dr === null)
	{
		return true; //lets say its busy then
	}
	if (!defined(percent))
	{
		if (dr.propulsion === "hover01")
		{
			percent = 80;
		}
		else
		{
			percent = 65;
		}
	}

	if (dr.order === DORDER_RTR && dr.health < 100)
	{
		return true;
	}

	if (dr.order !== DORDER_RTR && countStruct(REPAIR_FACILITY) > 0 && dr.health < percent)
	{
		orderDroid(dr, DORDER_RTR);
		return true;
	}

	return false;
}

//Return all enemy players that are still alive. An optional argument can be
//passed to determine if that specific player is alive or not.
function getAliveEnemyPlayers(player)
{
	if (defined(player))
	{
		if (countStruct(FACTORY, player) + countStruct(CYBORG_FACTORY, player) + countDroid(DROID_CONSTRUCT, player) > 0)
		{
			return true;
		}

		return false;
	}

	var numEnemies = [];
	for (var i = 0; i < maxPlayers; ++i)
	{
		if (i !== me && !allianceExistsBetween(i, me))
		{
			//Are they alive (have factories and constructs)
			//Even if they still have attack droids, eventAttacked() will find them anyway if they do attack.
			if ((countStruct(FACTORY, i) + countStruct(CYBORG_FACTORY, i) + countDroid(DROID_CONSTRUCT, i)) > 0)
			{
				numEnemies.push(i); // count 'em, then kill 'em :)
			}
		}
	}

	return numEnemies;
}

//return the nearest factory ID (normal factory has precedence). undefined if none.
function findNearestFactoryID(playerNumber)
{
	var facs = enumStruct(playerNumber, FACTORY).sort(sortByDistToBase);
	var cybFacs = enumStruct(playerNumber, CYBORG_FACTORY).sort(sortByDistToBase);
	var target;

	if (facs.length > 0)
	{
		target = facs[0].id;
	}
	else if (cybFacs.length > 0)
	{
		target = cybFacs[0].id;
	}

	return target;
}

//Return closest player construct ID. Undefined if none.
function findNearestConstructID(playerNumber)
{
	var constructs = enumDroid(playerNumber, DROID_CONSTRUCT).sort(sortByDistToBase);
	var target;

	if (constructs.length > 0)
	{
		target = constructs[0].id;
	}

	return target;
}

//Return closest player derrick ID. Undefined if none.
function findNearestDerrickID(playerNumber)
{
	var derr = enumStruct(playerNumber, DERRICK).sort(sortByDistToBase);
	var target;

	if (derr.length > 0)
	{
		target = derr[0].id;
	}

	return target;
}

//returns the current enemy that is being targeted (changes every two minutes).
//returns undefined if none.
function getCurrentEnemy()
{
	if (!defined(currentEnemy) || (gameTime > (currentEnemyTick + 120000)))
	{
		currentEnemyTick = gameTime;
		var enemies = getAliveEnemyPlayers();
		if (!enemies.length)
		{
			return undefined; //no more enemy players are alive.
		}

		currentEnemy = enemies[random(enemies.length)];
	}

	return currentEnemy;
}

//Attack the current selected enemy.
function attackEnemy()
{
	const MIN_GROUP_SIZE = 9;
	const MIN_VTOL_SIZE = 5;

	if (groupSizes[attackGroup] > MIN_GROUP_SIZE)
	{
		var isDroid = false;
		var attackers = enumGroup(attackGroup);
		//log("-- Military offensive --");
		// Attack! Find a random enemy, since that is more fun.

		var selectedEnemy = getCurrentEnemy();
		if (!defined(selectedEnemy) || (gameTime < lastBeaconDrop))
		{
			return; //Either no enemy players remain or that we are going to a beacon.
		}

		var target = findNearestDerrickID(selectedEnemy);
		if (!defined(target))
		{
			target = findNearestFactoryID(selectedEnemy);

			if (!defined(target))
			{
				target = findNearestConstructID(selectedEnemy);
				isDroid = true;
			}
		}

		var loc;
		var realObject;
		if (defined(target))
		{
			if (!isDroid)
			{
				realObject = getObject(STRUCTURE, selectedEnemy, target);
			}
			else
			{
				realObject = getObject(DROID, selectedEnemy, target);
			}

			if (realObject === null)
			{
				return; // just for extra precaution
			}

			loc = {x: realObject.x, y: realObject.y};
		}
		else
		{
			return;
		}
		//log("ATTACKING player " + selectedEnemy);

		for (var j = 0, a = attackers.length; j < a; ++j)
		{
			var tank = attackers[j];
			if (tank.order !== DORDER_RECYCLE && !droidNeedsRepair(tank.id) && random(100) < 50)
			{
				orderDroidLoc(tank, DORDER_SCOUT, loc.x, loc.y);
			}
		}

		var vtols = enumGroup(vtolGroup);
		var vtolLen = vtols.length;
		if (vtolLen > MIN_VTOL_SIZE)
		{
			for (var j = 0; j < vtolLen; ++j)
			{
				var vt = vtols[j];
				if (vtolReady(vt.id))
				{
					orderDroidLoc(vt, DORDER_SCOUT, loc.x, loc.y);
					//logObj(vt, "sent to attack");
				}
			}
		}
	}
}

//Use a slim version of the hover map checking code from Cobra AI.
function isHoverMap()
{
	var hoverMap = false;

	for (var i = 0; i < maxPlayers; ++i)
	{
		if (!propulsionCanReach("wheeled01", BASE.x, BASE.y, startPositions[i].x, startPositions[i].y))
		{
			//Check if hover can not reach this area.
			var temp = 0;
			for (var t = 0; t < maxPlayers; ++t)
			{
				var b1 = startPositions[i];
				var b2 = startPositions[t];
				if(!propulsionCanReach("hover01", b1.x, b1.y, b2.x, b2.y))
				{
					temp = temp + 1;
				}
			}

			if (temp !== maxPlayers - 1)
			{
				hoverMap = true;
				break;
			}
		}
	}

	return hoverMap;
}

function recycleDroidsForHover()
{
	const MIN_FACTORY = 1;
	var systems = enumDroid(me, DROID_CONSTRUCT).filter(function(dr) { return dr.propulsion !== "hover01"; });
	var unfinishedStructures = enumStruct(me).filter(function(obj) { return obj.status !== BUILT && obj.stattype !== RESOURCE_EXTRACTOR; });
	const NON_HOVER_SYSTEMS = systems.length;

	if (countStruct(FACTORY) > MIN_FACTORY && componentAvailable("hover01"))
	{
		if (!unfinishedStructures.length && NON_HOVER_SYSTEMS)
		{
			for(var i = 0; i < NON_HOVER_SYSTEMS; ++i)
			{
				orderDroid(systems[i], DORDER_RECYCLE);
			}
		}

		if (!isSeaMap && !NON_HOVER_SYSTEMS)
		{
			removeTimer("recycleDroidsForHover");
		}

		if (isSeaMap)
		{
			var tanks = enumGroup(attackGroup).filter(function(dr) { return (dr.droidType == DROID_WEAPON && dr.propulsion !== "hover01"); });
			const NON_HOVER_TANKS = tanks.length;

			for(var j = 0; j < NON_HOVER_TANKS; ++j)
			{
				orderDroid(tanks[j], DORDER_RECYCLE);
			}

			if (!(NON_HOVER_TANKS + NON_HOVER_SYSTEMS))
			{
				removeTimer("recycleDroidsForHover");
			}
		}
	}
}
