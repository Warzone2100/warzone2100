//Contains functions that are either used everywhere or do not have
//a better file to be placed in yet.

// Random number between 0 and max-1.
function random(max)
{
	return (max <= 0) ? 0 : Math.floor(Math.random() * max);
}

function chance(probability)
{
	return random(100) < probability;
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
	const __dist1 = distBetweenTwoPoints(_MY_BASE.x, _MY_BASE.y, obj1.x, obj1.y);
	const __dist2 = distBetweenTwoPoints(_MY_BASE.x, _MY_BASE.y, obj2.x, obj2.y);

	return (__dist1 - __dist2);
}

function addDroidsToGroup(group, droids)
{
	for (let i = 0, d = droids.length; i < d; ++i)
	{
		groupAdd(group, droids[i]);
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

		const __highOil = highOilMap();
		const _targets = [];
		const _derr = (!__highOil) ? findNearestEnemyDerrick(player) : undefined;
		const _struc = findNearestEnemyStructure(player);
		const _droid = findNearestEnemyDroid(player);

		if (_derr)
		{
			_targets.push(getObject(_derr.typeInfo, _derr.playerInfo, _derr.idInfo));
		}

		if (_struc)
		{
			_targets.push(getObject(_struc.typeInfo, _struc.playerInfo, _struc.idInfo));
		}

		if (_droid)
		{
			_targets.push(getObject(_droid.typeInfo, _droid.playerInfo, _droid.idInfo));
		}

		if (_targets.length)
		{
			if (!__highOil && _derr && (chance(7) || (countStruct(_STRUCTURES.derrick, me) <= Math.floor(1.5 * averageOilPerPlayer()))))
			{
				return objectInformation(_derr);
			}
			else
			{
				return objectInformation(_targets.sort(distanceToBase)[0]);
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

	const _players = [];

	for (let i = 0; i < maxPlayers; ++i)
	{
		if (i === me)
		{
			continue;
		}

		if ((!ally && !allianceExistsBetween(i, me)) || (ally && allianceExistsBetween(i, me)))
		{
			_players.push(i);
		}
	}

	return _players;
}

//return real power levels.
function getRealPower(player)
{
	if (!isDefined(player))
	{
		player = me;
	}

	const __power = playerPower(player) - queuedPower(player);

	if (!currently_dead && playerAlliance(true).length && (player === me) && (__power < -300))
	{
		sendChatMessage("need power", ALLIES);
	}

	return __power;
}

//Find enemies that are still alive.
function findLivingEnemies()
{
	function uncached()
	{
		const _alive = [];

		for (let x = 0; x < maxPlayers; ++x)
		{
			if ((x !== me) && !allianceExistsBetween(x, me) && (countDroid(DROID_ANY, x) || enumStruct(x).length))
			{
				_alive.push(x);
			}
			else
			{
				if (allianceExistsBetween(x, me) || (x === me))
				{
					grudgeCount[x] = -2; //Friendly player.
				}
				else
				{
					grudgeCount[x] = -1; //Dead enemy.
				}
			}
		}

		return _alive;
	}

	return cacheThis(uncached, [], "findLivingEnemies" + me, 8000);
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
		let mostHarmful = 0;
		const _enemies = findLivingEnemies();

		if (!_enemies.length)
		{
			const _allEnemies = playerAlliance(false);

			if (_allEnemies.length)
			{
				return _allEnemies[0]; //just focus on a dead enemy then.
			}
			// shouldn't really happen unless someone is doing something really strange.
			return 0; //If nothing to attack, then attack player 0 (happens only after winning).
		}

		for (let x = 0, c = _enemies.length; x < c; ++x)
		{
			if ((grudgeCount[_enemies[x]] >= 0) && (grudgeCount[_enemies[x]] > grudgeCount[mostHarmful]))
			{
				mostHarmful = _enemies[x];
			}
		}

		// Don't have an enemy yet, so pick one randomly (likely start of the game or the counters are all the same).
		if (((me === mostHarmful) || allianceExistsBetween(me, mostHarmful)) && _enemies.length)
		{
			mostHarmful = _enemies[random(_enemies.length)];
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
		let chosenGroup;

		switch (group)
		{
			case "ATTACK": chosenGroup = enumGroup(attackGroup); break;
			case "CYBORG": chosenGroup = enumGroup(attackGroup).filter((dr) => (dr.droidType === DROID_CYBORG)); break;
			case "VTOL": chosenGroup = enumGroup(vtolGroup); break;
			case "TRUCK": chosenGroup = enumGroup(constructGroup).concat(enumGroup(oilGrabberGroup)).concat(enumGroup(constructGroupNTWExtra)); break;
			default: chosenGroup = enumGroup(attackGroup); break;
		}

		const __droidLen = chosenGroup.length;

		if ((__droidLen >= __MIN_ATTACK_DROIDS) || (group === "TRUCK" && __droidLen >= __MIN_TRUCKS_PER_GROUP))
		{
			let idx = 0;
			let amount;

			if (group !== "TRUCK")
			{
				amount = random(__droidLen - (__MIN_ATTACK_DROIDS - 2)) + 1;
			}
			else
			{
				amount = random(__droidLen - (__MIN_TRUCKS_PER_GROUP - 1)) + 1;
			}

			while (idx < amount)
			{
				donateObject(chosenGroup[idx], from);
				++idx;
			}

			return true;
		}
	}

	return false;
}

//Remove timers. May pass a string or an array of strings.
function removeThisTimer(timer)
{
	if (timer instanceof Array)
	{
		for (let i = 0, l = timer.length; i < l; ++i)
		{
			removeTimer(timer[i]);
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
	if (!(countDroid(DROID_ANY, me) || countStruct(_STRUCTURES.factory, me) || countStruct(_STRUCTURES.cyborgFactory, me)))
	{
		currently_dead = true;

		// Remind players to help me... (other Cobra allies will respond)
		if (playerAlliance(true).length)
		{
			const __goodPowerSupply = 200;

			if (playerPower(me) < __goodPowerSupply)
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

	const _cons = enumDroid(me, DROID_CONSTRUCT);

	for (let i = 0, l = _cons.length; i < l; ++i)
	{
		const _con = _cons[i];

		eventDroidBuilt(_con, null);
	}
}

function initCobraVars()
{
	const __isHoverMap = checkIfSeaMap();

	initCobraGroups();
	lastMsg = "eventStartLevel";
	lastMsgThrottle = 0;
	currently_dead = false;
	researchComplete = false;
	initializeGrudgeCounter();
	forceHover = __isHoverMap;
	turnOffCyborgs = __isHoverMap;
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
	randomResearchLabStart = chance(20);
	cyborgOnlyGame = (!getStructureLimit(_STRUCTURES.factory, me) && getStructureLimit(_STRUCTURES.cyborgFactory));
	resObj = {
		lab: undefined,
		cybCheck: false,
		antiPersonnelChance: 0,
		isHighOil: false,
		hasAlly: false,
		highOilResPrice: 0,
		defensiveLimit: false,
		forceLaser: false,
		forceDefenseRes: false
	};
	// We can't just detect no bases cause some maps might just give an HQ on Bases/advanced bases.
	const __lowStartingBaseStructCount = (enumStruct(me).length <= 3);
	noBasesHighTechStart = ((getMultiTechLevel() > 1) && ((baseType === CAMP_CLEAN) || __lowStartingBaseStructCount));
	weirdMapBaseDesign = ((baseType !== CAMP_CLEAN) && __lowStartingBaseStructCount);
}

// T2+ No Bases Low Power. The ultimate meta.
function strangeStartSettingOver()
{
	return (!noBasesHighTechStart || (startAttacking || (countStruct(_STRUCTURES.derrick, me) > 6) || (gameTime > 360000)));
}

// A simple way to make sure a set of xy coordinates are within the map. If the `off` parameter
// is defined, it will offset away from the map. Defaults to 2 as that is roughly where the dark area ends.
function clipToMapBounds(obj, off)
{
	if (!isDefined(obj) || (!isDefined(obj.x) || !isDefined(obj.y)))
	{
		return undefined;
	}

	const __offset = (isDefined(off) ? off : 2);
	let tmp = obj;

	if (tmp.x < __offset)
	{
		tmp.x = __offset;
	}
	else if (tmp.x > mapWidth - __offset)
	{
		tmp.x = mapWidth - __offset;
	}

	if (tmp.y < __offset)
	{
		tmp.y = __offset;
	}
	else if (tmp.y > mapHeight - __offset)
	{
		tmp.y = mapHeight - __offset;
	}

	return tmp;
}

//Attempt to workaround a bug with pickStructLocation() failing to find valid locations
//for base structures (or others) on some maps. Returns an object with a xy position pair.
function randomOffsetLocation(location)
{
	function uncached(location)
	{
		const __mapEdge = 2;
		const __tileOffsetMax = 3;
		let newValueX = chance(50) ? location.x + random(__tileOffsetMax) : location.x - random(__tileOffsetMax);
		let newValueY = chance(50) ? location.y + random(__tileOffsetMax) : location.y - random(__tileOffsetMax);

		return clipToMapBounds({x: newValueX, y: newValueY}, __mapEdge);
	}

	return cacheThis(uncached, [location], "randomOffsetLocation" + me + location.x + location.y, 2000);
}
