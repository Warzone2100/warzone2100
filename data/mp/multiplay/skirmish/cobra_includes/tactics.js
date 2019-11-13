
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
	return isDefined(weaponName) && (weaponName.name === "Laser4-PlasmaCannon");
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

		var facs = enumStruct(enemyNumber, FACTORY);
		facs.concat(enumStruct(enemyNumber, CYBORG_FACTORY));
		facs.concat(enumStruct(enemyNumber, VTOL_FACTORY));

		if (facs.length > 0)
		{
			facs = facs.sort(distanceToBase);
			return objectInformation(facs[0]);
		}

		return undefined;
	}

	return cacheThis(uncached, [enemyNumber]);
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

	const FORCE_REPAIR_PERCENT = 66;
	const EXPERIENCE_DIVISOR = 26;
	const HEALTH_TO_REPAIR = 67 + Math.floor(droid.experience / EXPERIENCE_DIVISOR);

	if (!isDefined(force))
	{
		force = droid.health < FORCE_REPAIR_PERCENT;
	}

	if (Math.floor(droid.health) <= FORCE_REPAIR_PERCENT)
	{
		force = true;
	}

	if ((droid.order === DORDER_RTR) && ((Math.floor(droid.health) < 100) || force))
	{
		return true;
	}

	if (countStruct(structures.extras[0]) && (force || (Math.floor(droid.health) <= HEALTH_TO_REPAIR)))
	{
		orderDroid(droid, DORDER_RTR);
		return true;
	}

	return false;
}

//Continuously check a random ground group for repair
function checkAllForRepair()
{
	if (!countStruct(structures.extras[0]))
	{
		return;
	}

	var droids = enumGroup(attackGroup);
	for (var i = 0, l = droids.length; i < l; ++i)
	{
		repairDroid(droids[i].id);
	}
}

//choose land attackers.
function chooseGroup()
{
	return enumGroup(attackGroup).filter(function(dr) { return droidReady(dr.id); });
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

	return cacheThis(uncached, [enemy], enemy, 16000);
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

	return cacheThis(uncached, [enemy], enemy, 16000);
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
				attackThisObject(ARTILLERY_UNITS[i].id, obj);
			}
		}
	}
}

//Defend or attack.
function groundTactics()
{
	donateSomePower();

	if (!currently_dead && shouldCobraAttack())
	{
		var target = rangeStep();
		if (isDefined(target))
		{
			const WHO = chooseGroup();
			if (WHO.length < MIN_ATTACK_DROIDS)
			{
				return;
			}
			for (var i = 0, l = WHO.length; i < l; ++i)
			{
				attackThisObject(WHO[i].id, target);
			}
		}
	}
}

