
////////////////////////////////////////////////////////////////////////////////
// Misc useful stuff.
////////////////////////////////////////////////////////////////////////////////

/**
 * Returns false if something is JavaScript-undefined, true otherwise.
 * @param {*} something
 * @returns {boolean}
 */
function camDef(something)
{
	return typeof something !== "undefined";
}

/**
 * Returns true if something is a string, false otherwise.
 * @param {*} something
 * @returns {boolean}
 */
function camIsString(something)
{
	return typeof something === "string";
}

/**
 * A non-synchronous random integer in range [0, max - 1].
 * @param {number} max
 * @returns {number}
 */
function camRand(max)
{
	if (max > 0)
	{
		return Math.floor(Math.random() * max);
	}
	camDebug("Max should be positive");
}

/**
 * Call a function by name, but only if it has not been called yet.
 * @param {string} functionName
 * @returns {void}
 */
function camCallOnce(functionName)
{
	if (camDef(__camCalledOnce[functionName]) && __camCalledOnce[functionName])
	{
		return;
	}
	__camCalledOnce[functionName] = true;
	__camGlobalContext()[functionName]();
}

/**
 * Remove a game object (by value or label) if it exists, do nothing otherwise.
 * @param {string|Object} obj
 * @param {boolean} [specialEffects]
 * @returns {void}
 */
function camSafeRemoveObject(obj, specialEffects)
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
		removeObject(obj, specialEffects);
	}
}

/**
 * Make a POSITION-like object, unless already done.
 * Often useful for making functions that would accept positions in both x,y and {x:x,y:y} forms.
 * Also accepts labels. If label of AREA is given, returns the center of the area.
 * If an existing object or label of such is given, returns a safe JavaScript object containing its x, y and id.
 * @param {number|string|Object|undefined} x
 * @param {number} [y]
 * @returns {Object|undefined}
 */
