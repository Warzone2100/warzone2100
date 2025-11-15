
//Droids that are attacking should not be pulled away until
//they destroy whatever thay are attacking or need repair.
function droidReady(droidID)
{
	const _droid = getObject(DROID, me, droidID);

	if (_droid === null)
	{
		return false;
	}

	return (!repairDroid(droidID) &&
		_droid.order !== DORDER_ATTACK &&
		_droid.order !== DORDER_RTR &&
		_droid.order !== DORDER_RECYCLE &&
		vtolReady(droidID) //True for non-VTOL units
	);
}

//Check if a passed in weapon name is a plasma cannon.
function isPlasmaCannon(weaponName)
{
	return isDefined(weaponName) && (weaponName === "Laser4-PlasmaCannon");
}

//Check if the area around the active beacon has anything worth investigating.
function beaconAreaHasEnemies()
{
	if (beacon.disabled || (beacon.endTime < gameTime))
	{
		return false;
	}

	const __maxScanRange = 15;

	return (enumRange(beacon.x, beacon.y, __maxScanRange, ENEMIES, false).length > 0);
}

//Modified from Nullbot.
//Returns true if the VTOL has ammo. False if empty.
//NOTE: Expects the .armed property being passed.
function vtolArmed(armedValue, percent)
{
	return armedValue >= percent;
}

//Return information about the closest factory for an enemy. Undefined if none.
function returnClosestEnemyFactory(enemyNumber)
{
	function uncached(enemyNumber)
	{
		if (!isDefined(enemyNumber))
		{
			enemyNumber = getMostHarmfulPlayer();
		}

		const _facs = enumStruct(enemyNumber, FACTORY)
		.concat(enumStruct(enemyNumber, CYBORG_FACTORY))
		.concat(enumStruct(enemyNumber, VTOL_FACTORY)).sort(distanceToBase);

		if (_facs.length)
		{
			return objectInformation(_facs[0]);
		}

		return undefined;
	}

	return cacheThis(uncached, [enemyNumber], "returnClosestEnemyFactory" + enemyNumber, 8000);
}

//Should the vtol attack when ammo is high enough?
function vtolReady(droidID)
{
	const _droid = getObject(DROID, me, droidID);

	if (_droid === null)
	{
		return true;  //Pretend it is busy
	}

	if (!isVTOL(_droid))
	{
		return true; //See droidReady().
	}

	const __armedPercent = 1;

	if ((_droid.order === DORDER_ATTACK) || (_droid.order === DORDER_REARM))
	{
		return false;
	}

	if (vtolArmed(_droid.weapons[0].armed, __armedPercent))
	{
		return true;
	}

	if (_droid.order !== DORDER_REARM)
	{
		orderDroid(_droid, DORDER_REARM);
	}

	return false;
}

//Repair a droid with the option of forcing it to.
function repairDroid(droidID, force)
{
	const _droid = getObject(DROID, me, droidID);

	if (_droid === null)
	{
		return true; //pretend it is busy
	}

	if (_droid.droidType === DROID_WEAPON && isPlasmaCannon(_droid.weapons[0].name))
	{
		return false; //Plasma Cannon shall never repair
	}

	const __highOil = highOilMap();

	if (_droid.health < 15 || (__highOil && gameTime < 600000))
	{
		return false;
	}

	const __safeExtremeOilIgnoreNum = 100;
	const __extraRepairThreshold = 5;
	const __forceRepairPercent = 60;
	const __healthToRepair = __forceRepairPercent + ((_droid.experience > 2) ? __extraRepairThreshold : 0);

	//Ignore repairing combat units if on super high oil map if we have X amount of units.
	if (countDroid(DROID_ANY, me) >= __safeExtremeOilIgnoreNum && __highOil)
	{
		if ((_droid.droidType === DROID_WEAPON || _droid.droidType === DROID_CYBORG) && _droid.health <= __forceRepairPercent)
		{
			return false;
		}
	}

	if (!isDefined(force))
	{
		force = _droid.health < __forceRepairPercent;
	}

	if (Math.floor(_droid.health) <= __forceRepairPercent)
	{
		force = true;
	}

	if ((_droid.order === DORDER_RTR) && ((Math.floor(_droid.health) < 100) || force))
	{
		return true;
	}

	if (countStruct(_STRUCTURES.repair, me) && (force || (Math.floor(_droid.health) <= __healthToRepair)))
	{
		orderDroid(_droid, DORDER_RTR);
		return true;
	}

	return false;
}

