
/*
 * This file describes building construction procedures.
 *
 */

(function(_global) {
////////////////////////////////////////////////////////////////////////////////////////////

function randomLocation() {
	var x = baseLocation.x + random(baseScale) - baseScale / 2;
	var y = baseLocation.y + random(baseScale) - baseScale / 2;
	if (x < 3 || y < 3 || x > mapWidth - 4 || y > mapHeight - 4)
		return baseLocation;
	return {x: x, y: y};

}

// a function for checking the presence of enemy units at the construction site
function safeSpot(x, y) {
	return dangerLevel({x: x, y: y}) <= 0;
}

function truckFree(truck) {
	if (truck.droidType !== DROID_CONSTRUCT)
		return false;
	if (truck.order === DORDER_BUILD)
		return false;
	if (truck.order === DORDER_HELPBUILD)
		return false;
	if (truck.order === DORDER_LINEBUILD)
		return false;
	if (truck.order === DORDER_DEMOLISH)
		return false;
	return true;
}

// returns one or two free trucks
function getTwoFreeTrucks() {
	var trucks = enumTrucks().filter(truckFree);
	if (trucks.length > 2) {
		var ret = naiveFindClusters(trucks, baseScale / 2);
		if (ret.maxCount >= 2)
			trucks = ret.clusters[ret.maxIdx];
	}
	if (trucks.length > 2)
		trucks.length = 2;
	return trucks;
}

function getFreeTruckAround(x, y) {
	var list = enumTrucks().filter(truckFree).filter(function(droid) {
		return droidCanReach(droid, x, y);
	}).sort(function(one, two) {
		return distance(one, x, y) - distance(two, x, y);
	});
	if (list.length > 0)
		return list[0];
}

function buildModule(struct) {
	trucks = getTwoFreeTrucks();
	if (trucks.length <= 0)
		return BUILDRET.FAILURE;
	var moduleInfo = modules.filter(function(item) { return isAvailable(item.module) && item.base === struct.stattype; }).last();
	if (!defined(moduleInfo))
		return BUILDRET.UNAVAILABLE;
	if (struct.modules >= moduleInfo.count)
		return BUILDRET.UNAVAILABLE;
	var success = false;
	for (var i = 0; i < trucks.length; ++i)
		success = orderDroidBuild(trucks[i], DORDER_BUILD, moduleInfo.module, struct.x, struct.y) || success;
	if (success)
		return BUILDRET.SUCCESS;
	return BUILDRET.FAILURE;
}

function buildBasicStructure(statlist, importance) {
	if (throttled(5000, statlist[0])) return BUILDRET.FAILURE;
	// by default, don't try building things in dangerous locations
	if (!defined(importance))
		importance = IMPORTANCE.MANDATORY;
	trucks = getTwoFreeTrucks();
	if (trucks.length <= 0)
		return BUILDRET.FAILURE;
	// choose structure type (out of the statlist),
	// together with suitable location
	var idx, loc, avail = false;
	for (var i = 0; i < statlist.length; ++i)
		if (isAvailable(statlist[i])) {
			avail = true;
			if (distanceToBase(trucks[0]) <= baseScale)
				loc = pickStructLocation(trucks[0], statlist[i], trucks[0].x, trucks[0].y);
			else {
				var rndLoc = randomLocation();
				loc = pickStructLocation(trucks[0], statlist[i],rndLoc.x, rndLoc.y);
			}
			idx = i;
			break;
		}
	if (!avail)
		return BUILDRET.UNAVAILABLE;
	if (!defined(loc))
		return BUILDRET.FAILURE;
	if (importance === IMPORTANCE.PEACETIME && !safeSpot(loc.x, loc.y))
		return BUILDRET.FAILURE;
	// now actually build
	var success = false;
	for (var i = 0; i < trucks.length; ++i)
		success = orderDroidBuild(trucks[i], DORDER_BUILD, statlist[idx], loc.x, loc.y) || success;
	if (success)
		return BUILDRET.SUCCESS;
	return BUILDRET.FAILURE;
}

function finishStructures() {
	var success = false;
	var list = enumStruct(me).filterProperty("status", BEING_BUILT);
	for (var i = 0; i < list.length; ++i) {
		if (success)
			return;
		if (throttled(10000, list[i].id))
			return;
		if (list[i].stattype === RESOURCE_EXTRACTOR)
			return;
		var truck = getFreeTruckAround(list[i].x, list[i].y);
		if (!defined(truck))
			return;
		if (orderDroidObj(truck, DORDER_HELPBUILD, list[i]))
			success = true;
	};
	return success;
}

function buildStructureAround(statlist, loc, unique) {
	if (!defined(statlist))
		return BUILDRET.UNAVAILABLE;
	var truck = getFreeTruckAround(loc.x, loc.y);
	if (!defined(truck))
		return BUILDRET.FAILURE;
	var stat = statlist.filter(isAvailable).filter(function(s) {
		if (unique !== true)
			return true;
		var list = enumStruct(me, s);
		for (var i = 0; i < list.length; ++i)
			if (distance(list[i], loc) < baseScale / 2)
				return false;
		return true;
	}).last();
	if (!defined(stat))
		return BUILDRET.UNAVAILABLE;
	var loc2 = pickStructLocation(truck, stat, loc.x, loc.y);
	if (!defined(loc2))
		return BUILDRET.FAILURE;
	// if we're not into turtling, don't build too many towers
	if (personality.defensiveness < 100 && distance(loc2, loc) > baseScale / 5)
		return BUILDRET.FAILURE;
	if (orderDroidBuild(truck, DORDER_BUILD, stat, loc2.x, loc2.y))
		return BUILDRET.SUCCESS;
	return BUILDRET.FAILURE;
}

function captureOil(oil) {
	if (!defined(oil))
		return BUILDRET.FAILURE;
	var truck = getFreeTruckAround(oil.x, oil.y);
	if (!defined(truck))
		return BUILDRET.FAILURE;
	var stat = structures.derricks.filter(isAvailable).last();
	if (!defined(stat))
		return BUILDRET.UNAVAILABLE;
	if (throttled(90000, oil.y * mapWidth + oil.x))
		return BUILDRET.FAILURE;
	if (orderDroidBuild(truck, DORDER_BUILD, stat, oil.x, oil.y))
		return BUILDRET.SUCCESS;
	return BUILDRET.FAILURE;
}

function chooseDefense(defrole) {
	return weaponStatsToDefenses(chooseAvailableWeaponPathByRoleRatings(getProductionPaths(), chooseDefendWeaponRole(), 2, defrole), defrole);
}

function buildTowers() {
	var oils = enumStructList(structures.derricks);
	if (oils.length === 0)
		return false;
	if (withChance(70))
		return buildStructureAround(chooseDefense(DEFROLE.STANDALONE), oils.random()) !== BUILDRET.UNAVAILABLE;
	return buildStructureAround(chooseDefense(DEFROLE.FORTRESS).concat(structures.sensors), oils.random(), true) !== BUILDRET.UNAVAILABLE;
}

function buildGateways() {
	function uncached() {
		var oils = countStructList(structures.derricks);
		if (oils <= 0)
			return BUILDRET.FAILURE;
		// lets not cycle through all gateways on the map
		var gates = whereAreTheGateways();
		if (gates.length === 0)
			return BUILDRET.FAILURE;
		var gates = gates.filter(function(gate) {
			var l = gate.x1 - gate.x2 + gate.y1 - gate.y2;
			if (l < 0)
				l = -l;
			var cnt = enumRange(gate.x1, gate.y1, l, ALLIES).filterProperty("stattype", DEFENSE).length;
			cnt    += enumRange(gate.x2, gate.y2, l, ALLIES).filterProperty("stattype", DEFENSE).length;
			cnt    -= enumRange(gate.x1, gate.y1, l, ENEMIES).filterProperty("stattype", DEFENSE).length;
			cnt    -= enumRange(gate.x2, gate.y2, l, ENEMIES).filterProperty("stattype", DEFENSE).length;
			return cnt >= 0 && (cnt < l || (personality.defensiveness === 100 && withChance(70))); // turtle AI needs to keep building towers
		}).sort(function(one, two) { return distanceToBase({x: one.x1, y: one.y1}) - distanceToBase({x: two.x1, y: two.y1}); });
		if (gates.length === 0)
			return BUILDRET.FAILURE;
		if (withChance(50))
			return buildStructureAround(chooseDefense(DEFROLE.GATEWAY), {x: gates[0].x1, y: gates[0].y1}) !== BUILDRET.UNAVAILABLE;
		else
			return buildStructureAround(chooseDefense(DEFROLE.GATEWAY), {x: gates[0].x2, y: gates[0].y2}) !== BUILDRET.UNAVAILABLE;
	}
	return cached(uncached, 200);
}

function buildArty() {
	return buildBasicStructure(chooseDefense(DEFROLE.ARTY), IMPORTANCE.PEACETIME);
}

_global.buildMinimum = function(statlist, count, importance) {
	if (countStructList(statlist) < count)
		if (buildBasicStructure(statlist, importance) !== BUILDRET.UNAVAILABLE)
			return true;
	return false;
}

_global.captureSomeOil = function() {
	if (throttled(500))
		return true;
	function getOilList() {
		var oils = [];
		oilResources.forEach(function(stat) { oils = oils.concat(enumFeature(-1, stat)); });
		oils = oils.concat(enumStructList(structures.derricks).filterProperty("status", BEING_BUILT));
		oils = oils.sort(function(one, two) {
			return distanceToBase(one) - distanceToBase(two);
		});
		if (oils.length > 10)
			oils.length = 10;
		return oils;
	}
	var oils = cached(getOilList, 5000);
	if (countFinishedStructList(structures.derricks) >= 4 * structListLimit(structures.gens))
		return false;
	for (var i = 0; i < oils.length; ++i)
		if (captureOil(oils[i]) === BUILDRET.SUCCESS)
			return true;
	return false;
}

_global.buildMinimumDerricks = function(count) {
	if (countFinishedStructList(structures.derricks) < count)
		if (captureSomeOil())
			return true;
	return false;
}

function buildExpand() {
	if (myPower() > personality.maxPower) {
		switch (chooseObjectType()) {
			case 0:
				if (needFastestResearch() === PROPULSIONUSAGE.GROUND)
					if (buildMinimum(structures.factories, Infinity, IMPORTANCE.PEACETIME))
						return true;
				// fall-through
			case 1:
				if (needFastestResearch() === PROPULSIONUSAGE.GROUND)
					if (buildMinimum(structures.templateFactories, Infinity, IMPORTANCE.PEACETIME))
						return true;
				// fall-through
			case 3:
				if (buildMinimum(structures.vtolFactories, Infinity, IMPORTANCE.PEACETIME))
					return true;
		}
	}
	return false;
}

function buildEnergy() {
	var oils = countFinishedStructList(structures.derricks);
	var gens = countStructList(structures.gens);
	if (oils > 4 * gens)
		if (buildBasicStructure(structures.gens, IMPORTANCE.PEACETIME) !== BUILDRET.UNAVAILABLE)
			return true;
	if (withChance(50))
		if (captureSomeOil())
			return true;
	return false;
}

function buildModules() {
	var str = [];
	for (var i = 0; i < modules.length; ++i) {
		if (modules[i].base === FACTORY && needFastestResearch() !== PROPULSIONUSAGE.GROUND)
			continue;
		str = enumStruct(me, modules[i].base);
		for (var j = 0; j < str.length; ++j)
			if (buildModule(str[j]) !== BUILDRET.UNAVAILABLE)
				return true;
	}
	return false;
}

_global.buildVtols = function() {
	if (buildMinimum(structures.vtolPads, enumDroid(me, DROID_WEAPON).filter(isVTOL).length / 2), IMPORTANCE.PEACETIME)
		return true;
	return false;
}

function buildExtras() {
	if (throttled(180000))
		return false;
	if (buildBasicStructure(structures.extras, IMPORTANCE.PEACETIME) !== BUILDRET.UNAVAILABLE)
		return true;
	return false;
}

_global.buildDefenses = function() {
	if (chooseObjectType() !== 2)
		return false;
	if (withChance(33) && areThereGW() === true) {
		if (buildGateways())
			return true;
	} else if (withChance(50)) {
		if (buildTowers()) // includes sensor towers and forts
			return true;
	} else
		if (buildArty())
			return true;
	return false;
}

function listOutdatedDefenses() {
	for (var path in weaponStats) {
		for (var role in DEFROLE) {
			var list = weaponStatsToDefenses(weaponStats[path], DEFROLE[role]);
			for (var i = 0; i < list.length - 2; ++i)
				if (isAvailable(list[i + 2])) {
					if (countStruct(list[i]) > 0)
						return enumStruct(me, list[i]);
				}
		}
	};
	return [];
}

function recycleDefenses() {
	var trucks = enumTrucks().filter(truckFree);
	if (trucks.length <= 0)
		return false;
	var list = listOutdatedDefenses();
	for (var i = 0; i < list.length; ++i)
		for (var j = 0; j < trucks.length; ++j)
			if (droidCanReach(trucks[j], list[i].x, list[i].y)) {
				orderDroidObj(trucks[j], DORDER_DEMOLISH, list[i]);
				return true;
			}
	return false;
}

_global.checkConstruction = function() {
	if (enumTrucks().filter(truckFree).length === 0)
		return;
	if (functionSeries("construction", [
		finishStructures,
		buildOrder,
		buildExpand,
		buildEnergy,
		buildModules,
		buildVtols,
		buildExtras,
		recycleDefenses,
		buildDefenses,
	]))
		queue("checkConstruction");
}

////////////////////////////////////////////////////////////////////////////////////////////
})(this);
