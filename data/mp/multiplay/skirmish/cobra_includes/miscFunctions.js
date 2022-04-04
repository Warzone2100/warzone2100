//Contains functions that are either used everywhere or do not have
//a better file to be placed in yet.

// Random number between 0 and max-1.
function random(max)
{
	return (max <= 0) ? 0 : Math.floor(Math.random() * max);
}

// Returns true if something is defined
function isDefined(data)
{
	return typeof data !== "undefined";
}

//Sort an array from smallest to largest in terms of droid health.
function sortDroidsByHealth(a, b)
{
	return a.health - b.health;
}

//Used for deciding if a truck will capture oil.
function isUnsafeEnemyObject(obj)
{
	return ((obj.type === DROID) || ((obj.type === STRUCTURE) && (obj.stattype === DEFENSE) && (obj.status === BUILT)));
}

//Sort by distance to base and reverse.
function sortAndReverseDistance(arr)
{
	return (arr.sort(distanceToBase)).reverse();
}

//Return the alias of the primary weapon.
function returnPrimaryAlias()
{
	return subPersonalities[personality].primaryWeapon.alias;
}

//Return the alias of the secondary weapon.
function returnSecondaryAlias()
{
	return subPersonalities[personality].secondaryWeapon.alias;
}

//Return the alias of the anti-air weaponry.
function returnAntiAirAlias()
{
	return subPersonalities[personality].antiAir.alias;
}

//Return the alias of the artillery weapon.
function returnArtilleryAlias()
{
	return subPersonalities[personality].artillery.alias;
}

function personalityIsRocketMain()
{
	return ((returnPrimaryAlias() === "rkt") || (returnSecondaryAlias() === "rkt"));
}

//Distance between an object and the Cobra base.
function distanceToBase(obj1, obj2)
{
	var dist1 = distBetweenTwoPoints(MY_BASE.x, MY_BASE.y, obj1.x, obj1.y);
	var dist2 = distBetweenTwoPoints(MY_BASE.x, MY_BASE.y, obj2.x, obj2.y);
	return (dist1 - dist2);
}

function addDroidsToGroup(group, droids)
{
	for (const droid of droids)
	{
		groupAdd(group, droid);
	}
}

function nearbyStructureCount(location)
{
	return enumRange(location.x, location.y, 8, ALLIES, false).filter((obj) => (
		obj.type === STRUCTURE
	)).length;
}

//Returns closest enemy object.
function rangeStep(player)
{
	function uncached(player)
	{
		if (!isDefined(player))
		{
			player = getMostHarmfulPlayer();
		}

		var highOil = highOilMap();
		var targets = [];
		var derr;
		var struc = findNearestEnemyStructure(player);
		var droid = findNearestEnemyDroid(player);

		if (!highOil)
		{
			derr = findNearestEnemyDerrick(player);
		}

		if (derr)
		{
			targets.push(getObject(derr.typeInfo, derr.playerInfo, derr.idInfo));
		}
		if (struc)
		{
			targets.push(getObject(struc.typeInfo, struc.playerInfo, struc.idInfo));
		}
		if (droid)
		{
			targets.push(getObject(droid.typeInfo, droid.playerInfo, droid.idInfo));
		}

		if (targets.length > 0)
		{
			if (!highOil && derr && ((random(100) < 7) || (countStruct(structures.derrick) <= Math.floor(1.5 * averageOilPerPlayer()))))
			{
				return objectInformation(derr);
			}
			else
			{
				targets = targets.sort(distanceToBase);
				return objectInformation(targets[0]);
			}
		}

		return undefined;
	}

	return cacheThis(uncached, [player], "rangeStep" + player, 10000);
}

//passing true finds allies and passing false finds enemies.
function playerAlliance(ally)
{
	if (!isDefined(ally))
	{
		ally = false;
	}

	var players = [];

	for (let i = 0; i < maxPlayers; ++i)
	{
		if (i === me)
		{
			continue;
		}

		if ((!ally && !allianceExistsBetween(i, me)) || (ally && allianceExistsBetween(i, me)))
		{
			players.push(i);
		}
	}

	return players;
}

