
/*
 * This file includes event definitions only.
 * 
 */

function eventStartLevel() {
	queue("setTimers", me * 100);
	if (alliancesType === ALLIANCES_TEAMS) {
		// initialize subpersonality pseudo-randomly here
		// to make sure teammates have the same personality
		var j = 1, s = 0;
		for (var i = 0; i < maxPlayers; ++i) {
			if (allianceExistsBetween(me, i))
				s += j;
			j *= 2;
		}
		// the random "s" number obtained here is the same for all players in any team
		var s = s + (new Date()).getMinutes();
		j = 0;
		for (var i in subpersonalities) // count the amount of subpersonalities
			++j;
		s = s % j;
		j = 0;
		for (var i in subpersonalities) {
			if (j === s)
				personality = subpersonalities[i];
			++j;
		}
	} else {
		// if teams are not sharing research, or there are no teams at all,
		// initialize the subpersonality randomly and don't care
		personality = randomItem(subpersonalities);
	}
	enumDroid(me).forEach(function(droid) {
		if (droid.droidType === DROID_CONSTRUCT)
		{
			// the following code is necessary to avoid some strange game bug when droids that
			// are initially buried into the ground fail to move out of the way when a building
			// is being placed right above them
			orderDroidLoc(droid, DORDER_MOVE, droid.x + random(3) - 1, droid.y + random(3) - 1);
		}
		else
		{	// add other droids to groups so they won't be idle
			groupDroid(droid);
		}
	});
}

function eventDroidBuilt(droid, structure) {
	groupDroid(droid);
}

function eventStructureBuilt(structure) {
	queue("checkConstruction");
}

function eventAttacked(victim, attacker) {
	if (attacker === null)
		return; // no idea why it happens sometimes
	if (isAlly(attacker.player))
		return; // don't respond to accidental friendly fire
	if (victim.type === DROID) {
		if (!isVTOL(victim) && defined(victim.group)) {
			fallBack(victim, attacker);
			setTarget(attacker, victim.group);
			touchGroup(victim.group);
		} else if (isVTOL(victim) && victim.player == me && !throttled(5000, victim.id)) {
			orderDroidObj(victim, DORDER_ATTACK, attacker);
			pushVtols(attacker);
		}
	} else if (victim.type === STRUCTURE) {
		if (throttled(5000) && victim.player != me)
			return;
		if (inPanic())
			for (var i = 0; i < MAX_GROUPS; ++i)
				if (groupSize(i) > 0)
					setTarget(attacker, i);
		setTarget(attacker, miscGroup);
		setTarget(attacker);
	}
}

function eventStructureReady(structure) {
	fireLassat(structure);
}

function eventChat(from, to, message) {
	// we are not case-sensitive
	message = message.toLowerCase();
    handleChatMessage(from, to, message)
}

function eventObjectTransfer(object, from) {
	if (object.player !== me)
		return; // object was transferred from me, not to me
	if (object.type === DROID)
		groupDroid(object);
}

function eventBeacon(x, y, from, to) {
	noticeBeacon(x, y, from);
}

function eventBeaconRemoved(from, to) {
	unnoticeBeacon(from);
}

function eventDestroyed(object) {
	if (isEnemy(object.player))
		pushVtols(object);
}
