function eventDroidBuilt(droid, struct)
{
	if (isVTOL(droid))
	{
		groupAdd(vtolGroup, droid);
	}
	else if (droid.droidType === DROID_WEAPON || droid.droidType === DROID_CYBORG)
	{
		if (droid.weapons[0].name === "Rocket-BB")
		{
			groupAdd(busterGroup, droid);
		}
		else
		{
			groupAdd(attackGroup, droid);
		}
	}
	else if (droid.droidType === DROID_CONSTRUCT)
	{
		if (enumGroup(oilBuilders).length < MIN_OIL_TRUCKS)
		{
			groupAdd(oilBuilders, droid);
		}
		else
		{
			groupAdd(baseBuilders, droid);
		}
		checkLocalJobs();
	}
}

function eventAttacked(victim, attacker)
{
	const MIN_GROUND_UNITS = 5;
	const MIN_VTOL_UNITS = 5;
	const GROUP_SCAN_RADIUS = 7;

	// TBD, for now -- SEND EVERYONE!!!
	if (attacker && victim && attacker.player !== me && !allianceExistsBetween(attacker.player, me))
	{
		//Flee if we are outnumbered to an extent
		if (victim.type === DROID && victim.player === me)
		{
			var seenEnemyGroupSize = enumRange(victim.x, victim.y, GROUP_SCAN_RADIUS, ENEMIES, false).length;
			if (isVTOL(victim))
			{
				vtolReady(victim.id); //check if it needs repair or rearming
			}
			else if (victim.order !== DORDER_RTR &&
				victim.order !== DORDER_RECYCLE &&
				!droidNeedsRepair(victim.id) &&
				enumRange(victim.x, victim.y, GROUP_SCAN_RADIUS, me, false).length < seenEnemyGroupSize)
			{
				orderDroidLoc(victim, DORDER_MOVE, BASE.x, BASE.y);
			}
		}

		var enemyNumber = getCurrentEnemy();
		if (!defined(enemyNumber))
		{
			return;
		}

		//Set this player as the current enemy
		if (enemyNumber !== attacker.player)
		{
			setPlayerAsTarget(attacker.player);
		}

		if (attacker.type === DROID && isVTOL(attacker))
		{
			enemyHasVtol = true; //Definitely has VTOLs.
			return; //Ignore VTOLs
		}
		if (ThrottleThis("eventAttacked_Throttle_1", 1500))
		{
			return;
		}

		//log("Defend!");
		var loc = {x: attacker.x, y: attacker.y };
		var defenders = enumGroup(attackGroup);
		var defLen = defenders.length;
		if (defLen > MIN_GROUND_UNITS)
		{
			for (var i = 0; i < defLen; ++i)
			{
				var dr = defenders[i];
				if (dr.order !== DORDER_RECYCLE && !droidNeedsRepair(dr.id) && dr.id !== victim.id)
				{
					orderDroidLoc(dr, DORDER_SCOUT, loc.x, loc.y);
				}
			}
		}

		var vtols = enumGroup(vtolGroup);
		var vtolLen = vtols.length;
		if (vtolLen > MIN_VTOL_UNITS)
		{
			for (var j = 0; j < vtolLen; ++j)
			{
				var vt = vtols[j];
				if (vtolReady(vt.id))
				{
					orderDroidLoc(vt, DORDER_SCOUT, loc.x, loc.y);
				}
			}
		}
	}
}

function eventStartLevel()
{
	//log("== level started ==");

	//setup groups
	attackGroup = newGroup();
	busterGroup = newGroup();
	vtolGroup = newGroup();
	baseBuilders = newGroup();
	oilBuilders = newGroup();
	truckRoleSwapped = false;
	enumDroid(me).forEach(function(droid) {
		if (droid.droidType !== DROID_CONSTRUCT)
		{
			eventDroidBuilt(droid, null);
		}
	});

	var cons = enumDroid(me, DROID_CONSTRUCT);
	for (var i = 0, l = cons.length; i < l; ++i)
	{
		if (l < MIN_BASE_TRUCKS)
		{
			if (countStruct(FACTORY_STAT) === 0)
			{
				groupAdd(baseBuilders, cons[i]);
			}
			else
			{
				groupAdd(oilBuilders, cons[i]);
			}
		}
		else
		{
			if (i < Math.floor(l / 2))
			{
				groupAdd(baseBuilders, cons[i]);
			}
			else
			{
				groupAdd(oilBuilders, cons[i]);
			}
		}
	}

	recycleDroidsForHover();
	buildFundamentals();
	isSeaMap = isHoverMap();
	researchDone = false;
	enemyHasVtol = false;

	// Set the timer call randomly so as not to compute on the same tick if more than one semperfi is on map.
	setTimer("produceAndResearch", 400 + ((1 + random(4)) * random(70)));
	setTimer("buildFundamentals", 900 + ((1 + random(3)) * random(60))); // build stuff
	setTimer("lookForOil", 1200 + ((1 + random(4)) * random(30)));
	setTimer("recycleDroidsForHover", 2000 + ((1 + random(4)) * random(100)));
	setTimer("attackEnemy", 6000 + ((1 + random(4)) * random(100)));
	setTimer("scanForVTOLs", 10000 + ((1 + random(5)) * random(60)));
}

// Build defenses.
function eventStructureBuilt(structure, droid)
{
	//don't go crazy defending stuff we just built relavtively close to the base.
	var dist = distBetweenTwoPoints(BASE.x, BASE.y, structure.x, structure.y);
	if (!droid || dist <= AVG_BASE_RADIUS)
	{
		return;
	}

	scanAndDefendPosition(structure, droid);
}

function eventDroidIdle(droid)
{
	if (droid.droidType === DROID_CONSTRUCT)
	{
		const ENEMY_DERRICK_SCAN_RANGE = 4;
		var enemyDerrs = enumRange(droid.x, droid.y, ENEMY_DERRICK_SCAN_RANGE, ENEMIES, false).filter(isDerrick);

		//most likely an enemy truck got the oil before us, so try to build a defense near it.
		if (enemyDerrs.length > 0)
		{
			scanAndDefendPosition(undefined, droid);
		}
	}
}

//Target enemy player closest to whose objects are closest to the beacon.
function eventBeacon(x, y, from, to, message)
{
	if (allianceExistsBetween(from, to) && to !== from)
	{
		//log(from + " sent a beacon. Location [" + x + ", " + y + "]");
		const BEACON_SCAN_RADIUS = 4;
		var enemyObjects = enumRange(x, y, BEACON_SCAN_RADIUS, ENEMIES, false);
		if (enemyObjects.length > 0)
		{
			for (var i = 0, l = enemyObjects.length; i < l; ++i)
			{
				var obj = enemyObjects[i];
				if (obj)
				{
					setPlayerAsTarget(obj.player);
					break;
				}
			}
		}
	}
}

function eventObjectTransfer(obj, from)
{
	if (obj.player === me)
	{
		if (obj.type === DROID)
		{
			eventDroidBuilt(obj, null); //put it in a group
		}
	}
}
