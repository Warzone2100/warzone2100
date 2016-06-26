
/*
 * This file contain functions for manipulating stats defined by rulesets.
 * 
 */

(function(_global) {
////////////////////////////////////////////////////////////////////////////////////////////

_global.isHoverPropulsion = function(str) {
	return propulsionStats.some(function(val) { return val.usage === PROPULSIONUSAGE.HOVER && val.stat === str; });
}

_global.isVtolPropulsion = function(str) {
	return propulsionStats.some(function(val) { return val.usage === PROPULSIONUSAGE.VTOL && val.stat === str; });
}

_global.iHaveHover = function() {
	return propulsionStats.some(function(val) { return val.usage === PROPULSIONUSAGE.HOVER && componentAvailable(val.stat);} );
}

_global.iHaveVtol = function() {
    return propulsionStats.some(function(val) { return val.usage === PROPULSIONUSAGE.VTOL && componentAvailable(val.stat);} );
}

_global.iHaveArty = function() {
	for (var stat in weaponStats)
		for (var i = 0; i < weaponStats[stat].defenses.length; ++i)
			if (weaponStats[stat].defenses[i].defrole === DEFROLE.ARTY)
				if (countStruct(weaponStats[stat].defenses[i].stat) > 0)
					return true;
	return false;
}

// works with stored droid objects too!
_global.safeIsVtol = function(droid) {
	return isVtolPropulsion(droid.propulsion);
}

_global.enumStructList = function(list, player) {
	if (!defined(player))
		player = me;
	return list.reduce(function(summ, new_value) { return summ.concat(enumStruct(player, new_value)); }, []);
}

_global.countStructList = function(list, player) {
	if (!defined(player))
		player = me;
	return list.reduce(function(summ, new_value) { return summ + countStruct(new_value, player); }, 0);
}

_global.enumFinishedStructList = function(list, player) {
	return enumStructList(list, player).filterProperty("status", BUILT);
}

_global.countFinishedStructList = function(list, player) {
	return enumFinishedStructList(list, player).length;
}

_global.enumIdleStructList = function(list, player) {
	return enumFinishedStructList(list, player).filter(structureIdle);
}

_global.structListLimit = function(list) {
	return list.reduce(function (summ, val) {return summ + getStructureLimit(val)}, 0)
}

_global.guessWeaponRole = function (name) {
	for (var stat in weaponStats) {
		if (
			weaponStats[stat].weapons.someProperty("stat", name) ||
			weaponStats[stat].vtols.someProperty("stat", name) ||
			weaponStats[stat].templates.some(function (i) {return i.weapons.indexOf(name) > -1;})
		)
		return weaponStats[stat].roles;
	}
	niceDebug("Ruleset warning: Couldn't guess weapon role of", name);
}

function guessWeaponMicro(name) {
	function uncached() {
		for (var stat in weaponStats)
			if (weaponStats[stat].weapons.someProperty("stat", name))
				return weaponStats[stat].micro;
			if (weaponStats[stat].templates.some(function(i) { return i.weapons.indexOf(name) > -1; }))
				return weaponStats[stat].micro;
	}
	return cached(uncached, Infinity, name);
}

_global.guessDroidMicro = function(droid) {
	for (var i = 0; i < droid.weapons.length; ++i) {
		var ret = guessWeaponMicro(droid.weapons[i].name);
		if (ret !== MICRO.RANGED)
			return ret;
	}
	return MICRO.RANGED;
}

_global.guessBodyArmor = function(name) {
    var body = bodyStats.filterProperty("stat", name).last()
    if (defined(body))
        return body.armor;
    else
        niceDebug("Ruleset warning: Couldn't guess body class of", name);
}

function weaponPathIsAvailable(path, objectType, defrole) {
	switch(objectType) {
		case 0:
			return path.weapons.some(function(val) { return componentAvailable(val.stat); })
		case 1:
			return path.templates.some(function(val) {
				for (var i = 0; i < val.weapons.length; ++i)
					if (!componentAvailable(val.weapons[i]))
						return false;
				return componentAvailable(val.body) && componentAvailable(val.prop);
			});
		case 2:
			return path.defenses.some(function(val) { return val.defrole === defrole && isAvailable(val.stat); });
		case 3:
			return path.vtols.some(function(val) { return componentAvailable(val.stat); });
		default: // research
			return true;
	}
}

_global.getProductionPaths = function() {
	if (!defined(fallbackWeapon) || gameTime > 600000)
		return personality.weaponPaths;
	return [weaponStats[fallbackWeapon]].concat(personality.weaponPaths);
}

_global.chooseAvailableWeaponPathByRoleRatings = function(paths, rating, objectType, defrole) {
	var minDist = Infinity, minPath;
	paths.forEach(function(path) {
		if (!weaponPathIsAvailable(path, objectType, defrole))
			return;
		var dist = 0;
		for (var i = 0; i < ROLE.LENGTH; ++i) {
			var newDist = Math.abs(rating[i] - path.roles[i])
			if (newDist > dist)
				dist = newDist;
		}
		if (dist < minDist) {
			minDist = dist;
			minPath = path;
		}
	});
	return minPath;
}

//
// here be functions for querying out research paths
//

function statsToResList(path) {
	return path.map(function (val) { return val.res; });
}

// todo make one StatsToResList and do filtering for path outside
_global.bodyStatsToResList = function(armor) {
    return statsToResList(filterBodyStatsByUsage(armor)).reverse();
}

_global.propulsionStatsToResList = function(usage) {
	return statsToResList(filterDataByFlag(propulsionStats, 'usage', usage));
}

_global.weaponStatsToResList = function(path, objType) {
	if (!defined(path))
		return [];
	var ret = [];
	switch(objType) {
		case 0:
			ret = statsToResList(path.weapons); break;
		case 1:
			ret = statsToResList(path.templates); break;
		case 2:
			ret = statsToResList(path.defenses); break;
		case 3:
			ret = statsToResList(path.vtols); break;
	}
	if (ret.length === 0)
		ret = ret.concat(
			statsToResList(path.weapons),
			statsToResList(path.templates),
			statsToResList(path.defenses),
			statsToResList(path.vtols)
		);
	return ret;
}

//
// here be functions for querying out component lists
//

// TODO: move this to math. If we have same structure of data <list of objects> we can use it all around.
_global.filterDataByFlag = function(data, attr_name, flag) {
     return data.filter(function(obj) { return obj[attr_name] & flag; });
}

_global.filterBodyStatsByUsage = function(usage, armor) {
	var data;
    if (defined(armor))
        data = filterDataByFlag(bodyStats, 'armor', armor)
    else
        data = bodyStats;
    return filterDataByFlag(data, 'usage', usage).reverse();
}

_global.getPropulsionStatsComponents = function(usage) {
    var data = filterDataByFlag(propulsionStats, 'usage', usage)
	return data.map(function(val) { return val.stat; }).reverse()
}

//
// here be functions for querying out defensive structures
//

_global.weaponStatsToDefenses = function(stats, defrole) {
	if (!defined(stats))
		return [];
	var ret = [];
	for (var i = 0; i < stats.defenses.length; ++i)
		if (!defined(defrole) || stats.defenses[i].defrole === defrole)
			ret.push(stats.defenses[i].stat);
	// reverse not needed here
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////
})(this);
