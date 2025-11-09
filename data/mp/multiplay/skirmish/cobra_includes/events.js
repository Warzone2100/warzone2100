//This file contains generic events. Chat and research events are split into
//their own seperate files.

function eventGameLoaded()
{
	initCobraGroups();
}

//Setup timers mostly
function eventStartLevel()
{
	initCobraVars();
	recycleForHover();
	buildOrders(); //Start building right away.

	//That (me * 100) part is to help reduce multiple Cobra AI's from coinciding stuff on the same frame. Of course,
	//higher "me"s may impact the bot in some, hopefully, minimal manner.
	const __delay = (me * 100);
	//Make Cobra think "slower" on easy difficulty for some of its basic functions
	const __easyTimeDelay = (difficulty === EASY) ? 5000 : 0;

	setTimer("buildOrders", 300 + __delay + (2 * __easyTimeDelay));
	setTimer("produce", 400 + __delay + __easyTimeDelay);
	setTimer("retreatTactics", 500 + __delay);
	setTimer("checkAllForRepair", 600 + __delay + (4 * __easyTimeDelay));
	setTimer("cobraDoResearch", 800 + __delay + (3 * __easyTimeDelay));
	setTimer("lookForOil", 1000 + __delay);
	setTimer("artilleryTactics", 1400 + __delay);
	setTimer("vtolTactics", 1600 + __delay);
	setTimer("groundTactics", 2000 + __delay);
	setTimer("switchOffMG", 5000 + __delay);
	setTimer("recycleForHover", 8000 + __delay);
	setTimer("checkIfDead", 9000 + __delay);
}

//This is meant to check for nearby oil resources next to the construct. also
//defend our derrick if possible.
function eventStructureBuilt(structure, droid)
{
	if (!droid || (droid.player !== me) || (droid.group !== oilGrabberGroup))
	{
		return;
	}

	const _nearbyOils = enumRange(droid.x, droid.y, 8, ALL_PLAYERS, false).filter((obj) => (
		obj.type === FEATURE && obj.stattype === OIL_RESOURCE
	)).sort(distanceToBase);

	if (_nearbyOils.length && !skipOilGrabIfEasy())
	{
		orderDroidBuild(droid, DORDER_BUILD, _STRUCTURES.derrick, _nearbyOils[0].x, _nearbyOils[0].y);
		return;
	}
	else if (forceDerrickBuildDefense)
	{
		const __minDistFromBase = 10;
		const __dist = distBetweenTwoPoints(_MY_BASE.x, _MY_BASE.y, structure.x, structure.y);

		if (__dist >= __minDistFromBase && (getRealPower() > (-3 * __SUPER_LOW_POWER)))
		{
			fastDefendSpot(structure, droid);
			return;
		}
	}
}

//Make droids attack hidden close by enemy object.
function eventDroidIdle(droid)
{
	if (shouldCobraAttack() && (droid.droidType === DROID_WEAPON || droid.droidType === DROID_CYBORG || isVTOL(droid)))
	{
		const _enemyObjects = enumRange(droid.x, droid.y, 6, ENEMIES, false).sort(distanceToBase);

		if (_enemyObjects.length)
		{
			attackThisObject(droid.id, objectInformation(_enemyObjects[0]));
		}
	}
	else if (forceDerrickBuildDefense && droid.droidType === DROID_CONSTRUCT && droid.group === oilGrabberGroup)
	{
		const __scanRange = 7;
		const __enemyDerrsLen = enumRange(droid.x, droid.y, __scanRange, ENEMIES, false).filter((obj) => (
			(obj.type === DROID && obj.droidType === DROID_CONSTRUCT) || (obj.type === STRUCTURE && obj.stattype === RESOURCE_EXTRACTOR)
		)).length;

		//most likely an enemy truck got the oil before us, so try to build a defense near it.
		if (__enemyDerrsLen)
		{
			fastDefendSpot(undefined, droid);
		}
	}
}

//Groups droid types.
function eventDroidBuilt(droid, struct)
{
	if (isConstruct(droid.id))
	{
		const __isEngineer = (droid.body === "CyborgLightBody");

		if ((!__isEngineer || cyborgOnlyGame) && (enumGroup(constructGroup).length >= 2) && !enumGroup(oilGrabberGroup).length)
		{
			groupAdd(oilGrabberGroup, droid); //Get oil faster
		}
		else if (enumGroup(constructGroup).length < __MIN_TRUCKS_PER_GROUP)
		{
			groupAdd(constructGroup, droid);
		}
		else if ((!__isEngineer || cyborgOnlyGame) && (enumGroup(oilGrabberGroup).length < __MIN_TRUCKS_PER_GROUP))
		{
			groupAdd(oilGrabberGroup, droid);
		}
		else if (highOilMap() && (enumGroup(constructGroupNTWExtra).length < __MIN_TRUCKS_PER_GROUP))
		{
			groupAdd(constructGroupNTWExtra, droid);
		}
		else
		{
			groupAdd(constructGroup, droid);
		}
	}
	else if (droid.droidType === DROID_SENSOR)
	{
		groupAdd(sensorGroup, droid);
	}
	else if (isVTOL(droid))
	{
		groupAdd(vtolGroup, droid);
	}
	else if (droid.droidType === DROID_WEAPON || droid.droidType === DROID_CYBORG)
	{
		//Anything with splash damage or CB abiliities go here.
		if (droid.isCB || droid.hasIndirect)
		{
			groupAdd(artilleryGroup, droid);
		}
		else
		{
			groupAdd(attackGroup, droid);
		}
	}
}

