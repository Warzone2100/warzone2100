
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
	__camVtolTimer = timer;
	__camVtolSpawnStopObject = obj;
	__camVtolExtras = extras;

	camSetVtolSpawn(true);
	__checkVtolSpawnObject();
	__camSpawnVtols();
	__camRetreatVtols();
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
		}
		else
		{
			queue("__checkVtolSpawnObject", camSecondsToMilliseconds(5));
		}
	}
}

function __camSpawnVtols()
{
	if (__camVtolSpawnActive === false)
	{
		return;
	}

	var amount = 5 + camRand(2);
	var droids = [];
	var i = 0;
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
		for (i = 0; i < amount; ++i)
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

		for (i = 0; i < lim; ++i)
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

	queue("__camSpawnVtols", __camVtolTimer);
}

function __camRetreatVtols()
{
	if (camDef(__camVtolExitPosition.x) &&
		camDef(__camVtolExitPosition.y) &&
		countStruct("A0VtolPad", __camVtolPlayer) <= 0)
	{
		var vtols = enumDroid(__camVtolPlayer).filter(function(obj) {
			return isVTOL(obj);
		});

		for (var i = 0, l = vtols.length; i < l; ++i)
		{
			var vt = vtols[i];
			for (var c = 0, d = vt.weapons.length; c < d; ++c)
			{
				if ((vt.order === DORDER_RTB) || (vt.weapons[c].armed < 1) || (vt.health < 40))
				{
					orderDroidLoc(vt, DORDER_MOVE, __camVtolExitPosition.x, __camVtolExitPosition.y);
					break;
				}
			}
		}
	}

	queue("__camRetreatVtols", camSecondsToMilliseconds(0.8));
}
