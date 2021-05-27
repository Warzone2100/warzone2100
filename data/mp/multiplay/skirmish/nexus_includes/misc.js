
//Check is something has been defined. DO NOT PASS GAME OBJECTS!
function defined(what)
{
	return (typeof(what) !== "undefined");
}

//A simple throttle function to make sure something is not executed too much.
function throttleThis(throttleThis, time)
{
	if (!defined(throttleThis))
	{
		debugMessage("throttleThis. Undefined throttle name.");
		return true; // Seems like a good fail-safe
	}

	if (!defined(time))
	{
		time = 2000;
	}

	if (!defined(debugGetCallerFuncObject().throttleTimes))
	{
		debugGetCallerFuncObject().throttleTimes = {};
	}

	if (!defined(debugGetCallerFuncObject().throttleTimes[throttleThis]))
	{
		debugGetCallerFuncObject().throttleTimes[throttleThis] = gameTime;
		return false;
	}

	if (gameTime - debugGetCallerFuncObject().throttleTimes[throttleThis] < time)
	{
		return true;
	}

	debugGetCallerFuncObject().throttleTimes[throttleThis] = gameTime;

	return false;
}

function random(max)
{
	return (max <= 0) ? 0 : Math.floor(Math.random() * max);
}

function getRealPower(player)
{
	if (!defined(player))
	{
		player = me;
	}

	return (playerPower(player) - queuedPower(player));
}

function sortByDistToBase(obj1, obj2)
{
	var dist1 = distBetweenTwoPoints(BASE.x, BASE.y, obj1.x, obj1.y);
	var dist2 = distBetweenTwoPoints(BASE.x, BASE.y, obj2.x, obj2.y);

	return (dist1 - dist2);
}

function coordinatesOutOfBounds(x, y)
{
	return ((x < DARK_ZONE_TILES)
		|| (x > mapWidth - DARK_ZONE_TILES)
		|| (y < DARK_ZONE_TILES)
		|| (y > mapHeight - DARK_ZONE_TILES));
}

function arrayContains(what, array)
{
	if (!defined(what) || !defined(array))
	{
		debugMessage("arrayContains. Undefined parameters.");
		return false;
	}

	for (var i = 0, len = array.length; i < len; ++i)
	{
		if (what === array[i])
		{
			return true;
		}
	}

	return false;
}

function groupNearCoordinate(group, loc, range)
{
	if (!defined(group))
	{
		debugMessage("groupNearCoordinate. Undefined group.");
		return false;
	}

	if (!defined(loc))
	{
		debugMessage("groupNearCoordinate. Undefined location.");
		return false;
	}

	if (!defined(loc.x) || !defined(loc.y))
	{
		debugMessage("groupNearCoordinate. Undefined location coordinate.");
		return false;
	}

	if (!defined(range))
	{
		range = 4;
	}

	var count = 0;
	var members = enumGroup(group);

	for (var i = 0, len = members.length; i < len; ++i)
	{
		var dr = members[i];

		if (distBetweenTwoPoints(dr.x, dr.y, loc.x, loc.y) <= range)
		{
			++count;
		}
	}

	return (count >= Math.floor(members.length / 2)); //most are near this coordinate
}

//Check if all the members of a group can reach a coordinate.
function groupCanReach(group, x, y)
{
	if (!defined(group))
	{
		debugMessage("groupCanReach. Undefined group.");
		return false;
	}

	if (!defined(x) || !defined(y))
	{
		debugMessage("groupCanReach. Undefined location coordinate.");
		return false;
	}

	var members = enumGroup(group);
	var len = members.length;

	for (var i = 0; i < len; ++i)
	{
		var dr = members[i];

		if (!droidCanReach(dr, x, y))
		{
			return false;
		}
	}

	return true;
}