function camMakePos(x, y)
{
	if (camDef(y))
	{
		return { x: x, y: y };
	}
	if (!camDef(x))
	{
		return undefined;
	}
	var obj = x;
	if (camIsString(x))
	{
		obj = getObject(x);
	}
	if (!camDef(obj) || !obj)
	{
		camDebug("Failed at", x);
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

/**
 * A wrapper for `distBetweenTwoPoints()`.
 * camDist(x1, y1, x2, y2 | pos1, x2, y2 | x1, y1, pos2 | pos1, pos2)
 * @param {number|Object} x1
 * @param {number|Object} y1
 * @param {number} [x2]
 * @param {number} [y2]
 * @returns {number}
 */
function camDist(x1, y1, x2, y2)
{
	if (camDef(y2)) // standard
	{
		return distBetweenTwoPoints(x1, y1, x2, y2);
	}
	if (!camDef(x2)) // pos1, pos2
	{
		return distBetweenTwoPoints(x1.x, x1.y, y1.x, y1.y);
	}
	var pos2 = camMakePos(x2);
	if (camDef(pos2.x)) // x2 is pos2
	{
		return distBetweenTwoPoints(x1, y1, pos2.x, pos2.y);
	}
	else // pos1, x2, y2
	{
		return distBetweenTwoPoints(x1.x, x1.y, y1, x2);
	}
}

/**
 * A function to handle player filters in a way similar to how JS API functions (eg. `enumDroid(filter, ...)`) handle them.
 * @param {number} player
 * @param {number} playerFilter
 * @returns {boolean}
 */
function camPlayerMatchesFilter(player, playerFilter)
{
	switch (playerFilter) {
		case ALL_PLAYERS:
			return true;
		case ALLIES:
			return player === CAM_HUMAN_PLAYER;
		case ENEMIES:
			return player >= 0 && player < CAM_MAX_PLAYERS && player !== CAM_HUMAN_PLAYER;
		default:
			return player === playerFilter;
	}
}

/**
 * Remove duplicate items from an array.
 * @param {*[]} items
 * @returns {*[]}
 */
function camRemoveDuplicates(items)
{
	var prims = {"boolean":{}, "number":{}, "string":{}};
	var objs = [];

	return items.filter((item) => {
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

/**
 * Mimics wzscript's numStructsButNotWallsInArea().
 * @param {string} label
 * @param {number} [playerFilter]
 * @returns {number}
 */
function camCountStructuresInArea(label, playerFilter)
{
	if (!camDef(playerFilter))
	{
		playerFilter = CAM_HUMAN_PLAYER;
	}
	var list = enumArea(label, playerFilter, false);
	var ret = 0;
	for (let i = 0, l = list.length; i < l; ++i)
	{
		var object = list[i];
		if (object.type === STRUCTURE && object.stattype !== WALL && object.status === BUILT)
		{
			++ret;
		}
	}
	return ret;
}

/**
 * Change a numeric value based on campaign difficulty.
 * @param {number} numericValue
 * @returns {number}
 */
function camChangeOnDiff(numericValue)
{
	var modifier = 0;

	switch (difficulty)
	{
		case EASY:
			modifier = 1.5;
			break;
		case MEDIUM:
			modifier = 1.00;
			break;
		case HARD:
			modifier = 0.85;
			break;
		case INSANE:
			modifier = 0.70;
			break;
		default:
			modifier = 1.00;
			break;
	}

	return Math.floor(numericValue * modifier);
}

/**
 * Determine if the passed in object is a non-weapon based droid.
 * @param {Object} gameObject
 * @returns {boolean}
 */
function camIsSystemDroid(gameObject)
{
	if (!camDef(gameObject) || !gameObject)
	{
		return false;
	}

	if (gameObject.type !== DROID)
	{
		camTrace("Non-droid: " + gameObject.type + " pl: " + gameObject.name);
		return false;
	}

	return (gameObject.droidType === DROID_SENSOR || gameObject.droidType === DROID_CONSTRUCT || gameObject.droidType === DROID_REPAIR);
}

/**
 * Make a new group out of array of droids, single game object, or label string,
 * with fuzzy auto-detection of argument type.
 * Only droids would be added to the group. `playerFilter` can be one of a
 * player index, `ALL_PLAYERS`, `ALLIES` or `ENEMIES`; defaults to `ENEMIES`.
 * @param {string|Object|Object[]} what
 * @param {number} [playerFilter]
 * @returns {number|void}
 */
function camMakeGroup(what, playerFilter)
{
	if (!camDef(playerFilter))
	{
		playerFilter = ENEMIES;
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
		for (let i = 0, l = array.length; i < l; ++i)
		{
			var o = array[i];
			if (!camDef(o) || !o)
			{
				camDebug("Trying to add", o);
				continue;
			}
			if (o.type === DROID && o.droidType !== DROID_CONSTRUCT && camPlayerMatchesFilter(o.player, playerFilter))
			{
				groupAdd(group, o);
			}
		}
		return group;
	}
	camDebug("Cannot parse", what);
}

/**
 * Break alliances between all players.
 * @returns {void}
 */
function camBreakAlliances()
{
	for (let i = 0; i < CAM_MAX_PLAYERS; ++i)
	{
		for (let c = 0; c < CAM_MAX_PLAYERS; ++c)
		{
			if (i !== c && allianceExistsBetween(i, c) === true)
			{
				setAlliance(i, c, false);
			}
		}
	}
}

// Picks a random coordinate anywhere on the edge of the map.
function camGenerateRandomMapEdgeCoordinate(reachPosition)
{
	let limits = getScrollLimits();
	let loc;

	do
	{
		let location = {x: 0, y: 0};
		let xWasRandom = false;

		if (camRand(100) < 50)
		{
			location.x = camRand(limits.x2 + 1);
			if (location.x < (limits.x + 2))
			{
				location.x = limits.x + 2;
			}
			else if (location.x > (limits.x2 - 2))
			{
				location.x = limits.x2 - 2;
			}
			xWasRandom = true;
		}
		else
		{
			location.x = (camRand(100) < 50) ? (limits.x2 - 2) : (limits.x + 2);
		}

		if (!xWasRandom && (camRand(100) < 50))
		{
			location.y = camRand(limits.y2 + 1);
			if (location.y < (limits.y + 2))
			{
				location.y = limits.y + 2;
			}
			else if (location.y > (limits.y2 - 2))
			{
				location.y = limits.y2 - 2;
			}
		}
		else
		{
			location.y = (camRand(100) < 50) ? (limits.y2 - 2) : (limits.y + 2);
		}

		loc = location;
	} while (camDef(reachPosition) && reachPosition && !propulsionCanReach("wheeled01", reachPosition.x, reachPosition.y, loc.x, loc.y));

	return loc;
}

// Picks a random coordinate anywhere on the map.
function camGenerateRandomMapCoordinate(reachPosition, distFromReach, scanObjectRadius)
{
	if (!camDef(distFromReach))
	{
		distFromReach = 10;
	}
	if (!camDef(scanObjectRadius))
	{
		scanObjectRadius = 2;
	}

	let limits = getScrollLimits();
	let pos;

	do
	{
		let randomPos = {x: camRand(limits.x2), y: camRand(limits.y2)};

		if (randomPos.x < (limits.x + 2))
		{
			randomPos.x = limits.x + 2;
		}
		else if (randomPos.x > (limits.x2 - 2))
		{
			randomPos.x = limits.x2 - 2;
		}

		if (randomPos.y < (limits.y + 2))
		{
			randomPos.y = limits.y;
		}
		else if (randomPos.y > (limits.y2 - 2))
		{
			randomPos.y = limits.y2 - 2;
		}

		pos = randomPos;
	} while (camDef(reachPosition) &&
		reachPosition &&
		!propulsionCanReach("wheeled01", reachPosition.x, reachPosition.y, pos.x, pos.y) &&
		(camDist(pos, reachPosition) < distFromReach) &&
		(enumRange(pos.x, pos.y, scanObjectRadius, ALL_PLAYERS, false).length > 0));

	return pos;
}

// Figures out what campaign we are in without reliance on the source at all.
function camDiscoverCampaign()
{
	for (let i = 0, len = ALPHA_LEVELS.length; i < len; ++i)
	{
		if (__camNextLevel === ALPHA_LEVELS[i] || __camNextLevel === BETA_LEVELS[0])
		{
			return ALPHA_CAMPAIGN_NUMBER;
		}
	}
	for (let i = 0, len = BETA_LEVELS.length; i < len; ++i)
	{
		if (__camNextLevel === BETA_LEVELS[i] || __camNextLevel === GAMMA_LEVELS[0])
		{
			return BETA_CAMPAIGN_NUMBER;
		}
	}
	for (let i = 0, len = GAMMA_LEVELS.length; i < len; ++i)
	{
		if (__camNextLevel === GAMMA_LEVELS[i] || __camNextLevel === CAM_GAMMA_OUT)
		{
			return GAMMA_CAMPAIGN_NUMBER;
		}
	}

	return UNKNOWN_CAMPAIGN_NUMBER;
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
	for (let i = list.length - 1; i >= 0; --i)
	{
		var x = list[i].x;
		var y = list[i].y;
		var found = false;
		var n = 0;
		for (let j = 0; j < ret.clusters.length; ++j)
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
	for (let i = 1; i < CAM_MAX_PLAYERS; ++i)
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
