
/*
 * This file controls the AI's research choices.
 * 
 */

(function(_global) {
////////////////////////////////////////////////////////////////////////////////////////////

var forcedResearch;

_global.setForcedResearch = function(list) {
	forcedResearch = list;
}

_global.needFastestResearch = function() {
	var ret = scopeRatings();
	if (ret.land === 0 && ret.sea === 0 && !iHaveVtol())
		return PROPULSIONUSAGE.VTOL;
	if (ret.land === 0 && ret.sea !== 0 && !iHaveHover() && !iHaveVtol())
		return PROPULSIONUSAGE.HOVER;
	return PROPULSIONUSAGE.GROUND;
}

function doResearch(lab) {
	if (defined(forcedResearch)) {
		if (forcedResearch === null)
			return false;
		if (pursueResearch(lab, forcedResearch))
			return true;
	}
	// if we need to quickly get a certain propulsion to reach the enemy, prioritize that.
	var fastest = needFastestResearch();
	if (fastest === PROPULSIONUSAGE.VTOL)
		if (pursueResearch(lab, [
			propulsionStatsToResList(PROPULSIONUSAGE.VTOL),
			fastestResearch,
		].random())) 
			return true;
	if (fastest === PROPULSIONUSAGE.HOVER)
		if (pursueResearch(lab, [
			propulsionStatsToResList(PROPULSIONUSAGE.HOVER),
			propulsionStatsToResList(PROPULSIONUSAGE.VTOL),
			fastestResearch,
		].random()))
			return true;
	// otherwise, start with completing the fixed research path
	if (defined(personality.earlyResearch))
		if (pursueResearch(lab, personality.earlyResearch))
			return true;
	// then, see if we want to research some weapons
	var objType = chooseObjectType();
	if (withChance(70)) { // TODO: make a more thoughtful decision here
		var list = weaponStatsToResList(chooseAvailableWeaponPathByRoleRatings(personality.weaponPaths, chooseWeaponRole()), objType);
		if (pursueResearch(lab, list))
			return true;
	}
	if (withChance(65)) { // TODO: make a more thoughtful decision here
		if (chooseBodyClass() === BODYCLASS.KINETIC) {
			if (withChance(40))
				if (pursueResearch(lab, classResearch.kinetic[objType]))
					return true;
			if (objType === OBJTYPE.TANK || objType === OBJTYPE.VTOL || (objType === OBJTYPE.DEFS && personality.defensiveness < 100))
				if (pursueResearch(lab, bodyStatsToResList(BODYCLASS.KINETIC)))
					return true;
		} else {
			if (withChance(40))
				if (pursueResearch(lab, classResearch.thermal[objType]))
					return true;
			if (objType === OBJTYPE.TANK || objType === OBJTYPE.VTOL || (objType === OBJTYPE.DEFS && personality.defensiveness < 100))
				if (pursueResearch(lab, bodyStatsToResList(BODYCLASS.THERMAL)))
					return true;
		}
	}
	// if nothing of the above holds, do some generic research
	if (pursueResearch(lab, fundamentalResearch))
		return true;
	return false;
	
}

_global.checkResearch = function() {
	enumIdleStructList(structures.labs).forEach(doResearch);
}

////////////////////////////////////////////////////////////////////////////////////////////
})(this);
