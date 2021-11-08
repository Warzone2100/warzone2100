
//Droids that are attacking should not be pulled away until
//they destroy whatever thay are attacking or need repair.
function droidReady(droidID)
{
	var droid = getObject(DROID, me, droidID);
	if (droid === null)
	{
		return false;
	}

	return (!repairDroid(droidID) &&
		droid.order !== DORDER_ATTACK &&
		droid.order !== DORDER_RTR &&
		droid.order !== DORDER_RECYCLE &&
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

	const MAX_SCAN_RANGE = 15;

	return enumRange(beacon.x, beacon.y, MAX_SCAN_RANGE, ENEMIES, false).length !== 0;
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

		var facs = enumStruct(enemyNumber, FACTORY)
		.concat(enumStruct(enemyNumber, CYBORG_FACTORY))
		.concat(enumStruct(enemyNumber, VTOL_FACTORY));

		if (facs.length > 0)
		{
			facs = facs.sort(distanceToBase);
			return objectInformation(facs[0]);
		}

		return undefined;
	}

	return cacheThis(uncached, [enemyNumber], "returnClosestEnemyFactory" + enemyNumber, 8000);
}

//Should the vtol attack when ammo is high enough?
function vtolReady(droidID)
{
	var droid = getObject(DROID, me, droidID);
	if (droid === null)
	{
		return true;  //Pretend it is busy
	}

	if (!isVTOL(droid))
	{
		return true; //See droidReady().
	}

	const ARMED_PERCENT = 1;
	if ((droid.order === DORDER_ATTACK) || (droid.order === DORDER_REARM))
	{
		return false;
	}
	if (vtolArmed(droid.weapons[0].armed, ARMED_PERCENT))
	{
		return true;
	}
	if (droid.order !== DORDER_REARM)
	{
		orderDroid(droid, DORDER_REARM);
	}

	return false;
}

//Repair a droid with the option of forcing it to.
function repairDroid(droidID, force)
{
	var droid = getObject(DROID, me, droidID);
	if (droid === null)
	{
		return true; //pretend it is busy
	}

	if (droid.droidType === DROID_WEAPON && isPlasmaCannon(droid.weapons[0].name))
	{
		return false; //Plasma Cannon shall never repair
	}

	var highOil = highOilMap();

	if (droid.health < 15 || (highOil && gameTime < 600000))
	{
		return false;
	}

	const SAFE_EXTREME_OIL_IGNORE_NUM = 80;

	var forceRepairPercent = 50;
	const EXPERIENCE_DIVISOR = 26;
	const HEALTH_TO_REPAIR = forceRepairPercent + Math.floor(droid.experience / EXPERIENCE_DIVISOR);

	//Ignore repairing combat units if one super high oil map if we have X amount of units.
	if (countDroid(DROID_ANY, me) >= SAFE_EXTREME_OIL_IGNORE_NUM && highOilMap)
	{
		if ((droid.droidType === DROID_WEAPON || droid.droidType == DROID_CYBORG) && droid.health <= forceRepairPercent)
		{
			return false;
		}
	}

	if (!isDefined(force))
	{
		force = droid.health < forceRepairPercent;
	}

	if (Math.floor(droid.health) <= forceRepairPercent)
	{
		force = true;
	}

	if ((droid.order === DORDER_RTR) && ((Math.floor(droid.health) < 100) || force))
	{
		return true;
	}

	if (countStruct(structures.repair) && (force || (Math.floor(droid.health) <= HEALTH_TO_REPAIR)))
	{
		orderDroid(droid, DORDER_RTR);
		return true;
	}

	return false;
}

