function eventStructureReady(structure) {
	if (!structure) {
		const LASER = enumStruct(me, LASERLINK_STAT);

		if (LASER.length > 0) {
			structure = LASER[0];
		} else {
			return;
		}
	}

	// If no enemy structure was found, try again in 1 minute
	const RETRY_TIME = 60000 + random(25) + random(50);

	const struct = returnClosestEnemyStructure();

	if (struct) {
		const obj = getObject(struct.typeInfo, struct.playerInfo, struct.idInfo);
		if (!obj) {
			queue("eventStructureReady", RETRY_TIME, structure);
			return;
		}

		activateStructure(structure, obj);

		const enemyPlayerName = getPlayerName(obj.player) || "Hmmm";
		chat(ALL_PLAYERS, `${enemyPlayerName}... Laser is ready, I aim it steady!`);
	} else {
		queue("eventStructureReady", RETRY_TIME, structure);
	}
}

function returnClosestEnemyStructure() {
	let enemyIndex = currentEnemy ? currentEnemy.position : undefined;

	if (enemyIndex === undefined) {
		const aliveEnemies = getAliveEnemyPlayersData();
		if (!aliveEnemies || aliveEnemies.length === 0) {
			return;
		}
		enemyIndex = aliveEnemies[0].position;
	}

	if (enemyIndex !== undefined) {
		const structureTypes = [LASSAT, VTOL_FACTORY, FACTORY, CYBORG_FACTORY, HQ, null];

		for (const type of structureTypes) {
			const structs = type ? enumStruct(enemyIndex, type) : enumStruct(enemyIndex);
			if (structs.length > 0) {
				// Pick the structure surrounded by the highest concentration of enemy objects (maximizes LasSat splash damage)
				let bestStruct = structs[0];
				let maxDensity = -1;

				for (let i = 0; i < Math.min(structs.length, 10); i++) {
					const s = structs[i];
					const density = enumRange(s.x, s.y, 6, ENEMIES, false).length;
					if (density > maxDensity) {
						maxDensity = density;
						bestStruct = s;
					}
				}

				return objectInformation(bestStruct);
			}
		}
	}
}


function objectInformation(object) {
	return {
		"typeInfo": object.type,
		"playerInfo": object.player,
		"idInfo": object.id
	};
}
