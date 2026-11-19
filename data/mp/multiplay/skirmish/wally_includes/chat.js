function eventChat(from, to, message) {
	if (from === me || !friendlyPlayer(from)) {
		return;
	}

	const ally = alliesList.find(a => a.position === from) || playerDataObject(from);
	if (!ally) return;

	const msg = (message || "").trim().toLowerCase();

	if (msg === "need truck" || msg === "truck") {
		const myTrucksCount = countDroid(DROID_CONSTRUCT, me);

		if (to === me && myTrucksCount >= 5) {
			const closestTruckFromAlly = enumDroid(me, DROID_CONSTRUCT)
				.sort((obj1, obj2) => sortByDistToPlayerBase(obj1, obj2, from))[0];

			if (!closestTruckFromAlly) {
				return;
			}

			let realObject = getObject(closestTruckFromAlly.type, me, closestTruckFromAlly.id);
			if (!realObject) {
				return; // Just for extra precaution
			}

			// Check if there is at least 1 truck available
			if (closestTruckFromAlly.health > 0) {
				if (donateObject(closestTruckFromAlly, from)) {
					chat(ALLIES, `${ally.name} I've sent you a truck`);
				}
			}
		} else if (to === me) {
			chat(ALLIES, `${ally.name} I'm sorry, I don't have so many trucks`);
		}
	}

	if (msg === "help me" || msg === "need help" || msg === "help") {
		if (isHelpingAlly === true) {
			chat(ALLIES, `Sorry ${ally.name}! I'm busy right now helping someone else.`);
			return;
		}

		helpAlly(ally);
	}

	if (msg === "need power" || msg === "power") {
		const currentPower = getRealPower(me);
		if (currentPower > 600) {
			const powerToDonate = Math.floor(currentPower / 3);
			donatePower(powerToDonate, from);
			chat(from, `I've sent you ${powerToDonate} power!`);
		} else if (to === me) {
			chat(from, `Sorry ${ally.name}, I'm low on power right now.`);
		}
	}

	if (msg === "attack" || msg === "target") {
		if (currentEnemy && currentEnemy.name) {
			chat(ALLIES, `I am currently targeting ${currentEnemy.name} (Player ${currentEnemy.position}). Drop a beacon on an enemy if you want me to switch target!`);
		}
	}
}
