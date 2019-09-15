
//See if we can design this droid.
function isDesignable(item, body, prop)
{
	if (!isDefined(item))
	{
		return false;
	}

	if (!isDefined(body))
	{
		body = "Body1REC";
	}

	if (!isDefined(prop))
	{
		prop = "wheeled01";
	}

	var virDroid = makeTemplate(me, "Virtual Droid", body, prop, "", "", item, item);
	return (virDroid !== null);
}

//Pick a random weapon line. May return undefined for machineguns.
function chooseRandomWeapon()
{
	var weaps;

	switch (random(5))
	{
		case 0: weaps = subPersonalities[personality].primaryWeapon; break;
		case 1: if (useArti) { weaps = subPersonalities[personality].artillery; } break;
		case 2: if (subPersonalities[personality].useLasers === true) { weaps = weaponStats.lasers; } break;
		case 3: weaps = subPersonalities[personality].secondaryWeapon; break;
		case 4: weaps = weaponStats.AS; break;
		default: weaps = subPersonalities[personality].primaryWeapon; break;
	}

	if (!isDefined(weaps))
	{
		weaps = weaponStats.lasers;
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

//Either fastFire or normal.
function chooseWeaponType(weaps)
{
	var weaponType = weaps.weapons;

	if (isDefined(weaps.fastFire) && (random(100) < 50))
	{
		weaponType = weaps.fastFire;
	}

	return weaponType;
}

//Choose a random cyborg weapon line. May return undefined.
function chooseRandomCyborgWeapon()
{
	var weaps;

	//grenadier cyborgs can only be built as long as Cobra does not Have
	//access to pepperpot. They are too weak after that.
	switch (random(4))
	{
		case 0: weaps = subPersonalities[personality].primaryWeapon; break;
		case 1: if (subPersonalities[personality].useLasers === true) { weaps = weaponStats.lasers; } break;
		case 2: weaps = subPersonalities[personality].secondaryWeapon; break;
		case 3: if(!componentAvailable("Mortar3ROTARYMk1") && useArti) { weaps = subPersonalities[personality].artillery; } break;
		default: weaps = subPersonalities[personality].primaryWeapon; break;
	}

	return weaps;
}

//Choose random VTOL weapon line. Defaults to bombs if undefined.
function chooseRandomVTOLWeapon()
{
	var weaps;
	var isEMP = false;

	switch (random(5))
	{
		case 0: if((returnPrimaryAlias() !== "mg") && (returnPrimaryAlias() !== "fl")) { weaps = subPersonalities[personality].primaryWeapon; } break;
		case 1: if (subPersonalities[personality].useLasers === true) { weaps = weaponStats.lasers; } break;
		case 2: weaps = subPersonalities[personality].secondaryWeapon; break;
		case 3: weaps = weaponStats.bombs; break;
		case 4: weaps = weaponStats.empBomb; isEMP = true; break;
		default: weaps = weaponStats.bombs; break;
	}

	if (!isDefined(weaps) || (!isEMP && (weaps.vtols.length - 1 <= 0)))
	{
		weaps = weaponStats.bombs;
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
		const PLASMA_LAUNCHER = "PlasmaHeavy";
		weaps = chooseRandomWeapon();
		weaponList = shuffleWeaponList(chooseWeaponType(weaps));

		//on hard difficulty and above.
		if (componentAvailable("tracked01") && (random(100) <= 1))
		{
			if((difficulty === HARD) || (difficulty === INSANE))
			{
				weaponList.push(PLASMA_LAUNCHER);
			}
		}

		if (!turnOffMG && !isDesignable(weaponList))
		{
			weaponList = [];
 			for (var i = weaponStats.machineguns.weapons.length - 1; i >= 0; --i)
			{
 				weaponList.push(weaponStats.machineguns.weapons[i].stat);
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

	var useHover = false;

	for (var i = 0, w = weap.length; i < w; ++i)
	{
		const NAME = weap[i];

		if ((NAME === "Flame1Mk1") || (NAME === "Flame2") || (NAME === "PlasmiteFlamer"))
		{
			useHover = true;
			break;
		}

		if ((NAME === "Rocket-LtA-T") || (NAME === "Rocket-HvyA-T") || (NAME === "Missile-A-T"))
		{
			useHover = (random(100) <= 75);
			break;
		}

		if ((NAME === "Laser3BEAMMk1") || (NAME === "Laser2PULSEMk1") || (NAME === "HeavyLaser"))
		{
			useHover = (random(100) <= 55);
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

	const TIME_FOR_HALF_TRACKS = 1200000;
	var tankProp = [
		"tracked01", // tracked01
		"HalfTrack", // half-track
		"wheeled01", // wheels
	];

	if ((random(100) < 45) || (gameTime < TIME_FOR_HALF_TRACKS))
	{
		tankProp.shift();
	}

	return tankProp;
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

	if (isDefined(weap) && isDefined(secondary) && (secondary[0] !== "Laser4-PlasmaCannon"))
	{
		if (fac !== null)
		{
			if (!random(3) && componentAvailable("Body14SUP") && componentAvailable("EMP-Cannon"))
			{
				secondary = "EMP-Cannon";
			}

			return getRealPower() > PRODUCTION_POWER && buildDroid(fac, "Droid", TANK_BODY, pickPropulsion(weap), "", "", weap, secondary);
		}
	}

	return false;
}

//Create trucks or sensors with a light/medium body. Default to a sensor.
function buildSys(id, weap)
{
	var fac = getObject(STRUCTURE, me, id);
	if (!isDefined(weap))
	{
		weap = ["Sensor-WideSpec", "SensorTurret1Mk1"];
	}

	return (fac !== null && buildDroid(fac, "System unit", random(2) ? SYSTEM_BODY : VTOL_BODY, SYSTEM_PROPULSION, "", "", weap));
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

//Create a vtol fighter with a medium body.
function buildVTOL(id)
{
	var weap = choosePersonalityWeapon("VTOL");
	var fac = getObject(STRUCTURE, me, id);
	return (getRealPower() > PRODUCTION_POWER && fac !== null && isDefined(weap) && buildDroid(fac, "VTOL unit", VTOL_BODY, "V-Tol", "", "", weap, weap));
}

//Check what system units are queued in a regular factory. Returns an object
//containing the number or trucks/sensors/repairs queued.
function analyzeQueuedSystems()
{
	var fac = enumStruct(me, FACTORY);
	var trucks = 0;
	var sens = 0;
	var reps = 0;

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
			if (TYPE === DROID_REPAIR)
			{
				reps += 1;
			}
		}
	}

	return { "truck": trucks, "sensor": sens, "repair": reps };
}


//Produce a unit when factories allow it.
function produce()
{
	if (countDroid(DROID_ANY) >= 150)
	{
		return; //Stop spamming about having the droid limit reached.
	}
	const MIN_SENSORS = 1;
	const MIN_REPAIRS = 2;
	var useCybEngineer = !countStruct(structures.factories); //use them if we have no factory
	var systems = analyzeQueuedSystems();

	var attackers = groupSizes[attackGroup];
	var allowSpecialSystems = attackers > 10;
	var buildSensors = ((enumGroup(sensorGroup).length + systems.sensor) < MIN_SENSORS);
	var buildRepairs = ((enumGroup(repairGroup).length + systems.repair) < MIN_REPAIRS);
	var buildTrucks = ((enumGroup(constructGroup).length + enumGroup(oilGrabberGroup).length + systems.truck) < MIN_TRUCKS);

	//Loop through factories in the order the personality likes.
	for (var i = 0; i < 3; ++i)
	{
		var facType = subPersonalities[personality].factoryOrder[i];
		var fac = enumStruct(me, facType);
		if (!((facType === CYBORG_FACTORY) && !forceHover && turnOffCyborgs))
		{
			if (facType !== FACTORY && !countDroid(DROID_CONSTRUCT))
			{
				continue;
			}
			for (var x = 0, l = fac.length; x < l; ++x)
			{
				const FC = fac[x];
				if (FC && FC.status === BUILT && structureIdle(FC))
				{
					if (facType === FACTORY)
					{
						if (buildTrucks)
						{
							buildSys(FC.id, "Spade1Mk1");
						}
						else if (buildSensors &&
							enumGroup(artilleryGroup).length &&
							componentAvailable("SensorTurret1Mk1"))
						{
							buildSys(FC.id);
						}
						else if (allowSpecialSystems &&
							buildRepairs &&
							isDesignable("LightRepair1"))
						{
							buildSys(FC.id, REPAIR_TURRETS);
						}
						else
						{
							//Do not produce weak body units if we can give this factory a module.
							if (!countStruct(structures.gens))
							{
								continue;
							}

							buildAttacker(FC.id);
						}
					}
					else
					{
						if (countStruct(structures.gens))
						{
							if (facType === CYBORG_FACTORY && (!turnOffCyborgs || !forceHover))
							{
								buildCyborg(FC.id, useCybEngineer);
							}
							else
							{
								if (useVtol && facType === VTOL_FACTORY)
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
}
