
function havePrimaryOrArtilleryWeapon()
{
	var primary = componentAvailable(subPersonalities[personality].primaryWeapon.weapons[0].stat);
	var artillery = componentAvailable(subPersonalities[personality].artillery.weapons[0].stat);

	return (primary || artillery);
}

function earlyT1MachinegunChance()
{
	return ((getMultiTechLevel() === 1) && (gameTime < 900000) && (random(100) < 35));
}

function superLowOnProductionPower()
{
	return (getRealPower() < (PRODUCTION_POWER + 200));
}

//Pick a random weapon line.
function chooseRandomWeapon()
{
	var weaps = subPersonalities[personality].primaryWeapon;

	switch (random(6))
	{
		case 0: weaps = subPersonalities[personality].primaryWeapon; break;
		case 1: if (useArti) { weaps = subPersonalities[personality].artillery; } break;
		case 2: if (subPersonalities[personality].useLasers === true) { weaps = weaponStats.lasers; } break;
		case 3: weaps = subPersonalities[personality].secondaryWeapon; break;
		case 4: weaps = weaponStats.AS; break;
		case 5: weaps = weaponStats.nexusTech; break;
		default: weaps = subPersonalities[personality].primaryWeapon; break;
	}

	if (!isDefined(weaps))
	{
		weaps = weaponStats.lasers;
	}

	if ((weaps.weapons.length === 0) || !componentAvailable(weaps.weapons[0].stat))
	{
		weaps = subPersonalities[personality].primaryWeapon;
	}

	return weaps;
}

//Prepare the weapon list.
function shuffleWeaponList(weaps)
{
	var weaponList = [];

	for (var i = 0, w = weaps.length; i < w; ++i)
	{
		weaponList.push(weaps[i].stat);
	}

	weaponList.reverse();
	return weaponList;
}

//Either fastFire, normal, or emp-like.
function chooseWeaponType(weaps)
{
	var weaponType = weaps.weapons;

	// "good enough" randomization of electronic/super weapons cause these are hard to say which is best
	if (weaps.alias === "nex" || weaps.alias === "as")
	{
		var rdm = [];

		while (rdm.length !== weaps.weapons.length)
		{
			rdm.push(weaps.weapons[random(weaps.weapons.length)]);
		}

		weaponType = rdm;
	}

	if (isDefined(weaps.fastFire) && (random(100) < 50))
	{
		weaponType = weaps.fastFire;
	}

	return weaponType;
}

//Choose a random cyborg weapon line.
function chooseRandomCyborgWeapon()
{
	var weaps = subPersonalities[personality].primaryWeapon;

	//grenadier cyborgs can only be built as long as Cobra does not have
	//access to howitzer. They are too weak after that.
	switch (random(4))
	{
		case 0: weaps = subPersonalities[personality].primaryWeapon; break;
		case 1: if (subPersonalities[personality].useLasers === true) { weaps = weaponStats.lasers; } break;
		case 2: weaps = subPersonalities[personality].secondaryWeapon; break;
		case 3: if (!componentAvailable("Howitzer105Mk1") && useArti) { weaps = subPersonalities[personality].artillery; } break;
		default: weaps = subPersonalities[personality].primaryWeapon; break;
	}

	if ((weaps.templates.length === 0) || !componentAvailable(weaps.templates[0].weapons))
	{
		weaps = subPersonalities[personality].primaryWeapon;

		if ((weaps.templates.length === 0) || !componentAvailable(weaps.templates[0].weapons))
		{
			weaps = subPersonalities[personality].secondaryWeapon;
		}
	}

	return weaps;
}

