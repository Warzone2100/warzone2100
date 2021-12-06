
function haveStructure(stat, player)
{
	if (!defined(stat))
	{
		debugMessage("haveStructure. No stat parameter specified.");
		return false;
	}

	if (!defined(player))
	{
		player = me;
	}

	return (countStruct(stat, player) > 0);
}

function hasPowerSource(player)
{
	if (!defined(player))
	{
		player = me;
	}

	return (haveStructure(BASE_STRUCTURES.gens, player) && haveStructure(BASE_STRUCTURES.derricks, player));
}

// The structInfo parameter can either be the .stattype property of a structure or
// a special propery like .isSensor passed as a string. See the switch for all current
// options.
//NOTE: Returns an array of objects = {type: object.type, player: object.player, id: object.id}
function structuresBuiltInRange(structInfo, x, y, playerType, range, visible)
{
	if (!defined(structInfo))
	{
		debugMessage("structuresBuiltInRange. No stat specified.");
		return [];
	}

	if (!defined(x) || !defined(y))
	{
		debugMessage("structuresBuiltInRange. Invalid coordinates specified.");
		return [];
	}

	if (!defined(playerType))
	{
		playerType = ENEMIES;
	}

	if (!defined(range))
	{
		range = 5;
	}

	if (!defined(visible))
	{
		visible = true;
	}

	var infoStore = [];
	var objects = enumRange(x, y, range, playerType, visible);
	var isString = (typeof(structInfo) === "string");
	var isNum = (typeof(structInfo) === "number");

	if (!isNum && !isString)
	{
		debugMessage("structuresBuiltInRange. Invalid stat variable type.");
		return [];
	}

	for (var i = 0, len = objects.length; i < len; ++i)
	{
		var obj = objects[i];
		var tmp = {type: obj.type, player: obj.player, id: obj.id};

		if (obj.type !== STRUCTURE)
		{
			continue;
		}

		if (isString)
		{
			switch (structInfo)
			{
				case "isSensor": if (obj.isSensor) { infoStore.push(tmp); } break;
				case "isCB": if (obj.isCB) { infoStore.push(tmp); } break;
				case "canHitAir": if (obj.canHitAir) { infoStore.push(tmp); } break;
				case "canHitGround": if (obj.canHitGround) { infoStore.push(tmp); } break;
				case "isRadarDetector": if (obj.isRadarDetector) { infoStore.push(tmp); } break;
				case "hasIndirect": if (obj.hasIndirect) { infoStore.push(tmp); } break;
				default:
					debugMessage("structuresBuiltInRange. Unknown special object property." + structInfo);
					break;
			}
		}
		else if (isNum && obj.stattype === structInfo)
		{
			infoStore.push(tmp);
		}
	}

	return infoStore;
}

function conCanHelp(droidID, x, y, help)
{
	if (!defined(x) || !defined(y))
	{
		return false;
	}

	if (!defined(help))
	{
		help = true;
	}

	var droid = getObject(DROID, me, droidID);

	if (droid === null)
	{
		return false;
	}

	return (droid.order !== DORDER_BUILD &&
		droid.order !== DORDER_LINEBUILD &&
		(!help || (droid.order !== DORDER_HELPBUILD)) &&
		droid.order !== DORDER_RTR &&
		droid.order !== DORDER_DEMOLISH &&
		droidCanReach(droid, x, y)
	);
}

