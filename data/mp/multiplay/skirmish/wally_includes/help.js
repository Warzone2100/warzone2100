function checkAllyBaseInTrouble() {
	// A help mission lasts 3 minutes at most
	if (isHelpingAlly && gameTime >= helpDeadline) {
		stopHelpingAlly();
		return;
	}

	// Help an ally, only if my base is safe
	if (isMyBaseInTrouble) {
		return;
	}

	let anyAllyInTrouble = false;

	for (const ally of alliesList) {
		if (!isPlayerAlive(ally.position)) {
			continue; // Skip dead allies instead of returning
		}

		const allyBasePosition = ally.base;
		const isSafe = numWeapObjectsInRange(allyBasePosition.x, allyBasePosition.y, ENEMIES, ally.position, BASE_THREAT_RANGE, true);

		if (!isSafe) {
			anyAllyInTrouble = true;
			if (isHelpingAlly === false) {
				helpAlly(ally);
				return; // We only help one ally at a time
			}
		}
	}

	// If no allies are in trouble, retire the helpers
	if (!anyAllyInTrouble) {
		stopHelpingAlly();
	}
}

var lastSorryChatTime = 0;
// Where the helpers gather before advancing together on the ally's base ({x, y, until}), or null once they advance
var helpStage = null;
// playerIndex -> game time of our last truck / power gift to that ally
var lastTruckGiftTime = {};
var lastPowerGiftTime = {};
// Game time at which an ally needed a truck we could not spare (the factory then builds one to give away)
var allyTruckRequestTime = 0;
// Game time at which the current help mission ends regardless of the situation
var helpDeadline = 0;
// kind -> game time of the last team signal of that kind; "who:kind" -> game time of the last chat line of that kind
var lastTeamSignalTime = {};
var lastTeamChatTime = {};

// Tells the team what Wally is doing, the way a human teammate would: a beacon on the map plus a quick chat line.
//  "attacked"  -> beacon on our base + "I'm being attacked!"  (at most every 2 minutes)
//  "attacking" -> beacon on the target + "Attacking now"      (at most every 4 minutes)
// Allied Wallys understand both: they come to help, or pick the same enemy.
function notifyAllies(kind, x, y) {
	if (!alliesList || alliesList.length === 0) {
		return;
	}
	if (typeof lastTeamSignalTime !== "object" || lastTeamSignalTime === null) {
		lastTeamSignalTime = {};
	}
	const cooldown = kind === "attacked" ? 120000 : 240000;
	const last = lastTeamSignalTime[kind];
	if (last !== undefined && gameTime >= last && gameTime - last < cooldown) {
		return;
	}
	lastTeamSignalTime[kind] = gameTime;

	if (typeof addBeacon === "function") {
		addBeacon(Math.max(0, Math.min(mapWidth - 1, x)), Math.max(0, Math.min(mapHeight - 1, y)), ALLIES);
	}
	if (typeof quickChat === "function" && typeof WzQuickChatMessages !== "undefined") {
		quickChat(ALLIES, kind === "attacked"
			? WzQuickChatMessages.TEAM_NOTICE_BEING_ATTACKED
			: WzQuickChatMessages.TEAM_NOTICE_ATTACKING_NOW);
	}
	wallyLog("team signal: " + kind + " at " + x + "," + y);
}

// Sends a chat line to the team unless the same kind of line about the same player was sent less than `cooldownMs` ago
function chatOnce(key, cooldownMs, message) {
	if (typeof lastTeamChatTime !== "object" || lastTeamChatTime === null) {
		lastTeamChatTime = {};
	}
	const last = lastTeamChatTime[key];
	if (last !== undefined && gameTime >= last && gameTime - last < cooldownMs) {
		return;
	}
	lastTeamChatTime[key] = gameTime;
	chat(ALLIES, message);
}