//return real power levels.
function getRealPower(player)
{
	if (!isDefined(player))
	{
		player = me;
	}
	const POWER = playerPower(player) - queuedPower(player);
	if (!currently_dead && playerAlliance(true).length > 0 && player === me && POWER < -300)
	{
		sendChatMessage("need power", ALLIES);
	}

	return POWER;
}

//Find enemies that are still alive.
function findLivingEnemies()
{
	function uncached()
	{
		var alive = [];
		for (let i = 0; i < maxPlayers; ++i)
		{
			if ((i !== me) && !allianceExistsBetween(i, me) && ((countDroid(DROID_ANY, i) > 0) || (enumStruct(i).length > 0)))
			{
				alive.push(i);
			}
			else
			{
				if (allianceExistsBetween(i, me) || (i === me))
				{
					grudgeCount[i] = -2; //Friendly player.
				}
				else
				{
					grudgeCount[i] = -1; //Dead enemy.
				}
			}
		}

		return alive;
	}

	return cacheThis(uncached, [], "findLivingEnemies + me", 8000);
}

//The enemy of which Cobra is focusing on.
function getMostHarmfulPlayer()
{
	if (isDefined(scavengerPlayer) && (gameTime < lastAttackedByScavs + 25000))
	{
		return scavengerPlayer;
	}

	function uncached()
	{
		var mostHarmful = 0;
		var enemies = findLivingEnemies();
		var allEnemies = playerAlliance(false);

		if (enemies.length === 0)
		{
			if (allEnemies.length > 0)
			{
				return allEnemies[0]; //just focus on a dead enemy then.
			}
			// shouldn't really happen unless someone is doing something really strange.
			return 0; //If nothing to attack, then attack player 0 (happens only after winning).
		}

		for (const enemy of enemies)
		{
			if((grudgeCount[enemy] >= 0) && (grudgeCount[enemy] > grudgeCount[mostHarmful]))
			{
				mostHarmful = enemy;
			}
	 	}

		// Don't have an enemy yet, so pick one randomly (likely start of the game or the counters are all the same).
		if (((me === mostHarmful) || allianceExistsBetween(me, mostHarmful)) && enemies.length > 0)
		{
			mostHarmful = enemies[random(enemies.length)];
			grudgeCount[mostHarmful] += 1;
		}

		return mostHarmful;
	}

	return cacheThis(uncached, [], "getMostHarmfulPlayer" + me, 5000);
}

//Set the initial grudge counter to target a random enemy.
function initializeGrudgeCounter()
{
	grudgeCount = [];

	for (let i = 0; i < maxPlayers; ++i)
	{
		grudgeCount.push(0);
	}

	for (let i = 0; i < maxPlayers; ++i)
	{
		if ((!allianceExistsBetween(i, me)) && (i !== me))
		{
			grudgeCount[i] = random(30);
		}
		else
		{
			grudgeCount[i] = -2; //Otherwise bad stuff (attacking itself and allies) happens.
		}
	}
}

//Donate a droid from one of Cobra's groups.
function donateFromGroup(from, group)
{
	if (isDefined(group))
	{
		const MIN_HEALTH = 80;
		var chosenGroup;

		switch (group)
		{
			case "ATTACK": chosenGroup = enumGroup(attackGroup); break;
			case "CYBORG": chosenGroup = enumGroup(attackGroup).filter((dr) => (dr.droidType === DROID_CYBORG)); break;
			case "VTOL": chosenGroup = enumGroup(vtolGroup); break;
			default: chosenGroup = enumGroup(attackGroup); break;
		}

		var droids = chosenGroup.filter((dr) => (dr.health > MIN_HEALTH));
		const CACHE_DROIDS = droids.length;

		if (CACHE_DROIDS >= MIN_ATTACK_DROIDS)
		{
			donateObject(droids[random(CACHE_DROIDS)], from);
		}
	}
}

//Remove timers. May pass a string or an array of strings.
function removeThisTimer(timer)
{
	if (timer instanceof Array)
	{
		for (const timerItem of timer)
		{
			removeTimer(timerItem);
		}
	}
	else
	{
		removeTimer(timer);
	}
}

