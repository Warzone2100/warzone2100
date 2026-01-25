
////////////////////////////////////////////////////////////////////////////////
// Misc useful stuff.
////////////////////////////////////////////////////////////////////////////////

//;; ## camClassicMode()
//;;
//;; Returns `true` if classic balance mod is enabled.
//;;
//;; @returns {boolean}
//;;
function camClassicMode()
{
	return __camClassicModActive;
}

//;; ## camDef(something)
//;;
//;; Returns `false` if something is JavaScript-undefined, `true` otherwise.
//;;
//;; @param {*} something
//;; @returns {boolean}
//;;
function camDef(something)
{
	return typeof something !== "undefined";
}

//;; ## camIsString(something)
//;;
//;; Returns `true` if something is a string, `false` otherwise.
//;;
//;; @param {*} something
//;; @returns {boolean}
//;;
function camIsString(something)
{
	return typeof something === "string";
}

//;; ## camRand(max)
//;;
//;; A non-synchronous random integer in range [0, max - 1].
//;;
//;; @param {number} max
//;; @returns {number}
//;;
function camRand(max)
{
	if (max > 0)
	{
		return Math.floor(Math.random() * max);
	}
	camDebug("Max should be positive");
}

//;; ## camCallOnce(functionName)
//;;
//;; Call a function by name, but only if it has not been called yet.
//;;
//;; @param {string} functionName
//;; @returns {void}
//;;
function camCallOnce(functionName)
{
	if (camDef(__camCalledOnce[functionName]) && __camCalledOnce[functionName])
	{
		return;
	}
	__camCalledOnce[functionName] = true;
	__camGlobalContext()[functionName]();
}

//;; ## camSafeRemoveObject(obj[, specialEffects])
//;;
//;; Remove a game object (by value or label) if it exists, do nothing otherwise.
//;;
//;; @param {string|Object} obj
//;; @param {boolean} [specialEffects]
//;; @returns {void}
//;;
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

//;; ## camMakePos(label|object|x[, y])
//;;
//;; Make a `POSITION`-like object, unless already done.
//;; Often useful for making functions that would accept positions in both `x,y` and `{x: x, y: y}` forms.
//;; Also accepts labels. If label of `AREA` is given, returns the center of the area.
//;; If an existing object or label of such is given, returns a safe JavaScript object containing its `x`, `y` and `id`.
//;;
//;; @param {number|string|Object|undefined} x
//;; @param {number} [y]
//;; @returns {Object|undefined}
//;;
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
	let obj = x;
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

//;; ## camDist(x1, y1, x2, y2 | pos1, x2, y2 | x1, y1, pos2 | pos1, pos2)
//;;
//;; A wrapper for `distBetweenTwoPoints()`.
//;;
//;; @param {number|Object} x1
//;; @param {number|Object} y1
//;; @param {number} [x2]
//;; @param {number} [y2]
//;; @returns {number}
//;;
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
	const pos2 = camMakePos(x2);
	if (camDef(pos2.x)) // x2 is pos2
	{
		return distBetweenTwoPoints(x1, y1, pos2.x, pos2.y);
	}
	else // pos1, x2, y2
	{
		return distBetweenTwoPoints(x1.x, x1.y, y1, x2);
	}
}

//;; ## camPlayerMatchesFilter(playerId, playerFilter)
//;;
//;; A function to handle player filters in a way similar to how JS API functions (eg. `enumDroid(filter, ...)`) handle them.
//;;
//;; @param {number} playerId
//;; @param {number} playerFilter
//;; @returns {boolean}
//;;
function camPlayerMatchesFilter(playerId, playerFilter)
{
	switch (playerFilter)
	{
		case ALL_PLAYERS:
			return true;
		case ALLIES:
			return playerId === CAM_HUMAN_PLAYER;
		case ENEMIES:
			return playerId >= 0 && playerId < __CAM_MAX_PLAYERS && playerId !== CAM_HUMAN_PLAYER;
		default:
			return playerId === playerFilter;
	}
}

