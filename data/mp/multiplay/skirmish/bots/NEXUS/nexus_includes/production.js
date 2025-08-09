
// Return an object with the properties: constructs, sensors, and repairs that
// represent how many of those droid types are in queued production.
function getVirtualSystemCount()
{
	var counts = {constructs: 0, sensors: 0, repairs: 0,};
	var factories = enumStruct(me, BASE_STRUCTURES.factories);
	var cybFactories = enumStruct(me, BASE_STRUCTURES.templateFactories);
	var i = 0;
	var len = 0;
	var virDroid;

	for (i = 0, len = factories.length; i < len; ++i)
	{
		virDroid = getDroidProduction(factories[i]);

		if (virDroid !== null)
		{
			if (virDroid.droidType === DROID_CONSTRUCT)
			{
				counts.constructs = counts.constructs + 1;
			}
			else if (virDroid.droidType === DROID_SENSOR)
			{
				counts.sensors = counts.sensors + 1;
			}
			else if (virDroid.droidType === DROID_REPAIR)
			{
				counts.repairs = counts.repairs + 1;
			}
		}
	}

	for (i = 0, len = cybFactories.length; i < len; ++i)
	{
		virDroid = getDroidProduction(cybFactories[i]);

		if (virDroid !== null)
		{
			if (virDroid.droidType === DROID_CONSTRUCT)
			{
				counts.constructs = counts.constructs + 1;
			}
			else if (virDroid.droidType === DROID_REPAIR)
			{
				counts.repairs = counts.repairs + 1;
			}
		}
	}

	return counts;
}

function hoverPropulsionAvailable()
{
	for (let i = 0, len = HOVER_PROPULSIONS.length; i < len; ++i)
	{
		if (componentAvailable(HOVER_PROPULSIONS[i]))
		{
			return true;
		}
	}

	return false;
}

function isCyborgStat(stat)
{
	return arrayContains(stat, CYBORG_COMPONENTS);
}

function produceConstructors()
{
	const CONSTRUCT_LIMIT = getDroidLimit(me, DROID_CONSTRUCT);
	const MAX_TRUCK_FACTORIES = 3; //max factories to use for truck production
	var truckLimit = (getRealPower(me) > LOW_POWER) ? TRUCK_INFO.max : TRUCK_INFO.min;

	if (truckLimit > CONSTRUCT_LIMIT)
	{
		truckLimit = CONSTRUCT_LIMIT;
	}

	var totalTrucks = getVirtualSystemCount().constructs + enumDroid(me, DROID_CONSTRUCT).length;
	var occupiedFactories = 0;
	var success = false;
	var factories = enumStruct(me, BASE_STRUCTURES.factories);
	var cybFactories = enumStruct(me, BASE_STRUCTURES.templateFactories);

	for (let i = 0, len = factories.length; i < len; ++i)
	{
		var factory = factories[i];

		if (factory.status !== BUILT || !structureIdle(factory))
		{
			continue;
		}

		if (totalTrucks < truckLimit && occupiedFactories < MAX_TRUCK_FACTORIES)
		{
			for (let j = STANDARD_TRUCK_TEMPLATES.length - 1; j > -1; --j)
			{
				var tmp = STANDARD_TRUCK_TEMPLATES[j];

				if (buildDroid(factory, "Truck", tmp.body, tmp.prop, "", "", tmp.weaps[0]))
				{
					success = true;
					occupiedFactories += 1;
					totalTrucks += 1;
					break;
				}
			}
		}
	}

	// build cyborg engineers if needed, no building limit here
	for (let i = 0, len = cybFactories.length; i < len; ++i)
	{
		var cybFactory = cybFactories[i];

		if (cybFactory.status !== BUILT || !structureIdle(cybFactory))
		{
			continue;
		}

		if (totalTrucks < truckLimit)
		{
			for (let j = STANDARD_CYBORG_ENGINEER_TEMPLATES.length - 1; j > -1; --j)
			{
				var tmp = STANDARD_CYBORG_ENGINEER_TEMPLATES[j];

				if (buildDroid(cybFactory, "Combat Engineer", tmp.body, tmp.prop, "", "", tmp.weaps[0]))
				{
					success = true;
					totalTrucks += 1;
					break;
				}
			}
		}
		else
		{
			break;
		}
	}

	return success;
}

