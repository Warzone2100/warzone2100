// Game times at which a mobile anti-air tank was recently ordered, so several factories do not all fill the same gap
var recentAntiAirOrders = [];

// Number of mobile AA tanks we want escorting the army: scales with the enemy VTOL fleet, capped at a quarter of the army
function getDesiredMobileAA(armySize) {
	if (!vtolsEnabled || !enemiesHaveVTOLs || !AA_WEAPON_LIST.some(w => componentAvailable(w))) {
		return 0;
	}
	const wanted = Math.max(2, Math.ceil(countEnemyVTOLs() * 0.6));
	const cap = Math.max(3, Math.floor(armySize * 0.25));
	return Math.min(wanted, cap, 16);
}

// How many more mobile AA tanks should be ordered right now (built + already in production are subtracted)
function getMobileAADeficit(armySize) {
	const desired = getDesiredMobileAA(armySize);
	if (desired === 0) {
		return 0;
	}
	if (!Array.isArray(recentAntiAirOrders)) {
		recentAntiAirOrders = [];
	}
	recentAntiAirOrders = recentAntiAirOrders.filter(t => gameTime >= t && gameTime - t < 45000);
	return desired - enumGroup(aaGroup).length - recentAntiAirOrders.length;
}

function produceDroids() {
	// List of factory types
	const FAC_LIST = [FACTORY_STAT, VTOL_FACTORY_STAT, CYBORG_FACTORY_STAT];
	let virtualTrucks = 0; // Count of trucks currently being produced
	let virtualAttackers = 0; // Count of combat units currently being produced

	// Step 1: Count trucks and combat units currently being built across all factories
	const allGroundFactory = enumStruct(me, FACTORY_STAT).concat(enumStruct(me, CYBORG_FACTORY_STAT));

	if (allGroundFactory.length === 0) {
		return; // Stop function if there are no factories
	}

	const allFactories = allGroundFactory.concat(enumStruct(me, VTOL_FACTORY_STAT));
	for (let i = 0; i < allFactories.length; ++i) {
		const droid = getDroidProduction(allFactories[i]); // Get the droid being built
		if (droid !== null) {
			if (droid.droidType === DROID_CONSTRUCT) {
				virtualTrucks += 1;
			} else if (droid.droidType === DROID_WEAPON || droid.droidType === DROID_CYBORG || droid.isVTOL === true) {
				virtualAttackers += 1;
			}
		}
	}

	// Step 2: Manage unit production for all factory types
	for (let i = 0; i < FAC_LIST.length; ++i) {
		// Skip cyborg factories on sea maps
		if (FAC_LIST[i] === CYBORG_FACTORY_STAT && isSeaMap === true) {
			continue;
		}

		// Get list of factories for the current type
		let typeFactories = enumStruct(me, FAC_LIST[i]);

		for (let j = 0; j < typeFactories.length; ++j) {
			let factory = typeFactories[j];

			// Proceed only if the factory is idle
			if (structureIdle(factory)) {

				const numTrucks = countDroid(DROID_CONSTRUCT) + virtualTrucks;
				const desiredTrucks = getDesiredTruckCount();
				const currentPower = getRealPower();
				const armySize = enumGroup(attackerGroup).length + enumGroup(rallyGroup).length + virtualAttackers;
				// Low oil: until the army reaches the size the economy can support, units get first call on power
				const armyThin = !isHighOilMap && armySize < getLowOilArmyTarget();
				const totalGens = countStruct(POW_GEN_STAT, me);
				const completedGens = enumStruct(me, POW_GEN_STAT).filter(s => s.status === BUILT).length;
				const neededGens = Math.ceil(countStruct(DERRICK_STAT, me) / 4);

				// Only save power to pay for an unplaced Power Generator (not when already paid/under construction or before our first 4 combat units exist)
				const mustSavePowerForGen = !isMyBaseInTrouble && (
					(totalGens === 0 && currentPower < 120) ||
					(armySize >= 4 && numUnusedDerricks() > 0 && totalGens < neededGens && currentPower < 110)
				);

				if (mustSavePowerForGen && numTrucks > 0) {
					continue;
				}

				// Check if any research lab is sitting idle waiting for power
				const hasIdleLab = !researchDone && enumStruct(me, RES_LAB_STAT).some(
					lab => lab.status === BUILT && structureIdle(lab)
				);

				// Allow early combat production once at least 1 Power Generator and 1 Oil Derrick exist (or base is under attack)
				const hasEconomyForCombat = isMyBaseInTrouble || (totalGens > 0 && countStruct(DERRICK_STAT) > 0);
				const canAffordCombatUnit = hasEconomyForCombat && (
					isMyBaseInTrouble ||
					(armySize < 4 && currentPower > -50) ||
					(armySize < 6 && currentPower >= 25) ||
					(armyThin && currentPower >= 40) ||
					(currentPower >= MIN_PRODUCTION_POWER && (!hasIdleLab || currentPower >= 100))
				);

				if (FAC_LIST[i] === FACTORY_STAT) {
					// On low resources / no bases, only require a minimal workforce (3 trucks, then 4 after first 2 combat units)
					// before producing our initial tanks, then top up remaining trucks afterward
					const minEarlyTrucks = isMyBaseInTrouble ? 2 : (isHighOilMap ? 6 : (armySize < 2 ? 3 : 4));
					const minEarlyArmy = isHighOilMap ? 6 : 4;
					const canBuildEarlyCombat = canAffordCombatUnit && (!isSeaMap || componentAvailable("hover01") || isMyBaseInTrouble);
					// An ally has no truck left and we could not spare one: build one more so it can be given away
					const truckOwedToAlly = allyTruckRequestTime > 0 && gameTime >= allyTruckRequestTime &&
						gameTime - allyTruckRequestTime < 120000 && numTrucks <= MIN_TRUCKS_TO_KEEP;
					const shouldBuildTruck = numTrucks < desiredTrucks && (
						truckOwedToAlly ||
						numTrucks < minEarlyTrucks ||
						(!canBuildEarlyCombat && (completedGens > 0 || currentPower >= 150)) ||
						(armySize >= minEarlyArmy && (completedGens > 0 || currentPower >= 150))
					);

					if (shouldBuildTruck) {
						if (produceBuilder(factory)) {
							virtualTrucks += 1;
						}
					} else if (canAffordCombatUnit) {
						if (random(100) < createSensorProbability()) {
							produceSensors(factory); // Occasionally build sensors
						} else if (produceTank(factory, getMobileAADeficit(armySize))) {
							virtualAttackers += 1; // Build an attacker unit
						}
					}
				} else {
					if (FAC_LIST[i] === CYBORG_FACTORY_STAT) {
						const shouldBuildCyborgConstructor = numTrucks < desiredTrucks && !countStruct(FACTORY_STAT, me);

						if (shouldBuildCyborgConstructor) {
							if (produceCyborgBuilder(factory)) {
								virtualTrucks += 1;
							}
						} else if (canAffordCombatUnit) {
							if (produceCyborg(factory)) {
								virtualAttackers += 1; // Build cyborg units
							}
						}
					} else {
						// For VTOL factory, ensure power generator exists and we can afford combat units.
						// On low oil the ground army comes first: no VTOLs while it is still thin.
						if (!countStruct(POW_GEN_STAT) || !canAffordCombatUnit || (armyThin && !isMyBaseInTrouble)) {
							continue;
						}
						if (produceVTOL(factory)) {
							virtualAttackers += 1; // Build VTOL units
						}
					}
				}
			}
		}
	}
}

