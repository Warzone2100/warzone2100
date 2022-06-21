
////////////////////////////////////////////////////////////////////////////////
// AI droid movement automation.
////////////////////////////////////////////////////////////////////////////////
/**
 * ## camManageGroup(group, order, data)
 *
 * Tell `libcampaign.js` to manage a certain group. The group would
 * be permanently managed depending on the high-level orders given.
 * For each order, data parameter is a JavaScript object that controls
 * different aspects of behavior. The order parameter is one of:
 * * `CAM_ORDER_ATTACK` Pursue human player, preferably around the given position.
 *   The following optional data object fields are available, none of which is required:
 *   * `pos` Position or list of positions to attack. If pos is a list, first positions in the list will be attacked first.
 *   * `radius` Circle radius around `pos` to scan for targets.
 *   * `fallback` Position to retreat.
 *   * `morale` An integer from `1` to `100`. If that high percentage of the original group dies,
 *     fall back to the fallback position. If new droids are added to the group, it can recover and attack again.
 *   * `count` Override size of the original group. If unspecified, number of droids in the group at call time.
 *     Retreat on low morale and regroup is calculated against this value.
 *   * `repair` Health percentage to fall back to repair facility, if any.
 *   * `regroup` If set to `true`, the group will not move forward unless it has at least `count` droids in its biggest cluster.
 *     If `count` is set to `-1`, at least ⅔ of group's droids should be in the biggest cluster.
 * * `CAM_ORDER_DEFEND` Protect the given position. If too far, retreat back there ignoring fire.
 *   The following data object fields are available:
 *   * `pos` Position to defend.
 *   * `radius` Circle radius around `pos` to scan for targets.
 *   * `count` Override size of the original group. If unspecified, number of droids in the group at call time.
 *     Regroup is calculated against this value.
 *   * `repair` Health percentage to fall back to repair facility, if any.
 *   * `regroup` If set to `true`, the group will not move forward unless it has at least `count` droids in its biggest cluster.
 *     If `count` is set to `-1`, at least ⅔ of group's droids should be in the biggest cluster.
 * * `CAM_ORDER_PATROL` Move droids randomly between a given list of positions. The following data object fields are available:
 *   * `pos` An array of positions to patrol between.
 *   * `interval` Change positions every this many milliseconds.
 *   * `count` Override size of the original group. If unspecified, number of droids in the group at call time.
 *     Regroup is calculated against this value.
 *   * `repair` Health percentage to fall back to repair facility, if any.
 *   * `regroup` If set to `true`, the group will not move forward unless it has at least `count` droids in its biggest cluster.
 *     If `count` is set to `-1`, at least ⅔ of group's droids should be in the biggest cluster.
 * * `CAM_ORDER_COMPROMISE` Same as `CAM_ORDER_ATTACK`, just stay near the last (or only)
 *   attack position instead of looking for the player around the whole map. Useful for offworld missions,
 *   with player's LZ as the final position. The following data object fields are available:
 *   * `pos` Position or list of positions to compromise.
 *     If pos is a list, first positions in the list will be compromised first.
 *   * `radius` Circle radius around `pos` to scan for targets.
 *   * `count` Override size of the original group. If unspecified, number of droids in the group at call time.
 *     Regroup is calculated against this value.
 *   * `repair` Health percentage to fall back to repair facility, if any.
 *   * `regroup` If set to `true`, the group will not move forward unless it has at least `count` droids in its biggest cluster.
 *     If `count` is set to `-1`, at least ⅔ of group's droids should be in the biggest cluster.
 * * `CAM_ORDER_FOLLOW` Assign the group to commander. The sub-order is defined to be given to the commander.
 *   When commander dies, the group continues to execute the sub-order. The following data object fields are available:
 *   * `droid` Commander droid label.
 *   * `order` The order to give to the commander.
 *   * `data` Data of the commander's order.
 *   * `repair` Health percentage to fall back to repair facility, if any.
 * @param {string} group
 * @param {number} order
 * @param {Object} data
 * @returns {void}
 */
