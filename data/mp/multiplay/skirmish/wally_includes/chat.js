// An ally asked for a truck (typed "need truck" / "truck", or the quick chat "transfer me trucks so I can rebuild")
function handleTruckRequest(from, ally) {
	if (donateTruckTo(from)) {
		chat(ALLIES, `${ally.name} I've sent you a truck`);
	} else {
		if (enumDroid(me, DROID_CONSTRUCT).length <= MIN_TRUCKS_TO_KEEP) {
			allyTruckRequestTime = gameTime; // Build one for this ally as soon as possible
		}
		chat(ALLIES, `${ally.name} I'm sorry, I can't spare a truck right now`);
	}
}

// An ally asked for military help
function handleHelpRequest(ally, announceIfBusy) {
	if (isHelpingAlly === true) {
		if (announceIfBusy) {
			chat(ALLIES, `Sorry ${ally.name}! I'm busy right now helping someone else.`);
		}
		return;
	}

	helpAlly(ally);
}

// An ally asked for power
function handlePowerRequest(from, ally, announceIfPoor) {
	const currentPower = getRealPower(me);
	if (currentPower > 600) {
		const powerToDonate = Math.floor(currentPower / 3);
		donatePower(powerToDonate, from);
		chat(from, `I've sent you ${powerToDonate} power!`);
	} else if (announceIfPoor) {
		chat(from, `Sorry ${ally.name}, I'm low on power right now.`);
	}
}

function eventChat(from, to, message) {
	if (from === me || !friendlyPlayer(from)) {
		return;
	}

	const ally = alliesList.find(a => a.position === from) || playerDataObject(from);
	if (!ally) return;

	const msg = (message || "").trim().toLowerCase();

	if (msg === "need truck" || msg === "truck") {
		if (to === me) {
			handleTruckRequest(from, ally);
		}
	}

	if (msg === "help me" || msg === "need help" || msg === "help") {
		handleHelpRequest(ally, true);
		return;
	}

	if (msg === "need power" || msg === "power") {
		handlePowerRequest(from, ally, to === me);
	}

	if (msg === "attack" || msg === "target") {
		if (currentEnemy && currentEnemy.name) {
			chat(ALLIES, `I am currently targeting ${currentEnemy.name} (Player ${currentEnemy.position}). Drop a beacon on an enemy if you want me to switch target!`);
		}
	}
}

// Human teammates mostly talk through the quick chat menu: answer the same requests there
function eventQuickChat(from, to, messageEnum, teamSpecific) {
	if (from === me || !friendlyPlayer(from) || typeof WzQuickChatMessages === "undefined") {
		return;
	}

	const ally = alliesList.find(a => a.position === from) || playerDataObject(from);
	if (!ally) return;

	switch (messageEnum) {
		case WzQuickChatMessages.TEAM_REQUEST_TRANSFER_REBUILD: // "Can someone please transfer me trucks so I can rebuild?"
			handleTruckRequest(from, ally);
			break;
		case WzQuickChatMessages.TEAM_REQUEST_NEED_HELP: // "I need help!"
			handleHelpRequest(ally, true);
			break;
		case WzQuickChatMessages.TEAM_NOTICE_BEING_ATTACKED: // "I'm being attacked!"
			handleHelpRequest(ally, false);
			break;
		case WzQuickChatMessages.TEAM_NOTICE_DONT_HAVE_POWER: // "I don't have enough power"
			handlePowerRequest(from, ally, false);
			break;
		case WzQuickChatMessages.TEAM_SUGGESTION_BUILD_ANTI_AIR: // "Build anti-air"
			if (vtolsEnabled) {
				enemiesHaveVTOLs = true;
			}
			break;
		default:
			break;
	}
}
