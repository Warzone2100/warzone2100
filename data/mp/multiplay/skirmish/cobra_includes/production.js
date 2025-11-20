
function havePrimaryOrArtilleryWeapon()
{
	const __primary = componentAvailable(subPersonalities[personality].primaryWeapon.weapons[0].stat);
	const __artillery = componentAvailable(subPersonalities[personality].artillery.weapons[0].stat);

	return (__primary || __artillery);
}

function firstRocketLikeArtilleryAvailable()
{
	if (((returnArtilleryAlias() === "rkta") || (returnArtilleryAlias() === "missa")) &&
		isDefined(subPersonalities[personality].artillery.fastFire) &&
		componentAvailable(subPersonalities[personality].artillery.fastFire[0].stat))
	{
		return true;
	}

	return false;
}

function earlyT1MachinegunChance()
{
	return ((getMultiTechLevel() === 1) && (gameTime < 900000) && chance(35));
}

function superLowOnProductionPower()
{
	return (getRealPower() < (__PRODUCTION_POWER + 200));
}

//Pick a random weapon line.
function chooseRandomWeapon()
{
	let weaps = subPersonalities[personality].primaryWeapon;

	switch (random(6))
	{
		case 0: weaps = subPersonalities[personality].primaryWeapon; break;
		case 1: if (useArti) { weaps = subPersonalities[personality].artillery; } break;
		case 2: if (subPersonalities[personality].useLasers === true) { weaps = _WEAPON_STATS.lasers; } break;
		case 3: weaps = subPersonalities[personality].secondaryWeapon; break;
		case 4: weaps = _WEAPON_STATS.AS; break;
		case 5: weaps = _WEAPON_STATS.nexusTech; break;
		default: weaps = subPersonalities[personality].primaryWeapon; break;
	}

	if (!isDefined(weaps))
	{
		weaps = _WEAPON_STATS.lasers;
	}

	if ((weaps.weapons.length === 0) || !componentAvailable(weaps.weapons[0].stat) || !strangeStartSettingOver())
	{
		weaps = subPersonalities[personality].primaryWeapon;
	}

	return weaps;
}

//Prepare the weapon list.
function shuffleWeaponList(weaps)
{
	const _weaponList = [];

	for (let i = 0, w = weaps.length; i < w; ++i)
	{
		_weaponList.push(weaps[i].stat);
	}

	_weaponList.reverse();
	return _weaponList;
}

//Either fastFire, normal, or emp-like.
function chooseWeaponType(weaps)
{
	let weaponType = weaps.weapons;

	// "good enough" randomization of electronic/super weapons cause these are hard to say which is best
	if (weaps.alias === "nex" || weaps.alias === "as")
	{
		const _rdm = [];

		while (_rdm.length !== weaps.weapons.length)
		{
			_rdm.push(weaps.weapons[random(weaps.weapons.length)]);
		}

		weaponType = _rdm;
	}

	if (isDefined(weaps.fastFire) && chance(50))
	{
		weaponType = weaps.fastFire;
	}

	return weaponType;
}

//Choose a random cyborg weapon line.
function chooseRandomCyborgWeapon()
{
	let weaps = subPersonalities[personality].primaryWeapon;

	//grenadier cyborgs can only be built as long as Cobra does not have
	//access to howitzer. They are too weak after that.
	switch (random(4))
	{
		case 0: weaps = subPersonalities[personality].primaryWeapon; break;
		case 1: if (subPersonalities[personality].useLasers === true) { weaps = _WEAPON_STATS.lasers; } break;
		case 2: weaps = subPersonalities[personality].secondaryWeapon; break;
		case 3: if (!componentAvailable("Howitzer105Mk1") && useArti) { weaps = subPersonalities[personality].artillery; } break;
		default: weaps = subPersonalities[personality].primaryWeapon; break;
	}

	if (!weaps.templates.length || !componentAvailable(weaps.templates[0].weapons) || !strangeStartSettingOver())
	{
		weaps = subPersonalities[personality].primaryWeapon;

		if (!weaps.templates.length || !componentAvailable(weaps.templates[0].weapons))
		{
			weaps = subPersonalities[personality].secondaryWeapon;
		}
	}

	return weaps;
}