function camManageGroup(group, order, data)
{
	var saneData = data;
	if (!camDef(saneData))
	{
		saneData = {};
	}
	if (camDef(saneData.pos)) // sanitize pos now to make ticks faster
	{
		if (camIsString(saneData.pos)) // single label?
		{
			saneData.pos = [ saneData.pos ];
		}
		else if (!camDef(saneData.pos.length)) // single position object?
		{
			saneData.pos = [ saneData.pos ];
		}
		for (let i = 0, l = saneData.pos.length; i < l; ++i) // array of labels?
		{
			saneData.pos[i] = camMakePos(saneData.pos[i]);
		}
	}
	if (camDef(__camGroupInfo[group]) && order !== __camGroupInfo[group].order)
	{
		camTrace("Group", group, "receives a new order:", camOrderToString(order));
	}
	__camGroupInfo[group] = {
		target: undefined,
		order: order,
		data: saneData,
		count: camDef(saneData.count) ? saneData.count : groupSize(group)
	};
	if (order === CAM_ORDER_FOLLOW)
	{
		var commanderGroup = camMakeGroup([getObject(data.droid)]);
		camManageGroup(commanderGroup, data.order, data.data);
	}
	// apply orders instantly
	queue("__camTacticsTickForGroup", CAM_TICKS_PER_FRAME, group);
}

/**
 * ## camStopManagingGroup(group)
 *
 * Tell `libcampaign.js` to stop managing a certain group.
 * @param {string} group
 * @returns {void}
 */
function camStopManagingGroup(group)
{
	if (!camDef(__camGroupInfo[group]))
	{
		camTrace("Not managing", group, "anyway");
		return;
	}
	camTrace("Cease managing", group);
	delete __camGroupInfo[group];
}

/**
 * ## camOrderToString(order)
 *
 * Print campaign order as string, useful for debugging.
 * @param {number} order
 * @returns {string}
 */
function camOrderToString(order)
{
	var orderString;
	switch (order)
	{
		case CAM_ORDER_ATTACK:
			orderString = "ATTACK";
			break;
		case CAM_ORDER_DEFEND:
			orderString = "DEFEND";
			break;
		case CAM_ORDER_PATROL:
			orderString = "PATROL";
			break;
		case CAM_ORDER_COMPROMISE:
			orderString = "COMPROMISE";
			break;
		case CAM_ORDER_FOLLOW:
			orderString = "FOLLOW";
			break;
		default:
			orderString = "UNKNOWN";
	}
	return orderString;
}

//////////// privates
function __camFindGroupAvgCoordinate(groupID)
{
	var droids = enumGroup(groupID);
	var len = droids.length;
	var avgCoord = {x: 0, y: 0};

	if (droids.length === 0)
	{
		return null;
	}

	for (let i = 0; i < len; ++i)
	{
		var droid = droids[i];
		avgCoord.x += droid.x;
		avgCoord.y += droid.y;
	}

	// This global is constantly changing for the tactics code per group
	__camGroupAvgCoord.x = Math.floor(avgCoord.x / len);
	__camGroupAvgCoord.y = Math.floor(avgCoord.y / len);
}

function __camDistToGroupAverage(obj1, obj2)
{
	var dist1 = distBetweenTwoPoints(__camGroupAvgCoord.x, __camGroupAvgCoord.y, obj1.x, obj1.y);
	var dist2 = distBetweenTwoPoints(__camGroupAvgCoord.x, __camGroupAvgCoord.y, obj2.x, obj2.y);
	return (dist1 - dist2);
}