// Returns true if the ally is helped, false otherwise
function helpAlly(allyPlayerData) {
	currentHelpedAlly = allyPlayerData;

	const allyData = allyPlayerData;
	const allAttackers = enumGroup(attackerGroup);

	const minAttackersForHelp = isHighOilMap ? 10 : 4;
	if (allAttackers.length < minAttackersForHelp) {
		if (gameTime - lastSorryChatTime > 120000) {
			chat(ALLIES, `Sorry ${allyData.name}! I don't have enough units to help you right now.`);
			lastSorryChatTime = gameTime;
		}
		return false; // Early exit if not enough droids
	}

	const enemiesDroids = enumRange(allyData.base.x, allyData.base.y, BASE_THREAT_RANGE, ENEMIES, true)
		.filter(obj => obj.type === DROID && (obj.droidType === DROID_WEAPON || obj.droidType === DROID_CYBORG || obj.droidType === DROID_PERSON));

	if (enemiesDroids.length === 0) {
		return false; // No enemiesDroids found
	}

	// Sort our attackers by distance to the ally's base (closest first)
	const sortedAttackers = allAttackers.sort((a, b) => {
		return distBetweenTwoPoints(a.x, a.y, allyData.base.x, allyData.base.y) -
			   distBetweenTwoPoints(b.x, b.y, allyData.base.x, allyData.base.y);
	});

	// Transfer up to half of our attackers (max 20) to the helperGroup
	const numToTransfer = Math.min(20, Math.floor(sortedAttackers.length / 2));
	const helpers = sortedAttackers.slice(0, numToTransfer);
	for (const helper of helpers) {
		groupAdd(helperGroup, helper);
	}

	// Arriving one by one only feeds the enemy. The helper closest to the ally waits where it stands and the others
	// gather on it (for at most 25 seconds); then the whole group advances together (see helpAttackEnemy).
	// If that helper is already next to the fight, there is no time to gather: everybody attack-moves in at once.
	const leader = helpers[0];
	const leaderDist = distBetweenTwoPoints(leader.x, leader.y, allyData.base.x, allyData.base.y);
	if (helpers.length > 1 && leaderDist > BASE_THREAT_RANGE * 1.5) {
		helpStage = { x: leader.x, y: leader.y, until: gameTime + 25000 };
		for (const helper of helpers) {
			orderDroidLoc(helper, DORDER_MOVE, helpStage.x, helpStage.y);
		}
	} else {
		helpStage = null;
		for (const helper of helpers) {
			orderDroidLoc(helper, DORDER_SCOUT, allyData.base.x, allyData.base.y);
		}
	}

	// (One announcement per ally every 5 minutes: a long siege used to repeat this line for every 3-minute mission)
	chatOnce("help:" + allyData.position, 300000, `Hold on ${allyData.name}! I'm coming to help you, ${numToTransfer} units are on the way`);

	helpingAlly();

	return true;
}

function helpingAlly() {
	// Set helping state
	isHelpingAlly = true;

	// After 3 minutes, stop helping the ally regardless (checked by checkAllyBaseInTrouble).
	// A single deadline instead of queue(): queued calls piled up and cut the next mission short.
	helpDeadline = gameTime + 180000;
}

function stopHelpingAlly() {
	if (isHelpingAlly === false) {
		return;
	}

	// Transfer all helpers back to the main attackerGroup
	const helpers = enumGroup(helperGroup);
	for (const droid of helpers) {
		groupAdd(attackerGroup, droid);
	}

	// (The checkAllyBaseInTrouble timer is never removed, so it must not be registered again here:
	// every extra setTimer call would add one more copy of it.)
	setCurrentEnemy();
	helpStage = null;
	isHelpingAlly = false;
}

function myBaseInTrouble() {
	const isSafe = numWeapObjectsInRange(BASE.x, BASE.y, ENEMIES, me, BASE_THREAT_RANGE, true);

	if (isSafe) {
		isMyBaseInTrouble = false;
		return false;
	} else {
		isMyBaseInTrouble = true;
		// Ground enemies outnumber us at home: let the team know (beacon on our base + quick chat)
		notifyAllies("attacked", BASE.x, BASE.y);
		if (isHelpingAlly === true) {
			stopHelpingAlly();
			chatOnce("stophelp", 300000, "Sorry! I have to stop helping you, my base is in danger!");
		}
		return true;
	}
}