//Choose random VTOL weapon line.
function chooseRandomVTOLWeapon()
{
	var weaps = weaponStats.bombs;

	switch (random(5))
	{
		case 0: if (returnPrimaryAlias() !== "fl") { weaps = subPersonalities[personality].primaryWeapon; } break;
		case 1: if (subPersonalities[personality].useLasers === true) { weaps = weaponStats.lasers; } break;
		case 2: weaps = subPersonalities[personality].secondaryWeapon; break;
		case 3: weaps = weaponStats.bombs; break;
		case 4: weaps = weaponStats.empBomb; break;
		default: weaps = weaponStats.bombs; break;
	}

	//Rare chance to make a Sunburst VTOL if we use Rocket AA.
	if ((returnAntiAirAlias() === "rktaa") && (random(100) < 8) && playerVtolRatio(getMostHarmfulPlayer()) >= 0.25)
	{
		weaps = weaponStats.rockets_AA;
	}
	//Rare chance to make a bunker buster VTOL if we are a rocket personality.
	if ((random(100) < 5) && personalityIsRocketMain() && (playerStructureUnitRatio(getMostHarmfulPlayer()) >= 0.08))
	{
		weaps = weaponStats.rockets_AS;
	}

	if ((weaps.vtols.length === 0) || !componentAvailable(weaps.vtols[0].stat))
	{
		weaps = weaponStats.bombs;
	}

	//randomize the big bombs for variety
	if (weaps.alias === "bomb")
	{
		var rdm = [];
		while (rdm.length !== weaps.vtols.length)
		{
			rdm.push(weaps.vtols[random(weaps.vtols.length)]);
		}
		return rdm;
	}

	return weaps.vtols;
}


//Randomly choose the best weapon with current technology.
//Defaults to machine-guns when other choices are unavailable (if allowed). May return undefined.
//Also cyborgs will not return the actual stat list with this function due to how they are built.
function choosePersonalityWeapon(type)
{
	var weaps;
	var weaponList = [];

	if (!isDefined(type))
	{
		type = "TANK";
	}

	if (type === "TANK")
	{
		var skip = false;
		weaps = chooseRandomWeapon();
		weaponList = shuffleWeaponList(chooseWeaponType(weaps));

		// Choose an anti-air weapon instead... checks target player and then total player vtols.
		if (((playerVtolRatio(getMostHarmfulPlayer()) >= 0.06) || (countEnemyVTOL() >= 7)) && random(100) < 20)
		{
			weaponList = [];
			skip = true;

			// The lasers are the most powerful...
			if (componentAvailable(weaponStats.lasers_AA.weapons[0].stat) && (random(100) <= 50))
			{
				var lasers = weaponStats.lasers_AA.weapons;
				for (var i = lasers.length - 1; i >= 0; --i)
				{
					var weapObj = lasers[i];
	 				weaponList.push(weapObj.stat);
	 			}
			}

			var aa = subPersonalities[personality].antiAir.weapons;
			// Default to machinegun AA line if our current line doesn't exist (useful on team battles)
			if (!componentAvailable(subPersonalities[personality].antiAir.weapons[0].stat))
			{
				aa = weaponStats.AA.weapons;
			}

 			for (var i = aa.length - 1; i >= 0; --i)
			{
				var weapObj = aa[i];
 				weaponList.push(weapObj.stat);
 			}
		}

		// Allow small chance for Bunker Busters if main weapons lines are rockets.
		if (!skip && ((random(100) < 7) && personalityIsRocketMain() && (playerStructureUnitRatio(getMostHarmfulPlayer()) >= 0.25)))
		{
			weaponList = [];
			skip = true;
			var bunkerBusters = weaponStats.rockets_AS.weapons;

 			for (var i = bunkerBusters.length - 1; i >= 0; --i)
			{
				var weapObj = bunkerBusters[i];
 				weaponList.push(weapObj.stat);
 			}
		}

		// Maybe choose a machinegun.
		if (!skip && ((!turnOffMG && (random(100) < Math.floor(playerCyborgRatio(getMostHarmfulPlayer()) * 100))) ||
			!havePrimaryOrArtilleryWeapon() ||
			earlyT1MachinegunChance()))
		{
			weaponList = [];
			var generalAntiCyborgWeapons = weaponStats.machineguns.weapons;

 			for (var i = generalAntiCyborgWeapons.length - 1; i >= 0; --i)
			{
				var weapObj = generalAntiCyborgWeapons[i];
 				weaponList.push(weapObj.stat);
 			}
		}
	}
	else if (type === "CYBORG")
	{
		weaps = chooseRandomCyborgWeapon();
	}
	else if (type === "VTOL")
	{
		weaps = chooseRandomVTOLWeapon();
		for (var i = weaps.length - 1; i >= 0; --i)
		{
			weaponList.push(weaps[i].stat);
		}
	}

	return ((type === "CYBORG") || !isDefined(weaps)) ? weaps : weaponList;
}