function eventAttacked(victim, attacker)
{
	if ((attacker === null) || (victim === null) || (victim.player !== me) || allianceExistsBetween(attacker.player, victim.player))
	{
		return;
	}

	const __scavAttacker = isDefined(scavengerPlayer) && (attacker.player === scavengerPlayer);
	const __groupScanRadius = subPersonalities[personality].retreatScanRange;

	const _nearbyUnits = enumRange(victim.x, victim.y, __groupScanRadius, ALLIES, false).filter((obj) => (
		obj.type === DROID
	));

	//Custom SemperFi-JS's localized regrouping code to be used to retreat away from highly outnumbered contests.
	if (victim.type === DROID && victim.player === me)
	{
		let nearbyScavs = 0;
		const _nearbyEnemies = enumRange(victim.x, victim.y, __scavAttacker ? (__groupScanRadius * 0.75) : __groupScanRadius, ENEMIES, false);

		if (isVTOL(victim))
		{
			droidReady(victim.id);
		}
		else if (victim.order !== DORDER_RTR &&
			victim.order !== DORDER_RECYCLE &&
			!repairDroid(victim.id) &&
			_nearbyUnits.length < _nearbyEnemies.length &&
			distBetweenTwoPoints(_MY_BASE.x, _MY_BASE.y, victim.x, victim.y) >= 20)
		{
			let run = true;

			//Be more aggressive with scavenger stuff
			if (__scavAttacker)
			{
				_nearbyEnemies.forEach((obj) => {
					nearbyScavs += (obj.player === scavengerPlayer);
				});

				if (Math.floor(_nearbyUnits.length * 2) > nearbyScavs)
				{
					run = false;
				}
			}

			if (run)
			{
				orderDroidLoc(victim, DORDER_MOVE, _MY_BASE.x, _MY_BASE.y); //Move now
				groupAdd(retreatGroup, victim);
			}
		}
	}

	if (__scavAttacker)
	{
		lastAttackedByScavs = gameTime;
		return;
	}

	if (!startAttacking && ((gameTime > 420000) || !highOilMap()))
	{
		startAttacking = true; //well, they want to play so...
	}

	if (attacker.player !== me && !allianceExistsBetween(attacker.player, victim.player))
	{
		grudgeCount[attacker.player] += (victim.type === STRUCTURE) ? 20 : 5;

		//Check if a droid needs repair.
		if ((victim.type === DROID) && !isVTOL(victim) && countStruct(_STRUCTURES.repair, me))
		{
			repairDroid(victim.id);
		}

		if (stopExecution("throttleEventAttacked1", 1000))
		{
			return;
		}

		const _units = _nearbyUnits.filter((dr) => (
			dr.id !== victim.id &&
			dr.group !== retreatGroup &&
			!isConstruct(dr.id, false) &&
			((isVTOL(dr) && droidReady(dr.id)) ||
			(!repairDroid(dr.id)) && droidCanReach(dr, attacker.x, attacker.y))
		));

		const __unitLen = _units.length;

		if (__unitLen >= __MIN_ATTACK_DROIDS && shouldCobraAttack())
		{
			for (let i = 0; i < __unitLen; ++i)
			{
				if ((subPersonalities[personality].resPath === "offensive") || chance(33))
				{
					const _unit = _units[i];

					if (_unit !== null && distBetweenTwoPoints(_unit.x, _unit.y, attacker.x, attacker.y) < (__groupScanRadius + 4))
					{
						orderDroidObj(_unit, DORDER_ATTACK, attacker);
					}
					else if (_unit !== null && isDefined(attacker.x) && isDefined(attacker.y))
					{
						orderDroidLoc(_unit, DORDER_SCOUT, attacker.x, attacker.y);
					}
				}
			}
		}
	}
}

function eventObjectTransfer(obj, from)
{
	if (from !== me)
	{
		if (allianceExistsBetween(from, me) || ((from === obj.player) && !allianceExistsBetween(obj.player, me)))
		{
			if (obj.type === DROID)
			{
				eventDroidBuilt(obj, null);
			}
		}
	}
	else if (!allianceExistsBetween(obj.player, me))
	{
		enemyUsedElectronicWarfare = true;
	}
}

//Basic Laser Satellite support.
function eventStructureReady(structure)
{
	const __retryTime = 10000;

	if (!structure)
	{
		const __laser = enumStruct(me, _STRUCTURES.lassat);

		if (__laser.length)
		{
			structure = __laser[0];
		}
		else
		{
			queue("eventStructureReady", __retryTime);
			return;
		}
	}

	let obj = returnClosestEnemyFactory();
	//Find something that exists, if possible.
	if (!isDefined(obj))
	{
		obj = rangeStep();
	}

	if (obj)
	{
		activateStructure(structure, getObject(obj.typeInfo, obj.playerInfo, obj.idInfo));
	}
	else
	{
		queue("eventStructureReady", __retryTime, structure);
	}
}

function eventBeacon(x, y, from, to, message)
{
	if (beacon.disabled)
	{
		return;
	}

	if (!allianceExistsBetween(from, me))
	{
		return;
	}

	if (from !== me)
	{
		startAttacking = true; // might as well attack now
	}

	beacon.x = x;
	beacon.y = y;
	beacon.startTime = gameTime;
	beacon.endTime = gameTime + 50000;
	beacon.wasVtol = isDefined(message) && (message === __BEACON_VTOL_ALARM);
}
