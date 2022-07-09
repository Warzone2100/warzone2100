
////////////////////////////////////////////////////////////////////////////////
// VTOL management.
// These functions create the hit and run vtols for the given player.
// Vtol rearming is handled in group management.
////////////////////////////////////////////////////////////////////////////////

//;; ## camSetVtolData(player, startPos, exitPos, templates, timer, [obj[, extras]])
//;;
//;; Setup hit and runner VTOLs. NOTE: Will almost immediately spawn VTOLs upon calling this function.
//;; `Player`: What player number the VTOLs will belong to.
//;; `StartPos`: Starting position object where VTOLs will spawn. Can be an array. Use undefined for random map edge location.
//;; `ExitPos`: Exit position object where VTOLs will despawn at.
//;; `Templates`: An array of templates that the spawn uses.
//;; `Timer`: How much time in milliseconds the VTOLs will wait to spawn again.
//;; `Obj`: A game object that will stop the spawn when it no longer exists. May be undefined for no explicit end condition.
//;; `Extras`: An object with possible members:
//;;		`limit`: Numeric limit of a VTOL design in regards to the parameter Templates. May be an array paired to Templates.
//;;		`alternate`: A boolean to force the spawn to use one of the designs at a time in parameter Templates.
//;;		`altIdx`: Which design index the spawn will first cycle through the list of templates from.
//;;		`minVTOLs`: Minimum amount of VTOLs that will spawn.
//;;		`maxRandomVTOLs`: Random amount of VTOLs that will spawn in addition to minVTOLs.
//;;
//;; @param {number} player
//;; @param {Object|Object[]|undefined} startPos
//;; @param {Object} exitPos
//;; @param {Object[]} templates
//;; @param {number} timer
//;; @param {Object} obj
//;; @param {Object} extras
//;; @returns {void}
//;;
function camSetVtolData(player, startPos, exitPos, templates, timer, obj, extras)
{
	__camVtolDataSystem.push({
		player: player,
		startPosition: startPos,
		exitPosition: camMakePos(exitPos),
		templates: templates,
		spawnStopObject: obj,
		extras: extras,
		timer: timer,
		nextSpawnTime: timer + gameTime,
		isFirstSpawn: true,
		active: true
	});
}

//;; ## camSetVtolSpawnState(state, identifier)
//;;
//;; Sets the active status of a VTOL spawn point. The identifier can either be the
//;; the index number or the label of the object that stops the spawn naturally.
//;;
//;; @param {boolean} state
//;; @param {number|string} identifier
//;; @returns {void}
//;;
function camSetVtolSpawnState(state, identifier)
{
	if (typeof identifier === "number")
	{
		__camVtolDataSystem[identifier].active = state;
	}
	else if (typeof identifier === "string")
	{
		for (let idx = 0, len = __camVtolDataSystem.length; idx < len; ++idx)
		{
			if (__camVtolDataSystem[idx].spawnStopObject === identifier)
			{
				__camVtolDataSystem[idx].active = state;
				break;
			}
		}
	}
	else
	{
		camDebug("camSetVtolSpawn() expected a number or game object label string");
	}
}

//;; ## camSetVtolSpawnStateAll(state)
//;;
//;; Sets the active status of all VTOL spawns to `state`.
//;;
//;; @param {boolean} state
//;; @returns {void}
//;;
function camSetVtolSpawnStateAll(state)
{
	for (let idx = 0, len = __camVtolDataSystem.length; idx < len; ++idx)
	{
		camSetVtolSpawnState(state, idx);
	}
}

//////////// privates

function __checkVtolSpawnObject()
{
	for (let idx = 0, len = __camVtolDataSystem.length; idx < len; ++idx)
	{
		if (__camVtolDataSystem[idx].active && camDef(__camVtolDataSystem[idx].spawnStopObject))
		{
			if (getObject(__camVtolDataSystem[idx].spawnStopObject) === null)
			{
				camSetVtolSpawnState(false, idx); //Deactivate hit and runner VTOLs.
			}
		}
	}
}