function grabTrucksAndBuild(stat, location, tileRange, maxBlockingTiles, group)
{
	if (!defined(stat))
	{
		debugMessage("grabTrucksAndBuild. Tried to build without stat.");
		return false;
	}

	if (!isStructureAvailable(stat))
	{
		return false;
	}

	if (!defined(location))
	{
		location = {x: BASE.x, y: BASE.y, force: false};
	}

	if (!defined(tileRange))
	{
		tileRange = 10;
	}

	if (!defined(maxBlockingTiles))
	{
		maxBlockingTiles = 0;
	}

	if (!defined(group))
	{
		group = groups.baseBuilders;
	}

	var loc2;
	var closestDroid;
	var closestDistance = Infinity;
	var numHelpDroids = 0;
	var droids = enumGroup(group);

	for (var i = 0, l = droids.length; i < l; ++i)
	{
		var droid = droids[i];

		if (conCanHelp(droid.id, droid.x, droid.y, true))
		{
			//Workaround pickStructLocation() disallowing building exactly onto some positions. Mostly
			//for gateways which are one of those.
			if (defined(location.force) &&
				location.force &&
				propulsionCanReach(droid.propulsion, droid.x, droid.y, location.x, location.y) &&
				getObject(location.x, location.y) === null)
			{
				if (((getObject(location.x - 1, location.y) === null ? 0 : 1) +
					(getObject(location.x, location.y - 1) === null ? 0 : 1) +
					(getObject(location.x + 1, location.y) === null ? 0 : 1) +
					(getObject(location.x, location.y + 1) === null ? 0 : 1) +
					(getObject(location.x - 1, location.y - 1) === null ? 0 : 1) +
					(getObject(location.x + 1, location.y - 1) === null ? 0 : 1) +
					(getObject(location.x - 1, location.y + 1) === null ? 0 : 1) +
					(getObject(location.x + 1, location.y + 1) === null ? 0 : 1)) <= maxBlockingTiles)
				{
					loc2 = location;
				}
			}
			else
			{
				loc2 = pickStructLocation(droid, stat, location.x, location.y, maxBlockingTiles);
			}

			if (loc2 && numWeapObjectsInRange(loc2.x, loc2.y, ENEMIES, 7, false).safe)
			{
				var currDist = distBetweenTwoPoints(loc2.x, loc2.y, droid.x, droid.y);

				if (currDist < tileRange)
				{
					orderDroidBuild(droid, DORDER_BUILD, stat, loc2.x, loc2.y);
					++numHelpDroids; //Original Nexus assumes!
				}
				else if (currDist < closestDistance)
				{
					closestDistance = currDist;
					closestDroid = droid;
				}
			}
		}
	}

	//Found none within help radius, so force someone to go long distance traveling
	if (numHelpDroids === 0 && closestDroid)
	{
		orderDroidBuild(closestDroid, DORDER_BUILD, stat, loc2.x, loc2.y); //You, book a plane ticket and go!
		return true;
	}

	return (numHelpDroids > 0);
}

function bringBackOilTrucks()
{
	var builders = enumGroup(groups.oilBuilders);

	for (var i = 0, len = builders.length; i < len; ++i)
	{
		if (builders[i].order !== DORDER_RECYCLE &&
			builders[i].order !== DORDER_RTB)
		{
			orderDroid(builders[i], DORDER_RTB);
		}
	}
}

function skipOilGrabIfEasy()
{
	if (difficulty === EASY)
	{
		var myDerrickCount = enumStruct(me, BASE_STRUCTURES.derricks).filter(function(obj) {
			return obj.status === BUILT;
		}).length;
		var enemies = getAliveEnemyPlayers();

		for (var i = 0, len = enemies.length; i < len; ++i)
		{
			if (myDerrickCount >= 3 && (myDerrickCount + 1) >= countStruct(BASE_STRUCTURES.derricks, enemies[i]) && enemies[i] !== scavengerPlayer)
			{
				bringBackOilTrucks();
				return true;
			}
		}
	}

	return false;
}

function buildDerrick()
{
	if (skipOilGrabIfEasy())
	{
		return;
	}

	var numBusy = 0;
	var droids = enumGroup(groups.oilBuilders);
	var drLen = droids.length;
	droids.forEach(function(d) {
		if (!conCanHelp(d.id, d.x, d.y, false))
		{
			++numBusy;
		}
	});

	//Don't check stuff if all our oil getters are busy!
	if (numBusy === drLen)
	{
		return;
	}

	var oils = enumFeature(ALL_PLAYERS, FEATURE_STATS.oils).sort(sortByDistToBase);

	for (var i = 0, oilLen = oils.length; i < oilLen; ++i)
	{
		var targetOil;
		var bestDroid;
		var bestDist = Infinity;

		//And don't waste time if all trucks became busy.
		if (numBusy == drLen)
		{
			break;
		}

		for (var j = 0; j < drLen; ++j)
		{
			var oil = oils[i];
			var droid = droids[j];

			if (!droid.busy &&
				conCanHelp(droid.id, oil.x, oil.y, false) &&
				!threatInRange(oil.x, oil.y, ENEMIES, OIL_THREAT_RADIUS, false))
			{
				var dist = distBetweenTwoPoints(droid.x, droid.y, oil.x, oil.y);

				if (dist < bestDist)
				{
					targetOil = oil;
					bestDroid = droid;
					bestDist = dist;
				}
			}
		}

		//TODO: unthrottle once we can access a blueprint location OR where the droid is ordered to go
		if (bestDroid && !throttleThis("oil" + targetOil.y * mapWidth * targetOil.x, secondsToMilliseconds(50)))
		{
			bestDroid.busy = true;
			numBusy += 1;
			orderDroidBuild(bestDroid, DORDER_BUILD, BASE_STRUCTURES.derricks, targetOil.x, targetOil.y);
		}
	}
}

