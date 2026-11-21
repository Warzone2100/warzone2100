function checkAllyBaseInTrouble() {
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
	for (let i = 0; i < numToTransfer; i++) {
		groupAdd(helperGroup, sortedAttackers[i]);
		// Immediately order them to move toward the ally's base
		orderDroidLoc(sortedAttackers[i], DORDER_MOVE, allyData.base.x, allyData.base.y);
	}

	chat(ALLIES, `Hold on ${allyData.name}! I'm coming to help you, ${numToTransfer} units are on the way`);

	helpingAlly();

	return true;
}

function helpingAlly() {
	// Set helping state
	isHelpingAlly = true;

	// Keep checking if ally still needs help (don't remove the timer!)
	queue("stopHelpingAlly", 180000); // After 3 minutes, stop helping the ally regardless
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

	setCurrentEnemy();
	setTimer("checkAllyBaseInTrouble", reactionSpeedDelay + 15000 + ((1 + random(30)) * random(10))); // 15 Seconds
	isHelpingAlly = false;
}

function myBaseInTrouble() {
	const isSafe = numWeapObjectsInRange(BASE.x, BASE.y, ENEMIES, me, BASE_THREAT_RANGE, true);

	if (isSafe) {
		isMyBaseInTrouble = false;
		return false;
	} else {
		isMyBaseInTrouble = true;
		if (isHelpingAlly === true) {
			stopHelpingAlly();
			chat(ALLIES, "Sorry! I have to stop helping you, my base is in danger!");
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


function checkForDefeatedAllies() {
	if (countDroid(DROID_CONSTRUCT, me) < 5) {
		return;
	}

	const defeatedAllies = getDefeatedAllyPlayersData();

	for (const ally of defeatedAllies) {
		const trucks = enumDroid(me, DROID_CONSTRUCT);
		if (trucks.length < 5) break;

		const truckForAlly = trucks[0];
		if (truckForAlly && donateObject(truckForAlly, ally.position)) {
			chat(ALLIES, `${ally.name} I've sent you a truck :P`);
		}
	}
}