//Continuously check a random ground group for repair
function checkAllForRepair()
{
	if (!countStruct(structures.repair))
	{
		return;
	}

	var droids = enumGroup(attackGroup);
	for (var i = 0, l = droids.length; i < l; ++i)
	{
		repairDroid(droids[i].id);
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

		var badDroids = enumDroid(enemy);
		if (badDroids.length > 0)
		{
			var temp = badDroids.filter(function(dr) { return !isVTOL(dr); });
			if (temp.length === 0)
			{
				temp = badDroids;
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

		var s = enumStruct(enemy).filter(function(obj) { return (obj.stattype !== WALL); });
		if (s.length === 0)
		{
			s = enumStruct(enemy);
		}

		if (s.length > 0)
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

		var derr = enumStruct(enemy, structures.derricks).sort(distanceToBase);

		if (derr.length > 0)
		{
			return objectInformation(derr[0]);
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
	var sensors = enumGroup(sensorGroup).filter(function(dr) {
		return droidReady(dr.id);
	});
	const ARTILLERY_UNITS = enumGroup(artilleryGroup);
	const ARTI_LEN = ARTILLERY_UNITS.length;
	const SENS_LEN = sensors.length;

	if (SENS_LEN + ARTI_LEN > 0)
	{
		sensors = sortAndReverseDistance(sensors);
		var obj = rangeStep();

		if (isDefined(obj))
		{
			var tempObj = getObject(obj.typeInfo, obj.playerInfo, obj.idInfo);

			if (SENS_LEN)
			{
				orderDroidObj(sensors[0], DORDER_OBSERVE, tempObj);
			}

			for (var i = 0; i < ARTI_LEN; ++i)
			{
				//Send artillery to help at beacon, if possible
				if ((beacon.endTime > gameTime) &&
					beaconAreaHasEnemies() &&
					(i < Math.floor(ARTI_LEN * subPersonalities[personality].beaconArtilleryPercentage)))
				{
					if (!beacon.wasVtol || (beacon.wasVtol && ARTILLERY_UNITS[i].weapons[0].canHitAir))
					{
						//Attack something in this area, if possible.
						var xRand = (random(100) < 50) ? random(15) : -random(15);
						var yRand = (random(100) < 50) ? random(15) : -random(15);
						var xPos = beacon.x + xRand;
						var yPos = beacon.y + yRand;

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

						orderDroidLoc(ARTILLERY_UNITS[i], DORDER_SCOUT, xPos, yPos);
						continue;
					}
				}

				attackThisObject(ARTILLERY_UNITS[i].id, obj);
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

		var target = rangeStep();
		if (isDefined(target))
		{
			const UNITS = enumGroup(attackGroup).filter(function(dr) {
				return droidReady(dr.id);
			});

			if (UNITS.length < MIN_ATTACK_DROIDS)
			{
				return;
			}

			for (var i = 0, l = UNITS.length; i < l; ++i)
			{
				var id = UNITS[i].id;

				//Send most of army to beacon explicitly
				if ((beacon.endTime > gameTime) &&
					beaconAreaHasEnemies() &&
					(i < Math.floor(UNITS.length * subPersonalities[personality].beaconArmyPercentage)))
				{
					if (!beacon.wasVtol || (beacon.wasVtol && UNITS[i].weapons[0].canHitAir))
					{
						//Attack something in this area, if possible.
						var xRand = (random(100) < 50) ? random(15) : -random(15);
						var yRand = (random(100) < 50) ? random(15) : -random(15);
						var xPos = beacon.x + xRand;
						var yPos = beacon.y + yRand;

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

						orderDroidLoc(UNITS[i], DORDER_SCOUT, xPos, yPos);
						continue;
					}
				}

				attackThisObject(id, target);
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

	const MIN_FACTORY = 1;
	var systems = enumDroid(me, DROID_CONSTRUCT).concat(enumDroid(me, DROID_SENSOR)).filter(function(dr) {
		return (dr.body !== "CyborgLightBody" && dr.propulsion !== "hover01");
	});
	var unfinished = unfinishedStructures();
	const NON_HOVER_SYSTEMS = systems.length;

	if ((countStruct(structures.factory) > MIN_FACTORY) && componentAvailable("hover01"))
	{
		if (!unfinished.length && NON_HOVER_SYSTEMS)
		{
			for (var i = 0; i < NON_HOVER_SYSTEMS; ++i)
			{
				orderDroid(systems[i], DORDER_RECYCLE);
			}
		}

		if (!forceHover && !NON_HOVER_SYSTEMS)
		{
			removeThisTimer("recycleForHover");
		}

		if (forceHover)
		{
			var tanks = enumGroup(attackGroup).filter(function(dr) { return (dr.propulsion !== "hover01"); });
			const NON_HOVER_TANKS = tanks.length;
			for (var j = 0; j < NON_HOVER_TANKS; ++j)
			{
				orderDroid(tanks[j], DORDER_RECYCLE);
			}

			if (NON_HOVER_TANKS + NON_HOVER_SYSTEMS === 0)
			{
				removeThisTimer("recycleForHover");
			}
		}
	}
}

// Make Cobra focus on this player if asked.
function targetPlayer(playerNumber)
{
	const PREVIOUS_TARGET = getMostHarmfulPlayer();
	if (isDefined(scavengerPlayer) && ((playerNumber === scavengerPlayer) || (PREVIOUS_TARGET === scavengerPlayer)))
	{
		return; //No targeting scavs.
	}

	if (playerNumber !== PREVIOUS_TARGET)
	{
		const INC = 400;
		grudgeCount[playerNumber] = grudgeCount[PREVIOUS_TARGET] + INC;
	}
}

//VTOL units do there own form of tactics.
function vtolTactics()
{
	if (currently_dead)
	{
		return;
	}

	const MIN_VTOLS = 5;
	var vtols = enumGroup(vtolGroup).filter(function(dr) {
		return droidReady(dr.id);
	});
	const LEN = vtols.length;
	const D_CIRCLE = 40; //DORDER_CIRCLE = 40
	const SCOUT_TO_CIRCLE_DIST = 2; //when to switch from scout to circle order for beacon

	if (LEN >= MIN_VTOLS)
	{
		var target = rangeStep();

		if (isDefined(target))
		{
			for (var i = 0; i < LEN; ++i)
			{
				var id = vtols[i].id;

				if ((beacon.endTime > gameTime) &&
					(i < Math.floor(LEN * subPersonalities[personality].beaconVtolPercentage)) &&
					(!beacon.wasVtol || (beacon.wasVtol && vtols[i].weapons[0].canHitAir)))
				{
					var pos = {x: vtols[i].x, y: vtols[i].y};
					//Patrol this area for a bit.
					if (vtols[i].order === D_CIRCLE)
					{
						continue;
					}
					if (distBetweenTwoPoints(pos.x, pos.y, beacon.x, beacon.y) <= SCOUT_TO_CIRCLE_DIST)
					{
						orderDroidLoc(vtols[i], D_CIRCLE, beacon.x, beacon.y);
						continue;
					}

					orderDroidLoc(vtols[i], DORDER_SCOUT, beacon.x, beacon.y);
					continue;
				}

				attackThisObject(id, target);
			}
		}
	}
}

//Decide how to attack this target. droidID is Cobra's droid and target is
//the object returned by objectInformation().
function attackThisObject(droidID, target)
{
	var d = getObject(DROID, me, droidID);
	if (d === null || !isDefined(d.weapons[0]))
	{
		return;
	}

	var t = getObject(target.typeInfo, target.playerInfo, target.idInfo);
	if (t === null)
	{
		return;
	}

	var isScav = isDefined(scavengerPlayer) && t.player === scavengerPlayer;
	if (droidReady(droidID) && droidCanReach(d, t.x, t.y))
	{
		if (isScav && getMostHarmfulPlayer() !== scavengerPlayer)
		{
			orderDroid(d, DORDER_HOLD); //quick reset
			return;
		}

		if (!((t.type === DROID) && isVTOL(t) && (isVTOL(d) && !d.weapons[0].canHitAir)))
		{
			if (t.type === DROID || (t.type === STRUCTURE && t.stattype !== WALL))
			{
				orderDroidLoc(d, DORDER_SCOUT, t.x, t.y);
			}
			else
			{
				orderDroidObj(d, DORDER_ATTACK, t);
			}
		}
	}
}

//Check if enemy units are in or around Cobra base.
function enemyUnitsInBase()
{
	var area = cobraBaseArea();
	var enemyUnits = enumArea(area.x1, area.y1, area.x2, area.y2, ENEMIES, false).filter(function(obj) {
		return (obj.type === DROID && (obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG));
	}).sort(distanceToBase);

	//The attack code automatically chooses the closest object of the
	//most harmful player anyway so this should suffice for defense.
	if (enemyUnits.length > 0)
	{
		if (!startAttacking &&
			enemyUnits[0].droidType !== DROID_CONSTRUCT &&
			enemyUnits[0].droidType !== DROID_SENSOR)
		{
			startAttacking = enemyUnits[0].player !== scavengerPlayer;
		}

		targetPlayer(enemyUnits[0].player); //play rough.

		//Send a beacon that enemies are in my base area! Allied Cobra AI can interpret and help friends through this drop.
		if (!beacon.disabled && (beacon.endTime < gameTime))
		{
			var mes = isVTOL(enemyUnits[0]) ? BEACON_VTOL_ALARM : undefined;
			addBeacon(enemyUnits[0].x, enemyUnits[0].y, ALLIES, mes);
			//Set beacon data for me also since we won't receive our own beacon.
			eventBeacon(enemyUnits[0].x, enemyUnits[0].y, me, me, mes);
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
	if (!countStruct(structures.gen) || !countStruct(structures.derrick))
	{
		return;
	}

	const ALLY_PLAYERS = playerAlliance(true).filter(function(player) { return playerData[player].isAI; });
	const LEN = ALLY_PLAYERS.length;
	const ALIVE_ENEMIES = findLivingEnemies().length;

	if (LEN > 0 && ALIVE_ENEMIES > 0)
	{
		var ally = ALLY_PLAYERS[random(LEN)];
		if (getRealPower() > 300 && (getRealPower() > Math.floor(1.5 * getRealPower(ally))))
		{
			donatePower(playerPower(me) / 2, ally);
		}
	}
}

//Have Cobra sit and wait and build up a small army before starting attack tactics.
function haveEnoughUnitsForFirstAttack()
{
	if (!startAttacking)
	{
		var amountOfAttackers = groupSize(attackGroup) + groupSize(artilleryGroup) + groupSize(vtolGroup);
		// These amounts of units will build up in base if unprovoked
		startAttacking = amountOfAttackers >= (highOilMap() ? 120 : 20);
	}

	return startAttacking;
}

function baseShuffleDefensePattern()
{
	if (gameTime < lastShuffleTime + 40000)
	{
		return; //Prevent the dreadful jitter movement defense pattern.
	}

	var attackers = enumGroup(attackGroup).concat(enumGroup(artilleryGroup)).concat(enumGroup(vtolGroup));
	if (attackers.length === 0)
	{
		return;
	}

	var area = cobraBaseArea();
	var quad = [
		{x1: area.x1, x2: area.x2, y1: area.y1, y2: area.y1 + 10,},
		{x1: area.x1, x2: area.x2 + 10, y1: area.y1, y2: area.y2,},
		{x1: area.x2 - 10, x2: area.x2, y1: area.y1, y2: area.y2,},
		{x1: area.x1, x2: area.x2, y1: area.y2 - 10, y2: area.y2,},
	];

	const MAX_ATTEMPTS = 100;
	var sector;
	var x;
	var y;
	var attempts = 0;

	do
	{
		++attempts;
		sector = quad[random(quad.length)];
		x = sector.x1 + random(sector.x2);
		y = sector.y1 + random(sector.y2);
	} while (!propulsionCanReach("wheeled01", MY_BASE.x, MY_BASE.y, x, y) && attempts < MAX_ATTEMPTS);

	if (attempts > MAX_ATTEMPTS)
	{
		return;
	}

	if (x <= 2)
	{
		x = 2;
	}
	else if (x >= mapWidth - 2)
	{
		x = mapWidth - 2;
	}
	if (y <= 2)
	{
		y = 2;
	}
	else if (y >= mapHeight - 2)
	{
		y = mapHeight - 2;
	}
	// Given that the base area has an additional 20 tiles of territory around the furthest base structure in a rectangel/square
	// we can safely tell units to go into this territory zone to keep trucks from being obstructed, maybe.
	const MAX_NEARBY_STRUCTURES = 2;
	for (var i = 0, len = attackers.length; i < len; ++i)
	{
		if (attackers[i].order !== DORDER_HOLD)
		{
			if (nearbyStructureCount({x: attackers[i].x, y: attackers[i].y}) <= MAX_NEARBY_STRUCTURES)
			{
				orderDroid(attackers[i], DORDER_HOLD);
			}
			else
			{
				orderDroidLoc(attackers[i], DORDER_SCOUT, x, y);
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

	var derricks = enumStruct(me, structures.derrick);
	if (derricks.length === 0)
	{
		return;
	}

	var attackers = enumGroup(attackGroup).concat(enumGroup(artilleryGroup)).concat(enumGroup(vtolGroup));
	if (attackers.length === 0)
	{
		return;
	}

	for (var i = 0, len = attackers.length; i < len; ++i)
	{
		var derr = derricks[random(derricks.length)];
		orderDroidLoc(attackers[i], DORDER_SCOUT, derr.x, derr.y);
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
	const SCAN_RADIUS = subPersonalities[personality].retreatScanRange;
	var droids = enumGroup(retreatGroup);

	//Flee!
	for (var i = 0, len = droids.length; i < len; ++i)
	{
		var droid = droids[i];
		var friends = enumRange(droid.x, droid.y, SCAN_RADIUS, ALLIES, false).filter(function(obj) {
			return obj.type === DROID;
		});

		if (enumRange(droid.x, droid.y, SCAN_RADIUS, ENEMIES, !highOilMap()).length > friends.length)
		{
			orderDroidLoc(droid, DORDER_MOVE, MY_BASE.x, MY_BASE.y);
		}
		else
		{
			//Only flee as long as we see enemies.
			eventDroidBuilt(droid, null);
		}
	}
}
