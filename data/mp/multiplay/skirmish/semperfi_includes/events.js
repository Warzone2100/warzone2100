
function eventDroidBuilt(droid, struct)
{
	if (defined(droid) && (droid.player == me))
	{
		if (isVTOL(droid))
		{
			groupAdd(vtolGroup, droid);
		}
		else if (droid.droidType == DROID_WEAPON || droid.droidType == DROID_CYBORG)
		{
			groupAdd(attackGroup, droid);
		}
		else if (droid.droidType == DROID_CONSTRUCT)
		{
			checkLocalJobs();
		}
	}
}

function eventGameInit()
{
	attackGroup = newGroup();
	vtolGroup = newGroup();
}

function eventAttacked(victim, attacker)
{
	const MIN_GROUND_UNITS = 8;
	const MIN_VTOL_UNITS = 4;

	// TBD, for now -- SEND EVERYONE!!!
	if (attacker && victim && (attacker.player != me) && !allianceExistsBetween(attacker.player, me))
	{
		var loc = {x: attacker.x, y: attacker.y };
		//Set this player as the current enemy (as long as they are not scavengers).
		if((getCurrentEnemy() != attacker.player) && (attacker.player < maxPlayers))
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
				if(defenders[i].order != DORDER_RECYCLE)
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
				if (vtolReady(vtols[j]))
				{
					orderDroidLoc(vtols[j], DORDER_SCOUT, loc.x, loc.y);
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
	setTimer("buildFundamentals", 1200 + ((1 + random(3)) * random(60))); // build stuff
	setTimer("eventResearched", 11000 + ((1 + random(4)) * random(40)));
	setTimer("produce", 14000 + ((1 + random(4)) * random(70)));
	setTimer("attackEnemy", 20000 + ((1 + random(4)) * random(100)));
	setTimer("recycleDroidsForHover", 30000 + ((1 + random(4)) * random(100))); // will remove self
}

function eventGroupLoss(droid, group, size)
{
	//log("lost " + droid.id + " in group " + group + " which is now size " + size);

	//Drop a beacon if the last one was more than thirty seconds ago.
	if ((droid.player == me) && gameTime > (lastBeaconDrop + 30000))
	{
		lastBeaconDrop = gameTime;
		addBeacon(droid.x, droid.y, ALLIES);
	}
}

// Build defenses.
function eventStructureBuilt(structure, droid)
{
	if (defined(droid) && (droid.player == me))
	{
		const MIN_POWER = -230;
		const MIN_DEFENSES = 2;

		var checkArea = enumRange(droid.x, droid.y, 12, ENEMIES, false);
		var defenses = enumRange(droid.x, droid.y, 10, me, false).filter(function (obj) {
			return (obj.type == STRUCTURE && obj.stattype == DEFENSE);
		});
		var chance = ((structure.stattype == RESOURCE_EXTRACTOR) || (random(101) < 10)) ? true : false;

		//Build a defense structure here.
		if (chance && checkArea.length || (defenses.length < MIN_DEFENSES) && (getRealPower() > MIN_POWER))
		{
			buildDefenses(droid); // Build right where this droid is at.
		}
		else
		{
			// Or defend the base with anti-air if needed.
			buildAntiAir();
		}
	}
}

//Investigate the area around the beacon
function eventBeacon(x, y, from, to, message)
{
	if(allianceExistsBetween(from, to) && (to != from))
	{
		//log(from + " sent a beacon. Location [" + x + ", " + y + "]");
		const MIN_ATTACKERS = 8;
		const MIN_VTOLS = 4;
		var attackers = enumGroup(attackGroup);
		var vtols = enumGroup(vtolGroup);
		var attackLen = attackers.length;
		var vtolLen = vtols.length;

		if (attackLen > MIN_ATTACKERS)
		{
			for (var i = 0; i < attackLen; i++)
			{
				if(droidCanReach(attackers[i], x, y))
				{
					orderDroidLoc(attackers[i], DORDER_SCOUT, x, y);
				}
			}
		}

		if (vtolLen > MIN_VTOLS)
		{
			for (var j = 0; j < vtolLen; j++)
			{
				if (vtols[j].armed == 100)
				{
					orderDroidLoc(vtols[j], DORDER_PATROL, x, y);
				}
				else
				{
					orderDroid(vtols[j], DORDER_REARM);
				}
			}
		}
	}
}