//Continuously check a random ground group for repair
function checkAllForRepair()
{
	if (!countStruct(_STRUCTURES.repair, me))
	{
		return;
	}

	const _droids = enumGroup(attackGroup);

	for (let i = 0, l = _droids.length; i < l; ++i)
	{
		repairDroid(_droids[i].id);
	}
}

//Find information about the closest enemy droid. Returns undefined otherwise. Do not target VTOLs
//unless they are the only remaining droids.
function findNearestEnemyDroid(enemy)
{
	function uncached(enemy)
	{
		if (!isDefined(enemy))
		{
			enemy = getMostHarmfulPlayer();
		}

		const _badDroids = enumDroid(enemy);

		if (_badDroids.length)
		{
			let temp = _badDroids.filter((dr) => (!isVTOL(dr)));

			if (!temp.length)
			{
				temp = _badDroids;
			}

			temp = temp.sort(distanceToBase);

			return objectInformation(temp[0]);
		}

		return undefined;
	}

	return cacheThis(uncached, [enemy], "findNearestEnemyDroid" + enemy, 5000);
}

//Return information about the closest structure of an enemy. Returns undefined otherwise.
function findNearestEnemyStructure(enemy)
{
	function uncached(enemy)
	{
		if (!isDefined(enemy))
		{
			enemy = getMostHarmfulPlayer();
		}

		let s = enumStruct(enemy).filter((obj) => (obj.stattype !== WALL));

		if (!s.length)
		{
			s = enumStruct(enemy);
		}

		if (s.length)
		{
			s = s.sort(distanceToBase);

			return objectInformation(s[0]);
		}

		return undefined;
	}

	return cacheThis(uncached, [enemy], "findNearestEnemyStructure" + enemy, 5000);
}

//Return information about the closest derrick of an enemy. Returns undefined otherwise.
function findNearestEnemyDerrick(enemy)
{
	function uncached(enemy)
	{
		if (!isDefined(enemy))
		{
			enemy = getMostHarmfulPlayer();
		}

		const _derr = enumStruct(enemy, _STRUCTURES.derricks).sort(distanceToBase);

		if (_derr.length)
		{
			return objectInformation(_derr[0]);
		}

		return undefined;
	}

	return cacheThis(uncached, [enemy], "findNearestEnemyDerrick" + enemy, 5000);
}

//Sensors know all your secrets. They will observe what is closest to Cobra base.
function artilleryTactics()
{
	if (currently_dead || !shouldCobraAttack())
	{
		return;
	}

	let sensors = enumGroup(sensorGroup).filter((dr) => (droidReady(dr.id)));
	const _artilleryUnits = enumGroup(artilleryGroup);
	const __artiLen = _artilleryUnits.length;
	const __sensLen = sensors.length;

	if ((__sensLen + __artiLen) > 0)
	{
		sensors = sortAndReverseDistance(sensors);
		const _obj = rangeStep();

		if (isDefined(_obj))
		{
			const _tempObj = getObject(_obj.typeInfo, _obj.playerInfo, _obj.idInfo);

			if (__sensLen && _tempObj)
			{
				orderDroidObj(sensors[0], DORDER_OBSERVE, _tempObj);
			}

			for (let i = 0; i < __artiLen; ++i)
			{
				//Send artillery to help at beacon, if possible
				if ((beacon.endTime > gameTime) &&
					beaconAreaHasEnemies() &&
					(i < Math.floor(__artiLen * subPersonalities[personality].beaconArtilleryPercentage)))
				{
					if (!beacon.wasVtol || (beacon.wasVtol && _artilleryUnits[i].weapons[0].canHitAir))
					{
						//Attack something in this area, if possible.
						let xRand = chance(50) ? random(15) : -random(15);
						let yRand = chance(50) ? random(15) : -random(15);
						let xPos = beacon.x + xRand;
						let yPos = beacon.y + yRand;

						if (xPos < 2)
						{
							xPos = 2;
						}
						else if (xPos > mapWidth - 2)
						{
							xPos = mapWidth - 2;
						}
						if (yPos < 2)
						{
							yPos = 2;
						}
						else if (yPos > mapHeight - 2)
						{
							yPos = mapHeight - 2;
						}

						orderDroidLoc(_artilleryUnits[i], DORDER_SCOUT, xPos, yPos);
						continue;
					}
				}

				attackThisObject(_artilleryUnits[i].id, _obj);
			}
		}
	}
}