function __camPickTarget(group)
{
	var targets = [];
	var gi = __camGroupInfo[group];
	var droids = enumGroup(group);
	__camFindGroupAvgCoordinate(group);
	switch (gi.order)
	{
		case CAM_ORDER_ATTACK:
			if (camDef(gi.target))
			{
				targets = enumRange(gi.target.x, gi.target.y,__CAM_TARGET_TRACKING_RADIUS, CAM_HUMAN_PLAYER, false).filter((obj) => (
					obj.type === STRUCTURE || (obj.type === DROID && !isVTOL(obj))
				));
			}
			// fall-through! we just don't track targets on COMPROMISE
		case CAM_ORDER_COMPROMISE:
			if (camDef(gi.data.pos))
			{
				for (let i = 0; i < gi.data.pos.length; ++i)
				{
					var compromisePos = gi.data.pos[i];
					if (targets.length > 0)
					{
						break;
					}
					var radius = gi.data.radius;
					if (!camDef(radius))
					{
						radius = __CAM_PLAYER_BASE_RADIUS;
					}
					targets = enumRange(compromisePos.x, compromisePos.y, radius, CAM_HUMAN_PLAYER, false);
				}
			}
			if (gi.order === CAM_ORDER_COMPROMISE && targets.length === 0)
			{
				if (!camDef(gi.data.pos))
				{
					camDebug("'pos' is required for COMPROMISE order");
					return undefined;
				}
				else
				{
					targets = [ gi.data.pos[gi.data.pos.length - 1] ];
				}
			}
			var dr = droids[0];
			targets = targets.filter((obj) => (
				propulsionCanReach(dr.propulsion, dr.x, dr.y, obj.x, obj.y)
			));
			if (targets.length === 0)
			{
				targets = enumStruct(CAM_HUMAN_PLAYER).filter((obj) => (
					propulsionCanReach(dr.propulsion, dr.x, dr.y, obj.x, obj.y)
				));
				if (targets.length === 0)
				{
					targets = enumDroid(CAM_HUMAN_PLAYER).filter((obj) => (
						propulsionCanReach(dr.propulsion, dr.x, dr.y, obj.x, obj.y) &&
							(obj.type === STRUCTURE || (obj.type === DROID && !isVTOL(obj)))
					));
					if (targets.length === 0)
					{
						targets = enumDroid(CAM_HUMAN_PLAYER).filter((obj) => (
							propulsionCanReach(dr.propulsion, dr.x, dr.y, obj.x, obj.y)
						));
					}
				}
			}
			break;
		case CAM_ORDER_DEFEND:
			if (!camDef(gi.data.pos))
			{
				camDebug("'pos' is required for DEFEND order");
				return undefined;
			}
			var defendPos = gi.data.pos[0];
			var radius = gi.data.radius;
			if (!camDef(radius))
			{
				radius = __CAM_DEFENSE_RADIUS;
			}
			if (camDef(gi.target) && camDist(gi.target, defendPos) < radius)
			{
				targets = enumRange(gi.target.x, gi.target.y,
				                    __CAM_TARGET_TRACKING_RADIUS,
				                    CAM_HUMAN_PLAYER, false);
			}
			if (targets.length === 0)
			{
				targets = enumRange(defendPos.x, defendPos.y, radius, CAM_HUMAN_PLAYER, false);
			}
			if (targets.length === 0)
			{
				targets = [ defendPos ];
			}
			break;
		default:
			camDebug("Unsupported group order", gi.order, camOrderToString(gi.order));
			break;
	}
	if (targets.length === 0)
	{
		return undefined;
	}
	targets.sort(__camDistToGroupAverage);
	var target = targets[0];
	if (camDef(target) && camDef(target.type) && target.type === DROID && camIsTransporter(target))
	{
		return undefined;
	}
	__camGroupInfo[group].target = { x: target.x, y: target.y };
	return __camGroupInfo[group].target;
}

function __camTacticsTick()
{
	var dt = CAM_TICKS_PER_FRAME;
	for (const group in __camGroupInfo)
	{
		//Remove groups with no droids.
		if (groupSize(group) === 0)
		{
			var remove = true;
			var removable = __camGroupInfo[group].data.removable;
			//Useful if the group has manual management (seen in cam1-3 script).
			if (camDef(removable) && !removable)
			{
				remove = false;
			}
			if (remove)
			{
				camStopManagingGroup(group);
				break;
			}
		}
		queue("__camTacticsTickForGroup", dt, group);
		dt += CAM_TICKS_PER_FRAME;
	}
	//Emulate a queue...
	removeTimer("__camTacticsTick");
	setTimer("__camTacticsTick", dt);
}

//Return the range (in tiles) a droid will scout for stuff to attack around it.
function __camScanRange(order, drType)
{
	var rng = __CAM_TARGET_TRACKING_RADIUS; //default
	switch (order)
	{
		case CAM_ORDER_ATTACK:
		case CAM_ORDER_DEFEND:
		case CAM_ORDER_FOLLOW:
			rng = __CAM_TARGET_TRACKING_RADIUS;
			break;
		case CAM_ORDER_PATROL:
			rng = 5;
			break;
		case CAM_ORDER_COMPROMISE:
			rng = 2; //very small so they don't track stuff too far
			break;
		default:
			camDebug("Unsupported group order", order, camOrderToString(order));
	}

	if (drType === DROID_SENSOR)
	{
		rng = Math.floor(rng * 1.5);
	}

	return rng;
}