//Return the best defenses within some range. Assumes the best are at the end of the
//array.
function getBestRandomDefense(array, range)
{
	if (!defined(array))
	{
		debugMessage("getBestRandomDefense. Undefined array parameter.");
		return undefined;
	}

	if (!(array instanceof Array))
	{
		debugMessage("getBestRandomDefense. Expecting an array.");
		return undefined;
	}

	if (!defined(range))
	{
		range = 3; //Eh
	}

	var len = array.length;

	if (range >= len)
	{
		debugMessage("getBestRandomDefense. Range exceeds length of array. Range: " + range);
		range = (len - 1);
	}

	var count = len - 1;
	var count2 = 0;
	var defenses = [];

	while ((count2 < range) && (count >= 0))
	{
		if (isStructureAvailable(array[count]))
		{
			defenses.push(array[count]);
			++count2;
		}

		--count;
	}

	if (count2 > 0)
	{
		return defenses[random(count2)];
	}

	return undefined;
}

function buildDefenses()
{
	if (!isStructureAvailable(BASE_STRUCTURES.sensors[0]))
	{
		return;
	}

	if (sensorBuild())
	{
		return;
	}

	if (derrickDefense())
	{
		return;
	}
}

//Return coordinates or an undefended object. Undefined if none found.
function findUndefendedObjectLoc(whatToScan, defenses, objectLimit, tower)
{
	if (!defined(whatToScan) || !defined(defenses))
	{
		debugMessage("findUndefendedObjectLoc. Too many undefined parameters.");
		return undefined;
	}

	if (!defined(objectLimit))
	{
		objectLimit = 3;
	}

	if (!defined(tower))
	{
		tower = false;
	}

	const SCAN_RADIUS = 6;
	const MAX_BUILDERS = 2;
	var objectsToCheck = enumStruct(me, whatToScan);

	for (var i = 0, len = objectsToCheck.length; i < len; ++i)
	{
		var obj = objectsToCheck[i];

		if (threatInRange(obj.x, obj.y, ENEMIES, 3, false) ||
			!safeDest(me, obj.x, obj.y) ||
			numBuildSameBuilding(obj.x, obj.y) >= MAX_BUILDERS)
		{
			continue;
		}

		if (tower && structuresBuiltInRange("isSensor", obj.x, obj.y, me, SCAN_RADIUS, true).length > 0)
		{
			continue; //Stop building sensors here!
		}

		if (structuresBuiltInRange("hasIndirect", obj.x, obj.y, me, SCAN_RADIUS, true).length < objectLimit)
		{
			return {x: obj.x, y: obj.y};
		}
	}

	return undefined;
}

function derrickDefense()
{
	var best = bestStructureIn(STANDARD_INCENDIARIES);
	var success = false;
	var loc = findUndefendedObjectLoc(BASE_STRUCTURES.sensors, STANDARD_INCENDIARIES, 3, false);

	if (defined(loc) && defined(best) && grabTrucksAndBuild(best, loc, 6, 4, groups.oilBuilders))
	{
		success = true;
	}

	return success;
}

function sensorBuild()
{
	var best = bestStructureIn(BASE_STRUCTURES.sensors);
	var success = false;
	var loc = findUndefendedObjectLoc(BASE_STRUCTURES.derricks, BASE_STRUCTURES.sensors, Infinity, true);

	if (defined(loc) && defined(best) && grabTrucksAndBuild(best, loc, 6, 4, groups.oilBuilders))
	{
		success = true;
	}

	return success;
}