//Defend or attack.
function groundTactics()
{
	if (!currently_dead && shouldCobraAttack())
	{
		donateSomePower();

		const _target = rangeStep();
		if (isDefined(_target))
		{
			const _units = enumGroup(attackGroup).filter((dr) => (
				droidReady(dr.id)
			));

			if (!noBasesHighTechStart && (_units.length < __MIN_ATTACK_DROIDS))
			{
				return;
			}

			for (let i = 0, l = _units.length; i < l; ++i)
			{
				const __id = _units[i].id;

				//Send most of army to beacon explicitly
				if ((beacon.endTime > gameTime) &&
					beaconAreaHasEnemies() &&
					(i < Math.floor(_units.length * subPersonalities[personality].beaconArmyPercentage)))
				{
					if (!beacon.wasVtol || (beacon.wasVtol && _units[i].weapons[0].canHitAir))
					{
						//Attack something in this area, if possible.
						let xRand = chance(50) ? random(15) : -random(15);
						let yRand = chance(50) ? random(15) : -random(15);
						let xPos = beacon.x + xRand;
						let yPos = beacon.y + yRand;

						if (xPos < 2)
						{
							xPos = 2;
						}
						else if (xPos > mapWidth - 2)
						{
							xPos = mapWidth - 2;
						}
						if (yPos < 2)
						{
							yPos = 2;
						}
						else if (yPos > mapHeight - 2)
						{
							yPos = mapHeight - 2;
						}

						orderDroidLoc(_units[i], DORDER_SCOUT, xPos, yPos);
						continue;
					}
				}

				attackThisObject(__id, _target);
			}
		}
	}
}

//Recycle units when certain conditions are met.
function recycleForHover()
{
	if (currently_dead || !attackerCountsGood(true))
	{
		return;
	}

	if (highOilMap() && (!startAttacking || (gameTime < 900000) || (countDroid(DROID_ANY, me) < 75)))
	{
		return; //wait
	}

	const __minFactories = 1;
	const _systems = enumDroid(me, DROID_CONSTRUCT).concat(enumDroid(me, DROID_SENSOR)).filter((dr) => (
		dr.body !== "CyborgLightBody" && dr.propulsion !== "hover01"
	));
	const _unfinished = unfinishedStructures();
	const __nonHoverSystemsLen = _systems.length;

	if ((countStruct(_STRUCTURES.factory, me) > __minFactories) && componentAvailable("hover01"))
	{
		if (!_unfinished.length && __nonHoverSystemsLen)
		{
			for (let i = 0; i < __nonHoverSystemsLen; ++i)
			{
				orderDroid(_systems[i], DORDER_RECYCLE);
			}
		}

		if (!forceHover && !__nonHoverSystemsLen)
		{
			removeThisTimer("recycleForHover");
		}

		if (forceHover)
		{
			const _tanks = enumGroup(attackGroup).filter((dr) => (dr.propulsion !== "hover01"));
			const __nonHoverTanksLen = _tanks.length;

			for (let j = 0; j < __nonHoverTanksLen; ++j)
			{
				orderDroid(_tanks[j], DORDER_RECYCLE);
			}

			if (!__nonHoverTanksLen && !__nonHoverSystemsLen)
			{
				removeThisTimer("recycleForHover");
			}
		}
	}
}

// Make Cobra focus on this player if asked.
function targetPlayer(playerNumber)
{
	const __previousTarget = getMostHarmfulPlayer();

	if (isDefined(scavengerPlayer) && ((playerNumber === scavengerPlayer) || (__previousTarget === scavengerPlayer)))
	{
		return false; //No targeting scavs.
	}

	if (playerNumber === __previousTarget)
	{
		return false;
	}

	const __inc = 400;
	grudgeCount[playerNumber] = grudgeCount[__previousTarget] + __inc;

	return true;
}

//VTOL units do there own form of tactics.
function vtolTactics()
{
	if (currently_dead)
	{
		return;
	}

	const __minVtols = 5;
	const _vtols = enumGroup(vtolGroup).filter((dr) => (droidReady(dr.id)));
	const __len = _vtols.length;
	const __dorderCircle = 40; //DORDER_CIRCLE = 40
	const __scoutToCircleDist = 2; //when to switch from scout to circle order for beacon

	if (__len >= __minVtols)
	{
		const _target = rangeStep();

		if (isDefined(_target))
		{
			for (let i = 0; i < __len; ++i)
			{
				const __id = _vtols[i].id;

				if ((beacon.endTime > gameTime) &&
					(i < Math.floor(__len * subPersonalities[personality].beaconVtolPercentage)) &&
					(!beacon.wasVtol || (beacon.wasVtol && _vtols[i].weapons[0].canHitAir)))
				{
					const _pos = {x: _vtols[i].x, y: _vtols[i].y};
					//Patrol this area for a bit.
					if (_vtols[i].order === __dorderCircle)
					{
						continue;
					}

					if (distBetweenTwoPoints(_pos.x, _pos.y, beacon.x, beacon.y) <= __scoutToCircleDist)
					{
						orderDroidLoc(_vtols[i], __dorderCircle, beacon.x, beacon.y);
						continue;
					}

					orderDroidLoc(_vtols[i], DORDER_SCOUT, beacon.x, beacon.y);
					continue;
				}

				attackThisObject(__id, _target);
			}
		}
	}
}