function __camTacticsTickForGroup(group)
{
	var gi = __camGroupInfo[group];
	if (!camDef(gi))
	{
		return;
	}
	var rawDroids = enumGroup(group);
	if (rawDroids.length === 0)
	{
		return;
	}

	const CLOSE_Z = 1;
	var healthyDroids = [];
	var repair = {
		hasFacility: enumStruct(rawDroids[0].player, REPAIR_FACILITY).length > 0,
		pos: camDef(gi.data.repairPos) ? gi.data.repairPos : undefined,
		percent: camDef(gi.data.repair) ? gi.data.repair : 66,
	};

	//repair
	if (repair.hasFacility || camDef(repair.pos))
	{
		for (let i = 0, len = rawDroids.length; i < len; ++i)
		{
			var droid = rawDroids[i];
			var repairLikeAction = false;

			if (droid.order === DORDER_RTR)
			{
				continue;
			}

			//has a repair facility so use it
			if (repair.hasFacility && camDef(gi.data.repair))
			{
				if (droid.health < repair.percent)
				{
					orderDroid(droid, DORDER_RTR);
					repairLikeAction = true;
				}
			}
			//Or they have auto-repair and run to some position for a while
			else if (!repair.hasFacility && repair.pos)
			{
				if (droid.health < repair.percent)
				{
					var loc = camMakePos(repair.pos);
					orderDroidLoc(droid, DORDER_MOVE, loc.x, loc.y);
					repairLikeAction = true;
				}
			}

			if (!repairLikeAction)
			{
				healthyDroids.push(droid);
			}
		}
	}
	else
	{
		healthyDroids = rawDroids;
	}

	if (camDef(gi.data.regroup) && gi.data.regroup && healthyDroids.length > 0)
	{
		var ret = __camFindClusters(healthyDroids, __CAM_CLUSTER_SIZE);
		var groupX = ret.xav[ret.maxIdx];
		var groupY = ret.yav[ret.maxIdx];
		var droids = ret.clusters[ret.maxIdx];

		for (let i = 0, len = ret.clusters.length; i < len; ++i)
		{
			if (i !== ret.maxIdx) // move other droids towards main cluster
			{
				for (let j = 0, len2 = ret.clusters[i].length; j < len2; ++j)
				{
					var droid = ret.clusters[i][j];
					if (droid.order !== DORDER_RTR)
					{
						orderDroidLoc(droid, DORDER_MOVE, groupX, groupY);
					}
				}
			}
		}

		var hitRecently = (gameTime - gi.lastHit < __CAM_FALLBACK_TIME_ON_REGROUP);
		// not enough droids grouped?
		if (gi.count < 0 ? (ret.maxCount < groupSize(group) * 0.66) : (ret.maxCount < gi.count))
		{
			for (let i = 0, len = droids.length; i < len; ++i)
			{
				var droid = droids[i];
				if (droid.order === DORDER_RTR)
				{
					continue;
				}

				if (hitRecently && enumStruct(droid.player, HQ).length > 0)
				{
					if (droid.order !== DORDER_RTB)
					{
						orderDroid(droid, DORDER_RTB);
					}
				}
				else if (droid.order !== DORDER_HOLD)
				{
					orderDroid(droid, DORDER_HOLD);
				}
			}
			return;
		}
	}

	//Target choosing
	var target;
	var patrolPos;

	switch (gi.order)
	{
		case CAM_ORDER_ATTACK:
		case CAM_ORDER_DEFEND:
		case CAM_ORDER_COMPROMISE:
			target = __camPickTarget(group);
			if (!camDef(target))
			{
				return;
			}
			break;
		case CAM_ORDER_PATROL:
		case CAM_ORDER_FOLLOW:
			//do nothing here
			break;
		default:
			camDebug("Unknown group order given: " + gi.order);
			return;
	}

	var defending = (gi.order === CAM_ORDER_DEFEND);
	var track = (gi.order === CAM_ORDER_COMPROMISE);

	for (let i = 0, len = healthyDroids.length; i < len; ++i)
	{
		var droid = healthyDroids[i];
		var vtolUnit = (droid.type === DROID && isVTOL(droid));

		if (droid.player === CAM_HUMAN_PLAYER)
		{
			camDebug("Controlling a human player's droid");
		}

		//Rearm vtols.
		if (vtolUnit)
		{
			var arm = droid.weapons[0].armed;
			var isRearming = droid.order === DORDER_REARM;

			if ((arm < 1) || (isRearming && (arm < 100 || droid.health < 100)))
			{
				var havePads = enumStruct(droid.player, REARM_PAD).length > 0;
				if (havePads && !isRearming)
				{
					orderDroid(droid, DORDER_REARM);
				}
				continue; //Rearming. Try not to attack stuff.
			}
		}

		if (gi.order === CAM_ORDER_FOLLOW)
		{
			var commander = getObject(gi.data.droid);
			if (commander === null)
			{
				// the commander is dead? let the group execute his last will.
				camManageGroup(group, gi.data.order, gi.data.data);
				return;
			}
			if (droid.droidType !== DROID_COMMAND && droid.order !== DORDER_COMMANDERSUPPORT)
			{
				orderDroidObj(droid, DORDER_COMMANDERSUPPORT, commander);
				continue;
			}
		}

		if (gi.order === CAM_ORDER_DEFEND)
		{
			// fall back to defense position
			var dPos = gi.data.pos[0];
			var dist = camDist(droid.x, droid.y, dPos.x, dPos.y);
			var radius = gi.data.radius;
			if (!camDef(radius))
			{
				radius = __CAM_DEFENSE_RADIUS;
			}
			if (dist > radius)
			{
				orderDroidLoc(droid, DORDER_MOVE, target.x, target.y);
				continue;
			}
		}

		if (gi.order === CAM_ORDER_PATROL)
		{
			if (!camDef(gi.data.interval))
			{
				gi.data.interval = camSecondsToMilliseconds(60);
			}
			if (!camDef(gi.lastmove) || !camDef(gi.lastspot))
			{
				gi.lastspot = 0;
				gi.lastmove = 0;
			}
			else
			{
				if (gameTime - gi.lastmove > gi.data.interval)
				{
					// find random new position to visit
					var list = [];
					for (let j = 0, len2 = gi.data.pos.length; j < len2; ++j)
					{
						if (j !== gi.lastspot)
						{
							list.push(j);
						}
					}
					gi.lastspot = list[camRand(list.length)];
					gi.lastmove = gameTime;
				}
			}
			patrolPos = gi.data.pos[gi.lastspot];
			//I will leave this here for clarity so that it don't look
			//like patrol picks a target.
			if (camDef(patrolPos))
			{
				target = camMakePos(patrolPos);
			}
		}

		if (camDef(target) && camDist(droid.x, droid.y, target.x, target.y) >= __CAM_CLOSE_RADIUS)
		{
			var closeByObj;
			var artilleryLike = (droid.isCB || droid.hasIndirect || droid.isSensor);
			var closeBy = enumRange(droid.x, droid.y, __camScanRange(gi.order, droid.droidType), CAM_HUMAN_PLAYER, track);

			if (closeBy.length > 0)
			{
				__camFindGroupAvgCoordinate(group);
				closeBy.sort(__camDistToGroupAverage);
				closeByObj = closeBy[0];
			}

			//We only care about explicit observe/attack if the object is close
			//on the z coordinate. We should not chase things up or down hills
			//that may be far away, at least path-wise.
			if (closeByObj && !vtolUnit && !artilleryLike)
			{
				if (Math.abs(droid.z - closeByObj.z) > CLOSE_Z)
				{
					closeByObj = undefined;
				}
			}

			if (closeByObj && ((closeByObj.type === DROID) && isVTOL(closeByObj) && (isVTOL(droid) || !droid.canHitAir)))
			{
				closeByObj = undefined;
			}

			if (!defending && closeByObj)
			{
				if (droid.droidType === DROID_SENSOR)
				{
					orderDroidObj(droid, DORDER_OBSERVE, closeByObj);
				}
				else
				{
					orderDroidObj(droid, DORDER_ATTACK, closeByObj);
				}
			}
			else
			{
				if (defending || !(artilleryLike || vtolUnit))
				{
					orderDroidLoc(droid, DORDER_MOVE, target.x, target.y);
				}
				else
				{
					orderDroidLoc(droid, DORDER_SCOUT, target.x, target.y);
				}
			}
		}
	}
}