function buildPowerGenerators()
{
	if (!isStructureAvailable(BASE_STRUCTURES.gens))
	{
		debugMessage("buildPowerGenerators. Power generator is not available.");
		return false;
	}

	var unusedDerricks = (countStruct(BASE_STRUCTURES.derricks) - countStruct(BASE_STRUCTURES.gens) * 4);

	return ((!countStruct(BASE_STRUCTURES.gens) || (unusedDerricks > 0)) && grabTrucksAndBuild(BASE_STRUCTURES.gens, BASE, 12, 0));
}

function buildMinimumBase()
{
	for (var i = 0, len = STANDARD_BUILD_FUNDAMENTALS.length; i < len; ++i)
	{
		var s = STANDARD_BUILD_FUNDAMENTALS[i];

		if (countStruct(s) < 1 && grabTrucksAndBuild(s, BASE, 12, 0))
		{
			return true;
		}
	}

	return false;
}

//One of each structure in STANDARD_BUILD_FUNDAMENTALS and any additional power generators.
function buildBase()
{
	baseDetails(me);

	if (finishStructs())
	{
		return;
	}

	if (buildMinimumBase())
	{
		return;
	}

	if (buildPowerGenerators())
	{
		return;
	}

	if (buildModules())
	{
		return;
	}

	if (buildGates())
	{
		return;
	}

	if (rebuildStructures())
	{
		return;
	}

	if (random(100) < 50)
	{
		if (buildExpand())
		{
			return;
		}
	}

	if (random(100) < 50)
	{
		if (buildDefenses())
		{
			return;
		}

	}
}

// Structures that a personality can build before the default build order.
function personalityBuildOrder()
{
	var success = false;

	for (var i = 0, len = nexusBranch[branch].buildOrder.length; i < len; ++i)
	{
		if (success)
		{
			break;
		}

		var obj = nexusBranch[branch].buildOrder[i];
		var count;

		switch (obj.stat)
		{
			case "F":
				count = countStruct(BASE_STRUCTURES.factories);

				if (count < getStructureLimit(BASE_STRUCTURES.factories, me) && count < obj.count && grabTrucksAndBuild(BASE_STRUCTURES.factories, BASE, 12, 0))
				{
					success = true;
				}
				break;
			case "C":
				count = countStruct(BASE_STRUCTURES.templateFactories);

				if (count < getStructureLimit(BASE_STRUCTURES.templateFactories, me) && count < obj.count && grabTrucksAndBuild(BASE_STRUCTURES.templateFactories, BASE, 12, 0))
				{
					success = true;
				}
				break;
			case "V":
				count = countStruct(BASE_STRUCTURES.vtolFactories);

				if (count < getStructureLimit(BASE_STRUCTURES.vtolFactories, me) && count < obj.count && grabTrucksAndBuild(BASE_STRUCTURES.vtolFactories, BASE, 12, 0))
				{
					success = true;
				}
				break;
			case "R":
				count = countStruct(BASE_STRUCTURES.labs);

				if (count < getStructureLimit(BASE_STRUCTURES.labs, me) && count < obj.count && grabTrucksAndBuild(BASE_STRUCTURES.labs, BASE, 12, 0))
				{
					success = true;
				}
				break;
			default:
				debugMessage("personalityBuildOrder. Encountered an unknown structure alias: " + obj.stat);
				success = false;
		}
	}

	return success;
}

function buildMaxsBasedOnDerricks()
{
	const DERRICK_COUNT = countStruct(BASE_STRUCTURES.derricks, me);
	var amount;

	if (DERRICK_COUNT <= 8)
	{
		amount = 2;
	}
	else if (DERRICK_COUNT <= 12)
	{
		amount = 3;
	}
	else if (DERRICK_COUNT <= 16)
	{
		amount = 4;
	}
	else
	{
		amount = 5;
	}

	return amount;
}