// keep details about the size and position of the ai players base if Nexus. Otherwise,
// return an object exactly like baseLimits for other players.
function baseDetails(player)
{
	if (!defined(player))
	{
		player = me;
	}

	var baseRadius = 4;
	var tmpBase = {x1: mapWidth, y1: mapHeight, x2: 0, y2: 0};

	for (var i = 0, len = STANDARD_BASE_STRUCTURES.length; i < len; ++i)
	{
		var sObjs = enumStruct(player, STANDARD_BASE_STRUCTURES[i]);

		for (var j = 0, len2 = sObjs.length; j < len2; ++j)
		{
			var struct = sObjs[j];

			if (struct.x < tmpBase.x1)
			{
				tmpBase.x1 = struct.x;
			}
			if (struct.x > tmpBase.x2)
			{
				tmpBase.x2 = struct.x;
			}
			if (struct.y < tmpBase.y1)
			{
				tmpBase.y1 = struct.y;
			}
			if (struct.y > tmpBase.y2)
			{
				tmpBase.y2 = struct.y;
			}

			var result = distBetweenTwoPoints(BASE.x, BASE.y, struct.x, struct.y);

			if (result > baseRadius)
			{
				baseRadius = result;
			}
		}
	}

	// A map should have these coordinates at least...
	const DUMMY_X = 4;
	const DUMMY_Y = 4;
	// Extend the radius a few tiles for each limit
	var extraTilesBase = 10; //6
	var x1 = tmpBase.x1 - extraTilesBase;
	var y1 = tmpBase.y1 - extraTilesBase;
	var x2 = tmpBase.x2 + extraTilesBase;
	var y2 = tmpBase.y2 + extraTilesBase;
	var rad = baseRadius + extraTilesBase;

	if (coordinatesOutOfBounds(x1, DUMMY_Y))
	{
		x1 = DARK_ZONE_TILES;
	}
	if (coordinatesOutOfBounds(DUMMY_X, y1))
	{
		y1 = DARK_ZONE_TILES;
	}
	if (coordinatesOutOfBounds(x2 - DARK_ZONE_TILES, DUMMY_Y))
	{
		x2 = mapWidth - DARK_ZONE_TILES;
	}
	if (coordinatesOutOfBounds(DUMMY_X, y2 - DARK_ZONE_TILES))
	{
		y2 = mapHeight - DARK_ZONE_TILES;
	}

	if (player === me)
	{
		baseLimits.x1 = x1;
		baseLimits.x2 = x2;
		baseLimits.y1 = y1;
		baseLimits.y2 = y2;
		baseLimits.radius = rad;
	}
	else
	{
		return {x1: x1, y1: y1, x2: x2, y2: y2, radius: rad};
	}

	return undefined;
}

//Check if some coordinate is in base. An optional third argument can be passed
//to check a non-Nexus base (such as returned from baseDetails()).
function insideBase(x, y, object)
{
	if (!defined(x) || !defined(y))
	{
		debugMessage("insideBase. Undefined x or y coordinate.");
		return false;
	}

	var base = defined(object) ? object : baseLimits;

	if (!defined(base.x1) || !defined(base.y1) || !defined(base.x2) || !defined(base.y2))
	{
		debugMessage("insideBase. Base limits not yet calculated!");
		return false;
	}

	return !((x < base.x1) || (x > base.x2) || (y < base.y1) || (y > base.y2));
}

// Check if the center of the gate is in the base area.
function gateInBase(gate)
{
	if (!defined(gate))
	{
		debugMessage("gateInBase. Undefined gate parameter.");
		return false;
	}

	if (!defined(gate.x1) || !defined(gate.y1) || !defined(gate.x2) || !defined(gate.y2))
	{
		debugMessage("gateInBase. Potentially not a gate object?");
		return false;
	}

	var gx = Math.floor((gate.x1 + gate.x2) / 2);
	var gy = Math.floor((gate.y1 + gate.y2) / 2);

	return insideBase(gx, gy);
}

// Set the chance of choosing a VTOL personality.
function setTechBranch()
{
	const SMALL_SIDE_LENGTH = 90;
	const MEDIUM_SIDE_LENGTH = 125;
	const LARGE_SIDE_LENGTH =  165;
	var chance = 0;

	if (mapWidth >= LARGE_SIDE_LENGTH || mapHeight >= LARGE_SIDE_LENGTH)
	{
		chance = 90;
	}
	else if (mapWidth >= MEDIUM_SIDE_LENGTH || mapHeight >= MEDIUM_SIDE_LENGTH)
	{
		chance = 45;
	}
	else if (mapWidth >= SMALL_SIDE_LENGTH || mapHeight >= SMALL_SIDE_LENGTH)
	{
		chance = 20;
	}
	else
	{
		chance = 10;
	}

	if (random(100) < chance)
	{
		chooseVtolPersonality();
		debugMessage("going air.");
	}
	else
	{
		chooseLandPersonality();
		debugMessage("going land.");
	}
}

