
/*
 * This file is responsible for droid production.
 *
 */

(function(_global) {
////////////////////////////////////////////////////////////////////////////////////////////

function ourBuildDroid(factory, name, bodies, propulsions, weapons1, weapons2, weapons3)
{
	return buildDroid(factory, name, bodies, propulsions, "", "", weapons1, weapons2, weapons3);
}

function produceTruck(factory, turrets)
{
	let turret = truckTurrets.concat();

	if (defined(turrets))
	{
		turret = turrets.concat();
	}

	turret.reverse();

	// TODO: switch to using chooseBodyWeaponPair() here
	const bodies = filterBodyStatsByUsage(BODYUSAGE.TRUCK, BODYCLASS.KINETIC).map((val) => (val.stat));
	const propulsions = getPropulsionStatsComponents(PROPULSIONUSAGE.GROUND|PROPULSIONUSAGE.HOVER);

	return ourBuildDroid(factory, "Fancy Truck", bodies, propulsions, turret);
}

function chooseWeapon(forVtol)
{
	if (!defined(forVtol))
	{
		forVtol = false;
	}

	if (forVtol)
	{
		const ret = chooseAvailableWeaponPathByRoleRatings(getProductionPaths(), chooseAttackWeaponRole(), 3);

		if (defined(ret))
		{
			return ret.vtols.concat().reverse();
		}
	}
	else
	{
		const ret = chooseAvailableWeaponPathByRoleRatings(getProductionPaths(), chooseAttackWeaponRole(), 0);

		if (defined(ret))
		{
			return ret.weapons.concat().reverse();
		}
	}
}

function chooseBodyWeaponPair(bodies, weapons)
{
	if (!defined(bodies) || !defined(weapons))
	{
		return undefined;
	}

	for (let i = 0; i < weapons.length; ++i)
	{
		const w = weapons[i].stat;
		const ww = weapons[i].weight;

		if (!componentAvailable(w))
		{
			continue;
		}

		for (let j = 0; j < bodies.length; ++j)
		{
			const b = bodies[j].stat;
			const bw = bodies[j].weight;

			if (!componentAvailable(b))
			{
				continue;
			}

			/* eslint-disable no-unreachable */
			switch (ww)
			{
				case WEIGHT.ULTRALIGHT:
					if (bw <= WEIGHT.LIGHT)
					{
						return {b: b, w: w};
					}
					break;
				case WEIGHT.LIGHT:
					if (bw <= WEIGHT.MEDIUM)
					{
						return {b: b, w: w};
					}
					break;
				case WEIGHT.MEDIUM:
					return {b: b, w: w};
					break;
				case WEIGHT.HEAVY:
					if (bw >= WEIGHT.MEDIUM)
					{
						return {b: b, w: w};
					}
					break;
				case WEIGHT.ULTRAHEAVY:
					if (bw >= WEIGHT.HEAVY)
					{
						return {b: b, w: w};
					}
					break;
			}
			/* eslint-enable no-unreachable */
		}
	}
}

function produceTank(factory)
{
	// TODO: needs refactoring. Make some more clever sorting.
	let bodies = [];

	if (chooseBodyClass() === BODYCLASS.KINETIC)
	{
		bodies = bodies.concat(
			filterBodyStatsByUsage(BODYUSAGE.GROUND, BODYCLASS.KINETIC),
			filterBodyStatsByUsage(BODYUSAGE.GROUND, BODYCLASS.THERMAL)
		);
	}
	else
	{
		bodies = bodies.concat(
			filterBodyStatsByUsage(BODYUSAGE.GROUND, BODYCLASS.THERMAL),
			filterBodyStatsByUsage(BODYUSAGE.GROUND, BODYCLASS.KINETIC)
		);
	}

	let propulsions;
	const ret = scopeRatings();
	const rnd = random(ret.land + ret.sea);

	if (!defined(rnd)) // we need only vtols?
	{
		return false;
	}

	propulsions = getPropulsionStatsComponents(PROPULSIONUSAGE.GROUND);

	if (iHaveHover())
	{
		if (rnd >= ret.land)
		{
			propulsions = getPropulsionStatsComponents(PROPULSIONUSAGE.HOVER);
		}
	}
	else
	{
		if (ret.land === 0)
		{
			return false;
		}
	}

	const bwPair = chooseBodyWeaponPair(bodies, chooseWeapon());

	if (!defined(bwPair))
	{
		return false;
	}

	return ourBuildDroid(factory, "Tank", bwPair.b, propulsions, bwPair.w, bwPair.w, bwPair.w);
}

function produceVtol(factory)
{
	// TODO: consider thermal bodies
	const bodies = filterBodyStatsByUsage(BODYUSAGE.AIR, BODYCLASS.KINETIC);
	const propulsions = getPropulsionStatsComponents(PROPULSIONUSAGE.VTOL);
	const bwPair = chooseBodyWeaponPair(bodies, chooseWeapon(true));

	if (!defined(bwPair))
	{
		return false;
	}

	return ourBuildDroid(factory, "VTOL", bwPair.b, propulsions, bwPair.w, bwPair.w, bwPair.w);
}

function produceTemplateFromList(factory, list)
{
	const ret = scopeRatings();

	for (let i = list.length - 1; i >= 0; --i)
	{
		if ((ret.land === 0 && !isHoverPropulsion(list[i].prop) && !isVtolPropulsion(list[i].prop)) ||
			(ret.land === 0 && ret.sea === 0 && !isVtolPropulsion(list[i].prop)) ||
			(isVtolPropulsion(list[i].prop) !== (factory.stattype === VTOL_FACTORY)))
		{
			continue;
		}

		if ((!randomTemplates) || withChance(100 / (i + 1)))
		{
			if (ourBuildDroid(factory, "Template Droid", list[i].body, list[i].prop, list[i].weapons[0], list[i].weapons[1], list[i].weapons[2]))
			{
				return true;
			}
		}
	}

	return false;
}

function produceTemplate(factory)
{
	const path = chooseAvailableWeaponPathByRoleRatings(getProductionPaths(), chooseAttackWeaponRole(), 1);

	if (defined(path))
	{
		return produceTemplateFromList(factory, path.templates);
	}

	return false;
}

_global.checkTruckProduction = function() {
	const trucks = enumTrucks();
	const hoverTrucksCount = trucks.filter((droid) => (isHoverPropulsion(droid.propulsion))).length;

	if (iHaveHover() && hoverTrucksCount < personality.minHoverTrucks)
	{
		const groundTrucks = trucks.filter((droid) => (!isHoverPropulsion(droid.propulsion)));

		if (groundTrucks.length > personality.minTrucks)
		{
			groundTrucks.length -= personality.minTrucks;
			groundTrucks.forEach((droid) => { orderDroid(droid, DORDER_RECYCLE); });

			return false;
		}
	}

	if (trucks.length >= getDroidLimit(me, DROID_CONSTRUCT))
	{
		return false;
	}

	if ((trucks.length < personality.minTrucks) ||
		(myPower() > personality.maxPower) ||
		(iHaveHover() && hoverTrucksCount < personality.minHoverTrucks))
	{
		const f = enumFinishedStructList(structures.factories)[0];

		if (defined(f))
		{
			if (structureIdle(f))
			{
				if (produceTruck(f))
				{
					return true;
				}
			}
		}

		if (defined(f))
		{
			return false;
		}

		const ft = enumFinishedStructList(structures.templateFactories)[0];

		if (defined(ft))
		{
			if (structureIdle(ft))
			{
				if (produceTemplateFromList(ft, truckTemplates))
				{
					return true;
				}
			}
		}
	}

	if (!iHaveArty())
	{
		return false;
	}

	const sensors = enumDroid(me, DROID_SENSOR).length;

	if (withChance(100 - 100 * sensors / personality.maxSensors))
	{
		const f = enumFinishedStructList(structures.factories)[0];

		if (defined(f))
		{
			if (structureIdle(f))
			{
				if (produceTruck(f, sensorTurrets))
				{
					return true;
				}
			}
		}
	}

	return false;
};

function checkTankProduction()
{
	if (!iCanDesign())
	{
		return false; // don't cheat by producing tanks before design is available (also saves money for early generators)
	}

	let success = false;

	enumIdleStructList(structures.factories).forEach((factory) => {
		success = success || produceTank(factory);
	});

	return success;
}

function checkTemplateProduction()
{
	let success = false;

	enumIdleStructList(structures.templateFactories)
		.concat(enumIdleStructList(structures.vtolFactories))
		.forEach((factory) => {
		success = success || produceTemplate(factory);
	});

	return success;
}

function checkVtolProduction()
{
	let success = false;

	if (!iCanDesign())
	{
		return false; // don't cheat by producing vtols before design is available
	}

	enumIdleStructList(structures.vtolFactories).forEach((factory) => {
		success = success || produceVtol(factory);
	});

	return success;
}

_global.checkProduction = function() {
	switch (chooseObjectType())
	{
		case 1:
			if (checkTemplateProduction())
			{
				return;
			}
		case 3:
			if (checkVtolProduction())
			{
				return;
			}
		default:
			if (checkTankProduction())
			{
				return;
			}
	}

	// if having too much energy, don't care about what we produce
	if (myPower() > personality.maxPower)
	{
		queue("checkConstruction");
		checkTemplateProduction();
		checkTankProduction();
		checkVtolProduction();
	}
};

////////////////////////////////////////////////////////////////////////////////////////////
})(this);
