
function needsToRearm(droidID)
{
	if (!defined(droidID))
	{
		debugMessage("needsToRearm. Undefined droid id parameter.");
		return true;
	}

	var droid = getObject(DROID, me, droidID);

	if (droid === null)
	{
		return true; //Seems like a good fail-safe.
	}

	var armedPercent1 = Math.floor(droid.weapons[0].armed);

	if (droid.order === DORDER_REARM)
	{
		if (armedPercent1 < 100)
		{
			return true;
		}
	}
	else
	{
		if (armedPercent1 === 0)
		{
			orderDroid(droid, DORDER_REARM);
			return true;
		}
	}

	return false;
}

function getCurrentVtolGroupTarget(groupIdx)
{
	var targ = targetInfo.vtolGroupWhat[groupIdx]; //Use current target if any.

	if (defined(targ))
	{
		var clearIt = false;
		var obj = getObject(targ.type, targ.player, targ.id);

		if (obj === null)
		{
			clearIt = true;
		}
		else if (!throttleThis("vtolTargetReset_throttle" + groupIdx, secondsToMilliseconds(5)) &&
			obj.type === DROID &&
			obj.droidType !== DROID_CONSTRUCT &&
			!insideBase(obj.x, obj.y))
		{
			clearIt = true;
		}

		if (clearIt)
		{
			targ = undefined;
		}
	}

	return targ;
}

function tryChangingVtolGroupTarget(groupIdx)
{
	var newTargetWeight;
	var currentTarget = targetInfo.vtolGroupWhat[groupIdx];


	var newTarget = chooseVtolTarget(true);

	//reset the target once in a while.
	if (random(100) < 7)
	{
		currentTarget = undefined;
	}

	//find best attack target then
	if (!defined(newTarget))
	{
		newTarget = chooseVtolTarget(false);
	}

	//New one.
	if (!defined(currentTarget))
	{
		currentTarget = newTarget;
	}
	else
	{
		var currentTargetWeight = getVtolTargetWeight(currentTarget);
		newTargetWeight = getVtolTargetWeight(newTarget);

		//Or a much better one.
		if (newTargetWeight >= (currentTargetWeight + 20))
		{
			currentTarget = newTarget;
		}
	}

	return currentTarget;
}

function tryAttackingVtolTarget(group, targetData)
{
	if (!allianceExistsBetween(me, targetData.player))
	{
		var atkTarget = getObject(targetData.type, targetData.player, targetData.id);

		if (atkTarget === null)
		{
			return false;
		}

		var attackers = enumGroup(group);

		for (var i = 0, len = attackers.length; i < len; ++i)
		{
			var vtol = attackers[i];

			if (!needsToRearm(vtol.id))
			{
				orderDroidObj(vtol, DORDER_ATTACK, atkTarget);
			}
		}
	}
	else
	{
		return false;
	}

	return true;
}

function vtolAttack()
{
	for (var i = 0; i < nexusBranch[branch].numVtolGroups; ++i)
	{
		var currGroup = groups.vtolAttackers[i];
		var amtOfAttackers = enumGroup(currGroup).length;
		if (amtOfAttackers === 0)
		{
			continue;
		}

		var currentTarget = getCurrentVtolGroupTarget(i);

		//attack rules
		if (amtOfAttackers >= Math.floor(nexusBranch[branch].numVtolsPerGroup * 2 / 3))
		{
			currentTarget = tryChangingVtolGroupTarget(i);
		}

		//see if this group has something to attack
		if (defined(currentTarget))
		{
			if (!tryAttackingVtolTarget(currGroup, currentTarget))
			{
				currentTarget = undefined;
			}
		}

		//Set whatever the target is for later, if any.
		targetInfo.vtolGroupWhat[i] = currentTarget;
	}
}

//Pull VTOLs into nice evenly distributed groups.
function rearrangeAttackVtols()
{
	const MAX_VTOLS_PER_GROUP = nexusBranch[branch].numVtolsPerGroup;
	const NUM_GROUPS = nexusBranch[branch].numVtolGroups;
	var droids = [];
	var droidCount = 0;
	var counter = 0;

	//Simply add all the VTOLs in a group into an arrray.
	for (var i = 0; i < NUM_GROUPS; ++i)
	{
		var group = enumGroup(groups.vtolAttackers[i]);

		if (enumGroup(group).length > 0)
		{
			droids = droids.concat(group);
		}
	}

	droidCount = droids.length;

	if (droidCount === 0)
	{
		return;
	}

	for (var i = 0; i < NUM_GROUPS; ++i)
	{
		var groupIdx = groups.vtolAttackers[i];

		while (droidCount - counter > 0 && enumGroup(groupIdx).length < MAX_VTOLS_PER_GROUP)
		{
			groupAdd(groupIdx, droids[counter]);
			counter += 1;
		}
	}
}