function produceBuilder(factory) {
	return createDroid(factory, "Constructor", BUILDER_BODY_LIST, BUILDER_PROP_LIST, "Spade1Mk1");
}

function produceTank(factory, antiAirDeficit) {
	// Default components (strictly Rockets and Machineguns only, plus anti-air when VTOLs are around — no cannons, mortars, or howitzers)
	let firstWeapon;
	let secondWeapon;
	let tankProp = TANK_PROP_LIST;
	let tankBody = TANK_BODY_LIST;

	// Components with probability
	const ANTIPERSONELL_WEAPON_PROBABILITY = checkEnemyAntiPersonnelProbability();
	const THERMAL_BODY_PROBABILITY = checkFlameWeapons();
	const BUNKER_BUSTER_PROBABILITY = isMyBaseInTrouble ? 0 : checkCurrentEnemyDefenses();
	// Anti-air is built to a target count (see getDesiredMobileAA) instead of a flat dice roll:
	// the very first AA tank is immediate, the rest of the gap is filled with 60% of the new tanks
	const needsAntiAir = antiAirDeficit !== undefined && antiAirDeficit > 0;
	const ANTI_AIR_TANK_PROBABILITY = !needsAntiAir ? 0 : (enumGroup(aaGroup).length === 0 ? 100 : 60);

	let FAST_PROP_PROBABILITY = 0;

	if (isSeaMap === false) {
		FAST_PROP_PROBABILITY = checkTankKillerWeapons();
	}

	if (random(100) < THERMAL_BODY_PROBABILITY) {
		tankBody = TANK_BODY_LIST_THERMIC;
	} else if (!isHighOilMap && getMultiTechLevel() === 1 && gameTime < 600000 && getRealPower() < 260 && random(100) < 40) {
		// In early low-power T1, mix affordable light/medium bodies to put more hulls on the field faster
		tankBody = ["Body4ABT", "Body5REC", "Body1REC"];
	}

	if (getMultiTechLevel() === 1 && gameTime < 600000) {
		// Favor faster, more affordable Half-Track/Wheeled propulsion in the first 10 minutes of T1
		tankProp = ["HalfTrack", "wheeled01"];
	} else if (random(100) < FAST_PROP_PROBABILITY) {
		tankProp = "HalfTrack";
	}

	const hasRocketArray = ROCKET_ARRAY_WEAPON_LIST.some(w => componentAvailable(w));
	const hasMachinegun = MACHINEGUN_WEAPON_LIST.some(w => componentAvailable(w));
	let isAntiAirOrder = false;

	if (random(100) < ANTI_AIR_TANK_PROBABILITY) {
		firstWeapon = AA_WEAPON_LIST;
		secondWeapon = AA_WEAPON_LIST;
		isAntiAirOrder = true;
	} else if (random(100) < BUNKER_BUSTER_PROBABILITY) {
		firstWeapon = BUNKER_BUSTER_WEAPON;
		secondWeapon = BUNKER_BUSTER_WEAPON;
		tankProp = "hover01";
	} else if (random(100) < ANTIPERSONELL_WEAPON_PROBABILITY) {
		// Mixed MG + Rocket army: the anti-personnel role is shared between the Machinegun line
		// (Twin MG -> HMG -> Assault Gun -> Twin Assault Gun, direct fire) and Rocket Arrays (MRA/HRA/Seraph, splash)
		const useMachinegun = hasMachinegun && (!hasRocketArray || random(100) < MACHINEGUN_SHARE_OF_AP);
		const antiPersonnelList = useMachinegun ? MACHINEGUN_WEAPON_LIST : ROCKET_ARRAY_WEAPON_LIST;
		firstWeapon = antiPersonnelList;
		secondWeapon = antiPersonnelList;
	} else {
		firstWeapon = ROCKET_WEAPON_LIST;
		secondWeapon = ROCKET_WEAPON_LIST;
	}

	// Use Hover propulsion if map is a sea map OR if the current enemy's base cannot be reached by land propulsion
	const enemyUnreachableByLand = currentEnemy && currentEnemy.base && componentAvailable("hover01") &&
		!propulsionCanReach("wheeled01", BASE.x, BASE.y, currentEnemy.base.x, currentEnemy.base.y);

	if (isSeaMap === true || enemyUnreachableByLand) {
		tankProp = "hover01";
	}

	if (createDroid(factory, isAntiAirOrder ? "Anti-Air" : "Attacker", tankBody, tankProp, firstWeapon, secondWeapon)) {
		if (isAntiAirOrder) {
			recentAntiAirOrders.push(gameTime);
		}
		return true;
	}

	// Fallback: if the rolled weapon/propulsion isn't researched yet,
	// try any available Rocket, Rocket Array, or T1 MG weapon with standard TANK_PROP_LIST so the factory never sits idle
	const fallbackWeapons = ROCKET_WEAPON_LIST.concat(ROCKET_ARRAY_WEAPON_LIST).concat(MACHINEGUN_WEAPON_LIST);
	const fallbackProp = (isSeaMap === true || enemyUnreachableByLand) ? ["hover01", ...TANK_PROP_LIST] : TANK_PROP_LIST;
	return createDroid(factory, "Attacker", TANK_BODY_LIST, fallbackProp, fallbackWeapons, fallbackWeapons);
}

