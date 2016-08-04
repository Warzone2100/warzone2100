
/*
 * This file is responsible for the adaptation system. It gathers statistics about 
 * player choices and regulates the AI's choices accordingly.
 * 
 */

(function(_global) {
////////////////////////////////////////////////////////////////////////////////////////////

function adaptVote(our, their, verbose) {
	if (!defined(verbose))
		verbose = false;
	var l = our.length;
	var ourTotal = our.reduce(function(prev, curr) { return prev + curr; });
	var theirTotal = their.reduce(function(prev, curr) { return prev + curr; });
	if (theirTotal === 0)
		return verbose ? randomUnitArray(l) : random(l);
	var rating = [];
	for (var i = 0; i < l; ++i)
		rating[i] = their[i] / theirTotal;
	if (ourTotal > 0)
		for (var i = 0; i < l; ++i)
			rating[i] -= our[i] / ourTotal;
	if (verbose)
		return rating.map(function(val) { return (val + 1) / 2; });
	var maxRating = -Infinity;
	var maxIdx = 0;
	for (var i = 0; i < l; ++i) 
		if (rating[i] > maxRating) {
			maxRating = rating[i];
			maxIdx = i;
		}
	return maxIdx;
}

//
// here be functions for gathering player statistics
//

function SimpleStat() {
	// propulsion classes used by the player
	this.obj = zeroArray(ROLE.LENGTH);
	// weapon-propulsion classes used by the player
	this.role = zeroArray(ROLE.LENGTH);
	// armor used by the player
	this.kbody = 0;
	this.tbody = 0;
	// weapon-armor classes used by the player
	this.kweapon = 0;
	this.tweapon = 0;
}

function addStat(to, what, weight) {
	if (!defined(weight))
		weight = 1;
	for (var prop in to) {
		if (to[prop].constructor === Array)
			for (var i = 0; i < to[prop].length; ++i) 
				to[prop][i] += what[prop][i] * weight;
		else
			to[prop] += what[prop] * weight;
	}
}

const SCOPES = {
	land: 0,
	sea: 0,
	air: 0,
}

function ScopeStat() {
	// simple stats by scopes
	this.land = new SimpleStat();
	this.sea = new SimpleStat();
	this.air = new SimpleStat();
	// a function to convert this sort of stat to a SimpleStat
	this.collapse = function() {
		var ret = new SimpleStat();
		for (var i in SCOPES)
			addStat(ret, this[i]);
		return ret;
	}
}

function EnemyStat() {
	// separate stats for defense and offense
	this.defense = new ScopeStat();
	this.offense = new SimpleStat();
	// a function to convert this sort of stat to a SimpleStat
	this.collapse = function() {
		var ret = new SimpleStat();
		addStat(ret, this.defense.collapse());
		addStat(ret, this.offense);
		return ret;
	}
}

function MyStat() {
	// separate stats for defense and offense
	this.defense = new SimpleStat();
	this.offense = new ScopeStat();
	// a function to convert this sort of stat to a SimpleStat
	this.collapse = function() {
		var ret = new SimpleStat();
		addStat(ret, this.defense);
		addStat(ret, this.offense.collapse());
		return ret;
	}
}

var enemyStats = [], enemyStatsTemp = [];
for (var i = 0; i < maxPlayers; ++i) {
	enemyStats[i] = new EnemyStat;
	enemyStatsTemp[i] = new EnemyStat;
}

var myStats = new MyStat;
var myStatsTemp = new MyStat;

var stack = [];
var MAX_PER_CYCLE = 20;

function canReachBy(scope, location) {
	switch (scope) {
		case "land":
			return canReachFromBase(getPropulsionStatsComponents(PROPULSIONUSAGE.GROUND).last(), location);
		case "sea":
			return canReachFromBase(getPropulsionStatsComponents(PROPULSIONUSAGE.HOVER).last(), location);
		case "air":
			return canReachFromBase(getPropulsionStatsComponents(PROPULSIONUSAGE.AIR).last(), location);
	}
}

// works with stored droid objects too!
function threatensBase(droid) {
	if (isAlly(droid.player))
		return false;
	return canReachFromBase(droid.propulsion, droid.x, droid.y);
}

// count target structures reachable by land
function countLandTargets(player) {
	function uncached() {
		var currProp = getPropulsionStatsComponents(PROPULSIONUSAGE.GROUND).last();
		if (!defined(currProp)) 
			return 0;
		var list = enumStructList(targets, player);
		return list.filter(function(struct) {
			return canReachFromBase(currProp, struct);
		}).length;
	}
	return cached(uncached, 5000, player);
}

// count target structures reachable by sea but not by land
function countSeaTargets(player) {
	function uncached() {
		var currProp = getPropulsionStatsComponents(PROPULSIONUSAGE.HOVER)[0];
		if (!defined(currProp)) 
			return 0;
        var prevProp = getPropulsionStatsComponents(PROPULSIONUSAGE.GROUND)[0];
		return enumStructList(targets, player).filter(function(struct) {
			return (!defined(prevProp) || !canReachFromBase(prevProp, struct)) && canReachFromBase(currProp, struct);
		}).length;
	}
	return cached(uncached, 5000, player);
}

// count target structures reachable by air but not by land or by sea
function countAirTargets(player) {
	function uncached() {
		var currProp = getPropulsionStatsComponents(PROPULSIONUSAGE.VTOL)[0];
		if (!defined(currProp)) 
			return 0;
		var prevProp = getPropulsionStatsComponents(PROPULSIONUSAGE.GROUND|PROPULSIONUSAGE.HOVER)[0];
		return enumStructList(targets, player).filter(function(struct) {
			return (!defined(prevProp) || !canReachFromBase(prevProp, struct)) && canReachFromBase(currProp, struct);
		}).length;
	}
	return cached(uncached, 5000, player);
}

// TODO: this function needs to be updated once more object properties
// become available for the API to use.
function classifyObject(obj) {
	var ret = new SimpleStat();
	if (obj.type === STRUCTURE && obj.stattype !== DEFENSE)
		return ret;
	if (obj.type === DROID && obj.droidType !== DROID_WEAPON && obj.droidType !== DROID_CYBORG)
		return ret;
	if (obj.type === FEATURE)
		return ret;
	for (var i = 0; i < obj.weapons.length; ++i) {
		var roles = guessWeaponRole(obj.weapons[i].name); {
			if (!defined(roles)) {
				if (obj.canHitAir && obj.canHitGround)
					ret.role.addArray([1/4, 1/4, 1/4, 1/4]);
				else if (obj.canHitAir && !obj.canHitGround)
					ret.role.addArray([0, 0, 0, 1]);
				else
					ret.role.addArray([1/3, 1/3, 1/3, 0]);
			} else
				ret.role.addArray(roles);
		}
	}
	for (var i = 0; i < obj.weapons.length; ++i) {
		if (getWeaponInfo(obj.weapons[i].name).impactClass === "KINETIC")
			ret.kweapon += 1;
		else
			ret.tweapon += 1;
	}
	if (obj.type === STRUCTURE || (obj.type === DROID && safeIsVtol(obj)))
		ret.tbody += 1;
	else if (obj.type === DROID && obj.droidType === DROID_CYBORG)
		ret.kbody += 1;
	else {
		switch (guessBodyArmor(obj.body)) {
			case BODYCLASS.KINETIC:
				ret.kbody += 1;
				break;
			case BODYCLASS.THERMAL:
				ret.tbody += 1;
				break;
			default:
				ret.tbody += 1 / 2;
				ret.kbody += 1 / 2;
		}
	}
	if (obj.type === STRUCTURE)
		ret.obj[OBJTYPE.DEFS] += 1;
	if (obj.type === DROID) {
		if (safeIsVtol(obj))
			ret.obj[OBJTYPE.VTOL] += 1;
		else if (obj.droidType === DROID_CYBORG)
			ret.obj[OBJTYPE.BORG] += 1;
		else 
			ret.obj[OBJTYPE.TANK] += 1;
	}
	return ret;
}

function summUpEnemyObject(obj, stat) {
	var ret = classifyObject(obj);
	var w = obj.cost;
	if (obj.type === STRUCTURE) {
		for (var scope in SCOPES)
			if (canReachBy(scope, obj)) // structures don't move, so they are usually counted as defense
				addStat(stat.defense[scope], ret, w);
		if (obj.range > baseScale * 2) // unless they're covering the whole map with their range
			addStat(stat.offense, ret, w);
	}
	if (obj.type === DROID) {
		for (var scope in SCOPES) 
			if (canReachBy(scope, obj)) // if the droid can't reach your base, we count it as defense only
				addStat(stat.defense[scope], ret, w);
		if (threatensBase(obj)) // otherwise count them as offense as well
			addStat(stat.offense, ret, w);
	}
}

function summUpMyObject(obj, stat) {
	var ret = classifyObject(obj);
	var w = obj.cost;
	if (obj.type === STRUCTURE) {
		addStat(stat.defense, ret, w);
		if (obj.range > baseScale * 2) 
			for (var scope in SCOPES)
				addStat(stat.offense[scope], ret, w);
	}
	if (obj.type === DROID) {
		if (obj.group === miscGroup)
			addStat(stat.defense, ret, w);
		var list = enumLivingPlayers();
		list.forEach(function(p) {
			if (isEnemy(p)) {
				if (countLandTargets(p) > 0)
					addStat(stat.offense.land, ret, w / list.length);
				if (countSeaTargets(p) > 0)
					addStat(stat.offense.sea, ret, w / list.length);
				if (countAirTargets(p) > 0)
					addStat(stat.offense.air, ret, w / list.length);
			}
		});
	}
}

_global.adaptCycle = function() {
	if (!defined(adaptCycle.player))
		adaptCycle.player = me;
	if (!defined(adaptCycle.phase))
		adaptCycle.phase = 0;
	if (stack.length > 0) {
		var items = MAX_PER_CYCLE;
		if (items > stack.length)
			items = stack.length;
		for (var i = 0; i < items; ++i) {
			var obj = stack.pop();
			if (isEnemy(adaptCycle.player))
				summUpEnemyObject(obj, enemyStatsTemp[adaptCycle.player]);
			else if (adaptCycle.player === me)
				summUpMyObject(obj, myStatsTemp);
		}
		return;
	}
	++adaptCycle.phase;
	switch(adaptCycle.phase) {
		case 1:
			stack = enumStruct(adaptCycle.player, DEFENSE);
			return;
		case 2:
			stack = enumDroid(adaptCycle.player, DROID_WEAPON);
			return;
		case 3:
			stack = enumDroid(adaptCycle.player, DROID_CYBORG);
			return;
	}
	if (adaptCycle.player === me) {
		myStats = myStatsTemp;
		myStatsTemp = new MyStat;
	} else {
		enemyStats[adaptCycle.player] = enemyStatsTemp[adaptCycle.player];
		enemyStatsTemp[adaptCycle.player] = new EnemyStat;
	}
	do {
		++adaptCycle.player;
		if (adaptCycle.player >= maxPlayers)
			adaptCycle.player = 0;
	} while (adaptCycle.player !== me && !isEnemy(adaptCycle.player));
	adaptCycle.phase = 0;
}

function getMyGroupInfo(gr) {
	var ret = new MyStat();
	enumGroup(gr).forEach(function(obj) { summUpMyObject(obj, ret); });
	return ret;
}

//
// here be functions for querying statistics gathered above
// and making adaptive decisions
//

function groupOurs(gr) {
	function uncached() {
		if (defined(gr))
			return getMyGroupInfo(gr).collapse();
		else
			return myStats.collapse();
	}
	return cached(uncached, 500, gr);
}

function groupAttackOurs(gr) {
	function uncached() {
		if (defined(gr))
			return getMyGroupInfo(gr).collapse();
		else
			return myStats.offense.collapse();
	}
	return cached(uncached, 500, gr);
}

function enemyOffense() {
	var theirs = new SimpleStat();
	enumLivingPlayers().filter(isEnemy).forEach(function(p) {
		addStat(theirs, enemyStats[p].offense);
	});
	return theirs;
}

function groupTheirs(gr) {
	function uncached() {
		if (defined(gr)) {
			if (gr === miscGroup)
				return enemyOffense();
			else
				return enemyStats[gr].collapse();
		} else {
			var theirs = new SimpleStat();
			enumLivingPlayers().filter(isEnemy).forEach(function(p) {
				addStat(theirs, enemyStats[p].collapse());
			});
			return theirs;
		}
	}
	return cached(uncached, 500, gr);
}

// use this for research; 15% smooth
_global.chooseWeaponRole = function(gr) {
	function uncached() {
		if (withChance(20))
			return randomUnitArray(4);
		return adaptVote(groupOurs(gr).role, groupTheirs(gr).obj, true);
	}
	return cached(uncached, 5000, gr);
}

// use this for droids and long-range arty emplacements
_global.chooseAttackWeaponRole = function(gr) {
	function uncached() {
		return adaptVote(groupAttackOurs(gr).role, groupTheirs(gr).obj, true);
	}
	return cached(uncached, 5000, gr);
}

// use this for defenses; 15% smooth
_global.chooseDefendWeaponRole = function() {
	function uncached() {
		if (withChance(20))
			return randomUnitArray(4);
		return adaptVote(myStats.defense.role, enemyOffense().obj, true);
	}
	return cached(uncached, 5000);
}

_global.chooseBodyClass = function(gr) {
	function uncached() {
		var our = groupAttackOurs(gr), their = groupTheirs(gr);
		return adaptVote(
			[ our.kbody, our.tbody ],
			[ their.kweapon, their.tweapon ]
		) ? BODYCLASS.THERMAL : BODYCLASS.KINETIC;
	}
	return cached(uncached, 5000, gr);
}

_global.chooseObjectType = function() {
	function uncached() {
		var our = groupOurs(), their = groupTheirs();
		// behaviour specific for a turtle AI
		if (personality.defensiveness === 100)
			if (iHaveVtol() && withChance(personality.vtolness) && adaptVote(
				[ our.obj[OBJTYPE.DEFS], our.obj[OBJTYPE.VTOL] ],
				[ their.role[ROLE.AA], their.role[ROLE.AT] + their.role[ROLE.AP] + 2 * their.role[ROLE.AS] ]
			) === 1)
			return OBJTYPE.VTOL;
		else
			return OBJTYPE.DEFS;
		// behaviour of a generic AI
		if (withChance(personality.defensiveness) && adaptVote(
				[ our.obj[OBJTYPE.TANK] + our.obj[OBJTYPE.BORG] + our.obj[OBJTYPE.VTOL], our.obj[OBJTYPE.DEFS] ],
				[ their.role[ROLE.AS], their.role[ROLE.AT] + their.role[ROLE.AP] + their.role[ROLE.AA] ]
			) === 1)
			return OBJTYPE.DEFS;
		if (iHaveVtol() && withChance(personality.vtolness) && adaptVote(
				[ our.obj[OBJTYPE.TANK] + our.obj[OBJTYPE.BORG] + our.obj[OBJTYPE.DEFS], our.obj[OBJTYPE.VTOL] ],
				[ their.role[ROLE.AA], their.role[ROLE.AT] + their.role[ROLE.AP] + their.role[ROLE.AS] ]
			) === 1)
			return OBJTYPE.VTOL;
		return adaptVote(
			[ our.obj[OBJTYPE.TANK], our.obj[OBJTYPE.BORG] ],
			[ their.role[ROLE.AP], their.role[ROLE.AT] ]
		) ? OBJTYPE.BORG : OBJTYPE.TANK;
	}
	return cached(uncached, 5000);
}

_global.scopeRatings = function() {
	function uncached() {
		var ret = { land: 0, sea: 0, air: 0 };
		enumLivingPlayers().filter(isEnemy).forEach(function(player) {
			ret.land += countLandTargets(player);
			ret.sea += countSeaTargets(player);
			ret.air += countAirTargets(player);
		});
		return ret;
	}
	return cached(uncached, 5000);
}

_global.spendMoney = function() {
	queue("checkResearch", 100);
	if (checkTruckProduction())
		return; // will proceed on the next cycle
	if (chooseObjectType() == OBJTYPE.DEFS)
		queue("checkConstruction", 200);
	else
		queue("checkProduction", 300);
}

////////////////////////////////////////////////////////////////////////////////////////////
})(this);
