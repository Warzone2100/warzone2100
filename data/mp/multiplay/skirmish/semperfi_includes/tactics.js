
//Check if this vtol is free to do something.
function vtolReady(vtolID)
{
	var vtol = getObject(DROID, me, vtolID);
	if (vtol === null)
	{
		return false;
	}

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
		return true; //lets say it is busy then
	}
	if (!defined(percent))
	{
		if (dr.propulsion === "hover01")
		{
			percent = 80;
		}
		else
		{
			percent = 70;
		}
	}

	if (dr.order === DORDER_RTR && dr.health < 100)
	{
		return true;
	}

	if (dr.order !== DORDER_RTR && countStruct(REPAIR_FACILITY_STAT) > 0 && dr.health < percent)
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
		if (countStruct(FACTORY_STAT, player) + countStruct(CYBORG_FACTORY_STAT, player) + countDroid(DROID_CONSTRUCT, player) > 0)
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
			if ((countStruct(FACTORY_STAT, i) + countStruct(CYBORG_FACTORY_STAT, i) + countDroid(DROID_CONSTRUCT, i)) > 0)
			{
				numEnemies.push(i); // count 'em, then kill 'em :)
			}
		}
	}

	if (defined(scavengerPlayer) && (countStruct("A0BaBaFactory", scavengerPlayer) + countStruct(DERRICK_STAT, scavengerPlayer) + countDroid(DROID_CONSTRUCT, i)) > 0)
	{
		numEnemies.push(scavengerPlayer);
	}

	return numEnemies;
}

//return the nearest factory ID (normal factory has precedence). undefined if none.
function findNearestFactoryID(player)
{
	var facs = enumStruct(player, FACTORY_STAT).sort(sortByDistToBase);
	var cybFacs = enumStruct(player, CYBORG_FACTORY_STAT).sort(sortByDistToBase);
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
function findNearestConstructID(player)
{
	var constructs = enumDroid(player, DROID_CONSTRUCT).sort(sortByDistToBase);
	var target;

	if (constructs.length > 0)
	{
		target = constructs[0].id;
	}

	return target;
}

//Return closest player derrick ID. Undefined if none.
function findNearestDerrickID(player)
{
	var target;
	var derr = enumStruct(player, DERRICK_STAT).sort(sortByDistToBase);

	if (derr.length > 0)
	{
		target = derr[0].id;
	}

	return target;
}

function setPlayerAsTarget(player)
{
	currentEnemy = player;
	currentEnemyTick = gameTime;
}

//Returns the current enemy that is being targeted. undefined if none.
//Note that eventBeacon can reset the currentEnemyTick and target enemy player
//so an enemy could be be harassed longer.
function getCurrentEnemy()
{
	if (!defined(currentEnemy) || (gameTime > (currentEnemyTick + 120000)))
	{
		var enemies = getAliveEnemyPlayers();
		if (enemies.length === 0)
		{
			return undefined; //no more enemy players are alive.
		}

		setPlayerAsTarget(enemies[random(enemies.length)]);
	}

	return currentEnemy;
}

//Attack the current selected enemy.
function attackEnemy()
{
	const MIN_GROUP_SIZE = 6;
	const MIN_VTOL_SIZE = 5;

	if (groupSizes[attackGroup] > MIN_GROUP_SIZE)
	{
		var isDroid = false;
		var attackers = enumGroup(attackGroup);
		//log("-- Military offensive --");
		// Attack! Find a random enemy, since that is more fun.

		var selectedEnemy = getCurrentEnemy();
		if (!defined(selectedEnemy))
		{
			return; //No enemy players remain
		}

		var target = findNearestDerrickID(selectedEnemy);
		if (!target)
		{
			target = findNearestFactoryID(selectedEnemy);
			if (!target)
			{
				target = findNearestConstructID(selectedEnemy);
				isDroid = true;
			}
		}

		var loc;
		var realObject;
		if (target)
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
		var j = 0;
		var len = attackers.length;

		for (j = 0; j < len; ++j)
		{
			var tank = attackers[j];
			if (tank.order !== DORDER_RECYCLE && !droidNeedsRepair(tank.id) && random(100) < 50)
			{
				orderDroidLoc(tank, DORDER_MOVE, loc.x, loc.y);
			}
		}

		//Only send VTOLs if we got a few of them
		var vtols = enumGroup(vtolGroup);
		len = vtols.length;
		if (len > MIN_VTOL_SIZE)
		{
			for (j = 0; j < len; ++j)
			{
				var vt = vtols[j];
				if (vtolReady(vt.id))
				{
					orderDroidLoc(vt, DORDER_SCOUT, loc.x, loc.y);
					//logObj(vt, "sent to attack");
				}
			}
		}

		//Now send in bunker buster which only focus on structures.
		var busters = enumGroup(busterGroup);
		len = busters.length;
		var enemyStructs = enumRange(loc.x, loc.y, 10, ENEMIES, false).filter(function(obj) {
			return obj.type === STRUCTURE;
		});

		if (enemyStructs.length > 0)
		{
			for (j = 0; j < len; ++j)
			{
				var bust = busters[j];
				if (bust.order !== DORDER_RECYCLE && !droidNeedsRepair(bust.id))
				{
					orderDroidObj(bust, DORDER_ATTACK, enemyStructs[0]);
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
	if (componentAvailable("hover01") === false || countStruct(POW_GEN_STAT) === 0)
	{
		return;
	}

	const MIN_FACTORY = 1;
	var systems = enumDroid(me, DROID_CONSTRUCT).filter(function(dr) {
		return dr.propulsion !== "hover01";
	});
	var unfinishedStructures = enumStruct(me).filter(function(obj) {
		return obj.status !== BUILT && obj.stattype !== RESOURCE_EXTRACTOR && obj.stattype !== DEFENSE;
	});
	const NON_HOVER_SYSTEMS = systems.length;

	if (countStruct(FACTORY_STAT) > MIN_FACTORY)
	{
		if (unfinishedStructures.length === 0)
		{
			for (var i = 0; i < NON_HOVER_SYSTEMS; ++i)
			{
				orderDroid(systems[i], DORDER_RECYCLE);
			}

			if (isSeaMap === false && NON_HOVER_SYSTEMS === 0)
			{
				removeTimer("recycleDroidsForHover");
			}
		}

		if (isSeaMap)
		{
			var tanks = enumGroup(attackGroup).filter(function(dr) { return (dr.droidType == DROID_WEAPON && dr.propulsion !== "hover01"); });
			const NON_HOVER_TANKS = tanks.length;

			for (var j = 0; j < NON_HOVER_TANKS; ++j)
			{
				orderDroid(tanks[j], DORDER_RECYCLE);
			}

			if (NON_HOVER_TANKS + NON_HOVER_SYSTEMS === 0)
			{
				removeTimer("recycleDroidsForHover");
			}
		}
	}
}

// Adapt to an enemy who has shown us they have VTOL units.
function scanForVTOLs()
{
	if (enemyHasVtol)
	{
		removeTimer("scanForVTOLs");
		return; //already aware of VTOLs
	}

	var myEnemy = getCurrentEnemy();
	if (!defined(myEnemy))
	{
		return;
	}

	var visibleEnemyDroids = enumDroid(myEnemy, DROID_WEAPON, true);
	for (var i = 0, l = visibleEnemyDroids.length; i < l; ++i)
	{
		if (isVTOL(visibleEnemyDroids[i]))
		{
			enemyHasVtol = true;
			break;
		}
	}
}