function getBestRandomTemplate(type, offset)
{
	var what;

	if (!defined(type))
	{
		debugMessage("getBestRandomTemplate. Undefined template type.");
		return undefined;
	}

	if (!defined(offset))
	{
		offset = TEMPLATE_BEST_OFFSET;
	}

	switch (type)
	{
		case "TANK": what = STANDARD_TEMPLATES; break;
		case "CYBORG": what = STANDARD_LIGHT_CYBORG_TEMPLATES; break;
		case "CYBORG_SUPER": what = STANDARD_SUPER_CYBORG_TEMPLATES; break;
		case "VTOL": what = STANDARD_VTOL_TEMPLATES; break;
		case "REPAIR": what = STANDARD_TANK_REPAIRS; break;
		case "MECHANIC": what = STANDARD_CYBORG_MECHANIC_TEMPLATES; break;
		case "TRUCK": what = STANDARD_TRUCK_TEMPLATES; break;
		case "ENGINEER": what = STANDARD_CYBORG_ENGINEER_TEMPLATES; break;
		case "SENSOR": what = STANDARD_SENSOR_TEMPLATES; break;
		default:
			debugMessage("getBestRandomTemplate. Template type unknown.");
			return undefined;
	}

	var templates = [];
	var count = what.length - 1;
	var count2 = 0;

	if (offset > count)
	{
		debugMessage("getBestRandomTemplate. Template offset out of bound of template array. " + type);
		offset = count;
	}

	while ((count2 < offset) && (count >= 0))
	{
		var tmp = what[count];

		if (componentAvailable(tmp.body) && componentAvailable(tmp.prop))
		{
			var num = 0;

			for (let i = 0, len = tmp.weaps.length; i < len; ++i)
			{
				if (componentAvailable(tmp.weaps[i]))
				{
					++num;

					// All weapons available
					if (num === len)
					{
						++count2;
						templates.push(tmp);
					}
				}
			}
		}

		--count;
	}

	if (templates.length === 0)
	{
		return undefined;
	}

	//Return a random among the best templates
	return templates[random(count2)];
}

function bodyTurretCount(bodyStat)
{
	if (arrayContains(bodyStat, TWO_TURRET_BODY))
	{
		return 2;
	}

	return 1;
}

function needTank()
{
	if (enumGroup(groups.defenders).length < nexusBranch[branch].maximums.defenders)
	{
		return true;
	}

	if (enumGroup(groups.scouts).length < nexusBranch[branch].maximums.scouts)
	{
		return true;
	}

	if (enumGroup(groups.attackers).length < nexusBranch[branch].maximums.attackers)
	{
		return true;
	}

	return false;
}

//Return the best available repair. Specify a definable parameter for a cyborg mechanic.
//NOTE: multi-turret checks when that gets supported.
function getBestRepairTemplate(cyborgFlag)
{
	var templates = defined(cyborgFlag) ? STANDARD_CYBORG_MECHANIC_TEMPLATES : STANDARD_TANK_REPAIRS;

	for (let i = 0, len = templates.length; i < len; ++i)
	{
		var tmp = templates[i];

		if (componentAvailable(tmp.body) && componentAvailable(tmp.prop) && componentAvailable(tmp.weaps[0]))
		{
			return tmp;
		}
	}

	return undefined;
}

