
////////////////////////////////////////////////////////////////////////////////
// Reinforcement related stuff.
////////////////////////////////////////////////////////////////////////////////

//;; ## camSendReinforcement(playerId, position, templates, kind[, data])
//;;
//;; Give a single bunch of droids (template list) for a player at a position label. Kind can be one of:
//;; * `CAM_REINFORCE_NONE` Reinforcements are skipped this call.
//;; * `CAM_REINFORCE_GROUND` Reinforcements magically appear on the ground.
//;; * `CAM_REINFORCE_TRANSPORT` Reinforcements are unloaded from a transporter.
//;;   **NOTE:** the game engine doesn't seem to support two simultaneous incoming transporters for the same player.
//;;   Avoid this at all costs!
//;;   The following data fields are required:
//;;   * `entry` Transporter entry position.
//;;   * `exit` Transporter exit position.
//;;   * `message` `PROX_MSG` to display when transport is landing.
//;;   * `order` Order to give to newly landed droids
//;;   * `data` Order data.
//;;   **NOTE:** the game engine doesn't seem to support two simultaneous incoming transporters for the same player.
//;;   If a transporter is already on map, it will be correctly queued up and sent later.
//;;
//;; @param {number} playerId
//;; @param {string|Object|undefined} position
//;; @param {Object[]} templates
//;; @param {number} kind
//;; @param {Object} [data]
//;; @returns {void}
//;;
function camSendReinforcement(playerId, position, templates, kind, data)
{
	const pos = camMakePos(position);
	let order = CAM_ORDER_ATTACK;
	let order_data = { regroup: false, count: -1 };
	if (camDef(data) && camDef(data.order))
	{
		order = data.order;
		if (camDef(data.data))
		{
			order_data = data.data;
		}
	}
	if (playerId !== CAM_HUMAN_PLAYER)
	{
		camCleanTileOfObstructions(pos);
	}
	switch (kind)
	{
		case CAM_REINFORCE_NONE:
		{
			break; // Do nothing.
		}
		case CAM_REINFORCE_GROUND:
		{
			const droids = [];
			for (let i = 0, l = templates.length; i < l; ++i)
			{
				const template = templates[i];
				const __PROP = __camChangePropulsion(template.prop, playerId);
				const droid = addDroid(playerId, pos.x, pos.y, "Reinforcement", template.body, __PROP, "", "", template.weap);
				camSetDroidExperience(droid);
				droids.push(droid);
			}
			camManageGroup(camMakeGroup(droids), order, order_data);
			break;
		}
		case CAM_REINFORCE_TRANSPORT:
		{
			__camTransporterQueue.push({
				player: playerId,
				position: position,
				list: templates,
				data: data,
				order: order,
				order_data: order_data
			});
			__camDispatchTransporterSafe();
			break;
		}
		default:
		{
			camTrace("Unknown reinforcement type");
			break;
		}
	}
}

//;; ## camSendGenericSpawn(kind, player, condition, position, unitList, minimumUnits, maxRandoms)
//;;
//;; A simple means to send a group of units, either by spawn or transporter, on a simple attack order.
//;; Creating VTOLs this way isn't advised as they won't automatically retreat to a removal zone. However,
//;; for maps with VTOL Rearming Pads present, it could be ok as long as the mission script is careful.
//;; `kind`: Works similar to `camSendReinforcement()`.
//;; `player`: The player that will own these units.
//;; `condition`: Works similar to `camReinforcementConditionPasses()`.
//;; `position`: An object that contains coordinate data, such as {x: x, y: y}, to tell where units will appear.
//;;   May be a label or even an array of labels or objects containing coordinates, and if so, will be randomly selected.
//;; `unitList`: An array of units that will randomly be chosen from, or, a single template.
//;; May also be an object like so: {units: array_of_units|single_template, appended: array_of_units|single_template}.
//;; `appended` will always additionally be added to the randomly selected units.
//;; `minimumUnits`: The absolute minimum units this spawn will create.
//;; `maxRandoms`: How many units will randomly be added to the minimumUnits.
//;;
//;; @param {number} type
//;; @param {number} player
//;; @param {number} condition
//;; @param {object} position
//;; @param {array} unitList
//;; @param {number} minimumUnits
//;; @param {number} maxRandoms
//;; @returns {boolean}
//;;
function camSendGenericSpawn(kind, player, condition, position, unitList, minimumUnits, maxRandoms)
{
	if (camDef(condition) && !camReinforcementConditionPasses(condition, player))
	{
		return false;
	}
	let maxSpawns = minimumUnits;
	if (camDef(maxRandoms) && ((maxRandoms + 1) > 1))
	{
		 maxSpawns += camRand(maxRandoms + 1);
	}
	let realPosition;
	if (position instanceof Array)
	{
		realPosition = position[camRand(position.length)];
	}
	else
	{
		realPosition = position;
	}
	let droids = [];
	const __UNITS_ARE_OBJ = (unitList instanceof Object); // Note: Arrays are "Objects" too.
	const __UNITS_ARE_ARRAY = (unitList instanceof Array); // So check if actually Array.
	const realUnitList = (!__UNITS_ARE_ARRAY && __UNITS_ARE_OBJ && camDef(unitList.units)) ? unitList.units : unitList;
	for (let i = 0; i < maxSpawns; ++i)
	{
		if (__UNITS_ARE_ARRAY || (!__UNITS_ARE_ARRAY && __UNITS_ARE_OBJ && (realUnitList instanceof Array)))
		{
			droids.push(realUnitList[camRand(realUnitList.length)]);
		}
		else
		{
			droids.push(realUnitList);
		}
	}
	if (!__UNITS_ARE_ARRAY && __UNITS_ARE_OBJ && camDef(unitList.appended))
	{
		if (unitList.appended instanceof Array)
		{
			droids = droids.concat(unitList.appended);
		}
		else
		{
			droids.push(unitList.appended);
		}
	}
	camSendReinforcement(player, camMakePos(realPosition), droids, kind, {
		data: {regroup: false, repair: 66, count: -1},
		entry: camGenerateRandomMapEdgeCoordinate(), // These three below only get used for transporters.
		exit: camGenerateRandomMapEdgeCoordinate(),
		ignoreBlipRemoval: true
	});
	return true;
}