//Choose random VTOL weapon line.
function chooseRandomVTOLWeapon()
{
	let weaps = _WEAPON_STATS.bombs;

	switch (random(5))
	{
		case 0: if (returnPrimaryAlias() !== "fl") { weaps = subPersonalities[personality].primaryWeapon; } break;
		case 1: if (subPersonalities[personality].useLasers === true) { weaps = _WEAPON_STATS.lasers; } break;
		case 2: weaps = subPersonalities[personality].secondaryWeapon; break;
		case 3: weaps = _WEAPON_STATS.bombs; break;
		case 4: weaps = _WEAPON_STATS.empBomb; break;
		default: weaps = _WEAPON_STATS.bombs; break;
	}

	//Rare chance to make a Sunburst VTOL if we use Rocket AA.
	if ((returnAntiAirAlias() === "rktaa") && chance(8) && playerVtolRatio(getMostHarmfulPlayer()) >= 0.25)
	{
		weaps = _WEAPON_STATS.rockets_AA;
	}
	//Rare chance to make a bunker buster VTOL if we are a rocket personality.
	if (chance(5) && personalityIsRocketMain() && (playerStructureUnitRatio(getMostHarmfulPlayer()) >= 0.08))
	{
		weaps = _WEAPON_STATS.rockets_AS;
	}

	if (!strangeStartSettingOver())
	{
		weaps = _WEAPON_STATS.machineguns;
	}

	if (!weaps.vtols.length || !componentAvailable(weaps.vtols[0].stat))
	{
		weaps = _WEAPON_STATS.bombs;
	}

	//randomize the big bombs for variety
	if (weaps.alias === "bomb")
	{
		const _rdm = [];

		while (_rdm.length !== weaps.vtols.length)
		{
			_rdm.push(weaps.vtols[random(weaps.vtols.length)]);
		}

		return _rdm;
	}

	return weaps.vtols;
}


