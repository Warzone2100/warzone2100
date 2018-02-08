
function eventDroidBuilt(droid, struct)
{
	if (isVTOL(droid))
	{
		groupAdd(vtolGroup, droid);
	}
	else if (droid.droidType === DROID_WEAPON || droid.droidType === DROID_CYBORG)
	{
		groupAdd(attackGroup, droid);
	}
	else if (droid.droidType === DROID_CONSTRUCT)
	{
		if (enumGroup(oilBuilders).length < 3)
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

function eventGameInit()
{
	attackGroup = newGroup();
	vtolGroup = newGroup();

	enumDroid(me).forEach(function(droid) {
		if (droid.droidType !== DROID_CONSTRUCT)
		{
			eventDroidBuilt(droid, null);
		}
	});

	baseBuilders = newGroup();
	oilBuilders = newGroup();
	var cons = enumDroid(me, DROID_CONSTRUCT);

	for (var i = 0, l = cons.length; i < l; ++i)
	{
		if (l < MIN_TRUCKS)
		{
			!countStruct(FACTORY) ? groupAdd(baseBuilders, cons[i]) : groupAdd(oilBuilders, cons[i]);
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
}

function eventAttacked(victim, attacker)
{
	const MIN_GROUND_UNITS = 6;
	const MIN_VTOL_UNITS = 4;

	// TBD, for now -- SEND EVERYONE!!!
	if (attacker && victim && attacker.player !== me && !allianceExistsBetween(attacker.player, me))
	{
		if (attacker.type === DROID && isVTOL(attacker))
		{
			return; //ignore vtols
		}

		var currEnemy = getCurrentEnemy();
		if (!defined(currEnemy))
		{
			return;
		}

		var loc = {x: attacker.x, y: attacker.y };
		//Set this player as the current enemy
		if (currEnemy !== attacker.player)
		{
			currentEnemy = attacker; //Focus on this player
		}

		//log("Defend!");
		var defenders = enumGroup(attackGroup);
		var defLen = defenders.length;
		if (defLen > MIN_GROUND_UNITS)
		{
			for (var i = 0; i < defLen; i++)
			{
				if (defenders[i].order != DORDER_RECYCLE)
				{
					orderDroidLoc(defenders[i], DORDER_SCOUT, loc.x, loc.y);
				}
			}
		}

		var vtols = enumGroup(vtolGroup);
		var vtolLen = vtols.length;
		if (vtolLen > MIN_VTOL_UNITS)
		{
			for (var j = 0; j < vtolLen; j++)
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
	recycleDroidsForHover();
	buildFundamentals();
	isSeaMap = isHoverMap();

	// Set the timer call randomly so as not to compute on the same tick if more than one semperfi is on map.
	setTimer("buildFundamentals", 1500 + ((1 + random(3)) * random(60))); // build stuff
	setTimer("lookForOil", 2000 + ((1 + random(4)) * random(30)));
	setTimer("findResearch", 2500 + ((1 + random(4)) * random(40)));
	setTimer("produce", 3000 + ((1 + random(4)) * random(70)));
	setTimer("recycleDroidsForHover", 10000 + ((1 + random(4)) * random(100)));
	setTimer("attackEnemy", 20000 + ((1 + random(4)) * random(100)));
}

function eventGroupLoss(droid, group, size)
{
	//log("lost " + droid.id + " in group " + group + " which is now size " + size);

	//Drop a beacon if the last one was more than thirty seconds ago.
	if (droid.player === me && gameTime > lastBeaconDrop + 30000)
	{
		lastBeaconDrop = gameTime;
		addBeacon(droid.x, droid.y, ALLIES);
	}
}

// Build defenses.
function eventStructureBuilt(structure, droid)
{
	const MIN_DEFENSES = 2;

	var defenses = enumRange(droid.x, droid.y, 10, me, false).filter(function (obj) {
		return (obj.type === STRUCTURE && obj.stattype === DEFENSE);
	});
	var chance = (structure.stattype === RESOURCE_EXTRACTOR || random(101) < 10);

	//Build a defense structure here.
	if (chance || (defenses.length < MIN_DEFENSES))
	{
		buildDefenses(droid); // Build right where this droid is at.
	}
}

//Investigate the area around the beacon
function eventBeacon(x, y, from, to, message)
{
	if (allianceExistsBetween(from, to) && to !== from)
	{
		//log(from + " sent a beacon. Location [" + x + ", " + y + "]");
		const MIN_ATTACKERS = 15;
		var enemyObject = enumRange(x, y, 8, ENEMIES, false)[0];
		if (!defined(enemyObject))
		{
			return; //Not close enough.
		}

		var attackers = enumGroup(attackGroup).filter(function(dr) {
			return droidCanReach(dr, x, y);
		});
		var attackLen = attackers.length;

		if (attackLen > MIN_ATTACKERS)
		{
			for (var i = 0; i < attackLen; i++)
			{
				orderDroidLoc(attackers[i], DORDER_SCOUT, x, y);
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