//;; ## camRemoveDuplicates(items)
//;;
//;; Remove duplicate items from an array.
//;;
//;; @param {*[]} items
//;; @returns {*[]}
//;;
function camRemoveDuplicates(items)
{
	let prims = {"boolean":{}, "number":{}, "string":{}};
	const objs = [];
	return items.filter((item) => {
		const type = typeof item;
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

//;; ## camCountStructuresInArea(label[, playerFilter])
//;;
//;; Mimics wzscript's `numStructsButNotWallsInArea()`.
//;;
//;; @param {string} label
//;; @param {number} [playerFilter]
//;; @returns {number}
//;;
function camCountStructuresInArea(label, playerFilter)
{
	if (!camDef(playerFilter))
	{
		playerFilter = CAM_HUMAN_PLAYER;
	}
	const list = enumArea(label, playerFilter, false);
	let ret = 0;
	for (let i = 0, l = list.length; i < l; ++i)
	{
		const object = list[i];
		if (object.type === STRUCTURE && object.stattype !== WALL && object.status === BUILT)
		{
			++ret;
		}
	}
	return ret;
}

//;; ## camCleanTileOfObstructions(x, y | pos)
//;;
//;; Obliterates player structures and features near the tile around certain coordinates.
//;; Can be used for spawn locations or transport reinforcement spots. May not
//;; delete very large objects like factories or skyscrapers.
//;;
//;; @param {number|Object} x
//;; @param {number} [y]
//;; @returns {void}
//;;
function camCleanTileOfObstructions(x, y)
{
	if (!camDef(x))
	{
		camDebug("invalid parameters?");
		return;
	}
	const __TILE_SWEEP_RADIUS = 1;
	const pos = (camDef(y)) ? {x: x, y: y} : x;
	const objects = enumRange(pos.x, pos.y, __TILE_SWEEP_RADIUS, CAM_HUMAN_PLAYER, false);
	for (let i = 0, len = objects.length; i < len; ++i)
	{
		const obj = objects[i];
		if (obj.type !== DROID)
		{
			camSafeRemoveObject(obj, true);
		}
	}
}

//;; ## camChangeOnDiff(numericValue)
//;;
//;; Change a numeric value based on campaign difficulty.
//;;
//;; @param {number} numericValue
//;; @returns {number}
//;;
function camChangeOnDiff(numericValue)
{
	let modifier = 0;
	switch (difficulty)
	{
		case SUPEREASY:
			modifier = 2;
			break;
		case EASY:
			modifier = 1.5;
			break;
		case MEDIUM:
			modifier = 1;
			break;
		case HARD:
			modifier = 0.85;
			break;
		case INSANE:
			modifier = 0.70;
			break;
		default:
			modifier = 1;
			break;
	}
	return Math.floor(numericValue * modifier);
}

//;; ## camAllowInsaneSpawns()
//;;
//;; Allow additional Insane difficulty (or higher) spawns and behavior.
//;;
//;; @returns {boolean}
//;;
function camAllowInsaneSpawns()
{
	if (!camDef(tweakOptions.insanePlus) || !camDef(tweakOptions.insanePlusLowDiff))
	{
		return false;
	}
	const __INSANE_SPAWNS = ((difficulty >= INSANE) && tweakOptions.insanePlus);
	const __LOWER_DIFF_SPAWNS = ((difficulty < INSANE) && tweakOptions.insanePlusLowDiff);
	return (__INSANE_SPAWNS || __LOWER_DIFF_SPAWNS);
}

//;; ## camIsSystemDroid(gameObject)
//;;
//;; Determine if the passed in object is a non-weapon based droid.
//;;
//;; @param {Object} gameObject
//;; @returns {boolean}
//;;
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

//;; ## camMakeGroup(what[, playerFilter])
//;;
//;; Make a new group out of array of droids, single game object, or label string,
//;; with fuzzy auto-detection of argument type.
//;; Only droids would be added to the group. `playerFilter` can be one of a
//;; player index, `ALL_PLAYERS`, `ALLIES` or `ENEMIES`; defaults to `ENEMIES`.
//;;
//;; @param {string|Object|Object[]} what
//;; @param {number} [playerFilter]
//;; @returns {number|void}
//;;
function camMakeGroup(what, playerFilter)
{
	if (!camDef(playerFilter))
	{
		playerFilter = ENEMIES;
	}
	let array;
	let obj;
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
		switch (obj.type)
		{
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
		const group = camNewGroup();
		for (let i = 0, l = array.length; i < l; ++i)
		{
			const o = array[i];
			if (!camDef(o) || !o)
			{
				camDebug("Trying to add", o);
				continue;
			}
			if (o.type === DROID && o.droidType !== DROID_CONSTRUCT && !camIsTransporter(o) && camPlayerMatchesFilter(o.player, playerFilter))
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
//;; @returns {void}
//;;
function camBreakAlliances()
{
	for (let i = 0; i < __CAM_MAX_PLAYERS; ++i)
	{
		for (let c = 0; c < __CAM_MAX_PLAYERS; ++c)
		{
			if (i !== c && allianceExistsBetween(i, c) === true)
			{
				setAlliance(i, c, false);
			}
		}
	}
}

//;; ## camIsWaterPropulsion(propulsion)
//;;
//;; Check if a propulsion can traverse water tiles. Until the Stats object can tell us this
//;; information, it simply uses a very basic name check against the default hover propulsions.
//;;
//;; @param {String} propulsion
//;; @returns {Boolean}
//;;
function camIsWaterPropulsion(propulsion)
{
	return (propulsion.indexOf("hover") !== -1);
}

//;; ## camNearInaccessibleAreas(start, destination [, propulsion [, distance]])
//;;
//;; Determine if a start position can reach a destination position within the limits of
//;; the chosen propulsion, and if there are nearby cliffs/water tiles nearby within a certain distance.
//;; if `destination` is undefined it will cause this function to just scan around the start position.
//;;
//;; @param {Object} start
//;; @param {Object} destination
//;; @param {String} propulsion
//;; @param {Number} distance
//;; @returns {Boolean}
//;;
function camNearInaccessibleAreas(start, destination, propulsion, distance)
{
	if (!camDef(start) || !start)
	{
		camDebug("Undefined start position when scanning for inaccessible areas.");
		return true;
	}
	if (!camDef(propulsion))
	{
		propulsion = CAM_GENERIC_LAND_STAT;
	}
	if (!camDef(distance))
	{
		distance = 1;
	}
	if (!camDef(destination) || !destination)
	{
		destination = start; //Quick way to just scan for pits/cliffs at the start position.
	}
	for (let x = -distance; x <= distance; ++x)
	{
		for (let y = -distance; y <= distance; ++y)
		{
			const tmpStart = {x: start.x + x, y: start.y + y};
			if ((tmpStart.x < 0) || (tmpStart.y < 0) || (tmpStart.x >= mapWidth) || (tmpStart.y >= mapHeight))
			{
				continue; // We don't care about being outside the map here.
			}
			const TTYPE = terrainType(tmpStart.x, tmpStart.y);
			if (!propulsionCanReach(propulsion, destination.x, destination.y, tmpStart.x, tmpStart.y) ||
				(TTYPE === TER_CLIFFFACE) ||
				((TTYPE === TER_WATER) && !camIsWaterPropulsion(propulsion)))
			{
				return true;
			}
		}
	}
	return false;
}

//;; ## camGenerateRandomMapEdgeCoordinate(reachPosition [, propulsion [, distFromReach [, scanObjectRadius]]])
//;;
//;; Returns a random coordinate anywhere on the edge of the map that reaches a position.
//;; `reachPosition` may be undefined if you just want a random edge coordinate, without object scans.
//;; Which can be useful for spawning transporter entry/exit points or VTOL spawn positions.
//;; `scanObjectRadius` may be defined to scan possible spawn points for nearby objects,
//;; and should be above one tile if there are large skyscrapers at the edges of some maps.
//;;
//;; @param {Object} reachPosition
//;; @param {String} propulsion
//;; @param {Number} distFromReach
//;; @param {Number} scanObjectRadius
//;; @returns {Object}
//;;
function camGenerateRandomMapEdgeCoordinate(reachPosition, propulsion, distFromReach, scanObjectRadius)
{
	if (!camDef(propulsion))
	{
		propulsion = CAM_GENERIC_LAND_STAT;
	}
	if (!camDef(distFromReach))
	{
		distFromReach = 0;
	}
	const limits = getScrollLimits();
	const __MAX_ATTEMPTS = 10000;
	const __DEFINED_POS = (camDef(reachPosition) && reachPosition);
	const __DEFINED_SCAN = (camDef(scanObjectRadius) && scanObjectRadius);
	const __OFFSET = 3; // Gives transporters enough space to turn around near map edges.
	const __SCAN_DIST = 1;
	let attempts = 0;
	let breakOut = false;
	let loc;
	while (!breakOut)
	{
		++attempts;
		const location = {x: 0, y: 0};
		let xWasRandom = false;
		if (camRand(100) < 50)
		{
			location.x = camRand(limits.x2 + 1);
			if (location.x < (limits.x + __OFFSET))
			{
				location.x = limits.x + __OFFSET;
			}
			else if (location.x > (limits.x2 - __OFFSET))
			{
				location.x = limits.x2 - __OFFSET;
			}
			xWasRandom = true;
		}
		else
		{
			location.x = (camRand(100) < 50) ? (limits.x2 - __OFFSET) : (limits.x + __OFFSET);
		}
		if (!xWasRandom && (camRand(100) < 50))
		{
			location.y = camRand(limits.y2 + 1);
			if (location.y < (limits.y + __OFFSET))
			{
				location.y = limits.y + __OFFSET;
			}
			else if (location.y > (limits.y2 - __OFFSET))
			{
				location.y = limits.y2 - __OFFSET;
			}
		}
		else
		{
			location.y = (camRand(100) < 50) ? (limits.y2 - __OFFSET) : (limits.y + __OFFSET);
		}
		loc = location;
		if ((attempts > __MAX_ATTEMPTS) ||
			(!__DEFINED_POS ||
			((camDist(reachPosition.x, reachPosition.y, loc.x, loc.y) >= distFromReach) &&
			(!__DEFINED_SCAN || !enumRange(loc.x, loc.y, scanObjectRadius, ALL_PLAYERS, false).length) &&
			!camNearInaccessibleAreas(loc, reachPosition, propulsion, __SCAN_DIST))))
		{
			breakOut = true;
		}
	}
	return loc;
}

//;; ## camGenerateRandomMapCoordinate(reachPosition [, propulsion [, distFromReach [, scanObjectRadius, [, avoidNearbyCliffs]]]])
//;;
//;; Returns a random coordinate anywhere on the map.
//;;
//;; @param {Object} reachPosition
//;; @param {String} propulsion
//;; @param {Number} distFromReach
//;; @param {Number} scanObjectRadius
//;; @param {Boolean} avoidNearbyCliffs
//;; @returns {Object}
//;;
function camGenerateRandomMapCoordinate(reachPosition, propulsion, distFromReach, scanObjectRadius, avoidNearbyCliffs)
{
	if (!camDef(reachPosition) || !reachPosition)
	{
		camDebug("Undefined reachPosition when attempting to generate random coordinate.");
		return {x: (mapWidth / 2), y: (mapHeight / 2)}; // Better than nothing.
	}
	if (!camDef(distFromReach))
	{
		distFromReach = 10;
	}
	if (!camDef(scanObjectRadius))
	{
		scanObjectRadius = 1;
	}
	if (!camDef(propulsion))
	{
		propulsion = CAM_GENERIC_LAND_STAT;
	}
	if (!camDef(avoidNearbyCliffs))
	{
		avoidNearbyCliffs = true;
	}
	const limits = getScrollLimits();
	const __MAX_ATTEMPTS = 10000;
	const __OFFSET = 3; // Gives transporters enough space to turn around near map edges.
	const __SCAN_DIST = 2;
	let attempts = 0;
	let breakOut = false;
	let pos;
	while (!breakOut)
	{
		++attempts;
		let randomPos = {x: camRand(limits.x2), y: camRand(limits.y2)};
		let nearPitOrCliff = false;
		if (randomPos.x < (limits.x + __OFFSET))
		{
			randomPos.x = limits.x + __OFFSET;
		}
		else if (randomPos.x > (limits.x2 - __OFFSET))
		{
			randomPos.x = limits.x2 - __OFFSET;
		}
		if (randomPos.y < (limits.y + __OFFSET))
		{
			randomPos.y = limits.y + __OFFSET;
		}
		else if (randomPos.y > (limits.y2 - __OFFSET))
		{
			randomPos.y = limits.y2 - __OFFSET;
		}
		pos = randomPos;
		// Scan for nearby pits/hills so transporters don't put units inside inaccessible areas.
		if (avoidNearbyCliffs)
		{
			nearPitOrCliff = camNearInaccessibleAreas(pos, reachPosition, propulsion, __SCAN_DIST);
		}
		if ((attempts > __MAX_ATTEMPTS) ||
			((camDist(pos, reachPosition) >= distFromReach) &&
			propulsionCanReach(propulsion, reachPosition.x, reachPosition.y, pos.x, pos.y) &&
			(!avoidNearbyCliffs || !nearPitOrCliff) &&
			!enumRange(pos.x, pos.y, scanObjectRadius, ALL_PLAYERS, false).length))
		{
			breakOut = true;
		}
	}
	return pos;
}

//;; ## camDiscoverCampaign()
//;;
//;; Figures out what campaign we are in without reliance on the source at all.
//;;
//;; @returns {number}
//;;
function camDiscoverCampaign()
{
	for (let i = 0, len = __cam_alphaLevels.length; i < len; ++i)
	{
		if (__camNextLevel === __cam_alphaLevels[i] || __camNextLevel === __cam_betaLevels[0])
		{
			return __CAM_ALPHA_CAMPAIGN_NUMBER;
		}
	}
	for (let i = 0, len = __cam_betaLevels.length; i < len; ++i)
	{
		if (__camNextLevel === __cam_betaLevels[i] || __camNextLevel === __cam_gammaLevels[0])
		{
			return __CAM_BETA_CAMPAIGN_NUMBER;
		}
	}
	for (let i = 0, len = __cam_gammaLevels.length; i < len; ++i)
	{
		if (__camNextLevel === __cam_gammaLevels[i] || __camNextLevel === CAM_GAMMA_OUT)
		{
			return __CAM_GAMMA_CAMPAIGN_NUMBER;
		}
	}

	return __CAM_UNKNOWN_CAMPAIGN_NUMBER;
}

//;; ## camGetRankThreshold(rank [, command [, player]])
//;;
//;; Returns the rank threshold for a given rank.
//;;
//;; @param {String|Number} rank
//;; @param {Boolean} command
//;; @param {Number} player
//;; @returns {number}
//;;
function camGetRankThreshold(rankName, command, player)
{
	if (!camDef(command))
	{
		command = false;
	}
	if (!camDef(player))
	{
		player = CAM_HUMAN_PLAYER;
	}
	const __BRAIN_TYPE = (command) ? "Command Turret" : "Z NULL BRAIN";
	let rank = 0;
	if (typeof rankName === "string")
	{
		rank = __camRankStringToNumber(rankName);
	}
	return Upgrades[player]["Brain"][__BRAIN_TYPE]["RankThresholds"][rank];
}

//;; ## camSetExpLevel(rank)
//;;
//;; Sets what rank will be used for the AI when it creates units. Can be a rank threshold
//;; index or the name of the rank.
//;;
//;; @param {Number|String} rank
//;; @returns {void}
//;;
function camSetExpLevel(rank)
{
	if (!camDef(rank))
	{
		rank = 0;
	}
	__camExpLevel = (typeof rank === "string") ? __camRankStringToNumber(rank) : rank;
}

//;; ## camSetOnMapEnemyUnitExp()
//;;
//;; Sets all non-player units to the chosen rank set through camSetExpLevel().
//;;
//;; @returns {void}
//;;
function camSetOnMapEnemyUnitExp()
{
	enumDroid(CAM_NEW_PARADIGM)
	.concat(enumDroid(CAM_THE_COLLECTIVE))
	.concat(enumDroid(CAM_NEXUS))
	.concat(enumDroid(CAM_SCAV_6))
	.concat(enumDroid(CAM_SCAV_7))
	.forEach(function(obj) {
		if (!allianceExistsBetween(CAM_HUMAN_PLAYER, obj.player) && //may have friendly units as other player
			!camIsTransporter(obj) &&
			obj.droidType !== DROID_CONSTRUCT &&
			obj.droidType !== DROID_REPAIR)
		{
			camSetDroidExperience(obj);
		}
	});
}

//////////// privates

function __camGlobalContext()
{
	return Function('return this')(); // eslint-disable-line no-new-func
}

function __camFindClusters(list, size)
{
	// The good old cluster analysis algorithm taken from NullBot AI.
	const ret = { clusters: [], xav: [], yav: [], maxIdx: 0, maxCount: 0 };
	for (let i = list.length - 1; i >= 0; --i)
	{
		const __X = list[i].x;
		const __Y = list[i].y;
		let found = false;
		let n = 0;
		for (let j = 0; j < ret.clusters.length; ++j)
		{
			if (camDist(ret.xav[j], ret.yav[j], __X, __Y) < size)
			{
				n = ret.clusters[j].length;
				ret.clusters[j][n] = list[i];
				ret.xav[j] = Math.floor((n * ret.xav[j] + __X) / (n + 1));
				ret.yav[j] = Math.floor((n * ret.yav[j] + __Y) / (n + 1));
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
			ret.xav[n] = __X;
			ret.yav[n] = __Y;
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
	for (let i = 1; i < __CAM_MAX_PLAYERS; ++i)
	{
		setPower(__CAM_AI_POWER, i);
	}
}

function __camRankStringToNumber(rankName)
{
	if (!camDef(rankName))
	{
		camDebug("Undefined parameter");
		return 0;
	}
	if (typeof rankName !== "string")
	{
		camDebug("Please specify rank as a string");
		return 0;
	}
	let rank = 0;
	switch (rankName.toLowerCase())
	{
		case "rookie": rank = 0; break;
		case "green": rank = 1; break;
		case "trained": rank = 2; break;
		case "regular": rank = 3; break;
		case "professional": rank = 4; break;
		case "veteran": rank = 5; break;
		case "elite": rank = 6; break;
		case "special": rank = 7; break;
		case "hero": rank = 8; break;
		default: camDebug("Unknown rank encountered");
	}
	return rank;
}

function __camGetExpRangeLevel(useCommanderRanks)
{
	if (!camDef(useCommanderRanks))
	{
		useCommanderRanks = false;
	}
	const ranks = {
		rookie: camGetRankThreshold("rookie", useCommanderRanks),
		green: camGetRankThreshold("green", useCommanderRanks),
		trained: camGetRankThreshold("trained", useCommanderRanks),
		regular: camGetRankThreshold("regular", useCommanderRanks),
		professional: camGetRankThreshold("professional", useCommanderRanks),
		veteran: camGetRankThreshold("veteran", useCommanderRanks),
		elite: camGetRankThreshold("elite", useCommanderRanks),
		special: camGetRankThreshold("special", useCommanderRanks),
		hero: camGetRankThreshold("hero", useCommanderRanks)
	};
	let exp = [];
	switch (__camExpLevel)
	{
		case 0: // fall-through
		case 1:
			exp = [ranks.rookie, ranks.rookie];
			break;
		case 2:
			exp = [ranks.green, ranks.trained, ranks.regular];
			break;
		case 3:
			exp = [ranks.trained, ranks.regular, ranks.professional];
			break;
		case 4:
			exp = [ranks.regular, ranks.professional, ranks.veteran];
			break;
		case 5:
			exp = [ranks.professional, ranks.veteran, ranks.elite];
			break;
		case 6:
			exp = [ranks.veteran, ranks.elite, ranks.special];
			break;
		case 7:
			exp = [ranks.elite, ranks.special, ranks.hero];
			break;
		case 8:
			exp = [ranks.special, ranks.hero];
			break;
		case 9:
			exp = [ranks.hero, ranks.hero];
			break;
		default:
			__camExpLevel = 0;
			exp = [ranks.rookie, ranks.rookie];
	}
	return exp;
}

function camSetDroidExperience(droid)
{
	if (droid.droidType === DROID_REPAIR || droid.droidType === DROID_CONSTRUCT || camIsTransporter(droid))
	{
		return;
	}
	if (droid.player === CAM_HUMAN_PLAYER)
	{
		return;
	}
	const __CMD_RANK = (droid.droidType === DROID_COMMAND || droid.droidType === DROID_SENSOR);
	const expRange = __camGetExpRangeLevel(__CMD_RANK);
	const __EXP = expRange[camRand(expRange.length)];
	setDroidExperience(droid, __EXP);
}

// Only to prevent prebuilt units from team Gamma on Gamma 6 from having the NavGunSensor.
function __camRemoveNavGunSensorResearch()
{
	if (camDiscoverCampaign() !== __CAM_GAMMA_CAMPAIGN_NUMBER)
	{
		return;
	}
	for (let i = 0; i < __CAM_MAX_PLAYERS; ++i)
	{
		if (i !== CAM_NEXUS && getResearch("R-Sys-NEXUSsensor", i).done)
		{
			completeResearch("R-Sys-NEXUSsensorUndo", i, true);
		}
	}
}
