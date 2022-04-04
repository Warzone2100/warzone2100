
////////////////////////////////////////////////////////////////////////////////
// VTOL management.
// These functions create the hit and run vtols for the given player.
// Vtol rearming is handled in group management.
////////////////////////////////////////////////////////////////////////////////

// Setup hit and runner VTOLs. NOTE: Will almost immediately spawn VTOLs upon calling this function.
// Player: What player number the VTOLs will belong to.
// StartPos: Starting position object where VTOLs will spawn. Can be an array. Use undefined for random map edge location.
// ExitPos: Exit position object where VTOLs will despawn at.
// Templates: An array of templates that the spawn uses.
// Timer: How much time in milliseconds the VTOLs will wait to spawn again.
// Obj: A game object that will stop the spawn when it no longer exists. May be undefined for no explicit end condition.
// Extras: An object with possible members:
//		limit: Numeric limit of a VTOL design in regards to the parameter Templates. May be an array paired to Templates.
//		alternate: A boolean to force the spawn to use one of the designs at a time in parameter Templates.
//		altIdx: Which design index the spawn will first cycle through the list of templates from.
//		minVTOLs: Minimum amount of VTOLs that will spawn.
//		maxRandomVTOLs: Random amount of VTOLs that will spawn in addition to minVTOLs.
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

// Sets the active status of a VTOL spawn point. The identifier can either be the
// the index number or the label of the object that stops the spawn naturally.
function camSetVtolSpawnState(state, identifier)
{
	if (typeof identifier === "number")
	{
		__camVtolDataSystem[identifier].active = state;
	}
	else if (typeof identifier === "string")
	{
		for (const camVtolDataSystem of __camVtolDataSystem)
		{
			if (camVtolDataSystem.spawnStopObject === identifier)
			{
				camVtolDataSystem.active = state;
				break;
			}
		}
	}
	else
	{
		camDebug("camSetVtolSpawn() expected a number or game object label string");
	}
}

function camSetVtolSpawnStateAll(state)
{
	for (const idx of __camVtolDataSystem.keys())
	{
		camSetVtolSpawnState(state, idx);
	}
}

//////////// privates

function __checkVtolSpawnObject()
{
	for (const idx of __camVtolDataSystem.keys())
	{
		const camVtolDataSystem = __camVtolDataSystem[idx];
		if (camVtolDataSystem.active && camDef(camVtolDataSystem.spawnStopObject))
		{
			if (getObject(camVtolDataSystem.spawnStopObject) === null)
			{
				camSetVtolSpawnState(false, idx); //Deactivate hit and runner VTOLs.
			}
		}
	}
}

function __camSpawnVtols()
{
	for (const camVtolDataSystem of __camVtolDataSystem)
	{
		if (!camVtolDataSystem.active)
		{
			continue;
		}
		if (gameTime < camVtolDataSystem.nextSpawnTime)
		{
			if (camVtolDataSystem.isFirstSpawn)
			{
				camVtolDataSystem.isFirstSpawn = false;
			}
			else
			{
				continue;
			}
		}
		else
		{
			camVtolDataSystem.nextSpawnTime = gameTime + camVtolDataSystem.timer;
		}

		// Default VTOL amounts
		let minVtolAmount = 5;
		let maxRandomAdditions = 2;

		if (camDef(camVtolDataSystem.extras))
		{
			if (camDef(camVtolDataSystem.extras.minVTOLs))
			{
				minVtolAmount = camVtolDataSystem.extras.minVTOLs;
			}
			if (camDef(camVtolDataSystem.extras.maxRandomVTOLs))
			{
				maxRandomAdditions = camVtolDataSystem.extras.maxRandomVTOLs;
			}
		}

		let amount = minVtolAmount + camRand(maxRandomAdditions + 1);
		let droids = [];
		let pos;

		//Make sure to catch multiple start positions also.
		if(camVtolDataSystem.startPosition instanceof Array)
		{
			pos = camVtolDataSystem.startPosition[camRand(camVtolDataSystem.startPosition.length)];
		}
		else if (camDef(camVtolDataSystem.startPosition) && camVtolDataSystem.startPosition)
		{
			pos = camVtolDataSystem.startPosition;
		}
		else
		{
			pos = camGenerateRandomMapEdgeCoordinate();
		}

		if (!camDef(camVtolDataSystem.extras))
		{
			//Pick some droids randomly.
			for (let i = 0; i < amount; ++i)
			{
				droids.push(camVtolDataSystem.templates[camRand(camVtolDataSystem.templates.length)]);
			}
		}
		else
		{
			var lim = amount;
			var alternate = false;
			if (camDef(camVtolDataSystem.extras.alternate))
			{
				alternate = camVtolDataSystem.extras.alternate; //Only use one template type
			}
			if (!camDef(camVtolDataSystem.extras.altIdx))
			{
				camVtolDataSystem.extras.altIdx = 0;
			}
			if (camDef(camVtolDataSystem.extras.limit))
			{
				//support an array of limits for each template
				if (camVtolDataSystem.extras.limit instanceof Array)
				{
					lim = camVtolDataSystem.extras.limit[camVtolDataSystem.extras.altIdx]; //max templates to use
				}
				else
				{
					lim = camVtolDataSystem.extras.limit;
				}
			}

			for (let i = 0; i < lim; ++i)
			{
				if (!alternate)
				{
					droids.push(camVtolDataSystem.templates[camRand(camVtolDataSystem.templates.length)]);
				}
				else
				{
					droids.push(camVtolDataSystem.templates[camVtolDataSystem.extras.altIdx]);
				}
			}

			if (camVtolDataSystem.extras.altIdx < (camVtolDataSystem.templates.length - 1))
			{
				++camVtolDataSystem.extras.altIdx;
			}
			else
			{
				camVtolDataSystem.extras.altIdx = 0;
			}
		}

		//...And send them.
		camSendReinforcement(camVtolDataSystem.player, camMakePos(pos), droids, CAM_REINFORCE_GROUND, {
			order: CAM_ORDER_ATTACK,
			data: { regroup: false, count: -1 }
		});
	}
}

function __camRetreatVtols()
{
	for (const camVtolDataSystem of __camVtolDataSystem)
	{
		if (camDef(camVtolDataSystem.exitPosition.x) &&
			camDef(camVtolDataSystem.exitPosition.y) &&
			enumStruct(camVtolDataSystem.player, REARM_PAD).length === 0)
		{
			const VTOL_RETURN_HEALTH = 40; // run-away if health is less than...
			const VTOL_RETURN_ARMED = 1; // run-away if weapon ammo is less than...
			let vtols = enumDroid(camVtolDataSystem.player).filter((obj) => (isVTOL(obj)));

			for (const vt of vtols)
			{
				for (const weapon of vt.weapons)
				{
					if ((vt.order === DORDER_RTB) || (weapon.armed < VTOL_RETURN_ARMED) || (vt.health < VTOL_RETURN_HEALTH))
					{
						orderDroidLoc(vt, DORDER_MOVE, camVtolDataSystem.exitPosition.x, camVtolDataSystem.exitPosition.y);
						break;
					}
				}
			}
		}
	}
}