function __camSpawnVtols()
{
	for (let idx = 0; idx < __camVtolDataSystem.length; ++idx)
	{
		if (!__camVtolDataSystem[idx].active)
		{
			continue;
		}
		if (gameTime < __camVtolDataSystem[idx].nextSpawnTime)
		{
			if (__camVtolDataSystem[idx].isFirstSpawn)
			{
				__camVtolDataSystem[idx].isFirstSpawn = false;
			}
			else
			{
				continue;
			}
		}
		else
		{
			__camVtolDataSystem[idx].nextSpawnTime = gameTime + __camVtolDataSystem[idx].timer;
		}

		// Default VTOL amounts
		let minVtolAmount = 5;
		let maxRandomAdditions = 2;

		if (camDef(__camVtolDataSystem[idx].extras))
		{
			if (camDef(__camVtolDataSystem[idx].extras.minVTOLs))
			{
				minVtolAmount = __camVtolDataSystem[idx].extras.minVTOLs;
			}
			if (camDef(__camVtolDataSystem[idx].extras.maxRandomVTOLs))
			{
				maxRandomAdditions = __camVtolDataSystem[idx].extras.maxRandomVTOLs;
			}
		}

		let amount = minVtolAmount + camRand(maxRandomAdditions + 1);
		let droids = [];
		let pos;

		//Make sure to catch multiple start positions also.
		if (__camVtolDataSystem[idx].startPosition instanceof Array)
		{
			pos = __camVtolDataSystem[idx].startPosition[camRand(__camVtolDataSystem[idx].startPosition.length)];
		}
		else if (camDef(__camVtolDataSystem[idx].startPosition) && __camVtolDataSystem[idx].startPosition)
		{
			pos = __camVtolDataSystem[idx].startPosition;
		}
		else
		{
			pos = camGenerateRandomMapEdgeCoordinate();
		}

		if (!camDef(__camVtolDataSystem[idx].extras))
		{
			//Pick some droids randomly.
			for (let i = 0; i < amount; ++i)
			{
				droids.push(__camVtolDataSystem[idx].templates[camRand(__camVtolDataSystem[idx].templates.length)]);
			}
		}
		else
		{
			var lim = amount;
			var alternate = false;
			if (camDef(__camVtolDataSystem[idx].extras.alternate))
			{
				alternate = __camVtolDataSystem[idx].extras.alternate; //Only use one template type
			}
			if (!camDef(__camVtolDataSystem[idx].extras.altIdx))
			{
				__camVtolDataSystem[idx].extras.altIdx = 0;
			}
			if (camDef(__camVtolDataSystem[idx].extras.limit))
			{
				//support an array of limits for each template
				if (__camVtolDataSystem[idx].extras.limit instanceof Array)
				{
					lim = __camVtolDataSystem[idx].extras.limit[__camVtolDataSystem[idx].extras.altIdx]; //max templates to use
				}
				else
				{
					lim = __camVtolDataSystem[idx].extras.limit;
				}
			}

			for (let i = 0; i < lim; ++i)
			{
				if (!alternate)
				{
					droids.push(__camVtolDataSystem[idx].templates[camRand(__camVtolDataSystem[idx].templates.length)]);
				}
				else
				{
					droids.push(__camVtolDataSystem[idx].templates[__camVtolDataSystem[idx].extras.altIdx]);
				}
			}

			if (__camVtolDataSystem[idx].extras.altIdx < (__camVtolDataSystem[idx].templates.length - 1))
			{
				++__camVtolDataSystem[idx].extras.altIdx;
			}
			else
			{
				__camVtolDataSystem[idx].extras.altIdx = 0;
			}
		}

		//...And send them.
		camSendReinforcement(__camVtolDataSystem[idx].player, camMakePos(pos), droids, CAM_REINFORCE_GROUND, {
			order: CAM_ORDER_ATTACK,
			data: { regroup: false, count: -1 }
		});
	}
}

function __camRetreatVtols()
{
	for (let idx = 0; idx < __camVtolDataSystem.length; ++idx)
	{
		if (camDef(__camVtolDataSystem[idx].exitPosition.x) &&
			camDef(__camVtolDataSystem[idx].exitPosition.y) &&
			enumStruct(__camVtolDataSystem[idx].player, REARM_PAD).length === 0)
		{
			const VTOL_RETURN_HEALTH = 40; // run-away if health is less than...
			const VTOL_RETURN_ARMED = 1; // run-away if weapon ammo is less than...
			let vtols = enumDroid(__camVtolDataSystem[idx].player).filter((obj) => (isVTOL(obj)));

			for (let i = 0, len = vtols.length; i < len; ++i)
			{
				let vt = vtols[i];
				for (let c = 0, len2 = vt.weapons.length; c < len2; ++c)
				{
					if ((vt.order === DORDER_RTB) || (vt.weapons[c].armed < VTOL_RETURN_ARMED) || (vt.health < VTOL_RETURN_HEALTH))
					{
						orderDroidLoc(vt, DORDER_MOVE, __camVtolDataSystem[idx].exitPosition.x, __camVtolDataSystem[idx].exitPosition.y);
						break;
					}
				}
			}
		}
	}
}