//Decide how to attack this target. droidID is Cobra's droid and target is
//the object returned by objectInformation().
function attackThisObject(droidID, target)
{
	const _d = getObject(DROID, me, droidID);

	if (_d === null || !isDefined(_d.weapons[0]))
	{
		return;
	}

	const _t = getObject(target.typeInfo, target.playerInfo, target.idInfo);

	if (_t === null)
	{
		return;
	}

	const __isScav = isDefined(scavengerPlayer) && _t.player === scavengerPlayer;

	if (droidReady(droidID) && droidCanReach(_d, _t.x, _t.y))
	{
		if (__isScav && getMostHarmfulPlayer() !== scavengerPlayer)
		{
			orderDroid(_d, DORDER_HOLD); //quick reset
			return;
		}

		if (!((_t.type === DROID) && isVTOL(_t) && (isVTOL(_d) && !_d.weapons[0].canHitAir)))
		{
			if (_t.type === DROID || (_t.type === STRUCTURE && _t.stattype !== WALL))
			{
				orderDroidLoc(_d, DORDER_SCOUT, _t.x, _t.y);
			}
			else
			{
				orderDroidObj(_d, DORDER_ATTACK, _t);
			}
		}
	}
}

//Check if enemy units are in or around Cobra base.
function enemyUnitsInBase()
{
	const _area = cobraBaseArea();
	const _enemyUnits = enumArea(_area.x1, _area.y1, _area.x2, _area.y2, ENEMIES, false).filter((obj) => (
		obj.type === DROID && (obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG)
	)).sort(distanceToBase);

	//The attack code automatically chooses the closest object of the
	//most harmful player anyway so this should suffice for defense.
	if (_enemyUnits.length)
	{
		if (!startAttacking &&
			_enemyUnits[0].droidType !== DROID_CONSTRUCT &&
			_enemyUnits[0].droidType !== DROID_SENSOR)
		{
			startAttacking = _enemyUnits[0].player !== scavengerPlayer;
		}

		targetPlayer(_enemyUnits[0].player); //play rough.

		//Send a beacon that enemies are in my base area! Allied Cobra AI can interpret and help friends through this drop.
		if (!beacon.disabled && (beacon.endTime < gameTime))
		{
			const __mes = isVTOL(_enemyUnits[0]) ? __BEACON_VTOL_ALARM : undefined;

			addBeacon(_enemyUnits[0].x, _enemyUnits[0].y, ALLIES, __mes);
			//Set beacon data for me also since we won't receive our own beacon.
			eventBeacon(_enemyUnits[0].x, _enemyUnits[0].y, me, me, __mes);
		}

		return true;
	}

	return false;
}

//Donate my power to allies if I have too much. Only to other AI.
function donateSomePower()
{
	if (currently_dead)
	{
		return;
	}

	if (!countStruct(_STRUCTURES.gen, me) || !countStruct(_STRUCTURES.derrick, me))
	{
		return;
	}

	const __allyPlayers = playerAlliance(true).filter((player) => (playerData[player].isAI));
	const __len = __allyPlayers.length;
	const __alivePlayers = findLivingEnemies().length;

	if (__len && __alivePlayers)
	{
		const __ally = __allyPlayers[random(__len)];
		const __pow = 300;

		if (getRealPower() > __pow && (getRealPower() > Math.floor(1.5 * getRealPower(__ally))))
		{
			donatePower(playerPower(me) / 2, __ally);
		}
	}
}

//Have Cobra sit and wait and build up a small army before starting attack tactics.
function haveEnoughUnitsForFirstAttack()
{
	if (noBasesHighTechStart && !highOilMap())
	{
		return true;
	}

	if (!startAttacking)
	{
		const __amountOfAttackers = groupSize(attackGroup) + groupSize(artilleryGroup) + groupSize(vtolGroup);
		// These amounts of units will build up in base if unprovoked
		startAttacking = __amountOfAttackers >= (highOilMap() ? 120 : 20);
	}

	return startAttacking;
}