//Recycle units when certain conditions are met.
function recycleForHover()
{
	if (currently_dead)
	{
		return;
	}

	const MIN_FACTORY = 1;
	var systems = enumDroid(me, DROID_CONSTRUCT).concat(enumDroid(me, DROID_SENSOR)).concat(enumDroid(me, DROID_REPAIR)).filter(function(dr) {
		return (dr.body !== "CyborgLightBody" && dr.propulsion !== "hover01");
	});
	var unfinished = unfinishedStructures();
	const NON_HOVER_SYSTEMS = systems.length;

	if ((countStruct(FACTORY) > MIN_FACTORY) && componentAvailable("hover01"))
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

//Tell the repair group to go repair other droids.
function repairDroidTactics()
{
	if (currently_dead)
	{
		return;
	}

	var reps = enumGroup(repairGroup);
	const LEN = reps.length;

	if (LEN > 0)
	{
		var myDroids = enumGroup(attackGroup).filter(function(dr) {
			return dr.order !== DORDER_RTR;
		});

		if (myDroids.length > 0)
		{
			myDroids = myDroids.sort(sortDroidsByHealth);
			var weakest = myDroids[0];
			var dorder_droidrepair = 26; //FIXME: when DORDER_DROIDREPAIR can be called, remove this.

			for (var i = 0; i < LEN; ++i)
			{
				const REPAIR_UNIT = reps[i];
				if ((REPAIR_UNIT.order !== dorder_droidrepair) && (Math.ceil(weakest.health) < 100))
				{
					orderDroidLoc(weakest, DORDER_MOVE, REPAIR_UNIT.x, REPAIR_UNIT.y);
					orderDroidObj(REPAIR_UNIT, dorder_droidrepair, weakest);
				}
			}
		}
	}
}

// Make Cobra focus on this player if asked. Chat command only.
function targetPlayer(playerNumber)
{
	const INC = 100;
	const PREVIOUS_TARGET = getMostHarmfulPlayer();
	if (isDefined(scavengerPlayer) && ((playerNumber === scavengerPlayer) || (PREVIOUS_TARGET === scavengerPlayer)))
	{
		return; //No targeting scavs.
	}

	if (playerNumber !== PREVIOUS_TARGET)
	{
		if ((grudgeCount[playerNumber] + INC) < MAX_GRUDGE)
		{
			grudgeCount[playerNumber] = grudgeCount[PREVIOUS_TARGET] + INC;
		}
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

	if (LEN >= MIN_VTOLS)
	{
		var target = rangeStep();
		if (isDefined(target))
		{
			for (var i = 0; i < LEN; ++i)
			{
				attackThisObject(vtols[i].id, target);
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
			if (!isPlasmaCannon(d.weapons[0].name) && (t.type === DROID || (t.type === STRUCTURE && t.stattype !== WALL)))
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
	});

	var enemyNearBase = enemyUnits.sort(distanceToBase);

	//The attack code automatically chooses the closest object of the
	//most harmful player anyway so this should suffice for defense.
	if (enemyNearBase.length > 0)
	{
		targetPlayer(enemyUnits[0].player); //play rough.
	}

	return isDefined(enemyNearBase);
}

//Donate my power to allies if I have too much.
function donateSomePower()
{
	if (currently_dead)
	{
		return;
	}

	const ALLY_PLAYERS = playerAlliance(true);
	const LEN = ALLY_PLAYERS.length;
	const ALIVE_ENEMIES = findLivingEnemies().length;

	if (LEN > 0 && ALIVE_ENEMIES > 0)
	{
		var ally = ALLY_PLAYERS[random(LEN)];
		if (getRealPower() > 100 && (getRealPower() > Math.floor(1.5 * getRealPower(ally))))
		{
			donatePower(playerPower(me) / 2, ally);
		}
	}
}

//Does Cobra believe it is winning or could win?
function confidenceThreshold()
{
	if (gameTime < 1200000)
	{
		return true;
	}

	const DERR_COUNT = countStruct(structures.derricks);
	const DROID_COUNT = countDroid(DROID_ANY);
	var points = 0;

	points += DERR_COUNT >= Math.floor(countStruct(structures.derricks, getMostHarmfulPlayer()) / 2) ? 2 : -2;
	points += countDroid(DROID_ANY, getMostHarmfulPlayer()) < DROID_COUNT + 16 ? 2 : -2;

	if ((DROID_COUNT < 20 && (countDroid(DROID_ANY, getMostHarmfulPlayer()) > DROID_COUNT + 5)))
	{
		points -= 3;
	}

	if (points < 0 && random(100) < 25)
	{
		points = -points;
	}

	return points > -1;
}

//Check if our forces are large enough to take on the most harmful player.
//If it thinks it is losing it will go to a defensive research path.
function shouldCobraAttack()
{
	var confident = confidenceThreshold();
	if (confident || enemyUnitsInBase())
	{
		//Ok, restore the previous research path if necessary
		if (isDefined(prevResPath))
		{
			subPersonalities[personality].resPath = prevResPath;
			prevResPath = undefined;
		}

		return true;
	}
	else
	{
		if (subPersonalities[personality].resPath !== "defensive")
		{
			prevResPath = subPersonalities[personality].resPath;
			subPersonalities[personality].resPath = "defensive";
		}

		return false;
	}
}

//Controls how long localized group retreat happens. See also eventAttacked.
function retreatTactics()
{
	const SCAN_RADIUS = 7;
	var droids = enumGroup(retreatGroup);

	//Flee!
	for (var i = 0, len = droids.length; i < len; ++i)
	{
		var droid = droids[i];

		if (enumRange(droid.x, droid.y, SCAN_RADIUS, ENEMIES, true).length !== 0)
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