function threatInRange(x, y, player, scanRadius, visible)
{
	if (!defined(x) || !defined(y))
	{
		debugMessage("threatInRange. Undefined x or y coordinate.");
		return true; // Seems to be the best fail-safe
	}

	if (!defined(player))
	{
		player = ENEMIES;
	}

	if (!defined(scanRadius))
	{
		scanRadius = 7;
	}

	if (!defined(visible))
	{
		visible = false;
	}

	var stuff = enumRange(x, y, scanRadius, player, visible);

	for (var i = 0, l = stuff.length; i < l; ++i)
	{
		var obj = stuff[i];

		if (obj.type !== FEATURE && obj.player !== me && !allianceExistsBetween(me, obj.player))
		{
			if (obj.type === DROID && obj.droidType !== DROID_CONSTRUCT)
			{
				return true; //combat unit
			}
			else if (obj.type === STRUCTURE && obj.stattype !== RESOURCE_EXTRACTOR)
			{
				if (obj.stattype === DEFENSE && obj.status === BUILT)
				{
					return true; // Completed defense structure
				}
			}
		}
	}

	return false;
}

function numDroidsInBase(player)
{
	var len = 0;

	if (!defined(player))
	{
		player = me;
	}

	enumDroid(player).forEach(function (d) {
		if (insideBase(d.x, d.y))
		{
			++len;
		}
	});

	return len;
}

function numWeapObjectsInRange(x, y, player, scanRadius, visible)
{
	if (!defined(x) || !defined(y))
	{
		debugMessage("numWeapObjectsInRange. Undefined x or y coordinate.");
		return Infinity; // Seems to be the best fail-safe
	}

	if (!defined(player))
	{
		player = ALLIES;
	}

	if (!defined(scanRadius))
	{
		scanRadius = 5;
	}

	if (!defined(visible))
	{
		visible = false;
	}

	var stuff = enumRange(x, y, scanRadius, player, visible);
	var count = {structures: 0, droids: 0, safe: true};

	for (var i = 0, l = stuff.length; i < l; ++i)
	{
		var obj = stuff[i];

		if (obj.type === STRUCTURE && obj.stattype === DEFENSE && obj.status === BUILT)
		{
			count.structures = count.structures + 1;
		}
		else if (obj.type === DROID && (obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG) && obj.droidType !== DROID_CONSTRUCT)
		{
			count.droids = count.droids + 1;
		}
	}

	if ((count.structures + count.droids) > 0)
	{
		count.safe = false;
	}

	return count;
}

function numGroupSameOrder(group, order)
{
	if (!defined(group))
	{
		debugMessage("numGroupSameOrder. Undefined group number.");
		return Infinity; // Seems to be a good fail-safe.
	}

	if (!defined(order))
	{
		debugMessage("numGroupSameOrder. Undefined order.");
		return Infinity; // Seems to be a good fail-safe.
	}

	var numSame = 0;
	var grp = enumGroup(group);

	for (var i = 0, len = grp.length; i < len; ++i)
	{
		if (grp[i].order === order)
		{
			++numSame;
		}
	}

	return numSame;
}

function isHumanPlayer(player)
{
	if (!defined(player))
	{
		debugMessage("isHumanPlayer. Undefined player parameter.");
		return false; //Seems like a good fail-safe.
	}

	return playerData[player].isHuman;
}

function secondsToMilliseconds(seconds)
{
	if (!defined(seconds))
	{
		debugMessage("secondsToMilliseconds. Undefined parameter.");
		return undefined;
	}

	return Math.floor(seconds * 1000);
}

function minutesToMilliseconds(minutes)
{
	if (!defined(minutes))
	{
		debugMessage("minutesToMilliseconds. Undefined parameter.");
		return undefined;
	}

	return Math.floor(minutes * secondsToMilliseconds(60));
}