//Randomly choose the best weapon with current technology.
//Defaults to machine-guns when other choices are unavailable (if allowed). May return undefined.
//Also cyborgs will not return the actual stat list with this function due to how they are built.
function choosePersonalityWeapon(type)
{
	let weaps;
	let weaponList = [];

	if (!isDefined(type))
	{
		type = "TANK";
	}

	if (type === "TANK")
	{
		let skip = false;
		weaps = chooseRandomWeapon();
		weaponList = shuffleWeaponList(chooseWeaponType(weaps));

		// Choose an anti-air weapon instead... checks target player and then total player vtols.
		if (((playerVtolRatio(getMostHarmfulPlayer()) >= 0.06) || (countEnemyVTOL() >= 7)) && chance(20))
		{
			weaponList = [];
			skip = true;

			// The lasers are the most powerful...
			if (componentAvailable(_WEAPON_STATS.lasers_AA.weapons[0].stat) && chance(51))
			{
				const _lasers = _WEAPON_STATS.lasers_AA.weapons;

				for (let i = _lasers.length - 1; i >= 0; --i)
				{
					const _weapObj = _lasers[i];

					weaponList.push(_weapObj.stat);
				}
			}

			let aa = subPersonalities[personality].antiAir.weapons;
			// Default to machinegun AA line if our current line doesn't exist (useful on team battles)
			if (!componentAvailable(subPersonalities[personality].antiAir.weapons[0].stat))
			{
				aa = _WEAPON_STATS.AA.weapons;
			}

			for (let i = aa.length - 1; i >= 0; --i)
			{
				const _weapObj = aa[i];

				weaponList.push(_weapObj.stat);
			}
		}

		// Allow small chance for Bunker Busters if main weapons lines are rockets.
		if (!skip && (chance(7) && personalityIsRocketMain() && (playerStructureUnitRatio(getMostHarmfulPlayer()) >= 0.25)))
		{
			weaponList = [];
			skip = true;
			const _bunkerBusters = _WEAPON_STATS.rockets_AS.weapons;

			for (let i = _bunkerBusters.length - 1; i >= 0; --i)
			{
				const _weapObj = _bunkerBusters[i];

				weaponList.push(_weapObj.stat);
			}
		}

		// Maybe choose a machinegun.
		if (!skip && ((!turnOffMG && chance(Math.floor(playerCyborgRatio(getMostHarmfulPlayer()) * 100))) ||
			!havePrimaryOrArtilleryWeapon() ||
			earlyT1MachinegunChance() ||
			!strangeStartSettingOver()))
		{
			weaponList = [];
			let _antiCybWeaps = _WEAPON_STATS.machineguns.weapons;

			if (strangeStartSettingOver() && firstRocketLikeArtilleryAvailable())
			{
				_antiCybWeaps = subPersonalities[personality].artillery.fastFire;
			}

			for (let i = _antiCybWeaps.length - 1; i >= 0; --i)
			{
				const _weapObj = _antiCybWeaps[i];

				weaponList.push(_weapObj.stat);
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

		for (let i = weaps.length - 1; i >= 0; --i)
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

	const _propulsions = playerLandPropRatio(getMostHarmfulPlayer());

	if (Math.floor(_propulsions.hover * 100) >= 30 && chance(20))
	{
		return true;
	}

	if (Math.floor(_propulsions.track * 100) >= 50 && chance(80))
	{
		return false;
	}

	let useHover = false;

	for (let i = 0, w = weap.length; i < w; ++i)
	{
		const __name = weap[i];

		if ((__name === "Flame1Mk1") || (__name === "Flame2") || (__name === "PlasmiteFlamer"))
		{
			useHover = chance(41);
			break;
		}

		if ((__name === "Rocket-LtA-T") || (__name === "Rocket-HvyA-T") || (__name === "Missile-A-T"))
		{
			useHover = chance(34);
			break;
		}

		if ((__name === "Laser3BEAMMk1") || (__name === "Laser2PULSEMk1") || (__name === "HeavyLaser"))
		{
			useHover = chance(41);
			break;
		}
	}

	return (((useHover === true) || chance(16)) && (weap[0] !== "Laser4-PlasmaCannon"));
}

//Choose our ground propulsion. Non-hover units will have a preference for tracks.
function pickPropulsion(weap)
{
	if (useHover(weap))
	{
		return "hover01";
	}

	const __timeForHalfTracks = 600000;

	if (gameTime < __timeForHalfTracks && getMultiTechLevel() === 1)
	{
		return ["HalfTrack", "wheeled01"];
	}

	const _tankProp = [
		"tracked01", // tracked01
		"HalfTrack", // half-track
		"wheeled01", // wheels
	];

	const _propulsions = playerLandPropRatio(getMostHarmfulPlayer());

	if (Math.floor(_propulsions.track * 100) <= 55 && chance(superLowOnProductionPower() ? 60 : 40))
	{
		_tankProp.shift();
	}

	return _tankProp;
}

function pickTankBody()
{
	//Early-game now has a focus on small or medium body and will then shift
	//a preference towards heavy or medium bodies (if power is relatively low).
	//This helps keep things competitive among a player rushing with small/medium bodies.
	let body;
	const __bodySwitchTime = 720000;
	const __sizeRatio = playerBodySizeRatio(getMostHarmfulPlayer());
	const __lowOnPower = superLowOnProductionPower();

	if ((Math.floor(__sizeRatio.small * 100) >= 66 && chance(40)) ||
		(!noBasesHighTechStart && (gameTime < __bodySwitchTime) && chance(__lowOnPower ? 40 : 25)))
	{
		body = chance(__lowOnPower ? 25 : 15) ? _SYSTEM_BODY : _VTOL_BODY;
	}
	else
	{
		body = chance(__lowOnPower ? 30 : 15) ? _VTOL_BODY : _TANK_BODY;
	}

	// If they go super big, we go super big
	if (Math.floor(__sizeRatio.heavy * 100) >= 45 && chance(66))
	{
		body = _TANK_BODY;
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

	const _weap = choosePersonalityWeapon("TANK");
	const _secondary = choosePersonalityWeapon("TANK");
	const _fac = getObject(STRUCTURE, me, id);

	if (isDefined(_weap) && isDefined(_secondary) && (_fac !== null))
	{
		return getRealPower() > __PRODUCTION_POWER && buildDroid(_fac, "Droid", pickTankBody(), pickPropulsion(_weap), "", "", _weap, _secondary);
	}

	return false;
}

//Create trucks or sensors. Default to a sensor.
function buildSys(id, weap)
{
	const _fac = getObject(STRUCTURE, me, id);

	if (_fac === null)
	{
		return false;
	}

	if (!isDefined(weap))
	{
		weap = _ARTILLERY_SENSORS;
	}

	let body;

	if (!superLowOnProductionPower() && (gameTime > 600000))
	{
		body = chance(80) ? _VTOL_BODY : _TANK_BODY;
	}
	else
	{
		body = chance(75) ? _SYSTEM_BODY : _VTOL_BODY;

		if (!strangeStartSettingOver())
		{
			body = "Body4ABT"; // Systems use Bug for early T2+ No Bases to save power and rush harder.
		}
	}

	if (buildDroid(_fac, "System unit", body, _SYSTEM_PROPULSION, "", "", weap))
	{
		return true;
	}

	return false;
}

//Create a cyborg with available research. Expects a boolean for useEngineer or can undefined.
function buildCyborg(id, useEngineer)
{
	let weap = "CyborgSpade";
	let body = "CyborgLightBody";
	let prop = "CyborgLegs";
	const _fac = getObject(STRUCTURE, me, id);

	if (_fac === null)
	{
		return false;
	}

	//Build combat engineer if requested.
	if (isDefined(useEngineer) && (useEngineer === true))
	{
		return buildDroid(_fac, "Combat Engineer", body, prop, "", "", weap);
	}

	let weaponLine = choosePersonalityWeapon("CYBORG");

	//Choose MG instead if enemy has enough cyborgs.
	if (((!turnOffMG || (cyborgOnlyGame && !useLasersForCyborgControl() && chance(66)) &&
		(chance(Math.floor(playerCyborgRatio(getMostHarmfulPlayer()) * 100)))) ||
		!havePrimaryOrArtilleryWeapon() ||
		earlyT1MachinegunChance() ||
		!strangeStartSettingOver()))
	{
		if (strangeStartSettingOver() && firstRocketLikeArtilleryAvailable())
		{
			// Tanks will handle cyborgs for personalities using the rocket artillery line.
		}
		else
		{
			weaponLine = _WEAPON_STATS.machineguns;
		}
	}

	if (isDefined(weaponLine))
	{
		for (let x = weaponLine.templates.length - 1; x >= 0; --x)
		{
			body = weaponLine.templates[x].body;
			prop = weaponLine.templates[x].prop;
			weap = weaponLine.templates[x].weapons[0];

			if (getRealPower() > __PRODUCTION_POWER && buildDroid(_fac, weap + " Cyborg", body, prop, "", "", weap))
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
	if (getRealPower() < __PRODUCTION_POWER)
	{
		return false;
	}

	const _weap = choosePersonalityWeapon("VTOL");
	const _weap2 = choosePersonalityWeapon("VTOL");
	const _fac = getObject(STRUCTURE, me, id);

	if (_fac !== null && isDefined(_weap) && isDefined(_weap2))
	{
		const _body = chance(60) ? _VTOL_BODY : _TANK_BODY;

		return buildDroid(_fac, "VTOL unit", _body, "V-Tol", "", "", _weap, _weap2);
	}

	return false;
}

//Check what system units are queued in a regular factory. Returns an object
//containing the number or trucks/sensors/repairs queued.
function analyzeQueuedSystems()
{
	const _fac = enumStruct(me, FACTORY);
	let trucks = 0;
	let sens = 0;

	for (let i = 0, l = _fac.length; i < l; ++i)
	{
		const _virDroid = getDroidProduction(_fac[i]);

		if (_virDroid !== null)
		{
			const TYPE = _virDroid.droidType;

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

	const __highOilExtras = highOilMap() ? 15 : 0;
	const __recycleExtras = recycle ? 20 : 0;

	const __amountOfAttackers = groupSize(attackGroup) + groupSize(artilleryGroup) + groupSize(vtolGroup);

	return __amountOfAttackers >= (__MIN_ATTACK_DROIDS + __recycleExtras + __highOilExtras);
}


//Produce a unit when factories allow it.
function produce()
{
	if ((countDroid(DROID_ANY, me) >= 150) || currently_dead)
	{
		return; //Stop spamming about having the droid limit reached.
	}

	const __minSensors = 1;
	const _systems = analyzeQueuedSystems();

	const __attackers = enumGroup(attackGroup).length;
	const __buildSensors = ((enumGroup(sensorGroup).length + _systems.sensor) < __minSensors);
	const __buildTrucks = ((enumGroup(constructGroup).length +
		enumGroup(oilGrabberGroup).length +
		enumGroup(constructGroupNTWExtra).length +
		_systems.truck) < minTruckCount());
	const __useCybEngineer = !countStruct(_STRUCTURES.factory, me) && __buildTrucks && (countDroid(DROID_CONSTRUCT, me) < getDroidLimit(me, DROID_CONSTRUCT)); //use them if we have no factory

	//Loop through factories in the order the personality likes.
	for (let i = 0; i < 3; ++i)
	{
		const __facType = subPersonalities[personality].factoryOrder[i];
		const _fac = enumStruct(me, __facType);

		if (!((__facType === _STRUCTURES.cyborgFactory) && !forceHover && turnOffCyborgs))
		{
			if (__facType === _STRUCTURES.vtolFactory && !countDroid(DROID_CONSTRUCT, me))
			{
				continue;
			}

			for (let x = 0, l = _fac.length; x < l; ++x)
			{
				const _fc = _fac[x];

				if (!(_fc && _fc.status === BUILT))
				{
					continue;
				}
				//Now accounts for overproduction of trucks stalling combat unit production (only seen on higher oil based maps)
				const _virDroid = getDroidProduction(_fc);
				if (!structureIdle(_fc) &&
					!(countDroid(DROID_CONSTRUCT, me) >= getDroidLimit(me, DROID_CONSTRUCT) &&
					_virDroid !== null &&
					_virDroid.droidType === DROID_CONSTRUCT))
				{
					continue;
				}

				if (__facType === _STRUCTURES.factory)
				{
					if (gameTime > 40000 &&
						highOilMap() &&
						componentAvailable("MG1Mk1") &&
						__attackers < 2 &&
						(countStruct(_STRUCTURES.gen, me) && countStruct(_STRUCTURES.derrick, me)))
					{
						buildAttacker(_fc.id);
					}
					else if (__buildTrucks &&
						(attackerCountsGood(false) ||
						(gameTime < 240000 && (highOilMap() || weirdMapBaseDesign)) ||
						!havePrimaryOrArtilleryWeapon() ||
						(getMultiTechLevel() > 1) ||
						(gameTime > 240000 && chance(25))))
					{
						buildSys(_fc.id, "Spade1Mk1");
					}
					else if (__buildSensors &&
						enumGroup(artilleryGroup).length &&
						componentAvailable("SensorTurret1Mk1"))
					{
						buildSys(_fc.id);
					}
					else
					{
						if (!countStruct(_STRUCTURES.gen, me) || !countStruct(_STRUCTURES.derrick, me))
						{
							continue;
						}

						buildAttacker(_fc.id);
					}
				}
				else
				{
					const __cyb = (__facType === _STRUCTURES.cyborgFactory);
					//In some circumstances the bot could be left with no generators and no factories
					//but still needs to produce combat engineers to, maybe, continue surviving.
					if (countStruct(_STRUCTURES.gen, me) || (__cyb && __useCybEngineer && (cyborgOnlyGame || (gameTime > 480000))))
					{
						if (__cyb && (!turnOffCyborgs || !forceHover))
						{
							if (!__useCybEngineer && !countStruct(_STRUCTURES.derrick, me))
							{
								continue; //no derricks while trying to build attack cyborg
							}
							buildCyborg(_fc.id, __useCybEngineer);
						}
						else
						{
							if (useVtol && __facType === _STRUCTURES.vtolFactory && countStruct(_STRUCTURES.derrick, me))
							{
								buildVTOL(_fc.id);
							}
						}
					}
				}
			}
		}
	}
}