function produceSensors(factory) {
	if (componentAvailable("hover01")) {
		const sensorTurret = componentAvailable("Sensor-WideSpec") ? "Sensor-WideSpec" : "SensorTurret1Mk1";
		return createDroid(factory, "Scanner", SCANNER_BODY_LIST, "hover01", sensorTurret);
	}
}

function produceCyborgBuilder(factory) {
	return createDroid(factory, "Combat Engineer Cyborg", "CyborgLightBody", "CyborgLegs", "CyborgSpade");
}

function produceCyborg(factory) {
	// Once Rocket Arrays cover anti-personnel duties, focus cyborgs on Anti-Tank (85%), using MG cyborgs early T1 or as fallback
	const hasRocketArray = ROCKET_ARRAY_WEAPON_LIST.some(w => componentAvailable(w));
	const CYBORG_ANTITANK_WEAPON_PROBABILITY = hasRocketArray ? 85 : 70;

	if (random(100) < CYBORG_ANTITANK_WEAPON_PROBABILITY) {
		if (createDroid(factory, "Cyborg Heavy Rocket", "CyborgHeavyBody", "CyborgLegs", CYBORG_HEAVY_ROCKETS)) {
			return true;
		}
		if (createDroid(factory, "Cyborg Rocket", "CyborgLightBody", "CyborgLegs", CYBORG_LIGHT_ROCKETS)) {
			return true;
		}
		return createDroid(factory, "Cyborg Machinegun", "CyborgLightBody", "CyborgLegs", CYBORG__LIGHT_MACHINEGUN);
	} else {
		if (createDroid(factory, "Cyborg Machinegun", "CyborgLightBody", "CyborgLegs", CYBORG__LIGHT_MACHINEGUN)) {
			return true;
		}
		if (createDroid(factory, "Cyborg Laser", "CyborgHeavyBody", "CyborgLegs", CYBORG_HEAVY_LASER)) {
			return true;
		}
		if (createDroid(factory, "Cyborg Heavy Rocket", "CyborgHeavyBody", "CyborgLegs", CYBORG_HEAVY_ROCKETS)) {
			return true;
		}
		return createDroid(factory, "Cyborg Rocket", "CyborgLightBody", "CyborgLegs", CYBORG_LIGHT_ROCKETS);
	}
}