function friendlyPlayer(player)
{
	if (!defined(player))
	{
		debugMessage("friendlyPlayer. Undefined player parameter.");
		return false;
	}

	return (player === me || allianceExistsBetween(me, player));
}

function getPlayerName(player)
{
	if (!defined(player))
	{
		player = me;
	}

	return playerData[player].name;
}

function groupCoordinateAverage(group)
{
	if (!defined(group))
	{
		debugMessage("groupCoordinateAverage. Undefined group.");
		return {x: 0, y: 0};
	}

	var droids = enumGroup(group);
	var len = droids.length;
	var xTotal = 0;
	var yTotal = 0;

	for (var i = 0; i < len; ++i)
	{
		xTotal += droids[i].x;
		yTotal += droids[i].y;
	}

	return ({x: Math.floor(xTotal / len), y: Math.floor(yTotal / len)});
}

function numAlliesInBase()
{
	var counts = numWeapObjectsInRange(BASE.x, BASE.y, ALLIES, BASE_THREAT_RANGE, true);

	return (counts.droids + Math.floor(counts.structures / 3));
}

function numEnemiesInBase()
{
	var counts = numWeapObjectsInRange(BASE.x, BASE.y, ENEMIES, BASE_THREAT_RANGE, true);

	return (counts.droids + Math.floor(counts.structures / 4));
}

function baseInTrouble()
{
	var friendlyForce = numAlliesInBase();
	var enemyForce = numEnemiesInBase();

	return (enemyForce > 0 && enemyForce >= friendlyForce);
}

function initPersonalityData(personality)
{
	var factories = enumStruct(me, FACTORY);
	var trucks = enumDroid(me, DROID_CONSTRUCT);
	var i = 0;
	var len = 0;

	helpInfo = {
		lastHelpTime: [],
		lastHelpRequest: [],
		lastHelpPlayer: undefined,
		helpTimeout: [],
		location: [],
	};
	defendInfo = {
		location: {x: undefined, y: undefined},
		timeout: undefined,
		startTime: undefined,
		moveType: undefined,
		busy: false,
	};
	scoutInfo = {
		location: {x: undefined, y: undefined}, //Where the scouts are going to.
		base: {x: undefined, y: undefined}, //Where to expand our scouting from.
		expandTileRate: 2, //How many tiles to add to the exploration depth every minute.
		currentRadius: 0, //How far we will explore away from "base".
	};
	groups = {
		attackers: newGroup(),
		defenders: newGroup(),
		scouts: newGroup(),
		repairs: newGroup(),
		baseBuilders: newGroup(),
		oilBuilders: newGroup(),
		vtolDefenders: newGroup(),
		vtolAttackers: [],
	};
	baseLimits = {
		x1: undefined,
		y1: undefined,
		x2: undefined,
		y2: undefined,
		radius: 2,
	};

	resetTargetData();

	if (defined(personality))
	{
		branch = personality;
	}
	else
	{
		setTechBranch();
	}

	for (i = 0, len = nexusBranch[branch].numVtolGroups; i < len; ++i)
	{
		groups.vtolAttackers.push(newGroup()); // An array of group numbers
		targetInfo.vtolGroupWhat.push(undefined);
	}

	for (i = 0; i < maxPlayers; ++i)
	{
		helpInfo.lastHelpTime.push(undefined);
		helpInfo.lastHelpRequest.push(undefined);
		helpInfo.helpTimeout.push(undefined);
		helpInfo.location.push({x: undefined, y: undefined});
	}

	//Now set the scout base. Seems pointless but oh well, original.
	if (factories.length > 0)
	{
		var fac = factories[0];
		scoutInfo.base.x = fac.x;
		scoutInfo.base.y = fac.y;
	}
	else
	{
		scoutInfo.base.x = BASE.x;
		scoutInfo.base.y = BASE.y;
	}

	for (i = 0, len = trucks.length; i < len; ++i)
	{
		var droid = trucks[i];
		groupAdd(groups.baseBuilders, droid);
	}

	//If switching the personality branch then got to readd the units to the groups again.
	enumDroid(me).forEach(function(droid) {
		if (droid.droidType !== DROID_CONSTRUCT)
		{
			eventDroidBuilt(droid, null);
		}
	});
}
