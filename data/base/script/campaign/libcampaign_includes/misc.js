
////////////////////////////////////////////////////////////////////////////////
// Misc useful stuff.
////////////////////////////////////////////////////////////////////////////////

//;; ## camDef(something)
//;;
//;; Returns false if something is JavaScript-undefined, true otherwise.
//;;
function camDef(something)
{
	return typeof(something) !== "undefined";
}

//;; ## camIsString(something)
//;;
//;; Returns true if something is a string, false otherwise.
//;;
function camIsString(something)
{
	return typeof(something) === "string";
}

//;; ## camRand(max)
//;;
//;; A non-synchronous random integer in range [0, max - 1].
//;;
function camRand(max)
{
	if (max > 0)
	{
		return Math.floor(Math.random() * max);
	}
	camDebug("Max should be positive");
}

//;; ## camCallOnce(function name)
//;;
//;; Call a function by name, but only if it has not been called yet.
//;;
function camCallOnce(callback)
{
	if (camDef(__camCalledOnce[callback]) && __camCalledOnce[callback])
	{
		return;
	}
	__camCalledOnce[callback] = true;
	__camGlobalContext()[callback]();
}

//;; ## camSafeRemoveObject(obj[, special effects?])
//;;
//;; Remove a game object (by value or label) if it exists, do nothing otherwise.
//;;
function camSafeRemoveObject(obj, flashy)
{
	if (__camLevelEnded)
	{
		return;
	}
	if (camIsString(obj))
	{
		obj = getObject(obj);
	}
	if (camDef(obj) && obj)
	{
		removeObject(obj, flashy);
	}
}

//;; ## camMakePos(x, y | label | object)
//;;
//;; Make a POSITION-like object, unless already done. Often useful
//;; for making functions that would accept positions in both xx,yy and {x:xx,y:yy} forms.
//;; Also accepts labels. If label of AREA is given, returns the center of the area.
//;; If an existing object or label of such is given, returns a safe JavaScript
//;; object containing its x, y and id.
//;;
function camMakePos(xx, yy)
{
	if (camDef(yy))
	{
		return { x : xx, y : yy };
	}
	if (!camDef(xx))
	{
		return undefined;
	}
	var obj = xx;
	if (camIsString(xx))
	{
		obj = getObject(xx);
	}
	if (!camDef(obj) || !obj)
	{
		camDebug("Failed at", xx);
		return undefined;
	}
	switch (obj.type)
	{
		case DROID:
		case STRUCTURE:
		case FEATURE:
			// store ID for those as well.
			return { x: obj.x, y: obj.y, id: obj.id };
		case POSITION:
		case RADIUS:
			return obj;
		case AREA:
			return {
				x: Math.floor((obj.x + obj.x2) / 2),
				y: Math.floor((obj.y + obj.y2) / 2)
			};
		case GROUP:
		default:
			// already a pos-like object?
			if (camDef(obj.x) && camDef(obj.y))
			{
				return { x: obj.x, y: obj.y };
			}
			camDebug("Not implemented:", obj.type);
			return undefined;
	}
}

//;; ## camDist(x1, y1, x2, y2 | pos1, x2, y2 | x1, y1, pos2 | pos1, pos2)
//;;
//;; A wrapper for ```distBetweenTwoPoints()```.
//;;
function camDist(x1, y1, x2, y2)
{
	if (camDef(y2)) // standard
	{
		return distBetweenTwoPoints(x1, y1, x2, y2);
	}
	var pos2 = camMakePos(x2, y2);
	if (!camDef(pos2)) // pos1, pos2
	{
		return distBetweenTwoPoints(x1.x, x1.y, y1.x, y1.y);
	}
	if (camDef(pos2.x)) // x2 is pos2
	{
		return distBetweenTwoPoints(x1, y1, pos2.x, pos2.y);
	}
	else // pos1, x2, y2
	{
		return distBetweenTwoPoints(x1.x, x1.y, y1, x2);
	}
}

//;; ## camPlayerMatchesFilter(player, filter)
//;;
//;; A function to handle player filters in a way similar to
//;; how JS API functions (eg. ```enumDroid(filter, ...)```) handle them.
//;;
function camPlayerMatchesFilter(player, filter)
{
	switch(filter) {
		case ALL_PLAYERS:
			return true;
		case ALLIES:
			return player === CAM_HUMAN_PLAYER;
		case ENEMIES:
			return player >= 0 && player < CAM_MAX_PLAYERS && player !== CAM_HUMAN_PLAYER;
		default:
			return player === filter;
	}
}

//;; ## camRemoveDuplicates(array)
//;;
//;; Remove duplicate items from an array.
//;;
function camRemoveDuplicates(array)
{
	var prims = {"boolean":{}, "number":{}, "string":{}};
	var objs = [];

	return array.filter(function(item) {
		var type = typeof item;
		if (type in prims)
		{
			return prims[type].hasOwnProperty(item) ? false : (prims[type][item] = true);
		}
		else
		{
			return objs.indexOf(item) >= 0 ? false : objs.push(item);
		}
	});
}

//;; ## camCountStructuresInArea(label, [player])
//;;
//;; Mimics wzscript's numStructsButNotWallsInArea().
//;;
function camCountStructuresInArea(lab, player)
{
	if (!camDef(player))
	{
		player = CAM_HUMAN_PLAYER;
	}
	var list = enumArea(lab, player, false);
	var ret = 0;
	for (var i = 0, l = list.length; i < l; ++i)
	{
		var object = list[i];
		if (object.type === STRUCTURE && object.stattype !== WALL && object.status === BUILT)
		{
			++ret;
		}
	}
	return ret;
}