// Sensors and repairs can go in here, too.
function produceGroundUnits()
{
	var success = false;
	var factories = enumStruct(me, BASE_STRUCTURES.factories);
	var totalReps = (getVirtualSystemCount().repairs + enumDroid(me, DROID_REPAIR).length);

	for (let i = 0, len = factories.length; i < len; ++i)
	{
		var factory = factories[i];

		if (factory.status !== BUILT || !structureIdle(factory) || getRealPower(me) < LOW_POWER)
		{
			return;
		}

		var tmp = getBestRepairTemplate();

		if (defined(tmp) && totalReps < 3)
		{
			if (buildDroid(factory, "Nexus unit", tmp.body, tmp.prop, "", "", tmp.weaps[0], "", ""))
			{
				success = true;
				++totalReps;
			}
		}
		else if (needTank())
		{
			tmp = getBestRandomTemplate("TANK", TEMPLATE_BEST_OFFSET);

			if (defined(tmp))
			{
				var t = bodyTurretCount(tmp.body);

				if (buildDroid(factory, "Nexus unit", tmp.body, tmp.prop, "", "",
					tmp.weaps[0],
					t > 1 ? tmp.weaps[1] : "",
					t > 2 ? tmp.weaps[2] : ""))
				{
					success = true;
				}
			}
		}
	}

	return success;
}

function produceCyborgs()
{
	if (enumGroup(groups.cyborgs).length > nexusBranch[branch].maximums.cyborgs)
	{
		return false;
	}

	var success = false;
	var cybFactories = enumStruct(me, BASE_STRUCTURES.templateFactories);

	for (let i = 0, len = cybFactories.length; i < len; ++i)
	{
		var cybFactory = cybFactories[i];

		if (cybFactory.status !== BUILT || !structureIdle(cybFactory) || getRealPower(me) < LOW_POWER)
		{
			return;
		}

		var tmp = getBestRepairTemplate(true);

		//Build a mechanic randomly.
		if (defined(tmp) && random(5) === 0)
		{
			if (buildDroid(cybFactory, "Nexus repair", tmp.body, tmp.prop, "", "", tmp.weaps[0], "", ""))
			{
				success = true;
			}
		}
		else
		{
			tmp = getBestRandomTemplate("CYBORG_SUPER", 3); //Go for super cyborg

			if (!defined(tmp))
			{
				tmp = getBestRandomTemplate("CYBORG", 3); //go for a little cyborg
			}

			if (defined(tmp) && buildDroid(cybFactory, "Nexus cyborg", tmp.body, tmp.prop, "", "", tmp.weaps[0], "", ""))
			{
				success = true;
			}
		}
	}

	return success;
}

function produceVtols()
{
	if (numVtolUnits >= nexusBranch[branch].maxVTOLs)
	{
		return false; // Have too many of them.
	}

	var success = false;
	var vtolFactories = enumStruct(me, BASE_STRUCTURES.vtolFactories);

	for (let i = 0, len = vtolFactories.length; i < len; ++i)
	{
		var vtolFactory = vtolFactories[i];

		if (vtolFactory.status !== BUILT || !structureIdle(vtolFactory) || getRealPower(me) < LOW_POWER)
		{
			return;
		}

		var tmp = getBestRandomTemplate("VTOL", 3);

		if (defined(tmp))
		{
			var t = bodyTurretCount(tmp.body);

			if (buildDroid(vtolFactory, "Nexus VTOL", tmp.body, tmp.prop, "", "",
				tmp.weaps[0],
				t > 1 ? tmp.weaps[1] : "",
				t > 2 ? tmp.weaps[2] : ""))
			{
				success = true;
			}
		}
	}

	return success;
}

//Spread over three ticks (for the non-truck units).
function productionMain()
{
	if (countDroid(me) >= MAX_DROID_LIMIT)
	{
		return;
	}

	if (produceConstructors())
	{
		return;
	}

	if (!hasPowerSource(me))
	{
		return;
	}

	//NOTE: Manufacturing is queued from here on out. 1 tick per "type" of factory.
	for (let i = 0, len = nexusBranch[branch].factoryPreference.length; i < len; ++i)
	{
		var pref = nexusBranch[branch].factoryPreference[i];

		if (pref === FACTORY)
		{
			queue("produceGroundUnits", (i + 1) * TICK);
		}
		else if (pref === CYBORG_FACTORY)
		{
			queue("produceCyborgs", (i + 1) * TICK);
		}
		else if (pref === VTOL_FACTORY)
		{
			queue("produceVtols", (i + 1) * TICK);
		}
	}
}