function baseShuffleDefensePattern()
{
	if (gameTime < lastShuffleTime + 40000)
	{
		return; //Prevent the dreadful jitter movement defense pattern.
	}

	const _attackers = enumGroup(attackGroup).concat(enumGroup(artilleryGroup)).concat(enumGroup(vtolGroup));

	if (!_attackers.length)
	{
		return;
	}

	const _area = cobraBaseArea();
	const _quad = [
		{x1: _area.x1, x2: _area.x2, y1: _area.y1, y2: _area.y1 + 10,},
		{x1: _area.x1, x2: _area.x2 + 10, y1: _area.y1, y2: _area.y2,},
		{x1: _area.x2 - 10, x2: _area.x2, y1: _area.y1, y2: _area.y2,},
		{x1: _area.x1, x2: _area.x2, y1: _area.y2 - 10, y2: _area.y2,},
	];

	const __maxAttempts = 100;
	const __areaOffset = 2;
	const _cords = {x: 0, y: 0};
	let sector;
	let attempts = 0;

	do
	{
		++attempts;
		sector = _quad[random(_quad.length)];
		_cords.x = sector.x1 + random(sector.x2);
		_cords.y = sector.y1 + random(sector.y2);
	} while (!propulsionCanReach("wheeled01", _MY_BASE.x, _MY_BASE.y, _cords.x, _cords.y) && attempts < __maxAttempts);

	if (attempts > __maxAttempts)
	{
		return;
	}

	const clippedObj = clipToMapBounds(_cords, __areaOffset);
	// Given that the base area has an additional 20 tiles of territory around the furthest base structure in a rectangel/square
	// we can safely tell units to go into this territory zone to keep trucks from being obstructed, maybe.
	const __maxNearbyStructures = 2;

	for (let i = 0, len = _attackers.length; i < len; ++i)
	{
		if (_attackers[i].order !== DORDER_HOLD)
		{
			if (nearbyStructureCount({x: _attackers[i].x, y: _attackers[i].y}) <= __maxNearbyStructures)
			{
				orderDroid(_attackers[i], DORDER_HOLD);
			}
			else
			{
				orderDroidLoc(_attackers[i], DORDER_SCOUT, clippedObj.x, clippedObj.y);
			}
		}
	}

	lastShuffleTime = gameTime;
}

function lowOilDefensePattern()
{
	if (gameTime < lastShuffleTime + 40000)
	{
		return; //visit a derrick for a bit... maybe
	}

	const _derricks = enumStruct(me, _STRUCTURES.derrick);

	if (!_derricks.length)
	{
		return;
	}

	const _attackers = enumGroup(attackGroup).concat(enumGroup(artilleryGroup)).concat(enumGroup(vtolGroup));

	if (!_attackers.length)
	{
		return;
	}

	for (let i = 0, len = _attackers.length; i < len; ++i)
	{
		const _derr = _derricks[random(_derricks.length)];

		orderDroidLoc(_attackers[i], DORDER_SCOUT, _derr.x, _derr.y);
	}

	lastShuffleTime = gameTime;
}

//Check if our forces are large enough to take on the most harmful player.
function shouldCobraAttack()
{
	if (enemyUnitsInBase() || haveEnoughUnitsForFirstAttack())
	{
		return true;
	}
	else if (!startAttacking)
	{
		if (highOilMap())
		{
			baseShuffleDefensePattern();
		}
		else
		{
			lowOilDefensePattern();
		}
	}

	return false;
}

//Controls how long localized group retreat happens. See also eventAttacked.
function retreatTactics()
{
	const __scanRadius = subPersonalities[personality].retreatScanRange;
	const _droids = enumGroup(retreatGroup);
	const __highOil = highOilMap();

	//Flee!
	for (let i = 0, len = _droids.length; i < len; ++i)
	{
		const _droid = _droids[i];
		const __friendsLen = enumRange(_droid.x, _droid.y, __scanRadius, ALLIES, false).filter((obj) => (
			obj.type === DROID
		)).length;

		if (enumRange(_droid.x, _droid.y, __scanRadius, ENEMIES, !__highOil).length > __friendsLen)
		{
			orderDroidLoc(_droid, DORDER_MOVE, _MY_BASE.x, _MY_BASE.y);
		}
		else
		{
			//Only flee as long as we see enemies.
			eventDroidBuilt(_droid, null);
		}
	}
}