//;; ## camChangeOnDiff(numeric value)
//;;
//;; Change a numeric value based on campaign difficulty.
//;;
function camChangeOnDiff(num)
{
	var modifier = 0;

	switch (difficulty)
	{
		case EASY:
			modifier = 1.25;
			break;
		case MEDIUM:
			modifier = 1.00;
			break;
		case HARD:
			modifier = 0.80;
			break;
		case INSANE:
			modifier = 0.67;
			break;
		default:
			modifier = 1.00;
			break;
	}

	return Math.floor(num * modifier);
}

//;; ## camIsSystemDroid(game object)
//;;
//;; Determine if the passed in object is a non-weapon based droid.
//;;
function camIsSystemDroid(obj)
{
	if(!camDef(obj) || !obj)
	{
		return false;
	}

	if (obj.type !== DROID)
	{
		camTrace("Non-droid: " + obj.type + " pl: " + obj.name);
		return false;
	}

	return (obj.droidType === DROID_SENSOR || obj.droidType === DROID_CONSTRUCT || obj.droidType === DROID_REPAIR);
}

//;; ## camMakeGroup(what, filter)
//;;
//;; Make a new group out of array of droids, single game object,
//;; or label string, with fuzzy auto-detection of argument type.
//;; Only droids would be added to the group. ```filter``` can be one of
//;; a player index, ```ALL_PLAYERS```, ```ALLIES``` or ```ENEMIES```;
//;; defaults to ```ENEMIES```.
//;;
function camMakeGroup(what, filter)
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
		switch(obj.type) {
			case POSITION:
				obj = getObject(obj.x, obj.y);
				// fall-through
			case DROID:
			case STRUCTURE:
			case FEATURE:
				array = [ obj ];
				break;
			case AREA:
				array = enumArea(obj.x, obj.y, obj.x2, obj.y2,
				                 ALL_PLAYERS, false);
				break;
			case RADIUS:
				array = enumRange(obj.x, obj.y, obj.radius,
				                 ALL_PLAYERS, false);
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
		var group = camNewGroup();
		for (var i = 0, l = array.length; i < l; ++i)
		{
			var o = array[i];
			if (!camDef(o) || !o)
			{
				camDebug("Trying to add", o);
				continue;
			}
			if (o.type === DROID && o.droidType !== DROID_CONSTRUCT && camPlayerMatchesFilter(o.player, filter))
			{
				groupAdd(group, o);
			}
		}
		return group;
	}
	camDebug("Cannot parse", what);
}

//;; ## camBreakAlliances()
//;;
//;; Break alliances between all players.
//;;
function camBreakAlliances()
{
	for (var i = 0; i < CAM_MAX_PLAYERS; ++i)
	{
		for (var c = 0; c < CAM_MAX_PLAYERS; ++c)
		{
			if (i !== c && allianceExistsBetween(i, c) === true)
			{
				setAlliance(i, c, false);
			}
		}
	}
}

//////////// privates

function __camGlobalContext()
{
	return Function('return this')(); // eslint-disable-line no-new-func
}

function __camFindClusters(list, size)
{
	// The good old cluster analysis algorithm taken from NullBot AI.
	var ret = { clusters: [], xav: [], yav: [], maxIdx: 0, maxCount: 0 };
	for (var i = list.length - 1; i >= 0; --i)
	{
		var x = list[i].x;
		var y = list[i].y;
		var found = false;
		var n = 0;
		for (var j = 0; j < ret.clusters.length; ++j)
		{
			if (camDist(ret.xav[j], ret.yav[j], x, y) < size)
			{
				n = ret.clusters[j].length;
				ret.clusters[j][n] = list[i];
				ret.xav[j] = Math.floor((n * ret.xav[j] + x) / (n + 1));
				ret.yav[j] = Math.floor((n * ret.yav[j] + y) / (n + 1));
				if (ret.clusters[j].length > ret.maxCount)
				{
					ret.maxIdx = j;
					ret.maxCount = ret.clusters[j].length;
				}
				found = true;
				break;
			}
		}
		if (!found)
		{
			n = ret.clusters.length;
			ret.clusters[n] = [list[i]];
			ret.xav[n] = x;
			ret.yav[n] = y;
			if (1 > ret.maxCount)
			{
				ret.maxIdx = n;
				ret.maxCount = 1;
			}
		}
	}
	return ret;
}

/* Called every second after eventStartLevel(). */
function __camTick()
{
	if (camDef(__camWinLossCallback))
	{
		__camGlobalContext()[__camWinLossCallback]();
	}
	__camBasesTick();
}

//Reset AI power back to highest storage possible.
function __camAiPowerReset()
{
	for (var i = 1; i < CAM_MAX_PLAYERS; ++i)
	{
		setPower(AI_POWER, i);
	}
}

function __camSetOffworldLimits()
{
	// These are the only structures that do not get
	// auto-disabled by the engine in off-world missions.
	setStructureLimits("A0CommandCentre", 0, CAM_HUMAN_PLAYER);
	setStructureLimits("A0ComDroidControl", 0, CAM_HUMAN_PLAYER);
}
