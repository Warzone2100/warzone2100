
////////////////////////////////////////////////////////////////////////////////
// Enemy base management.
////////////////////////////////////////////////////////////////////////////////

//;; ## camSetEnemyBases(bases)
//;;
//;; Tell ```libcampaign.js``` to manage a certain set of enemy bases.
//;; Management assumes auto-cleanup of leftovers on destruction, and also
//;; counting how many bases have been successfully destroyed by the player.
//;; The argument is a JavaScript map from group labels to base descriptions.
//;; Each label points to a group of vital base structures. If no group label
//;; with this name is defined, a group is created automatically
//;; based on ```cleanup``` area and labeled. Base description
//;; is a JavaScript object with the following optional fields:
//;;
//;; * ```cleanup``` An area label to clean up features in once base is
//;; 	destroyed. If base id is not a group label, this field is required
//;; 	in order to auto-create the group of stuff in the area which doesn't
//;; 	qualify as a valid leftover.
//;; * ```detectMsg``` A ```PROX_MSG``` message id to play when the base is detected.
//;; * ```detectSnd``` A sound file to play when the base is detected.
//;; * ```eliminateSnd``` A sound file to play when the base is eliminated.
//;; 	The sound is played in the center of the cleanup area,
//;; 	which needs to be defined.
//;; * ```player``` If base is detected by cleanup area, only objects
//;; 	matching this player filter would be added to the base group or
//;; 	cleaned up. Note that this most likely disables feature cleanup.
//;; Additionally, this function would call special event callbacks if they are
//;; defined in your script, which should be named as follows,
//;; where LABEL is the label of the base group:
//;; * ```camEnemyBaseDetected_LABEL``` Called when the player sees an object from
//;; 	the enemy base group for the first time.
//;; * ```camEnemyBaseEliminated_LABEL``` Called when the base is eliminated,
//;; 	right after leftovers were cleaned up.
//;;
function camSetEnemyBases(bases)
{
	var reload = !camDef(bases);
	if (!reload)
	{
		__camEnemyBases = bases;
		__camNumEnemyBases = 0;
	}
	// convert label strings to groups and store
	for (var blabel in __camEnemyBases)
	{
		var bi = __camEnemyBases[blabel];
		var obj = getObject(blabel);
		if (camDef(obj) && obj) // group already defined
		{
			if (!camDef(bi.group))
			{
				bi.group = obj.id;
			}
			else
			{
				var structures = enumGroup(bi.group);
				addLabel({ type: GROUP, id: bi.group }, blabel);
				for (var idx = 0, len = structures.length; idx < len; ++idx)
				{
					var s = structures[idx];
					if (s.type !== STRUCTURE || __camIsValidLeftover(s))
					{
						continue;
					}
					if (!camDef(bi.player) || camPlayerMatchesFilter(s.player, bi.player))
					{
						camTrace("Auto-adding", s.id, "to base", blabel);
						groupAdd(bi.group, s);
					}
				}
			}
			if (!camDef(bi.cleanup)) // auto-detect cleanup area
			{
				var objs = enumGroup(bi.group);
				if (objs.length > 0)
				{
					const OFFSET = 2; // increases size of the auto-detected base area a bit
					var a = {
						type: AREA,
						x: mapWidth, y: mapHeight,
						x2: 0, y2: 0
					};
					// smallest rectangle to contain all objects
					for (var idx = 0, len = objs.length; idx < len; ++idx)
					{
						var o = objs[idx];
						if (o.x < a.x) a.x = o.x;
						if (o.y < a.y) a.y = o.y;
						if (o.x > a.x2) a.x2 = o.x;
						if (o.y > a.y2) a.y2 = o.y;
					}
					a.x -= OFFSET; a.y -= OFFSET; a.x2 += OFFSET; a.y2 += OFFSET;
					camTrace("Auto-detected cleanup area for", blabel, ":", a.x, a.y, a.x2, a.y2);
					bi.cleanup = "__cam_enemy_base_cleanup__" + blabel;
					addLabel(a, bi.cleanup);
				}
			}
		}
		else // define a group automatically
		{
			if (!camDef(bi.cleanup))
			{
				camDebug("Neither group nor cleanup area found for", blabel);
				continue;
			}
			bi.group = camNewGroup();
			addLabel({ type: GROUP, id: bi.group }, blabel);
			var structs = enumArea(bi.cleanup, ENEMIES, false);
			for (var idx = 0, len = structs.length; idx < len; ++idx)
			{
				var s = structs[idx];
				if (s.type !== STRUCTURE || __camIsValidLeftover(s))
				{
					continue;
				}
				if (!camDef(bi.player) || camPlayerMatchesFilter(s.player, bi.player))
				{
					camTrace("Auto-adding", s.id, "to base", blabel);
					groupAdd(bi.group, s);
				}
			}
		}
		if (groupSize(bi.group) === 0)
		{
			camDebug("Base", blabel, "defined as empty group");
		}
		if(!reload)
		{
			bi.detected = false;
			bi.eliminated = false;
		}
		camTrace("Resetting label", blabel);
		resetLabel(blabel, CAM_HUMAN_PLAYER); // subscribe for eventGroupSeen
	}
}

