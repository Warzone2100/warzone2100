
/*
 * This file lists all timers used by the AI.
 * 
 */

(function(_global) {
////////////////////////////////////////////////////////////////////////////////////////////

// slightly shift all timers so that not to get too many of them on the same game frame,
// especially when multiple AI instances are running
function rnd() {
	return random(201) - 100;
}

_global.setTimers = function() {
	setTimer("spendMoney", 2000 + 3 * rnd());
	setTimer("checkConstruction", 3000 + 8 * rnd());
	setTimer("checkAttack", 100);
	setTimer("adaptCycle", 100);
	setTimer("rebalanceGroups", 10000 + 20 * rnd());
	if (difficulty === EASY)
		setTimer("goEasy", 30000);
}

////////////////////////////////////////////////////////////////////////////////////////////
})(this);
