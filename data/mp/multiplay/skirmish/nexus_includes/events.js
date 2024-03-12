
// Generally, initialize variables, personality information, and timers.
function eventStartLevel()
{
	var lenient = (difficulty <= MEDIUM);
	debugMode = true;
	numVtolUnits = 0;
	rebuildQueue = [];

	//Matches with alliances will fire off eventAllianceAccepted() before
	//we can get to eventStartLevel. So, set it in that event as a possiblity.
	allianceTimeInit();
	initPersonalityData();

	//See also the notes at each * function for how many ticks they span over.
	setTimer("buildBase", (lenient) ? 3600 : 900); //* 9
	setTimer("buildDerrick", (lenient) ? 4000 : 1000);
	setTimer("protectCloseDerrick", (lenient) ? 4800 : 1200);
	setTimer("doResearch", (lenient) ? 5600 : 1400);
	setTimer("productionMain", (lenient) ? 8000 : 1600); //* 3
	if (alliancesType === ALLIANCES)
	{
		setTimer("allianceMain", 2000); //* 2
	}
	setTimer("helpMain", 2400);
	setTimer("scoutMain", (lenient) ? 10800 : 2700); //* 2
	setTimer("tacticsMain", (lenient) ? 18000 : 3000); //* 2
	setTimer("vtolMain", (lenient) ? 10200 : 3400); //* 3
}

function eventAttacked(victim, attacker)
{
	if (throttleThis("evAttacked1", 200))
	{
		return;
	}

	if (victim === null || victim.player !== me || attacker === null || allianceExistsBetween(victim.player, attacker.player))
	{
		return;
	}

	//Try to repair a buddy
	if (victim.type === DROID)
	{
		//Supports repair bays now!
		if (countStruct(BASE_STRUCTURES.repairBays) > 0 && victim.health < 75 && victim.order !== DORDER_RTR)
		{
			orderDroid(victim, DORDER_RTR);
		}
		//We can snatch an attacker and make it focus on this threat first.
		if (victim.group === groups.attackers)
		{
			orderDroidObj(victim, DORDER_ATTACK, attacker);

			if (!throttleThis("evAttacked2", secondsToMilliseconds(2)) && !groupNearCoordinate(groups.repairs, {x: victim.x, y: victim.y}, 4))
			{
				orderGroupLoc(groups.repairs, victim.x, victim.y, DORDER_MOVE);
			}
		}
	}

	// See if our base is in trouble and we need help.
	if (!throttleThis("evAttacked4", secondsToMilliseconds(3)) && baseInTrouble())
	{
		if (!defendingOwnBase() && helpingAlly())
		{
			stopHelpingAlly();
		}

		helpPlayer(me, victim.x, victim.y);
		//Request help once in a while.
		requestHelp(BASE.x, BASE.y);
	}

	if (distBetweenTwoPoints(victim.x, victim.y, BASE.x, BASE.y) > BASE_DEFENSE_RANGE)
	{
		if (victim.group !== null && victim.group === groups.defenders && victim.order !== DORDER_RTR)
		{
			orderDroid(victim, DORDER_RTR);
		}

		return;
	}

	if (!throttleThis("evAttacked5", secondsToMilliseconds(1)) && attacker.type === DROID && isVTOL(attacker))
	{
		buildAANearLoc(victim.x, victim.y);
	}

	if (!throttleThis("evAttacked5", secondsToMilliseconds(4)))
	{
		orderGroupLoc(groups.defenders, attacker.x, attacker.y, DORDER_SCOUT);
	}
}

function eventDroidIdle(droid)
{
	var loc;

	if (droid.player !== me)
	{
		return;
	}

	if (droid.droidType === DROID_CONSTRUCT)
	{
		//HACK: bases+ hack to ensure we don't waste too much time around closeby derricks.
		if (gameTime > minutesToMilliseconds(1))
		{
			protectCloseDerrick();
		}
	}
	else if (droid.droidType === DROID_WEAPON || droid.droidType === DROID_CYBORG)
	{
		if (droid.order === DORDER_RTR)
		{
			return;
		}

		//Idle defenders move somewhere along the outside of the base.
		if (droid.group === groups.defenders && !throttleThis("evIdle_throttle" + droid.id, secondsToMilliseconds(8)))
		{
			loc = pickStructLocation(droid, BASE_STRUCTURES.gens, BASE.x, BASE.y, 1);

			if (loc)
			{
				orderDroidLoc(droid, DORDER_MOVE, loc.x, loc.y);
			}
		}
	}
}