//;; ## camDetectEnemyBase(base label)
//;;
//;; Plays the "enemy base detected" message and places a beacon
//;; for the enemy base defined by the label, as if the base
//;; was actually found by the player.
//;;
function camDetectEnemyBase(blabel)
{
	var bi = __camEnemyBases[blabel];
	if (bi.detected || bi.eliminated)
	{
		return;
	}
	camTrace("Enemy base", blabel, "detected");
	bi.detected = true;
	if (camDef(bi.detectSnd))
	{
		var pos = camMakePos(bi.cleanup);
		if (!camDef(pos)) // auto-detect sound position by group object pos
		{
			var objs = enumGroup(bi.group);
			if (objs.length > 0)
			{
				var firstObject = objs[0];
				pos = camMakePos(firstObject);
			}
		}
		if (camDef(pos))
		{
			playSound(bi.detectSnd, pos.x, pos.y, 0);
		}
	}
	if (camDef(bi.detectMsg))
	{
		hackAddMessage(bi.detectMsg, PROX_MSG, CAM_HUMAN_PLAYER, false);
	}
	var callback = __camGlobalContext()["camEnemyBaseDetected_" + blabel];
	if (camDef(callback))
	{
		callback();
	}
}

//;; ## camAllEnemyBasesEliminated()
//;;
//;; Returns true if all enemy bases managed by ```libcampaign.js```
//;; are destroyed.
//;;
function camAllEnemyBasesEliminated()
{
	// FIXME: O(n) lookup here
	return __camNumEnemyBases === Object.keys(__camEnemyBases).length;
}

//////////// privates

function __camCheckBaseSeen(seen)
{
	var group = seen; // group?
	if (camDef(seen.group)) // object?
	{
		group = seen.group;
	}
	if (!camDef(group) || !group)
	{
		return;
	}
	// FIXME: O(n) lookup here
	for (var blabel in __camEnemyBases)
	{
		var bi = __camEnemyBases[blabel];
		if (bi.group !== group)
		{
			continue;
		}
		camDetectEnemyBase(blabel);
	}
}

function __camIsValidLeftover(obj)
{
	if (camPlayerMatchesFilter(obj.player, ENEMIES))
	{
		if (obj.type === STRUCTURE && obj.stattype === WALL)
		{
			return true;
		}
	}
	if (obj.type === FEATURE)
	{
		if (obj.stattype === BUILDING)
		{
			return true;
		}
	}
	return false;
}

function __camShouldDestroyLeftover(objInfo, basePlayer)
{
	var object = getObject(objInfo.type, objInfo.player, objInfo.id);
	if (object === null)
	{
		return false;
	}
	return (objInfo.type === STRUCTURE &&
		object.status === BUILT &&
		__camIsValidLeftover(object) &&
		(!camDef(basePlayer) ||
		(camDef(basePlayer) && camPlayerMatchesFilter(objInfo.player, basePlayer))));
}

function __camCheckBaseEliminated(group)
{
	// FIXME: O(n) lookup here
	for (var blabel in __camEnemyBases)
	{
		var bi = __camEnemyBases[blabel];
		var leftovers = [];
		if (bi.eliminated || (bi.group !== group))
		{
			continue;
		}
		if (enumGroup(bi.group).length !== 0)
		{
			return; //still has something in the base group
		}
		if (camDef(bi.cleanup))
		{
			var objects = enumArea(bi.cleanup, ENEMIES, false);
			for (var i = 0, len = objects.length; i < len; ++i)
			{
				var object = objects[i];
				var objInfo = {
					type: object.type,
					player: object.player,
					id: object.id
				};
				if (__camShouldDestroyLeftover(objInfo, bi.player))
				{
					leftovers.push(object);
				}
			}
			for (var i = 0, len = leftovers.length; i < len; i++)
			{
				// remove with special effect
				var leftover = leftovers[i];
				camSafeRemoveObject(leftover, true);
			}
			if (camDef(bi.eliminateSnd))
			{
				// play sound
				var pos = camMakePos(bi.cleanup);
				playSound(bi.eliminateSnd, pos.x, pos.y, 0);
			}
		}
		else
		{
			camDebug("All bases must have a cleanup area : " + blabel);
			continue;
		}
		if (camDef(bi.detectMsg) && bi.detected) // remove the beacon
		{
			hackRemoveMessage(bi.detectMsg, PROX_MSG, CAM_HUMAN_PLAYER);
		}
		camTrace("Enemy base", blabel, "eliminated");
		bi.eliminated = true;
		// bump counter before the callback, so that it was
		// actual during the callback
		++__camNumEnemyBases;
		var callback = __camGlobalContext()["camEnemyBaseEliminated_" + blabel];
		if (camDef(callback))
		{
			callback();
		}

		queue("__camShowVictoryConditions", camSecondsToMilliseconds(1));
	}
}

function __camBasesTick()
{
	for (var blabel in __camEnemyBases)
	{
		var bi = __camEnemyBases[blabel];
		if (bi.eliminated || !camDef(bi.reinforce_kind))
		{
			continue;
		}
		if (gameTime - bi.reinforce_last < bi.reinforce_interval)
		{
			continue;
		}
		if (!camDef(bi.player))
		{
			camDebug("Enemy base player needs to be set for", blabel);
			return;
		}
		if (!camDef(bi.cleanup))
		{
			camDebug("Enemy base cleanup area needs to be set for", blabel);
			return;
		}
		bi.reinforce_last = gameTime;
		var list = profile(bi.reinforce_callback);
		var pos = camMakePos(bi.cleanup);
		camSendReinforcement(bi.player, pos, list, bi.reinforce_kind, bi.reinforce_data);
	}
}
