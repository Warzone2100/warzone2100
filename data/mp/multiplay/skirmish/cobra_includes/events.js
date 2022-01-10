//This file contains generic events. Chat and research events are split into
//their own seperate files.

function eventGameInit()
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
	var delay = me * 100;
	//Make Cobra think "slower" on easy difficulty for some of its basic functions
	var easyTimeDelay = (difficulty === EASY) ? 5000 : 0;

	setTimer("buildOrders", 300 + delay + (2 * easyTimeDelay));
	setTimer("produce", 400 + delay + easyTimeDelay);
	setTimer("retreatTactics", 500 + delay);
	setTimer("checkAllForRepair", 600 + delay + (4 * easyTimeDelay));
	setTimer("research", 800 + delay + (3 * easyTimeDelay));
	setTimer("lookForOil", 1000 + delay);
	setTimer("artilleryTactics", 1400 + delay);
	setTimer("vtolTactics", 1600 + delay);
	setTimer("groundTactics", 2000 + delay);
	setTimer("switchOffMG", 5000 + delay);
	setTimer("recycleForHover", 8000 + delay);
	setTimer("checkIfDead", 9000 + delay);
}

//This is meant to check for nearby oil resources next to the construct. also
//defend our derrick if possible.
function eventStructureBuilt(structure, droid)
{
	if (!droid || (droid.player !== me ) || (droid.group !== oilGrabberGroup))
	{
		return;
	}

	var nearbyOils = enumRange(droid.x, droid.y, 8, ALL_PLAYERS, false).filter(function(obj) {
		return (obj.type === FEATURE) && (obj.stattype === OIL_RESOURCE);
	}).sort(distanceToBase);

	if (nearbyOils.length > 0 && !skipOilGrabIfEasy())
	{
		orderDroidBuild(droid, DORDER_BUILD, structures.derrick, nearbyOils[0].x, nearbyOils[0].y);
		return;
	}
	else if (forceDerrickBuildDefense)
	{
		const MIN_DIST_FROM_BASE = 10;
		var dist = distBetweenTwoPoints(MY_BASE.x, MY_BASE.y, structure.x, structure.y);

		if (dist >= MIN_DIST_FROM_BASE && (getRealPower() > (-3 * SUPER_LOW_POWER)))
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
		var enemyObjects = enumRange(droid.x, droid.y, 6, ENEMIES, false);
		if (enemyObjects.length > 0)
		{
			enemyObjects = enemyObjects.sort(distanceToBase);
			attackThisObject(droid.id, objectInformation(enemyObjects[0]));
		}
	}
	else if (forceDerrickBuildDefense && droid.droidType === DROID_CONSTRUCT && droid.group === oilGrabberGroup)
	{
		const SCAN_RANGE = 7;
		var enemyDerrs = enumRange(droid.x, droid.y, SCAN_RANGE, ENEMIES, false).filter(function(obj) {
			return obj.type === STRUCTURE && obj.stattype === RESOURCE_EXTRACTOR;
		});

		//most likely an enemy truck got the oil before us, so try to build a defense near it.
		if (enemyDerrs.length > 0)
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
		var isEngineer = droid.body === "CyborgLightBody";

		if ((!isEngineer || cyborgOnlyGame) && baseType === CAMP_CLEAN && getMultiTechLevel() > 1 && enumGroup(oilGrabberGroup).length === 0)
		{
			groupAdd(oilGrabberGroup, droid); //Fix for crazy T2/T3/T4 no-bases config
		}
		else if ((!isEngineer || cyborgOnlyGame) && enumGroup(constructGroup).length >= 2 && enumGroup(oilGrabberGroup).length < 1)
		{
			groupAdd(oilGrabberGroup, droid); //Get oil faster
		}
		else if (enumGroup(constructGroup).length < MIN_TRUCKS_PER_GROUP)
		{
			groupAdd(constructGroup, droid);
		}
		else if ((!isEngineer || cyborgOnlyGame) && (enumGroup(oilGrabberGroup).length < MIN_TRUCKS_PER_GROUP))
		{
			groupAdd(oilGrabberGroup, droid);
		}
		else if (highOilMap() && (enumGroup(constructGroupNTWExtra).length < MIN_TRUCKS_PER_GROUP))
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

	const SCAV_ATTACKER = isDefined(scavengerPlayer) && (attacker.player === scavengerPlayer);
	const GROUP_SCAN_RADIUS = subPersonalities[personality].retreatScanRange;

	var nearbyUnits = enumRange(victim.x, victim.y, GROUP_SCAN_RADIUS, ALLIES, false).filter(function(obj) {
		return obj.type === DROID;
	});

	//Custom SemperFi-JS's localized regrouping code to be used to retreat away from highly outnumbered contests.
	if (victim.type === DROID && victim.player === me)
	{
		var nearbyScavs = 0;
		var nearbyEnemies = enumRange(victim.x, victim.y, SCAV_ATTACKER ? (GROUP_SCAN_RADIUS * 0.75) : GROUP_SCAN_RADIUS, ENEMIES, false);
		if (isVTOL(victim))
		{
			droidReady(victim.id);
		}
		else if (victim.order !== DORDER_RTR &&
			victim.order !== DORDER_RECYCLE &&
			!repairDroid(victim.id) &&
			nearbyUnits.length < nearbyEnemies.length &&
			distBetweenTwoPoints(MY_BASE.x, MY_BASE.y, victim.x, victim.y) >= 20)
		{
			var run = true;

			//Be more aggressive with scavenger stuff
			if (SCAV_ATTACKER)
			{
				nearbyEnemies.forEach(function(obj) {
					nearbyScavs += (obj.player === scavengerPlayer);
				});

				if (Math.floor(nearbyUnits.length * 2) > nearbyScavs)
				{
					run = false;
				}
			}

			if (run)
			{
				orderDroidLoc(victim, DORDER_MOVE, MY_BASE.x, MY_BASE.y); //Move now
				groupAdd(retreatGroup, victim);
			}
		}
	}

	if (SCAV_ATTACKER)
	{
		lastAttackedByScavs = gameTime;
		return;
	}

	if (!startAttacking && (gameTime > 420000) || !highOilMap())
	{
		startAttacking = true; //well, they want to play so...
	}

	if (attacker.player !== me && !allianceExistsBetween(attacker.player, victim.player))
	{
		grudgeCount[attacker.player] += (victim.type === STRUCTURE) ? 20 : 5;

		//Check if a droid needs repair.
		if ((victim.type === DROID) && !isVTOL(victim) && countStruct(structures.repair))
		{
			repairDroid(victim.id);
		}

		if (stopExecution("throttleEventAttacked1", 1000))
		{
			return;
		}

		var units = nearbyUnits.filter(function(dr) {
			return (dr.id !== victim.id &&
				dr.group !== retreatGroup &&
				!isConstruct(dr.id, false) &&
				((isVTOL(dr) && droidReady(dr.id)) ||
				(!repairDroid(dr.id)) && droidCanReach(dr, attacker.x, attacker.y))
			);
		});

		const UNIT_LEN = units.length;

		if (UNIT_LEN >= MIN_ATTACK_DROIDS && shouldCobraAttack())
		{
			for (var i = 0; i < UNIT_LEN; i++)
			{
				if ((subPersonalities[personality].resPath === "offensive") || (random(100) < 33))
				{
					var unit = units[i];
					if (unit !== null && distBetweenTwoPoints(unit.x, unit.y, attacker.x, attacker.y) < (GROUP_SCAN_RADIUS + 4))
					{
						orderDroidObj(unit, DORDER_ATTACK, attacker);
					}
					else if (unit !== null && isDefined(attacker.x) && isDefined(attacker.y))
					{
						orderDroidLoc(unit, DORDER_SCOUT, attacker.x, attacker.y);
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
	const RETRY_TIME = 10000;

	if (!structure)
	{
		const LASER = enumStruct(me, structures.lassat);
		if (LASER.length > 0)
		{
			structure = LASER[0];
		}
		else
		{
			queue("eventStructureReady", RETRY_TIME);
			return;
		}
	}

	var obj = returnClosestEnemyFactory();
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
		queue("eventStructureReady", RETRY_TIME, structure);
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
	beacon.wasVtol = isDefined(message) && (message === BEACON_VTOL_ALARM);
}