function produceVTOL(factory) {
	const hasRocketVTOL = VTOL_ROCKET_WEAPONS.some(w => componentAvailable(w));
	const hasHeavyBombVTOL = VTOL_BOMB_WEAPONS.some(w => componentAvailable(w));
	const hasBBVTOL = componentAvailable("Rocket-VTOL-BB");

	if (hasBBVTOL && currentEnemy && currentEnemy.position !== undefined &&
		enumStruct(currentEnemy.position, DEFENSE).length >= 3 && random(100) < 12) {
		if (createDroid(factory, "VTOL Bunker Buster", VTOL_BODY_LIST, "V-Tol", "Rocket-VTOL-BB")) {
			return true;
		}
	}

	if (hasRocketVTOL && (!hasHeavyBombVTOL || random(100) < 65)) {
		if (createDroid(factory, "VTOL Rocket", VTOL_BODY_LIST, "V-Tol", VTOL_ROCKET_WEAPONS)) {
			return true;
		}
	}

	if (hasHeavyBombVTOL) {
		if (createDroid(factory, "VTOL Bomber", VTOL_BODY_LIST, "V-Tol", VTOL_BOMB_WEAPONS)) {
			return true;
		}
	}

	const fallbackVTOL = VTOL_ROCKET_WEAPONS.concat(VTOL_BOMB_WEAPONS).concat(["Bomb1-VTOL-LtHE"]);
	return createDroid(factory, "VTOL Striker", VTOL_BODY_LIST, "V-Tol", fallbackVTOL);
}