//Check if Cobra is "alive". If not, the script is put in a very low perf impact state.
function checkIfDead()
{
	if (!(countDroid(DROID_ANY) || countStruct(structures.factory) || countStruct(structures.cyborgFactory)))
	{
		currently_dead = true;

		// Remind players to help me... (other Cobra allies will respond)
		if (playerAlliance(true).length > 0)
		{
			const GOOD_POWER_SUPPLY = 200;

			if (playerPower(me) < GOOD_POWER_SUPPLY)
			{
				sendChatMessage("need power", ALLIES);
			}
			else
			{
				sendChatMessage("need truck", ALLIES);
			}
		}
	}
	else
	{
		currently_dead = false;
	}
}

function initCobraGroups()
{
	attackGroup = newGroup();
	vtolGroup = newGroup();
	sensorGroup = newGroup();
	artilleryGroup = newGroup();
	constructGroup = newGroup();
	constructGroupNTWExtra = newGroup();
	oilGrabberGroup = newGroup();
	retreatGroup = newGroup();

	addDroidsToGroup(attackGroup, enumDroid(me, DROID_WEAPON).filter((obj) => (!obj.isCB)));
	addDroidsToGroup(attackGroup, enumDroid(me, DROID_CYBORG));
	addDroidsToGroup(vtolGroup, enumDroid(me).filter((obj) => (isVTOL(obj))));
	addDroidsToGroup(sensorGroup, enumDroid(me, DROID_SENSOR));
	addDroidsToGroup(artilleryGroup, enumDroid(me, DROID_WEAPON).filter((obj) => (obj.isCB)));

	var cons = enumDroid(me, DROID_CONSTRUCT);
	for (const con of cons)
	{
		eventDroidBuilt(con, null);
	}
}

function initCobraVars()
{
	var isHoverMap = checkIfSeaMap();

	lastMsg = "eventStartLevel";
	lastMsgThrottle = 0;
	currently_dead = false;
	researchComplete = false;
	initializeGrudgeCounter();
	forceHover = isHoverMap;
	turnOffCyborgs = isHoverMap;
	choosePersonality();
	turnOffMG = false;
	useArti = true;
	useVtol = true;
	lastAttackedByScavs = 0;
	beacon = {
		x: 0,
		y: 0,
		startTime: 0,
		endTime: 0,
		wasVtol: false,
		disabled: !highOilMap(), //disabled by default unless it's a high oil map
	};
	enemyUsedElectronicWarfare = false;
	startAttacking = false;
	lastShuffleTime = 0;
	forceDerrickBuildDefense = highOilMap(); //defend base derricks on high/NTW ASAP from rusher trucks
	randomResearchLabStart = (random(100) < 20);
	cyborgOnlyGame = (getStructureLimit(structures.factory, me) === 0 && getStructureLimit(structures.cyborgFactory) > 0);
}

//Attempt to workaround a bug with pickStructLocation() failing to find valid locations
//for base structures (or others) on some maps. Returns an object with a xy position pair.
function randomOffsetLocation(location)
{
	function uncached(location)
	{
		const MAP_EDGE = 2;
		const TILE_OFFSET_MAX = 3;
		var newValueX = (random(100) < 50) ? location.x + random(TILE_OFFSET_MAX) : location.x - random(TILE_OFFSET_MAX);
		var newValueY = (random(100) < 50) ? location.y + random(TILE_OFFSET_MAX) : location.y - random(TILE_OFFSET_MAX);

		if (newValueX < MAP_EDGE)
		{
			newValueX = MAP_EDGE;
		}
		if (newValueY < MAP_EDGE)
		{
			newValueY = MAP_EDGE;
		}
		if (newValueX > mapWidth - MAP_EDGE)
		{
			newValueX = mapWidth - MAP_EDGE;
		}
		if (newValueY > mapHeight - MAP_EDGE)
		{
			newValueY = mapHeight - MAP_EDGE;
		}

		return {x: newValueX, y: newValueY};
	}

	return cacheThis(uncached, [location], "randomOffsetLocation" + me, 2000);
}