function buildMaximums()
{
	var amount = buildMaxsBasedOnDerricks();
	var currStructCount = countStruct(BASE_STRUCTURES.labs);

	if (currStructCount < getStructureLimit(BASE_STRUCTURES.labs, me) && currStructCount < amount && grabTrucksAndBuild(BASE_STRUCTURES.labs, BASE, 25, 0))
	{
		return true;
	}

	currStructCount = countStruct(BASE_STRUCTURES.factories);
	if (currStructCount < getStructureLimit(BASE_STRUCTURES.factories, me) && currStructCount < amount && grabTrucksAndBuild(BASE_STRUCTURES.factories, BASE, 25, 0))
	{
		return true;
	}

	currStructCount = countStruct(BASE_STRUCTURES.repairBays);
	if (currStructCount < getStructureLimit(BASE_STRUCTURES.repairBays, me) && currStructCount < amount && grabTrucksAndBuild(BASE_STRUCTURES.repairBays, BASE, 25, 0))
	{
		return true;
	}

	currStructCount = countStruct(BASE_STRUCTURES.templateFactories);
	if (currStructCount < getStructureLimit(BASE_STRUCTURES.templateFactories, me) && currStructCount < amount && grabTrucksAndBuild(BASE_STRUCTURES.templateFactories, BASE, 25, 0))
	{
		return true;
	}

	currStructCount = countStruct(BASE_STRUCTURES.vtolFactories);
	if (currStructCount < getStructureLimit(BASE_STRUCTURES.vtolFactories, me) && currStructCount < amount && grabTrucksAndBuild(BASE_STRUCTURES.vtolFactories, BASE, 25, 0))
	{
		return true;
	}

	return false;
}

// build other stuff, grow the base slowly...
function buildExpand()
{
	if (getRealPower(me) < LOW_POWER)
	{
		return false; // do not expand base with low power
	}

	var count = countStruct(BASE_STRUCTURES.vtolPads);

	if (count < getStructureLimit(BASE_STRUCTURES.vtolPads, me) &&
		Math.floor((count * 4) / 3) <= numVtolUnits &&
		grabTrucksAndBuild(BASE_STRUCTURES.vtolPads, BASE, 25, 4))
	{
		return true;
	}

	//Do the personality specific build order before going for all the maximums
	if (personalityBuildOrder())
	{
		return true;
	}

	// And do whatever else to get to all the structure limits
	if (gameTime > minutesToMilliseconds(3) && buildMaximums())
	{
		return true;
	}

	return false;
}

function isPowerModule(stat)
{
	const POWER_MODULES = [
		"A0PowMod1",
	];

	return arrayContains(stat, POWER_MODULES);
}

function buildModules()
{
	if (!hasPowerSource(me))
	{
		return false;
	}

	var struct = null;
	var moduleObject = {}; //a copy of nexusBranch[branch].moduleOrder[x]
	var success = false;
	var i = 0;
	var j = 0;
	var len = 0;
	var len2 = 0;

	for (i = 0, len = nexusBranch[branch].moduleOrder.length; i < len; ++i)
	{
		var modObj = nexusBranch[branch].moduleOrder[i];
		var structList = enumStruct(me, modObj.structure);

		if (!isStructureAvailable(modObj.mod))
		{
			continue;
		}

		for (j = 0, len2 = structList.length; j < len2; ++j)
		{
			var s = structList[j];

			if (s.status === BUILT && s.modules < modObj.amount)
			{
				struct = s;
				moduleObject = modObj;
				break;
			}
		}

		if (struct !== null)
		{
			break;
		}
	}

	if (!struct)
	{
		return false;
	}

	if (!isPowerModule(moduleObject.mod) && getRealPower(me) < LOW_POWER && countStruct(BASE_STRUCTURES.derricks) < 12)
	{
		return false; //Everything not a power module costs power
	}

	var builders = enumGroup(groups.baseBuilders);

	for (i = 0, len = builders.length; i < len; ++i)
	{
		var dr = builders[i];

		if (conCanHelp(dr.id, struct.x, struct.y, true) &&
			numBuildSameBuilding(struct.x, struct.y) < moduleObject.maxBuilders &&
			orderDroidBuild(dr, DORDER_BUILD, moduleObject.mod, struct.x, struct.y))
		{
			success = true;
		}

	}

	return success;
}