//What conditions will allow hover use. There is a 15% chance regardless of weapon.
//Expects an array of weapons.
function useHover(weap)
{
	if (!(isDefined(weap) && componentAvailable("hover01")))
	{
		return false;
	}

	if (forceHover)
	{
		return true;
	}

	var propulsions = playerLandPropRatio(getMostHarmfulPlayer());

	if (Math.floor(propulsions.hover * 100) >= 30 && random(100) < 20)
	{
		return true;
	}
	if (Math.floor(propulsions.track * 100) >= 50 && random(100) < 80)
	{
		return false;
	}

	var useHover = false;

	for (var i = 0, w = weap.length; i < w; ++i)
	{
		const NAME = weap[i];

		if ((NAME === "Flame1Mk1") || (NAME === "Flame2") || (NAME === "PlasmiteFlamer"))
		{
			useHover = (random(100) <= 40);
			break;
		}

		if ((NAME === "Rocket-LtA-T") || (NAME === "Rocket-HvyA-T") || (NAME === "Missile-A-T"))
		{
			useHover = (random(100) <= 33);
			break;
		}

		if ((NAME === "Laser3BEAMMk1") || (NAME === "Laser2PULSEMk1") || (NAME === "HeavyLaser"))
		{
			useHover = (random(100) <= 40);
			break;
		}
	}

	return (((useHover === true) || (random(100) <= 15)) && (weap[0] !== "Laser4-PlasmaCannon"));
}

//Choose our ground propulsion. Non-hover units will have a preference for tracks.
function pickPropulsion(weap)
{
	if (useHover(weap))
	{
		return "hover01";
	}

	const TIME_FOR_HALF_TRACKS = 600000;

	if (gameTime < TIME_FOR_HALF_TRACKS && getMultiTechLevel() === 1)
	{
		return ["HalfTrack", "wheeled01"];
	}

	var tankProp = [
		"tracked01", // tracked01
		"HalfTrack", // half-track
		"wheeled01", // wheels
	];

	var propulsions = playerLandPropRatio(getMostHarmfulPlayer());

	if (Math.floor(propulsions.track * 100) <= 55 && random(100) < ((superLowOnProductionPower()) ? 85 : 60))
	{
		tankProp.shift();
	}

	return tankProp;
}

function pickTankBody()
{
	//Early-game now has a focus on small or medium body and will then shift
	//a preference towards heavy or medium bodies (if power is relatively low).
	//This helps keep things competitive among a player rushing with small/medium bodies.
	var body;
	var bodySwitchTime = 900000;
	var sizeRatio = playerBodySizeRatio(getMostHarmfulPlayer());

	if ((Math.floor(sizeRatio.small * 100) >= 66 && random(100) < 40) || (gameTime < bodySwitchTime && random(100) < 80))
	{
		body = (random(100) < ((superLowOnProductionPower()) ? 50 : 20)) ? SYSTEM_BODY : VTOL_BODY;
	}
	else
	{
		body = (random(100) < 30) ? VTOL_BODY : TANK_BODY;
	}

	// If they go super big, we go super big
	if (Math.floor(sizeRatio.heavy * 100) >= 45 && random(100) < 66)
	{
		body = TANK_BODY;
	}

	return body;
}

//Create a ground attacker tank with a heavy body when possible.
//Personality AR uses hover when possible. All personalities may use special weapons on Hard/Insane.
//Also when Cobra has Dragon body, the EMP Cannon may be selected as the second weapon if it is researched.
function buildAttacker(id)
{
	if (!(isDefined(forceHover) && isDefined(seaMapWithLandEnemy)))
	{
		return false;
	}
	if (forceHover && !seaMapWithLandEnemy && !componentAvailable("hover01"))
	{
		return false;
	}

	var weap = choosePersonalityWeapon("TANK");
	var secondary = choosePersonalityWeapon("TANK");
	var fac = getObject(STRUCTURE, me, id);

	if (isDefined(weap) && isDefined(secondary))
	{
		if (fac !== null)
		{
			return getRealPower() > PRODUCTION_POWER && buildDroid(fac, "Droid", pickTankBody(), pickPropulsion(weap), "", "", weap, secondary);
		}
	}

	return false;
}