/**
 * @param {*} body
 * @param {*} factory
 * @param {*} propulsion
 * @param {*} templateName
 * @description Returns true if the production was started
 */
function createDroid(factory, templateName, body, propulsion, turret1, turret2) {
	if (turret2 !== undefined) {
		return buildDroid(factory, templateName, body, propulsion, null, "", turret1, turret2);
	}
	return buildDroid(factory, templateName, body, propulsion, null, "", turret1);
}

// Build sensors according to the artillery number of the team, with a probability between a min of 3% and a max of 13%.
function createSensorProbability() {
	const sensorCount = enumDroid(me, DROID_SENSOR).length;

	if (sensorCount >= 3) {
		return 0;
	}

	const teamRippleNumber = countStruct("Emplacement-HvART-pit", ALLIES)
		+ countStruct("Emplacement-Rocket06-IDF", ALLIES);

	// Base probability with Spectral Sensor
	let probability = componentAvailable("Sensor-WideSpec") ? 6 : 3;

	// Adjust probability based on teamRippleNumber
	if (teamRippleNumber === 0) {
		probability = 0;
	} else if (teamRippleNumber < 10) {
		probability = 2;
	}

	return probability;
}

// Replace builders that have no hover propulsion, if hover is available
function recycleOldBuilders() {
	if (!componentAvailable("hover01") || !isHighOilMap) {
		return;
	}

	// Don't recycle if we don't have at least the basic base
	if (countStruct(PLAYER_HQ_STAT) === 0 || countStruct(FACTORY_STAT) === 0 || countStruct(POW_GEN_STAT) === 0) {
		return;
	}

	const buildersToRecycle = enumDroid(me, DROID_CONSTRUCT).filter(builder => builder.propulsion != "hover01");

	if (buildersToRecycle.length > 0) {
		buildersToRecycle.forEach(builder => {
			orderDroid(builder, DORDER_RECYCLE);
		});
	}
}

// Check if we have too many trucks and recycle excess ones
function checkTrucksCount() {
	const builders = enumDroid(me, DROID_CONSTRUCT); // Get all builder droids
	const excessCount = builders.length - MAX_BASE_TRUCKS;

	if (excessCount > 0) {
		const excessBuilders = builders.slice(-excessCount);

		// Recycle excess builders
		excessBuilders.forEach(builder => {
			orderDroid(builder, DORDER_RECYCLE);
		});
	}
}

// Recycle obsolete T1 units in late game to transfer veteran experience to heavy T3/T4 tanks
// and ensure we never get stuck at the 150 unit cap without trucks.
function recycleObsoleteUnits() {
	const totalDroids = countDroid(DROID_ANY, me);
	const numTrucks = countDroid(DROID_CONSTRUCT, me);

	// Emergency Unit Cap protection: if we are near the 150 droid cap and lack trucks, free up slots immediately
	if (totalDroids >= 142 && numTrucks < 2) {
		const combatDroids = enumGroup(attackerGroup);
		for (let i = 0; i < Math.min(3, combatDroids.length); i++) {
			orderDroid(combatDroids[i], DORDER_RECYCLE);
		}
		return;
	}

	// Only recycle obsolete combat units if base is safe, we have good power, and heavy bodies are unlocked
	if (isMyBaseInTrouble || getRealPower() < 800) {
		return;
	}

	const hasHeavyBodies = componentAvailable("Body9REC") || componentAvailable("Body10MBT") || componentAvailable("Body14SUP");
	if (!hasHeavyBodies) {
		return;
	}

	const OBSOLETE_BODIES = ["Body1REC", "Body4ABT"]; // Viper and Bug
	const attackers = enumGroup(attackerGroup);
	if (attackers.length < 25) {
		return; // Keep army size healthy before recycling
	}

	let recycled = 0;
	for (const droid of attackers) {
		if (droid.order === DORDER_RECYCLE) continue;
		if (OBSOLETE_BODIES.includes(droid.body)) {
			orderDroid(droid, DORDER_RECYCLE);
			recycled++;
			if (recycled >= 2) break; // Recycle at most 2 per cycle so we don't weaken the frontline
		}
	}
}