//FIXME: This is garbage.
function numBuildSameBuilding(x, y)
{
	return false;
	/*
	var numSameBuilding = 0;
	var trucks = enumGroup(groups.baseBuilders);
	for (var i = 0, len = trucks.length; i < len; ++i)
	{
		var dr = trucks[i];
		if (distBetweenTwoPoints(x, y, dr.x, dr.y) <= 1 && conCanHelp(dr.id, x, y, false))
		{
			++numSameBuilding;
		}
	}
	return numSameBuilding;
	*/
}

function finishStructs()
{
	const MAX_REBUILD_DIST = 30; //was 20
	var success = false;
	var trucks = enumGroup(groups.baseBuilders);
	var structures = enumStruct(me);

	for (var i = 0, len = trucks.length; i < len; ++i)
	{
		var dr = trucks[i];

		for (var j = 0, len2 = structures.length; j < len2; ++j)
		{
			var st = structures[j];

			if (st.status === BUILT || st.stattype === DEFENSE || st.stattype === WALL)
			{
				continue;
			}

			//NOTE: Consider stuff more than 20 tiles?
			if ((distBetweenTwoPoints(BASE.x, BASE.y, st.x, st.y) <= MAX_REBUILD_DIST) &&
				conCanHelp(dr.id, st.x, st.y, true) &&
				orderDroidObj(dr, DORDER_HELPBUILD, st))
			{
				success = true;
			}
		}
	}

	return success;
}

//Scan around the oil builders
function protectCloseDerrick()
{
	var trucks = enumGroup(groups.oilBuilders);

	for (var i = 0, len = trucks.length; i < len; ++i)
	{
		var droid = trucks[i];

		if (droid.order === DORDER_RTR || droid.order === DORDER_BUILD || droid.order === DORDER_LINEBUILD)
		{
			continue; //not idle
		}

		if (droid.health >= 85) //was 100
		{
			if (!threatInRange(droid.x, droid.y, ENEMIES, 4, true))
			{
				if (insideBase(droid.x, droid.y) || (numWeapObjectsInRange(droid.x, droid.y, me, 3, false).structures >= 2))
				{
					continue;
				}
				// Ok, at enemy derrick, and nobody has hurt us yet. Start being nasty.
				var loc;
				var choice = getBestRandomDefense(STANDARD_BASIC_DEFENSES, 5);

				if (defined(choice))
				{
					loc = pickStructLocation(droid, choice, droid.x, droid.y, 1);

					if (loc)
					{
						orderDroidBuild(droid, DORDER_BUILD, choice, loc.x, loc.y);
					}
				}

				if (!defined(choice) || !defined(loc))
				{
					orderDroid(droid, DORDER_RTB); // oh, well. nothing more to do here.
				}
			}
			else
			{
				if (!insideBase(droid.x, droid.y) && structuresBuiltInRange(HQ, droid.x, droid.y, me, 5, true).length === 0)
				{
					if (droid.order !== DORDER_RTB)
					{
						orderDroid(droid, DORDER_RTB);
					}
				}
			}
		}
		else if (!insideBase(droid.x, droid.y) && droid.order !== DORDER_RTR)
		{
			// bolt back to base now!
			orderDroid(droid, DORDER_RTB);
		}
	}
}

//Return the best available structure in an array. This assumes the best structures
//are at the end of the array.
function bestStructureIn(array)
{
	if (!defined(array))
	{
		debugMessage("bestStructureIn. No defined parameter.");
		return undefined;
	}

	if (!(array instanceof Array))
	{
		debugMessage("bestStructureIn. Expecting an array as the parameter.");
		return undefined;
	}

	for (var i = array.length - 1; i >= 0; --i)
	{
		var structure = array[i];

		if (isStructureAvailable(structure))
		{
			return structure;
		}
	}

	return undefined;
}

