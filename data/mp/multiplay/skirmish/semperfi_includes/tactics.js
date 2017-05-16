
//Check if this vtol is free to do something.
function vtolReady(vtol)
{
	if ((vtol.order == DORDER_REARM) && (vtol.weapons[0].armed < 100))
	{
		return false;
	}

	if((vtol.order != DORDER_REARM) && (vtol.weapons[0].armed == 0))
	{
		orderDroid(vtol, DORDER_REARM);
		return false;
	}

	return true;
}

//Return all enemy players that are still alive. An optional argument can be
//passed to determine if that specific player is alive or not.
function getAliveEnemyPlayers(player)
{
	if(defined(player))
	{
		if((countStruct(factory, player) + countStruct(cybFactory, player) + countDroid(DROID_CONSTRUCT, player)) > 0)
		{
			return true;
		}

		return false;
	}

	var numEnemies = [];
	for (var i = 0; i < maxPlayers; i++)
	{
		if (!allianceExistsBetween(me, i) && i != me)
		{
			//Are they alive (have factories and constructs)
			//Even if they still have attack droids, eventAttacked() will find them anyway if they do attack.
			if((countStruct(factory, i) + countStruct(cybFactory, i) + countDroid(DROID_CONSTRUCT, i)) > 0)
			{
				numEnemies.push(i); // count 'em, then kill 'em :)
			}
		}
	}

	return numEnemies;
}

//return the nearest factory (normal factory has precedence). undefined if none.
function findNearestFactory(playerNumber)
{
	var facs = enumStruct(playerNumber, factory).sort(sortByDistToBase);
	var cybFacs = enumStruct(playerNumber, cybFactory).sort(sortByDistToBase);
	var target;

	if(facs.length > 0)
	{
		target = facs[0];
	}
	else if(cybFacs.length > 0)
	{
		target = cybFacs[0];
	}

	return target;
}

//Return closest player construct. Undefined if none.
function findNearestConstruct(playerNumber)
{
	var constructs = enumDroid(playerNumber, DROID_CONSTRUCT).sort(sortByDistToBase);
	var target;

	if(constructs.length > 0)
	{
		target = constructs[0];
	}

	return target;
}

//returns the current enemy that is being targeted (changes every two minutes).
//returns -1 if none.
function getCurrentEnemy()
{
	if(!defined(currentEnemy) || (gameTime > (currentEnemyTick + 120000)))
	{
		currentEnemyTick = gameTime;
		var enemies = getAliveEnemyPlayers();
		if(!enemies.length)
		{
			return -1; //no more enemy players are alive.
		}

		currentEnemy = enemies[random(enemies.length)];
	}

	return currentEnemy
}

//Attack the current selected enemy.
function attackEnemy()
{
	const MIN_GROUP_SIZE = 15;
	const MIN_VTOL_SIZE = 5;

	if (groupSizes[attackGroup] > MIN_GROUP_SIZE)
	{
		var attackers = enumGroup(attackGroup);
		//log("-- Military offensive --");
		// Attack! Find a random enemy, since that is more fun.

		var selectedEnemy = getCurrentEnemy();
		if ((selectedEnemy == -1) || (gameTime < lastBeaconDrop))
		{
			return; //Either no enemy players remain or that we are going to a beacon.
		}

		var target = findNearestFactory(selectedEnemy);
		if (!defined(target))
		{
			target = findNearestConstruct(selectedEnemy);
		}

		var loc;
		if (defined(target))
		{
			loc = {x: target.x, y: target.y};
		}
		else
		{
			return;
		}
		//log("ATTACKING player " + selectedEnemy);

		for (var j = 0; j < attackers.length; ++j)
		{
			if (attackers[j].order != DORDER_RECYCLE && random(101) < 50)
			{
				orderDroidLoc(attackers[j], DORDER_SCOUT, loc.x, loc.y);
			}
		}

		var vtols = enumGroup(vtolGroup);
		if(vtols.length > MIN_VTOL_SIZE)
		{
			for (var j = 0; j < vtols.length; j++)
			{
				if (vtolReady(vtols[j]))
				{
					orderDroidLoc(vtols[j], DORDER_SCOUT, loc.x, loc.y);
					//logObj(vtols[j], "sent to attack");
				}
			}
		}
	}
	else {
		//wait to get a bigger army.
		var droids = enumGroup(attackGroup);
		for (var j = 0; j < droids.length; j++)
		{
			if(droids[j].order != DORDER_RECYCLE)
			{
				orderDroidLoc(droids[j], DORDER_SCOUT, startPositions[me].x, startPositions[me].y);
			}
		}
	}
}

//Use a slim version of the hover map checking code from Cobra AI.
function isHoverMap()
{
	var hoverMap = false;

	for(var i = 0; i < maxPlayers; ++i)
	{
		if(!propulsionCanReach("wheeled01", startPositions[me].x, startPositions[me].y, startPositions[i].x, startPositions[i].y))
		{
			//Check if hover can not reach this area.
			var temp = 0;
			for(var t = 0; t < maxPlayers; ++t)
			{
				if(!propulsionCanReach("hover01", startPositions[i].x, startPositions[i].y, startPositions[t].x, startPositions[t].y))
					temp = temp + 1;
			}

			if(temp !== maxPlayers - 1)
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
	var systems = enumDroid(me, DROID_CONSTRUCT).filter(function(dr) { return dr.propulsion != "hover01"; });
	var unfinishedStructures = enumStruct(me).filter(function(obj) { return obj.status != BUILT && obj.stattype != RESOURCE_EXTRACTOR; });

	if((countStruct(factory) > MIN_FACTORY) && componentAvailable("hover01")) {
		if(!unfinishedStructures.length && systems.length) {
			for(var i = 0; i < systems.length; ++i) {
				orderDroid(systems[i], DORDER_RECYCLE);
			}

			if(!isSeaMap && !systems.length)
			{
				removeTimer("recycleDroidsForHover");
			}
		}

		if(isSeaMap) {
			var tanks = enumGroup(attackGroup).filter(function(dr) { return (dr.droidType == DROID_WEAPON && dr.propulsion != "hover01"); });

			for(var j = 0; j < tanks.length; ++j) {
				orderDroid(tanks[j], DORDER_RECYCLE);
			}

			if(!tanks.length)
			{
				removeTimer("recycleDroidsForHover");
			}
		}
	}

}
