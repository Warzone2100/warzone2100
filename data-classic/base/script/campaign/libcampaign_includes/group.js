
////////////////////////////////////////////////////////////////////////////////
// Group functionality
////////////////////////////////////////////////////////////////////////////////

//;; ## camNewGroup()
//;;
//;; A saveload safe version of newGroup() so as not to create group ID clashes.
//;;
function camNewGroup()
{
	if (!camDef(__camNewGroupCounter))
	{
		__camNewGroupCounter = 0;
	}
	++__camNewGroupCounter;
	return __camNewGroupCounter;
}

//;; ## camInNeverGroup(droid)
//;;
//;; check if this droid is forced to never group.
//;;
function camInNeverGroup(droid)
{
	if (droid.type !== DROID)
	{
		camDebug("Non-droid in camInNeverGroup.");
		return false;
	}
	for (var i = 0, l = __camNeverGroupDroids.length; i < l; ++i)
	{
		var neverDroidID = __camNeverGroupDroids[i];
		if (droid.id === neverDroidID)
		{
			return true;
		}
	}

	return false;
}

//;; ## camNeverGroupDroid(what, [filter])
//;;
//;; A means to not auto group some droids.
//;;
function camNeverGroupDroid(what, filter)
{
	if (!camDef(filter))
	{
		filter = ENEMIES;
	}
	var array;
	var obj;
	if (camIsString(what)) // label
	{
		obj = getObject(what);
	}
	else if (camDef(what.length)) // array
	{
		array = what;
	}
	else if (camDef(what.type)) // object
	{
		obj = what;
	}
	if (camDef(obj))
	{
		switch (obj.type) {
			case POSITION:
				obj = getObject(obj.x, obj.y);
				// fall-through
			case DROID:
			case STRUCTURE:
			case FEATURE:
				array = [ obj ];
				break;
			case AREA:
				array = enumArea(obj.x, obj.y, obj.x2, obj.y2, ALL_PLAYERS, false);
				break;
			case RADIUS:
				array = enumRange(obj.x, obj.y, obj.radius, ALL_PLAYERS, false);
				break;
			case GROUP:
				array = enumGroup(obj.id);
				break;
			default:
				camDebug("Unknown object type", obj.type);
		}
	}
	if (camDef(array))
	{
		for (var i = 0, l = array.length; i < l; ++i)
		{
			var o = array[i];
			if (!camDef(o) || !o)
			{
				continue;
			}
			if (o.type === DROID && camPlayerMatchesFilter(o.player, filter))
			{
				__camNeverGroupDroids.push(o.id);
			}
		}
		return;
	}
	camDebug("Cannot parse", what);
}