function donateOil() {
	let currentPower = getRealPower(me);

	// Do not proceed if power is below 5000 or no allies exist
	if (currentPower < 5000 || alliesList.length === 0) {
		return;
	}

	// Sort allies by power (lowest first) so we always help the poorest ally
	const sortedAllies = [...alliesList]
		.filter(ally => isPlayerAlive(ally.position))
		.sort((a, b) => getRealPower(a.position) - getRealPower(b.position));

	for (const ally of sortedAllies) {
		const allyPower = getRealPower(ally.position);
		// Check if the ally needs power
		if (allyPower < 50 && friendlyPlayer(ally.position)) {
			let powerToDonate;

			// Calculate power to donate based on current power levels
			if (currentPower >= 15000) {
				powerToDonate = currentPower / 3.5;
			} else if (currentPower >= 10000) {
				powerToDonate = currentPower / 3;
			} else {
				powerToDonate = currentPower / 2.5;
			}

			// Ensure donation will not drop below 5000 power
			if (currentPower - powerToDonate < 5000) {
				continue; // Skip this ally if donation would drop power too low
			}

			// Donate power to the ally
			donatePower(powerToDonate, ally.position);
			chat(ally.position, `I've sent you: ${powerToDonate.toFixed(2)} power to help you out.`);

			// Update current power after donation
			currentPower -= powerToDonate;

			// Exit loop after donating to the poorest ally
			break;
		}
	}
}


// Gives one of our trucks to an ally. We always keep MIN_TRUCKS_TO_KEEP for ourselves, prefer a truck that is not
// in the middle of a job, and among those the one closest to the ally's base. Returns true if a truck was sent.
function donateTruckTo(playerIndex) {
	if (!isActiveAllySlot(playerIndex)) {
		return false;
	}
	if (typeof lastTruckGiftTime !== "object" || lastTruckGiftTime === null) {
		lastTruckGiftTime = {};
	}
	const last = lastTruckGiftTime[playerIndex];
	if (last !== undefined && gameTime >= last && gameTime - last < 45000) {
		return false; // One truck at a time: give the ally a moment to use it
	}

	const trucks = enumDroid(me, DROID_CONSTRUCT).filter(t => t.order !== DORDER_RECYCLE);
	if (trucks.length <= MIN_TRUCKS_TO_KEEP) {
		wallyLog("cannot give a truck to player " + playerIndex + ": only " + trucks.length + " left");
		return false;
	}

	const allyBase = startPositions[playerIndex] || BASE;
	const score = (truck) => (isTruckBusy(truck) ? 1000 : 0) + distBetweenTwoPoints(truck.x, truck.y, allyBase.x, allyBase.y);
	trucks.sort((a, b) => score(a) - score(b));

	const gift = trucks[0];
	if (!donateObject(gift, playerIndex)) {
		wallyLog("the game refused the truck transfer to player " + playerIndex + " (unit limit?)");
		return false;
	}

	lastTruckGiftTime[playerIndex] = gameTime;
	if (typeof pendingBuildOrders !== "undefined" && pendingBuildOrders instanceof Map) {
		pendingBuildOrders.delete(gift.id);
	}
	wallyLog("gave a truck to player " + playerIndex + ", " + (trucks.length - 1) + " left");
	return true;
}

// Every 20 seconds: any ally without trucks and without a working factory gets one of ours.
// If we cannot spare one right now, the factory is told to build a truck for that purpose (see produceDroids).
function checkForDefeatedAllies() {
	if (currentlyDead) {
		return;
	}

	for (let i = 0; i < maxPlayers; ++i) {
		if (!isActiveAllySlot(i) || countDroid(DROID_CONSTRUCT, i) > 0) {
			continue;
		}

		if (allyNeedsTruck(i)) {
			if (donateTruckTo(i)) {
				allyTruckRequestTime = 0;
				chat(ALLIES, `${getPlayerName(i)} I've sent you a truck :P`);
			} else if (enumDroid(me, DROID_CONSTRUCT).length <= MIN_TRUCKS_TO_KEEP) {
				allyTruckRequestTime = gameTime;
			}
			return; // One ally per check
		}

		// The ally still has a factory but no truck and no money to build one: a little power does the job
		if (typeof lastPowerGiftTime !== "object" || lastPowerGiftTime === null) {
			lastPowerGiftTime = {};
		}
		const lastGift = lastPowerGiftTime[i];
		if (playerPower(i) < 80 && getRealPower(me) > 300 &&
			(lastGift === undefined || gameTime < lastGift || gameTime - lastGift > 60000)) {
			donatePower(150, i);
			lastPowerGiftTime[i] = gameTime;
			chat(i, "I've sent you 150 power so you can build a truck.");
			return;
		}
	}
}