function __camCheckGroupMorale(group)
{
	var gi = __camGroupInfo[group];
	if (!camDef(gi.data.morale))
	{
		return;
	}
	// morale is %.
	var msize = Math.floor((100 - gi.data.morale) * gi.count / 100);
	var gsize = groupSize(group);
	switch (gi.order)
	{
		case CAM_ORDER_ATTACK:
			if (gsize > msize)
			{
				break;
			}
			camTrace("Group", group, "falls back");
			gi.order = CAM_ORDER_DEFEND;
			// swap pos and fallback
			var temp = gi.data.pos;
			gi.data.pos = [ camMakePos(gi.data.fallback) ];
			gi.data.fallback = temp;
			// apply orders instantly
			queue("__camTacticsTickForGroup", CAM_TICKS_PER_FRAME, group);
			break;
		case CAM_ORDER_DEFEND:
			if (gsize <= msize)
			{
				break;
			}
			camTrace("Group", group, "restores");
			gi.order = CAM_ORDER_ATTACK;
			// swap pos and fallback
			var temp = gi.data.pos;
			gi.data.pos = gi.data.fallback;
			gi.data.fallback = temp[0];
			// apply orders instantly
			queue("__camTacticsTickForGroup", CAM_TICKS_PER_FRAME, group);
			break;
		default:
			camDebug("Group order doesn't support morale", camOrderToString(gi.order));
			break;
	}
}