//Picks gates that are of a certain length, relatively empty, and none of its tile reside on water or cliff tiles.
function findSafeGateTile(gate)
{
	const MIN_GATE_SPACE = 2;
	const GATE_MIDDLE = {
		x1: Math.floor((gate.x1 + gate.x2) / 2),
		y1: Math.floor((gate.y1 + gate.y2) / 2),
		x2: Math.ceil((gate.x1 + gate.x2) / 2),
		y2: Math.ceil((gate.y1 + gate.y2) / 2),
	};
	var verticalCheck = false;

	if (gate.x1 === gate.x2)
	{
		verticalCheck = true;

		if (Math.abs(gate.y1 - gate.y2) <= MIN_GATE_SPACE)
		{
			return undefined;
		}
	}
	else if (gate.y1 === gate.y2)
	{
		if (Math.abs(gate.x1 - gate.x2) <= MIN_GATE_SPACE)
		{
			return undefined;
		}
	}
	else
	{
		return undefined; //weird gate.
	}

	for (var j = (verticalCheck ? gate.y1 : gate.x1); j <= (verticalCheck ? gate.y2 : gate.x2); ++j)
	{
		var xg = (verticalCheck ? gate.x1 : j);
		var yg = (verticalCheck ? j : gate.y1);

		//Skip out of bounds tiles
		if (coordinatesOutOfBounds(xg, yg))
		{
			continue;
		}

		var tileType = terrainType(xg, yg);

		if (tileType === TER_WATER || tileType === TER_CLIFFFACE)
		{
			continue; //Ignore gateways interuppted by certain terrain features.
		}

		if (getObject(xg, yg) === null)
		{
			var dist1 = distBetweenTwoPoints(xg, yg, GATE_MIDDLE.x1, GATE_MIDDLE.y1);
			var dist2 = distBetweenTwoPoints(xg, yg, GATE_MIDDLE.x2, GATE_MIDDLE.y2);

			if (dist1 > 0 && dist2 > 0)
			{
				return {x: xg, y: yg};
			}
		}
	}
	return undefined;
}

function buildGates()
{
	for (var i = 0, len = MAP_GATES.length; i < len; ++i)
	{
		var gate = MAP_GATES[i];

		//If not in base or at risk of being shot at then skip it
		if (!gateInBase(gate) || !safeDest(me, gate.x1, gate.y1) || !safeDest(me, gate.x2, gate.y2))
		{
			continue;
		}

		var tile = findSafeGateTile(gate);

		if (!defined(tile))
		{
			continue;
		}

		var loc = {x: tile.x, y: tile.y, force: true};
		var defense = getBestRandomDefense(STANDARD_FORTIFY_DEFENSES, 3);

		if (defined(defense) && grabTrucksAndBuild(defense, loc, 40, 4))
		{
			return true;
		}
	}

	return false;
}

//Rebuild anything in the rebuildQueue
function rebuildStructures(threatRange)
{
	if (!defined(threatRange))
	{
		threatRange = 8;
	}

	for (var i = 0, len = rebuildQueue.length; i < len; ++i)
	{
		var obj = rebuildQueue[i];

		if (getObject(obj.x, obj.y) !== null)
		{
			//Some structure or feature already exists here, or, we built it last time.
			rebuildQueue.splice(i, 1);
			break;
		}

		if (!threatInRange(obj.x, obj.y, ENEMIES, threatRange, false) && safeDest(me, obj.x, obj.y))
		{
			var loc = {x: obj.x, y: obj.y, force: true};

			if (grabTrucksAndBuild(obj.stat, loc, 50, 4))
			{
				return true;
			}
		}
	}

	return false;
}

//Uses the best available AA site and tries placing it somewhere around the passed
//xy coordinates. This is stored in the rebuildQueue variable.
function buildAANearLoc(x, y)
{
	var structure = bestStructureIn(STANDARD_ANTI_AIR_DEFENSES);

	if (!defined(structure))
	{
		return;
	}

	if (!defined(x) || !defined(y))
	{
		debugMessage("buildAANearLoc. Undefined location.");
		return;
	}

	//For now, we don't want to go defend some spot that is, potentially, very far away
	//from base.
	if (distBetweenTwoPoints(BASE.x, BASE.y, x, y) > BASE_VTOL_DEFENSE_RANGE)
	{
		return;
	}

	var trucks = enumDroid(me, DROID_CONSTRUCT);

	if (trucks.length > 0)
	{
		var truck = trucks[0];
		var loc = pickStructLocation(truck, structure, x, y, 0);

		if (loc)
		{
			rebuildQueue.push({x: loc.x, y: loc.y, stat: structure});
		}
	}
}
