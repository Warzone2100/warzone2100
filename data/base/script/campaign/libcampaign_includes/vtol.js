
////////////////////////////////////////////////////////////////////////////////
// VTOL management.
// These functions create the hit and run vtols for the given player.
// Vtol rearming is handled in group management.
////////////////////////////////////////////////////////////////////////////////

//Setup hit and runner VTOLs. Passing in obj is optional and will automatically
//disable the vtol spawner once that object is dead.
function camSetVtolData(player, startPos, exitPos, templates, timer, obj, extras)
{
	__camVtolPlayer = player;
	__camVtolStartPosition = startPos;
	__camVtolExitPosition = camMakePos(exitPos);
	__camVtolTemplates = templates;
	__camVtolSpawnStopObject = obj;
	__camVtolExtras = extras;

	camSetVtolSpawn(true);
	setTimer("__camRetreatVtols", camSecondsToMilliseconds(0.8));
	setTimer("__checkVtolSpawnObject", camSecondsToMilliseconds(5));
	setTimer("__camSpawnVtols", timer);
}

function camSetVtolSpawn(value)
{
	__camVtolSpawnActive = value;
}

//////////// privates

function __checkVtolSpawnObject()
{
	if (camDef(__camVtolSpawnStopObject))
	{
		if (getObject(__camVtolSpawnStopObject) === null)
		{
			camSetVtolSpawn(false); //Deactive hit and runner VTOLs.
			removeTimer("__checkVtolSpawnObject");
		}
	}
}

function __camSpawnVtols()
{
	if (__camVtolSpawnActive === false)
	{
		removeTimer("__camSpawnVtols");
		return;
	}

	const MIN_VTOL_AMOUNT = 5;
	const MAX_RANDOM_VTOL_ADDITIONS = 2;

	var amount = MIN_VTOL_AMOUNT + camRand(MAX_RANDOM_VTOL_ADDITIONS + 1);
	var droids = [];
	var pos;

	//Make sure to catch multiple start positions also.
	if(__camVtolStartPosition instanceof Array)
	{
		pos = __camVtolStartPosition[camRand(__camVtolStartPosition.length)];
	}
	else
	{
		pos = __camVtolStartPosition;
	}

	if (!camDef(__camVtolExtras))
	{
		//Pick some droids randomly.
		for (var i = 0; i < amount; ++i)
		{
			droids.push(__camVtolTemplates[camRand(__camVtolTemplates.length)]);
		}
	}
	else
	{
		var lim = amount;
		var alternate = false;
		if (camDef(__camVtolExtras.alternate))
		{
			alternate = __camVtolExtras.alternate; //Only use one template type
		}
		if (!camDef(__camVtolExtras.altIdx))
		{
			__camVtolExtras.altIdx = 0;
		}
		if (camDef(__camVtolExtras.limit))
		{
			//support an array of limits for each template
			if (__camVtolExtras.limit instanceof Array)
			{
				lim = __camVtolExtras.limit[__camVtolExtras.altIdx]; //max templates to use
			}
			else
			{
				lim = __camVtolExtras.limit;
			}
		}

		for (var i = 0; i < lim; ++i)
		{
			if (!alternate)
			{
				droids.push(__camVtolTemplates[camRand(__camVtolTemplates.length)]);
			}
			else
			{
				droids.push(__camVtolTemplates[__camVtolExtras.altIdx]);
			}
		}

		if (__camVtolExtras.altIdx < __camVtolTemplates.length)
		{
			++__camVtolExtras.altIdx;
		}
		else
		{
			__camVtolExtras.altIdx = 0;
		}
	}

	//...And send them.
	camSendReinforcement(__camVtolPlayer, camMakePos(pos), droids, CAM_REINFORCE_GROUND, {
		order: CAM_ORDER_ATTACK,
		data: { regroup: false, count: -1 }
	});
}

function __camRetreatVtols()
{
	if (camDef(__camVtolExitPosition.x) &&
		camDef(__camVtolExitPosition.y) &&
		enumStruct(__camVtolPlayer, REARM_PAD).length === 0)
	{
		const VTOL_RETURN_HEALTH = 40; // run-away if health is less than...
		const VTOL_RETURN_ARMED = 1; // run-away if weapon ammo is less than...
		var vtols = enumDroid(__camVtolPlayer).filter(function(obj) {
			return isVTOL(obj);
		});

		for (var i = 0, len = vtols.length; i < len; ++i)
		{
			var vt = vtols[i];
			for (var c = 0, len2 = vt.weapons.length; c < len2; ++c)
			{
				if ((vt.order === DORDER_RTB) || (vt.weapons[c].armed < VTOL_RETURN_ARMED) || (vt.health < VTOL_RETURN_HEALTH))
				{
					orderDroidLoc(vt, DORDER_MOVE, __camVtolExitPosition.x, __camVtolExitPosition.y);
					break;
				}
			}
		}
	}
}