//;; ## camSetBaseReinforcements(baseLabel, interval, callbackName, kind, data)
//;;
//;; Periodically brings reinforcements to an enemy base, until the base is eliminated.
//;; Interval is the pause, in milliseconds, between reinforcements.
//;; Callback is name of a function that returns a list of droid templates to spawn,
//;; which may be different every time. Kind and data work similarly to `camSendReinforcement()`.
//;; Use `CAM_REINFORCE_NONE` as kind to disable previously set reinforcements.
//;;
//;; @param {string} baseLabel
//;; @param {number} interval
//;; @param {string} callbackName
//;; @param {number} kind
//;; @param {Object} data
//;; @returns {void}
//;;
function camSetBaseReinforcements(baseLabel, interval, callbackName, kind, data)
{
	if (!camIsString(callbackName))
	{
		camDebug("Callback name must be a string (received", callbackName, ")");
	}
	const bi = __camEnemyBases[baseLabel];
	bi.reinforce_kind = kind;
	bi.reinforce_interval = interval;
	bi.reinforce_callback = callbackName;
	bi.reinforce_data = data;
}

//;; ## camReinforcementConditionPasses(condition, player)
//;;
//;; Determines if a generic reinforcement condition will allow a spawn to proceed.
//;; `condition`: Basic conditions to not spawn anything, can be one of:
//;;   * `CAM_REINFORCE_CONDITION_NONE` or `undefined`: Do not use a condition, will always attempt a spawn.
//;;   * `CAM_REINFORCE_CONDITION_BASES`: Do not spawn if all bases are eliminated.
//;;   * `CAM_REINFORCE_CONDITION_UNITS`: Do not spawn if `player` has zero units.
//;;   * An `Object` {condition: `CAM_REINFORCE_CONDITION_OBJECT`, object: `label_of_object`} or `label`: spawns only if object is alive.
//;;   * `CAM_REINFORCE_CONDITION_ARTIFACTS`: Do not spawn if the player has picked up all artifacts.
//;; `player`: The player to check conditions for. Doesn't need to be specified except for the units condition.
//;;
//;; @param {Number|Object} condition
//;; @param {Number} player
//;; @returns {Boolean}
//;;
function camReinforcementConditionPasses(condition, player)
{
	if (!camDef(condition))
	{
		return true; // undefined is treated as CAM_REINFORCE_CONDITION_NONE.
	}
	if (!camDef(player))
	{
		player = CAM_HUMAN_PLAYER;
	}
	let conditionObject = condition; // Can be a String directly for example.
	let realCondition = condition;
	let shouldSpawn = false;
	if (camIsString(condition))
	{
		realCondition = CAM_REINFORCE_CONDITION_OBJECT;
	}
	if (condition instanceof Object)
	{
		realCondition = condition.condition;
		conditionObject = condition.object;
	}
	switch (realCondition)
	{
		case CAM_REINFORCE_CONDITION_NONE: shouldSpawn = true; break;
		case CAM_REINFORCE_CONDITION_BASES: shouldSpawn = !camAllEnemyBasesEliminated(); break;
		case CAM_REINFORCE_CONDITION_UNITS: shouldSpawn = (countDroid(DROID_ANY, player) > 0); break;
		case CAM_REINFORCE_CONDITION_OBJECT: shouldSpawn = (getObject(conditionObject) !== null); break;
		case CAM_REINFORCE_CONDITION_ARTIFACTS: shouldSpawn = !camAllArtifactsPickedUp(); break;
		default: camDebug("Unknown generic spawn condition: " + condition);
	}
	return shouldSpawn;
}