//Create trucks or sensors. Default to a sensor.
function buildSys(id, weap)
{
	var fac = getObject(STRUCTURE, me, id);
	if (fac === null)
	{
		return false;
	}

	if (!isDefined(weap))
	{
		weap = ARTILLERY_SENSORS;
	}

	var body;
	if (gameTime > 600000)
	{
		body = (random(100) < 80) ? VTOL_BODY : TANK_BODY;
	}
	else
	{
		body = (random(100) < 60) ? SYSTEM_BODY : VTOL_BODY;
	}

	if (buildDroid(fac, "System unit", body, SYSTEM_PROPULSION, "", "", weap))
	{
		return true;
	}

	return false;
}

//Create a cyborg with available research. Expects a boolean for useEngineer or can undefined.
function buildCyborg(id, useEngineer)
{
	var weap = "CyborgSpade";
	var body = "CyborgLightBody";
	var prop = "CyborgLegs";
	var fac = getObject(STRUCTURE, me, id);

	if (fac === null)
	{
		return false;
	}

	//Build combat engineer if requested.
	if (isDefined(useEngineer) && (useEngineer === true))
	{
		return buildDroid(fac, "Combat Engineer", body, prop, "", "", weap);
	}

	var weaponLine = choosePersonalityWeapon("CYBORG");

	//Choose MG instead if enemy has enough cyborgs.
	if (((!turnOffMG || (cyborgOnlyGame && !useLasersForCyborgControl() && random(100) < 66)) &&
		(random(100) < Math.floor(playerCyborgRatio(getMostHarmfulPlayer()) * 100))) ||
		!havePrimaryOrArtilleryWeapon() ||
		earlyT1MachinegunChance())
	{
		weaponLine = weaponStats.machineguns;
	}

	if (isDefined(weaponLine))
	{
		for (var x = weaponLine.templates.length - 1; x >= 0; --x)
		{
			body = weaponLine.templates[x].body;
			prop = weaponLine.templates[x].prop;
			weap = weaponLine.templates[x].weapons[0];

			if (getRealPower() > PRODUCTION_POWER && buildDroid(fac, weap + " Cyborg", body, prop, "", "", weap))
			{
				return true;
			}
		}
	}

	return false;
}

//Create a vtol fighter with a medium/large body.
function buildVTOL(id)
{
	if (getRealPower() < PRODUCTION_POWER)
	{
		return false;
	}

	var weap = choosePersonalityWeapon("VTOL");
	var weap2 = choosePersonalityWeapon("VTOL");
	var fac = getObject(STRUCTURE, me, id);

	if (fac !== null && isDefined(weap) && isDefined(weap2))
	{
		var body = (random(100) < 60) ? VTOL_BODY : TANK_BODY;

		return buildDroid(fac, "VTOL unit", body, "V-Tol", "", "", weap, weap2);
	}

	return false;
}

//Check what system units are queued in a regular factory. Returns an object
//containing the number or trucks/sensors/repairs queued.
function analyzeQueuedSystems()
{
	var fac = enumStruct(me, FACTORY);
	var trucks = 0;
	var sens = 0;

	for (var i = 0, l = fac.length; i < l; ++i)
	{
		var virDroid = getDroidProduction(fac[i]);
		if (virDroid !== null)
		{
			const TYPE = virDroid.droidType;

			if (TYPE === DROID_CONSTRUCT)
			{
				trucks += 1;
			}
			if (TYPE === DROID_SENSOR)
			{
				sens += 1;
			}
		}
	}

	return { "truck": trucks, "sensor": sens };
}

