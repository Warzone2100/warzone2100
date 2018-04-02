
/*
 * This file is responsible for moving combat unit around.
 *
 */

(function(_global) {
////////////////////////////////////////////////////////////////////////////////////////////

_global.MAX_GROUPS = maxPlayers;
_global.miscGroup = MAX_GROUPS;
_global.vtolGroup = miscGroup + 1;
var groupInfo = [];
var firstTimeHarass = true;

function GroupInfo() {
	this.lastAttacked = undefined; // gameTime at the moment of the last combat
}

function safeGetObject(label) {
	var obj = getObject(label);
	if (obj === null)
		return undefined;
	return obj;
}

function groupsBySize() {
	var ret = [];
	for (var i = 0; i < MAX_GROUPS; ++i)
		if (isEnemy(i))
			ret.push(i);
	ret.sort(function(one, two) { return groupSize(two) - groupSize(one); });
	return ret;
}

function findLargestGroupIn(list) {
	var sizes = []
	for (var i = 0; i < MAX_GROUPS; ++i)
		sizes[i] = 0;
	list.forEach(function(object) {
		if (!defined(object.group) || object.group < 0 && object.group >= MAX_GROUPS)
			return;
		++sizes[object.group];
	});
	var maxCount = 0, maxIdx = 0;
	for (var i = 0; i < sizes.length; ++i)
		if (sizes[i] > maxCount) {
			maxCount = sizes[i];
			maxIdx = i;
		}
	return maxIdx;
}

function findNearestGroup(x, y) {
	var ret = naiveFindClusters(enumDroid(me).filter(function(droid) {
		return !isVTOL(droid) && (droid.droidType === DROID_WEAPON || droid.droidType === DROID_CYBORG);
	}), baseScale / 3);
	if (ret.maxCount === 0)
		return undefined;
	var minDist = Infinity, minIdx;
	var gr = [];
	for (var i = 0; i < ret.clusters.length; ++i) {
		gr[i] = findLargestGroupIn(ret.clusters[i]);
		if (groupSize(gr[i]) > attackGroupSize()) {
			var dist = distance(ret.xav[i], ret.yav[i], x, y);
			if (dist < minDist) {
				minDist = dist;
				minIdx = i;
			}
		}
	}
	if (defined(minIdx))
		return gr[minIdx];
	gr = groupsBySize();
	if (gr.length > 0)
		if (groupSize(gr[0]) > attackGroupSize())
			return gr[0];
}

function targetSuitableForHarass(object) {
	function uncached() {
		var ret = enumRange(object.x, object.y, baseScale / 2, ENEMIES, false).filter(function(obj) {
			return !(obj.type == STRUCTURE && obj.stattype != DEFENSE);
		}).length;
		return ret <= groupSize(miscGroup);
	}
	return cached(uncached, 60000, object.id);
}

function getGroupInfo(gr) {
	if (defined(groupInfo[gr]))
		return groupInfo[gr];
	groupInfo[gr] = new GroupInfo();
	return groupInfo[gr];
}

function groupTargetLabel(gr) {
	return "NullBot_" + me + "_GroupTarget_" + gr;
}

function groupMicroTargetLabel(gr) {
	return "NullBot_" + me + "_GroupMicroTarget_" + gr;
}

function vtolTargetLabel() {
	return "NullBot_" + me + "_VtolTarget";
}

function findTarget(gr) {
	var obj = safeGetObject(groupMicroTargetLabel(gr));
	var obj2 = safeGetObject(groupTargetLabel(gr));
	getGroupInfo(gr);
	if (gameTime > groupInfo[gr].lastAttacked + 10000 && defined(obj2) && obj2.type === POSITION)
		removeLabel(groupMicroTargetLabel(gr));
	if (!defined(obj))
		obj = obj2;
	if (defined(obj)) {
		if (gr === miscGroup) {
			// harass group should switch targets when its target gets protected, otherwise targets are permanent
			if (throttled(10000) || targetSuitableForHarass(obj))
				return obj;
		} else
			return obj;
	}
	// find harass targets for the misc group
	if (gr === miscGroup) {
		var list = enumStructList(miscTargets, enumLivingPlayers().filter(isEnemy).random());
		powerUps.forEach(function(stat) { // pick up oil drums and artifacts
			list = list.concat(enumFeature(-1, stat));
		});
		list = list.filter(targetSuitableForHarass).filter(function(feature) {
			if (iHaveHover())
				if (canReachFromBase(getPropulsionStatsComponents(PROPULSIONUSAGE.HOVER)[0], feature))
					return true;
			return canReachFromBase(getPropulsionStatsComponents(PROPULSIONUSAGE.GROUND)[0], feature);
		}).sort(function(one, two) {
			return distanceToBase(one) - distanceToBase(two);
		});
		obj = list[random(Math.min(3, list.length))];
		if (obj) {
			addLabel(obj, groupTargetLabel(gr));
			return obj;
		}
	}
	// fund structure targets
	var list = enumStructList(targets, gr);
	if (list.length > 0)
		obj = list.random();
	else {
		// find remaining droids
		list = enumDroid(gr);
		if (list.length > 0)
			obj = list.random();
	}
	if (defined(obj)) {
		addLabel(obj, groupTargetLabel(gr));
		return obj;
	}
}

function groupInDanger(gr) {
	getGroupInfo(gr); // make sure everything is defined
	if (!defined(groupInfo[gr].lastAttacked))
		return false;
	return gameTime - groupInfo[gr].lastAttacked < 10000;
}

function regroup(gr) {
	if (inPanic())
		return enumGroup(gr).filter(checkRepaired);
	var size = attackGroupSize();
	if (size < groupSize(gr) / 2)
		size = groupSize(gr) / 2;
	var ret = naiveFindClusters(enumGroup(gr).filter(checkRepaired), (baseScale / 3));
	if (ret.maxCount === 0)
		return [];
	for (var i = 0; i < ret.clusters.length; ++i)
		if (i !== ret.maxIdx) 
			for (var j = 0; j < ret.clusters[i].length; ++j) 
				orderDroidLoc(ret.clusters[i][j], DORDER_MOVE, ret.xav[ret.maxIdx], ret.yav[ret.maxIdx]);
	if (ret.maxCount < size) {
		for (var j = 0; j < ret.clusters[ret.maxIdx].length; ++j) {
			if (groupInDanger(gr))
				orderDroid(ret.clusters[ret.maxIdx][j], DORDER_RTB);
			else
				orderDroid(ret.clusters[ret.maxIdx][j], DORDER_STOP);
		}
		return [];
	}
	return ret.clusters[ret.maxIdx];
}

function weHaveRepair() {
	function uncached() {
		return enumStruct(me, REPAIR_FACILITY).length > 0;
	}
	return cached(uncached, 1000);
}

function checkRepaired(droid) {
	if (!weHaveRepair())
		return true;
	if (droid.order === DORDER_RTR)
		return false;
	if (droid.health < personality.repairAt) {
		orderDroid(droid, DORDER_RTR);
		return false;
	}
	return true;
}

function droidFree(droid) {
	if (droid.order === DORDER_SCOUT)
		return false;
	if (droid.order === DORDER_ATTACK)
		return false;
	if (droid.order === DORDER_RTR)
		return false;
	if (!checkRepaired(droid))
		return false;
	return true;
}

function attackTarget(droid) {
	var target = findTarget(droid.group);
	if (droid.group !== miscGroup)
		if (!defined(target) || !droidCanReach(droid, target.x, target.y)) {
			groupDroid(droid);
			return;
		}
	if (defined(target))
		switch (target.type) {
			case DROID:
				if (droid.canHitGround === true && !isVTOL(target))
					orderDroidObj(droid, DORDER_ATTACK, target);
				else if(droid.canHitAir === true && isVTOL(target))
					orderDroidObj(droid, DORDER_ATTACK, target);
				else
					orderDroidObj(droid, DORDER_OBSERVE, target);
				break;
			case FEATURE:
				orderDroidObj(droid, DORDER_RECOVER, target);
				break;
			case STRUCTURE:
				if (droid.droidType !== DROID_SENSOR)
					orderDroidLoc(droid, DORDER_SCOUT, target.x, target.y);
				else
					orderDroidObj(droid, DORDER_OBSERVE, target);
				break;
			default:
				orderDroidLoc(droid, DORDER_SCOUT, target.x, target.y);
				break;
		}
}

function vtolCanHit(droid, obj) {
	if (typeof(obj) === DROID && isVTOL(obj))
		return droid.canHitAir;
	else
		return droid.canHitGround;
}

function pickVtolTarget(droid) {
	function uncached() {
		function canHit(obj) {
			return vtolCanHit(droid, obj);
		}
		var enemy = enumLivingPlayers().filter(isEnemy).random();
		var list;
		list = enumStructList(miscTargets, enemy).filter(canHit);
		if (list.length > 0) return list.random();
		list = enumStruct(enemy, DEFENSE).filterProperty("canHitAir", true).filter(canHit);
		if (list.length > 0) return list.random();
		list = enumDroid(enemy, DROID_WEAPON).filterProperty("canHitAir", true).filter(canHit);
		if (list.length > 0) return list.random();
		list = enumDroid(enemy, DROID_CYBORG).filterProperty("canHitAir", true).filter(canHit);
		if (list.length > 0) return list.random();
		list = enumStructList(targets, enemy).filter(canHit);
		if (list.length > 0) return list.random();
		list = enumDroid(enemy).filter(canHit);
		if (list.length > 0) return list.random();
	}
	return cached(uncached, 100, droid.canHitAir + 2 * droid.canHitGround);
}

function vtolArmed(obj, percent) {
	if (obj.type != DROID)
		return;
	if (!isVTOL(obj))
		return false;
	for (var i = 0; i < obj.weapons.length; ++i)
		if (obj.weapons[i].armed >= 99)
			return true;
	return false;
}

function vtolReady(droid) {
	if (droid.order == DORDER_ATTACK)
		return false;
	if (vtolArmed(droid, 99))
		return true;
	if (droid.order != DORDER_REARM) {
		orderDroid(droid, DORDER_REARM);
		buildVtols() // actually pads
	}
	return false;
}

_global.attackGroupSize = function() {
	var ret = personality.minTanks + (gameTime / 300000) * personality.becomeHarder;
	if (ret > personality.maxTanks)
		ret = personality.maxTanks;
	return ret;
}

_global.setTarget = function(object, group) {
	if (!defined(group)) {
		group = findNearestGroup(object.x, object.y);
		if (object.type === STRUCTURE || object.type === DROID)
			addLabel(object, vtolTargetLabel());
	}
	if (!defined(group))
		return false;
	if (defined(safeGetObject(groupTargetLabel(group))))
		if (throttled(10000, group)) // don't switch targets too often
			return false;
	if (object.type === DROID || (object.type === STRUCTURE && object.stattype === DEFENSE))
		addLabel(object, groupMicroTargetLabel(group));
	else
		addLabel(object, groupTargetLabel(group));
	return true;
}

_global.unsetTarget = function(player) {
	for (var i = 0; i <= MAX_GROUPS; ++i) {
		var obj = safeGetObject(groupTargetLabel(i));
		if (defined(obj) && obj.type === POSITION && findBeaconPlayer(obj.x, obj.y) === player)
			removeLabel(groupTargetLabel(i));
	}
}

_global.groupDroid = function(droid) {
	if (droid.droidType === DROID_WEAPON || droid.droidType === DROID_CYBORG) {
		if (isVTOL(droid)) {
			groupAdd(vtolGroup, droid);
			return;
		}
		if (withChance(100 - groupSize(miscGroup) * 50 / personality.maxMiscTanks) || firstTimeHarass) {
			firstTimeHarass = false;
			groupAdd(miscGroup, droid);
			return;
		}
		var grp = groupsBySize().filter(function(i) {
			if (isAlly(i))
				return false;
			if (!defined(findTarget(i)))
				return false;
			if (!droidCanReach(droid, findTarget(i).x, findTarget(i).y))
				return false;
			return true;
		});
		var ret = grp.filter(function(i) {
			return groupSize(i) < attackGroupSize() * 2 && defined(findTarget(i));
		});
		if (ret.length === 0)
			ret = grp;
		if (ret.length === 0)
			ret = [ miscGroup ];
		groupAdd(ret[0], droid);
	}
	if (droid.droidType === DROID_SENSOR)
		groupAdd(miscGroup, droid);
}

_global.rebalanceGroups = function() {
	if (throttled(5000))
		return;
	if (groupSize(miscGroup) > personality.maxMiscTanks) {
		var list = enumGroup(miscGroup).shuffle();
		for (var i = personality.maxMiscTanks; i < personality.maxMiscTanks + 5 && i < list.length; ++i)
			groupDroid(list[i]);
	}
	var ret = groupsBySize();
	if (ret.length > 0)
		if (ret[0] > 0 && ret[0] < attackGroupSize())
			for (var i = 1; i < ret.length; ++i) {
				var list = enumGroup(ret[i]);
				for (var j = 0; j < list.length; ++j) {
					target = findTarget(ret[0]);
					if (defined(target))
						if (droidCanReach(list[j], target.x, target.y)) {
							groupAdd(ret[0], list[j]);
							return;
						}
				}
			}
}

_global.touchGroup = function(gr) {
	getGroupInfo(gr); // make sure everything is defined
	groupInfo[gr].lastAttacked = gameTime;
}

_global.fallBack = function(droid, threat) {
	if (droid.order === DORDER_MOVE)
		return;
	if (guessDroidMicro(droid) === MICRO.MELEE)
		return;
	var x = droid.x - (baseScale / 7) * (threat.x - droid.x) / distance(threat, droid);
	var y = droid.y - (baseScale / 7) * (threat.y - droid.y) / distance(threat, droid);
	if (x < 1 || y < 1 || x > mapWidth - 2 || y > mapHeight - 2)
		return;
	orderDroidLoc(droid, DORDER_MOVE, x, y);
}

_global.dangerLevel = function(loc) {
	return enumRange(loc.x, loc.y, baseScale / 2, ENEMIES, false).length;
}

_global.checkAttack = function() {
	for (var i = 0; i < MAX_GROUPS; ++i)
		if (!throttled(3000, i)) {
			regroup(i).forEach(attackTarget);
			break;
		}
	if (throttled(1000, "misc"))
		return;
	enumGroup(miscGroup).filter(checkRepaired).forEach(attackTarget);
	if (throttled(5000, "vtols"))
		return;
	var droids = enumGroup(vtolGroup).filter(vtolReady);
	if (droids.length > attackGroupSize() / 3.)
		droids.forEach(function(droid) {
			var target = pickVtolTarget(droid);
			if (defined(target))
				orderDroidObj(droid, DORDER_ATTACK, target);
		});
}

_global.pushVtols = function(object) {
	var vtols = enumRange(object.x, object.y, 20, me, false);
	var enemies = enumRange(object.x, object.y, 8, ENEMIES, true);
	for (var i = 0; i < vtols.length; ++i)
		if (vtolArmed(vtols[i], 1))
			for (var j = 0; j < enemies.length; ++j)
				if (vtolCanHit(vtols[i], enemies[j])) {
					orderDroidObj(vtols[i], DORDER_ATTACK, enemies[j]);
					break;
				}
}

_global.inPanic = function() {
	function uncached() {
		var badGuys = enumRange(baseLocation.x, baseLocation.y, baseScale, ENEMIES).length;
		var goodGuys = enumRange(baseLocation.x, baseLocation.y, baseScale, ALLIES).filter(function(object) {
			return object.type === DROID && (object.droidType === DROID_WEAPON || object.droidType === DROID_CYBORG);
		}).length;
		return 3 * badGuys > 2 * goodGuys;
	}
	return cached(uncached, 10000);
}

////////////////////////////////////////////////////////////////////////////////////////////
})(this);