function eventDroidBuilt(droid, structure)
{
	if (droid.player !== me)
	{
		return;
	}

	if (isVTOL(droid))
	{
		numVtolUnits += 1;

		if (enumGroup(groups.vtolDefenders).length < nexusBranch[branch].numVtolDefenders)
		{
			groupAdd(groups.vtolDefenders, droid);
		}
		else
		{
			for (let i = 0, len = groups.vtolAttackers.length; i < len; ++i)
			{
				if (enumGroup(groups.vtolAttackers[i]).length < nexusBranch[branch].numVtolsPerGroup)
				{
					groupAdd(groups.vtolAttackers[i], droid);
				}
			}
		}
	}
	else
	{
		if (droid.droidType === DROID_TRANSPORTER || droid.droidType === DROID_SUPERTRANSPORTER || droid.droidType === DROID_COMMAND)
		{
			debugMessage("eventDroidBuilt. Transport and commander groups not supported!");
			return;
		}
		else if (droid.droidType === DROID_CONSTRUCT)
		{
			var cyb = isCyborgStat(droid.body);

			if (!cyb && enumGroup(groups.oilBuilders).length < TRUCK_INFO.min)
			{
				groupAdd(groups.oilBuilders, droid);
				queue("buildDerrick", 100);
			}
			else if (cyb || enumGroup(groups.baseBuilders).length < TRUCK_INFO.min)
			{
				groupAdd(groups.baseBuilders, droid);
			}
			else if (random(100) < 80 && enumGroup(groups.baseBuilders).length < TRUCK_INFO.max)
			{
				groupAdd(groups.baseBuilders, droid);
			}
			else if (random(100) < 20 && enumGroup(groups.oilBuilders).length < TRUCK_INFO.max)
			{
				groupAdd(groups.oilBuilders, droid);
			}
			else
			{
				groupAdd(groups.baseBuilders, droid);
			}
		}
		else if (droid.droidType === DROID_REPAIR)
		{
			//Originally, these could be placed into any group; However, that is confusing and so they now get their own group.
			groupAdd(groups.repairs, droid);
		}
		else
		{
			var scoutLen = enumGroup(groups.scouts).length;
			var attackLen = enumGroup(groups.attackers).length;
			var defendLen = enumGroup(groups.defenders).length;

			if (attackLen < nexusBranch[branch].minimums.attackers)
			{
				groupAdd(groups.attackers, droid);
			}
			else if (defendLen < nexusBranch[branch].minimums.defenders)
			{
				groupAdd(groups.defenders, droid);
			}
			else if (scoutLen < nexusBranch[branch].minimums.scouts)
			{
				groupAdd(groups.scouts, droid);
			}
			// Now the maximums
			else if (scoutLen < nexusBranch[branch].maximums.scouts)
			{
				groupAdd(groups.scouts, droid);
			}
			else if (defendLen < nexusBranch[branch].maximums.defenders)
			{
				groupAdd(groups.defenders, droid);
			}
			else if (attackLen < nexusBranch[branch].maximums.attackers)
			{
				groupAdd(groups.attackers, droid);
			}
			else
			{
				groupAdd(groups.attackers, droid); //make them attack
			}
		}
	}
}

function eventDestroyed(what)
{
	if (what.player !== me)
	{
		return;
	}

	if (what.type === DROID)
	{
		//isVTOL() will fail cause the droid is dead.
		if (arrayContains(what.propulsion, VTOL_PROPULSIONS))
		{
			--numVtolUnits;
		}
	}
	else if (what.type === STRUCTURE)
	{
		// add certain structures to the rebuild list
		for (let i = 0, len = STANDARD_REBUILD_STRUCTURES.length; i < len; ++i)
		{
			var obj = STANDARD_REBUILD_STRUCTURES[i];

			//Some things like walls don't have a unique stattype.
			if (defined(obj.name))
			{
				if (what.name === obj.name)
				{
					rebuildQueue.push({x: what.x, y: what.y, stat: obj.stat});
					break;
				}
			}
			else if (what.stattype === obj.type)
			{
				rebuildQueue.push({x: what.x, y: what.y, stat: obj.stat});
				break;
			}
		}
	}
}

function eventStructureBuilt(structure, droid)
{
	if (structure.player !== me)
	{
		return;
	}

	if (droid && droid.player === me && structure.stattype === RESOURCE_EXTRACTOR)
	{
		const SCAN_RANGE = 10;
		var oils = enumRange(droid.x, droid.y, SCAN_RANGE, ALL_PLAYERS, false).filter((o) => (
			o.type === FEATURE && o.stattype === OIL_RESOURCE
		));

		if (oils.length > 0 && !skipOilGrabIfEasy())
		{
			orderDroidBuild(droid, DORDER_BUILD, BASE_STRUCTURES.derricks, oils[0].x, oils[0].y);
		}
		else
		{
			//HACK: bases+ hack to ensure we don't waste too much time around closeby derricks.
			if (gameTime > minutesToMilliseconds(1))
			{
				protectCloseDerrick();
			}
		}
	}
}

function eventObjectTransfer(object, from)
{
	if (object.player === me)
	{
		//Something given to me
		if (object.type === DROID)
		{
			eventDroidBuilt(object, null);
		}
	}
}

//The lassat is fully operational.
function eventStructureReady(structure)
{
	tryLassatStrike(structure.id);
}

function eventBeacon(x, y, from, to, message)
{
	if (from < 0 || from > maxPlayers)
	{
		return;
	}

	if (!allianceExistsBetween(me, from))
	{
		return;
	}

	helpInfo.location[from].x = x;
	helpInfo.location[from].y = y;
	helpInfo.lastHelpTime[from] = gameTime;
	helpInfo.lastHelpPlayer = from;

	processCommand(message, from, true);
}
