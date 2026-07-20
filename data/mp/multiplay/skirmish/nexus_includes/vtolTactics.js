
function needsToRearm(droidID)
{
	if (!defined(droidID))
	{
		debugMessage("needsToRearm. Undefined droid id parameter.");
		return true;
	}

	const droid = getObject(DROID, me, droidID);

	if (droid === null)
	{
		return true; //Seems like a good fail-safe.
	}

	const ARMED_PERCENT = Math.floor(droid.weapons[0].armed);

	if (droid.order === DORDER_REARM)
	{
		if (ARMED_PERCENT < 100)
		{
			return true;
		}
	}
	else
	{
		if (ARMED_PERCENT === 0)
		{
			orderDroid(droid, DORDER_REARM);
			return true;
		}
	}

	return false;
}

function getCurrentVtolGroupTarget(groupIdx)
{
	let targ = targetInfo.vtolGroupWhat[groupIdx]; //Use current target if any.

	if (defined(targ))
	{
		let clearIt = false;
		const obj = getObject(targ.type, targ.player, targ.id);

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
	let newTargetWeight;
	let currentTarget = targetInfo.vtolGroupWhat[groupIdx];
	let newTarget = chooseVtolTarget(true);

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
		const CURRENT_TARGET_WEIGHT = getVtolTargetWeight(currentTarget);
		newTargetWeight = getVtolTargetWeight(newTarget);

		//Or a much better one.
		if (newTargetWeight >= (CURRENT_TARGET_WEIGHT + 20))
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
		const atkTarget = getObject(targetData.type, targetData.player, targetData.id);

		if (atkTarget === null)
		{
			return false;
		}

		const attackers = enumGroup(group);

		for (let i = 0, len = attackers.length; i < len; ++i)
		{
			const vtol = attackers[i];

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
	for (let i = 0; i < nexusBranch[branch].numVtolGroups; ++i)
	{
		const currGroup = groups.vtolAttackers[i];
		const AMT_ATTACKERS = enumGroup(currGroup).length;

		if (AMT_ATTACKERS === 0)
		{
			continue;
		}

		let currentTarget = getCurrentVtolGroupTarget(i);

		//attack rules
		if (AMT_ATTACKERS >= Math.floor(nexusBranch[branch].numVtolsPerGroup * 2 / 3))
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
	let droids = [];
	let droidCount = 0;
	let counter = 0;

	//Simply add all the VTOLs in a group into an arrray.
	for (let i = 0; i < NUM_GROUPS; ++i)
	{
		const group = enumGroup(groups.vtolAttackers[i]);

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

	for (let i = 0; i < NUM_GROUPS; ++i)
	{
		const GROUP_IDX = groups.vtolAttackers[i];

		while (droidCount - counter > 0 && enumGroup(GROUP_IDX).length < MAX_VTOLS_PER_GROUP)
		{
			groupAdd(GROUP_IDX, droids[counter]);
			counter += 1;
		}
	}
}

//Returns an object = {type: DROID/STRUCTURE, player: player, id: id}
function chooseVtolTarget(exclusive)
{
	let bestTarget;
	let bestScore = 0;
	let currScore;

	for (let i = 0; i < maxPlayers; ++i)
	{
		if (i === me || allianceExistsBetween(me, i))
		{
			continue;
		}

		for (let j = 0, len = nexusBranch[branch].vtolTargets.length; j < len; ++j)
		{
			const TTYPE = nexusBranch[branch].vtolTargets[j].structure;
			const structs = enumStruct(i, TTYPE);

			for (let c = 0, len2 = structs.length; c < len2; ++c)
			{
				const st = structs[c];
				const obj = {type: st.type, player: st.player, id: st.id};

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
	let targetWeight = 0;
	const penalty = 9; //NOTE: A bit much? Penality points per nearby AA structure.
	let target;

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

	for (let i = 0, len = nexusBranch[branch].vtolTargets.length; i < len; ++i)
	{
		const type = nexusBranch[branch].vtolTargets[i];

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

	let total = 0;
	const stuff = enumRange(x, y, range, ENEMIES, false);

	for (let i = 0, len = stuff.length; i < len; ++i)
	{
		const s = stuff[i];

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
	let target;

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

	for (let i = 0; i < nexusBranch[branch].numVtolGroups; ++i)
	{
		const obj = targetInfo.vtolGroupWhat[i];

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
	const stuff = enumRange(BASE.x, BASE.y, BASE_VTOL_DEFENSE_RANGE, ENEMIES, true);

	if (stuff.length > 0)
	{
		const target = stuff[0];
		const defenders = enumGroup(groups.vtolDefenders);
		const LEN = defenders.length;

		if (LEN < 2)
		{
			return;
		}

		for (let i = 0; i < LEN; ++i)
		{
			const vtol = defenders[i];

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