function attackerCountsGood(recycle)
{
	if (checkIfSeaMap())
	{
		return true;
	}

	if (!isDefined(recycle))
	{
		recycle = false;
	}

	var highOilExtras = highOilMap() ? 15 : 0;
	var recycleExtras = recycle ? 20 : 0;

	var amountOfAttackers = groupSize(attackGroup) + groupSize(artilleryGroup) + groupSize(vtolGroup);

	return amountOfAttackers >= (MIN_ATTACK_DROIDS + recycleExtras + highOilExtras);
}


//Produce a unit when factories allow it.
function produce()
{
	if ((countDroid(DROID_ANY) >= 150) || currently_dead)
	{
		return; //Stop spamming about having the droid limit reached.
	}
	const MIN_SENSORS = 1;
	var systems = analyzeQueuedSystems();

	var attackers = enumGroup(attackGroup).length;
	var buildSensors = ((enumGroup(sensorGroup).length + systems.sensor) < MIN_SENSORS);
	var buildTrucks = ((enumGroup(constructGroup).length +
		enumGroup(oilGrabberGroup).length +
		enumGroup(constructGroupNTWExtra).length +
		systems.truck) < minTruckCount());
	var useCybEngineer = !countStruct(structures.factory) && buildTrucks && (countDroid(DROID_CONSTRUCT) < getDroidLimit(me, DROID_CONSTRUCT)); //use them if we have no factory

	//Loop through factories in the order the personality likes.
	for (var i = 0; i < 3; ++i)
	{
		var facType = subPersonalities[personality].factoryOrder[i];
		var fac = enumStruct(me, facType);
		if (!((facType === structures.cyborgFactory) && !forceHover && turnOffCyborgs))
		{
			if (facType === structures.vtolFactory && !countDroid(DROID_CONSTRUCT))
			{
				continue;
			}
			for (var x = 0, l = fac.length; x < l; ++x)
			{
				const FC = fac[x];
				if (!(FC && FC.status === BUILT))
				{
					continue;
				}
				//Now accounts for overproduction of trucks stalling combat unit production (only seen on higher oil based maps)
				const VIR_DROID = getDroidProduction(FC);
				if (!structureIdle(FC) &&
					!(countDroid(DROID_CONSTRUCT) >= getDroidLimit(me, DROID_CONSTRUCT) &&
					VIR_DROID !== null &&
					VIR_DROID.droidType === DROID_CONSTRUCT))
				{
					continue;
				}

				if (facType === structures.factory)
				{
					if (gameTime > 40000 &&
						highOilMap() &&
						componentAvailable("MG1Mk1") &&
						attackers < 2 &&
						(countStruct(structures.gen) && countStruct(structures.derrick)))
					{
						buildAttacker(FC.id);
					}
					else if (buildTrucks &&
						(attackerCountsGood(false) ||
						(gameTime < 240000 && highOilMap()) ||
						!havePrimaryOrArtilleryWeapon() ||
						(getMultiTechLevel() > 1) ||
						(gameTime > 240000 && random(100) < 25)))
					{
						buildSys(FC.id, "Spade1Mk1");
					}
					else if (buildSensors &&
						(enumGroup(artilleryGroup).length > 0) &&
						componentAvailable("SensorTurret1Mk1"))
					{
						buildSys(FC.id);
					}
					else
					{
						if (!countStruct(structures.gen) || !countStruct(structures.derrick))
						{
							continue;
						}

						buildAttacker(FC.id);
					}
				}
				else
				{
					var cyb = (facType === structures.cyborgFactory);
					//In some circumstances the bot could be left with no generators and no factories
					//but still needs to produce combat engineers to, maybe, continue surviving.
					if (countStruct(structures.gen) || (cyb && useCybEngineer && (cyborgOnlyGame || (gameTime > 480000))))
					{
						if (cyb && (!turnOffCyborgs || !forceHover))
						{
							if (!useCybEngineer && !countStruct(structures.derrick))
							{
								continue; //no derricks while trying to build attack cyborg
							}
							buildCyborg(FC.id, useCybEngineer);
						}
						else
						{
							if (useVtol && facType === structures.vtolFactory && countStruct(structures.derrick))
							{
								buildVTOL(FC.id);
							}
						}
					}
				}
			}
		}
	}
}