//Returns an object = {type: DROID/STRUCTURE, player: player, id: id}
function chooseVtolTarget(exclusive)
{
	var bestTarget;
	var bestScore = 0;
	var currScore;

	for (var i = 0; i < maxPlayers; ++i)
	{
		if (i === me || allianceExistsBetween(me, i))
		{
			continue;
		}

		for (var j = 0, len = nexusBranch[branch].vtolTargets.length; j < len; ++j)
		{
			var ttype = nexusBranch[branch].vtolTargets[j].structure;
			var structs = enumStruct(i, ttype);

			for (var c = 0, len2 = structs.length; c < len2; ++c)
			{
				var st = structs[c];
				var obj = {type: st.type, player: st.player, id: st.id};

				//in case we don't want all groups to attack the same target
				if (!(exclusive && vtolTargetAssigned(obj)))
				{
					currScore = getVtolTargetWeight(obj);

					if (currScore > bestScore)
					{
						bestScore = currScore;
						bestTarget = st;
					}
				}
			}
		}
	}

	return (bestTarget ? {type: bestTarget.type, player: bestTarget.player, id: bestTarget.id} : undefined);
}

//Expects an object = {type: DROID/STRUCTURE, player: player, id: id}
function getVtolTargetWeight(what)
{
	var targetWeight = 0;
	var penalty = 9; //NOTE: A bit much? Penality points per nearby AA structure.
	var target;

	if (!defined(what))
	{
		return 0;
	}

	if (!defined(what.type) || !defined(what.player) || !defined(what.id))
	{
		debugMessage("getVtolTargetWeight. Insufficient parameter properties.");
		return 0;
	}

	if (what.type !== STRUCTURE)
	{
		return 0;
	}

	target = getObject(what.type, what.player, what.id);

	if (target === null)
	{
		return 0;
	}

	for (var i = 0, len = nexusBranch[branch].vtolTargets.length; i < len; ++i)
	{
		var type = nexusBranch[branch].vtolTargets[i];

		if (target.stattype === type.structure)
		{
			targetWeight = (type.weight - (numEnemyAAInRange(target.x, target.y, SCAN_RANGE_FOR_AA) * penalty));
			break;
		}
	}

	if (target.status !== BUILT)
	{
		targetWeight = Math.floor(targetWeight / 10);
	}

	return targetWeight;
}

function numEnemyAAInRange(x, y, range)
{
	if (!defined(x) || !defined(y))
	{
		debugMessage("numEnemyAAInRange. Undefined x or y coordinate.");
		return 0;
	}

	if (!defined(range))
	{
		range = 7;
	}

	var total = 0;
	var stuff = enumRange(x, y, range, ENEMIES, false);

	for (var i = 0, len = stuff.length; i < len; ++i)
	{
		var s = stuff[i];

		if (s.type === STRUCTURE && s.status === BUILT && s.canHitAir)
		{
			++total;
		}
	}

	return total;
}

//see if a particular target is already assigned to one of the VTOL attack groups.
//Expects an object = {type: DROID/STRUCTURE, player: player, id: id}
function vtolTargetAssigned(what)
{
	var target;

	if (!defined(what))
	{
		debugMessage("vtolTargetAssigned. Undefined target.");
		return false;
	}

	target = getObject(what.type, what.player, what.id);

	if (target === null)
	{
		return false;
	}

	for (var i = 0; i < nexusBranch[branch].numVtolGroups; ++i)
	{
		var obj = targetInfo.vtolGroupWhat[i];

		if (defined(obj) && obj.type === what.type && obj.player === what.player && obj.id === what.id)
		{
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////
//Defense

// watch for incoming vtols
function vtolDefend()
{
	var stuff = enumRange(BASE.x, BASE.y, BASE_VTOL_DEFENSE_RANGE, ENEMIES, true);

	if (stuff.length > 0)
	{
		var target = stuff[0];
		var defenders = enumGroup(groups.vtolDefenders);
		var len = defenders.length;

		if (len < 2)
		{
			return;
		}

		for (var i = 0; i < len; ++i)
		{
			var vtol = defenders[i];

			if (!needsToRearm(vtol.id))
			{
				orderDroidLoc(vtol, DORDER_SCOUT, target.x, target.y);
			}
		}
	}
}

//NOTE: Function are presently spread out over 3 ticks.
function vtolMain()
{
	queue("rearrangeAttackVtols", TICK);
	queue("vtolDefend", TICK * 2);
	queue("vtolAttack", TICK * 3);
}
