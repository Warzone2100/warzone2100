
/*
 * This file is responsible for incoming laser satellite strikes.
 * Relies on lassatSplash variable of the ruleset to figure out
 * the radius of lassat.
 *
 */

(function(_global) {
////////////////////////////////////////////////////////////////////////////////////////////

// pick a target and fire
_global.fireLassat = function(structure) {
	var list = [];
	enumLivingPlayers().filter(isEnemy).forEach((i) => {
		list = list.concat(enumStruct(i), enumDroid(i));
	});
	var maxIdx, maxPrice = 0;
	list.forEach((obj, idx) => {
		var price = enumRange(obj.x, obj.y, lassatSplash / 2, ENEMIES, false).reduce((prev, curr) => (prev + curr.cost), 0);
		if (price > maxPrice) {
			maxPrice = price;
			maxIdx = idx;
		}
	});
	activateStructure(structure, list[maxIdx]);
}

////////////////////////////////////////////////////////////////////////////////////////////
})(this);
