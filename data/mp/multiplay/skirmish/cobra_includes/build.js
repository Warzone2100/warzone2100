
//Determine if we need a generator.
function needPowerGenerator()
{
	function uncached()
	{
		return ((countStruct(structures.derricks) - (countStruct(structures.gens) * 4)) > 0);
	}

	return cacheThis(uncached, [], undefined, 8000);
}

//Determine if this is a constructor droid. Specify and optional second paramter
//to find combat engineers.
function isConstruct(objID, countCybEng)
{
	var dr = getObject(DROID, me, objID);
	if (dr === null)
	{
		return false;
	}

	if ((dr.droidType === DROID_SENSOR) || (dr.droidType === DROID_REPAIR) || (dr.droidType === DROID_COMMAND))
	{
		return false;
	}

	if (!isDefined(countCybEng))
	{
		countCybEng = false;
	}

	return ((dr.droidType === DROID_CONSTRUCT) || (countCybEng && !isDefined(dr.weapons[0])));
}

//Returns unfinished structures in the form of IDs.
function unfinishedStructures()
{
	const SAFE_DIST = 30;
	var unfinished = [];
	var stuff = enumStruct(me).filter(function(obj) { return obj.status !== BUILT && obj.stattype !== RESOURCE_EXTRACTOR; });

	for (var i = 0, l = stuff.length; i < l; ++i)
	{
		var s = stuff[i];
		if (distBetweenTwoPoints(MY_BASE.x, MY_BASE.y, s.x, s.y) < SAFE_DIST)
		{
			unfinished.push(s.id);
		}
	}

	return unfinished;
}


//Can a construction droid do something right now.
function conCanHelp(mydroidID, bx, by)
{
	var mydroid = getObject(DROID, me, mydroidID);
	if (mydroid === null)
	{
		return false;
	}
	return (mydroid.order !== DORDER_BUILD &&
		mydroid.order !== DORDER_LINEBUILD &&
		mydroid.order !== DORDER_RECYCLE &&
		mydroid.busy !== true &&
		!repairDroid(mydroidID) &&
		droidCanReach(mydroid, bx, by)
	);
}

//Return all idle constructs object IDs.
function findIdleTrucks(type)
{
	const BUILDERS = isDefined(type) ? enumGroup(oilGrabberGroup) : enumGroup(constructGroup);
	var droidlist = [];

	for (var i = 0, s = BUILDERS.length; i < s; i++)
	{
		if (conCanHelp(BUILDERS[i].id, MY_BASE.x, MY_BASE.y))
		{
			droidlist.push(BUILDERS[i].id);
		}
	}

	return droidlist;
}

//Build a certain number of something
function countAndBuild(stat, count)
{
	if (countStruct(stat) < count)
	{
		if (isStructureAvailable(stat) && buildStuff(stat))
		{
			return true;
		}
	}

	return false;
}

//Find the closest derrick that is not guarded a defense.
function protectUnguardedDerricks(droid)
{
	var derrs = enumStruct(me, structures.derricks).sort(distanceToBase);
	const LEN = derrs.length;

	if (droid)
	{
		if (buildStructure(droid, returnDefense(), droid, 0))
		{
			return true;
		}

		return false;
	}

	if (LEN)
	{
		var undefended = [];

		for (var i = 0; i < LEN; ++i)
		{
			var found = false;
			var objects = enumRange(derrs[i].x, derrs[i].y, 8, me, false);

			for (var c = 0, u = objects.length; c < u; ++c)
			{
				if ((objects[c].type === STRUCTURE) && (objects[c].stattype === DEFENSE))
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				undefended.push(derrs[i]);
			}
		}

		if (undefended.length > 0)
		{
			if (buildStuff(returnDefense(), undefined, undefended[0], 0, true))
			{
				return true;
			}
		}
	}

	return false;
}

//Find a location to build something within a safe area.
//the parameter defendThis is used to build something (ie. a defensive structure)
//around what is passed to it.
function buildStructure(droid, stat, defendThis, blocking)
{
	if (!isStructureAvailable(stat, me))
	{
		return false;
	}
	if (!isDefined(blocking))
	{
		blocking = 0;
	}

	var loc;
	if (droid)
	{
		if (isDefined(defendThis))
		{
			loc = pickStructLocation(droid, stat, defendThis.x, defendThis.y, blocking);
		}
		else
		{
			loc = pickStructLocation(droid, stat, MY_BASE.x, MY_BASE.y, blocking);
		}

		if (isDefined(loc))
		{
			if ((droid.order !== DORDER_RTB) && !safeDest(me, loc.x, loc.y))
			{
				orderDroid(droid, DORDER_RTB);
				return false;
			}
			if (orderDroidBuild(droid, DORDER_BUILD, stat, loc.x, loc.y))
			{
				return true;
			}
		}
	}

	return false;
}

//Build some object. Builds modules on structures also.
function buildStuff(struc, module, defendThis, blocking, oilGroup)
{
	if (!isDefined(blocking))
	{
		blocking = 0;
	}

	var freeTrucks = findIdleTrucks(oilGroup);
	if (freeTrucks.length)
	{
		var truck = getObject(DROID, me, freeTrucks[0]);
		if (truck === null)
		{
			return false;
		}
		if (isDefined(module))
		{
			if (orderDroidBuild(truck, DORDER_BUILD, module, struc.x, struc.y))
			{
				return true;
			}
		}
		if (isDefined(defendThis))
		{
			if (buildStructure(truck, struc, defendThis, blocking))
			{
				return true;
			}
		}
		else
		{
			if (buildStructure(truck, struc, undefined, blocking))
			{
				return true;
			}
		}
	}

	return false;
}

//Check for unfinished structures and help complete them. Specify a droid ID
//and that droid will go help build something.
function checkUnfinishedStructures(droidID)
{
	var structs = unfinishedStructures();
	if (structs.length)
	{
		var def = isDefined(droidID);
		var trucks = findIdleTrucks();
		if (trucks.length || def)
		{
			var t = getObject(DROID, me, def ? droidID : trucks[0]);
			var s = getObject(STRUCTURE, me, structs[0]);
			if (t !== null && s !== null && orderDroidObj(t, DORDER_HELPBUILD, s))
			{
				return true;
			}
		}
	}

	return false;
}

function lookForOil()
{
	var droids = enumGroup(oilGrabberGroup);
	var oils = enumFeature(-1, OIL_RES).sort(distanceToBase);
	var bestDroid = null;
	var bestDist = 99999;

	for (var i = 0, oilLen = oils.length; i < oilLen; i++)
	{
		for (var j = 0, drLen = droids.length; j < drLen; j++)
		{
			var dist = distBetweenTwoPoints(droids[j].x, droids[j].y, oils[i].x, oils[i].y);
			var unsafe = enumRange(oils[i].x, oils[i].y, 6, ENEMIES, false).filter(isUnsafeEnemyObject);
			if (unsafe.length === 0 && bestDist > dist && conCanHelp(droids[j].id, oils[i].x, oils[i].y))
			{
				bestDroid = droids[j];
				bestDist = dist;
			}
		}

		if (bestDroid && !stopExecution("oil" + oils[i].y * mapWidth * oils[i].x, 50000))
		{
			bestDroid.busy = true;
			orderDroidBuild(bestDroid, DORDER_BUILD, structures.derricks, oils[i].x, oils[i].y);
			bestDist = 99999;
			bestDroid = null;
			return true;
		}
	}

	return false;
}

// Build CB, Wide-Spectrum or radar detector
// TODO: Find a way to space these out.
function buildSensors()
{
	const CB_TOWER = "Sys-CB-Tower01";
	const WS_TOWER = "Sys-SensoTowerWS";

	if (countAndBuild(WS_TOWER, 1))
	{
		return true;
	}

	if (countAndBuild(CB_TOWER, 1))
	{
		return true;
	}
}

//Builds an AA site for the personality. It will always use stormbringer AA
//once available.
function buildAAForPersonality()
{
	const VTOL_COUNT = countEnemyVTOL();

	//Use stormbringer if we have it.
	if (countAndBuild("P0-AASite-Laser", Math.floor(VTOL_COUNT / 3)))
	{
		return true;
	}
	else
	{
		var aaType = subPersonalities[personality].antiAir.defenses;
		for (var i = aaType.length - 1; i >= 0; --i)
		{
			if (countAndBuild(aaType[i].stat, Math.floor(VTOL_COUNT / 3)))
			{
				return true;
			}
		}
	}

	return false;
}

// type refers to either a hardpoint like structure or an artillery emplacement.
// returns undefined if no structure it can build can be built.
function returnDefense(type)
{
	if (!isDefined(type))
	{
		type = random(2);
	}

	const ELECTRONIC_CHANCE = 67;
	var defenses = (type === 0) ? subPersonalities[personality].primaryWeapon.defenses : subPersonalities[personality].artillery.defenses;
	var bestDefense = "Emplacement-MortarEMP"; //default

	//Choose a random electronic warfare defense if possible.
	if (random(100) < ELECTRONIC_CHANCE)
	{
		var avail = 0;
		for (var i = 0, t = ELECTRONIC_DEFENSES.length; i < t; ++i)
		{
			if(isStructureAvailable(ELECTRONIC_DEFENSES[i]))
			{
				avail += 1;
			}
		}

		if (avail > 0)
		{
			defenses = [];
			defenses.push(ELECTRONIC_DEFENSES[random(avail)]);
		}
	}

	for (var i = defenses.length - 1; i > -1; --i)
	{
		var def = isDefined(defenses[i].stat);
		if (def && isStructureAvailable(defenses[i].stat))
		{
			bestDefense = defenses[i].stat;
			break;
		}
		else if (!def && isStructureAvailable(defenses[i]))
		{
			bestDefense = defenses[i];
			break;
		}
	}

	return bestDefense;
}

// Immediately try building a defense near this truck.
function buildDefenseNearTruck(truck, type)
{
	if (!isDefined(type))
	{
		type = 0;
	}

	var defense = returnDefense(type);

	if (isDefined(defense))
	{
		var result = pickStructLocation(truck, defense, truck.x, truck.y, 1);
		if (result)
		{
			return orderDroidBuild(truck, DORDER_BUILD, defense, result.x, result.y);
		}
	}

	return false;
}

// Passing a truck will instruct that truck to pick
// a location to build a defense structure near it.
function buildDefenses(truck)
{
	var isDefensive = subPersonalities[personality].defensePriority >= 50 || subPersonalities[personality].resPath === "defensive";
	var pow = getRealPower();

	if ((gameTime > 120000) && (pow > SUPER_LOW_POWER || (isDefensive && (pow > MIN_POWER - 25))))
	{
		if (truck)
		{
			return buildDefenseNearTruck(truck, 0);
		}

		if (protectUnguardedDerricks() && pow > MIN_POWER)
		{
			return true;
		}

		if (buildSensors())
		{
			return true;
		}

		var def = returnDefense();
		if (isDefined(def) && pow > MIN_BUILD_POWER)
		{
			return countAndBuild(def, Infinity);
		}
	}

	return false;
}

//Build the basics when available. Has a different build order if NTW.
function buildBaseStructures()
{
	const GOOD_POWER_LEVEL = getRealPower() > 250;
	if (mapOilLevel() !== "NTW")
	{
		if ((!GOOD_POWER_LEVEL || getMultiTechLevel() > 1) && countAndBuild(structures.gens, 1))
		{
			return true;
		}
		if (countAndBuild(FACTORY, 2))
		{
			return true;
		}
		if (!researchComplete && countAndBuild(structures.labs, 2))
		{
			return true;
		}
		if (countAndBuild(structures.gens, 2))
		{
			return true;
		}
		if (countAndBuild(structures.hqs, 1))
		{
			return true;
		}
		if (!researchComplete && countAndBuild(structures.labs, 3))
		{
			return true;
		}
		if (needPowerGenerator() && countAndBuild(structures.gens, countStruct(structures.gens) + 1))
		{
			return true;
		}
		if (countAndBuild(CYBORG_FACTORY, 1))
		{
			return true;
		}
		if (countAndBuild(VTOL_FACTORY, 1))
		{
			return true;
		}
	}
	else
	{
		if (GOOD_POWER_LEVEL && !researchComplete && countAndBuild(structures.labs, 2))
		{
			return true;
		}
		if (countAndBuild(FACTORY, 1))
		{
			return true;
		}
		if (GOOD_POWER_LEVEL && countAndBuild(structures.hqs, 1))
		{
			return true;
		}
		if (GOOD_POWER_LEVEL && !researchComplete && countAndBuild(structures.labs, 5))
		{
			return true;
		}
		if (countAndBuild(structures.gens, 1))
		{
			return true;
		}
		if (GOOD_POWER_LEVEL && factoryBuildOrder())
		{
			return true;
		}
		if (!GOOD_POWER_LEVEL && needPowerGenerator() && countAndBuild(structures.gens, countStruct(structures.gens) + 1))
		{
			return true;
		}
	}

	//Build 1 repair facility
	if (countAndBuild(structures.extras[0], 1))
	{
		return true;
	}

	return false;
}

//Build factories.
function factoryBuildOrder()
{
	const MIN_FACTORY_COUNT = 1;
	const MAX_FACTORY_COUNT = 5;

	for (var i = 0; i < subPersonalities[personality].factoryOrder.length; ++i)
	{
		var fac = subPersonalities[personality].factoryOrder[i];

		if ((fac === VTOL_FACTORY && !useVtol) || (fac === CYBORG_FACTORY && (turnOffCyborgs || forceHover)))
		{
			continue;
		}

		var derrNum = countStruct(structures.derricks);
		var facNum = countStruct(fac);
		if (derrNum >= 18)
		{
			num = 5;
		}
		else if (derrNum >= 14)
		{
			num = 4;
		}
		else if (derrNum >= 9)
		{
			num = 3;
		}
		else if (derrNum >= 6)
		{
			num = 2;
		}
		else
		{
			num = MIN_FACTORY_COUNT;
		}

		if (num >= 3 && getRealPower() < MIN_POWER)
		{
			num = 2;
		}

		if (facNum < num && facNum < MAX_FACTORY_COUNT && countAndBuild(fac, num))
		{
			return true;
		}
	}

	return false;
}

function researchBuildOrder()
{
	var labs = countStruct(structures.labs);
	var seaMap = turnOffCyborgs || forceHover;
	const MAX_LAB_COUNT = 5;

	if (!researchComplete && labs < MAX_LAB_COUNT)
	{
		var amount = 3;
		var derrCount = countStruct(structures.derricks);

		if (derrCount >= 10)
		{
			amount = 5;
		}
		else if (derrCount >= 6 || seaMap === true)
		{
			amount = 4;
		}

		if (labs < amount && countAndBuild(structures.labs, amount))
		{
			return true;
		}
	}

	return false;
}

//Build minimum requirements of base structures.
function buildBaseStructures2()
{
	if (!countStruct(structures.gens))
	{
		return true;
	}

	if (researchBuildOrder())
	{
		return true;
	}

	if (factoryBuildOrder())
	{
		return true;
	}

	return false;
}

//Laser satellite/uplink center
function buildSpecialStructures()
{
	for (var i = 1, l = structures.extras.length; i < l; ++i)
	{
		if (countAndBuild(structures.extras[i], 1))
		{
			return true;
		}
	}

	return false;
}

//Build the minimum repairs and any vtol pads.
function buildExtras()
{
	var gens = countStruct(structures.gens);
	if (getRealPower() > SUPER_LOW_POWER && countStruct(structures.extras[0]) < 5 && countAndBuild(structures.extras[0], gens + 1))
	{
		return true;
	}

	var needVtolPads = ((2 * countStruct(structures.vtolPads)) < enumGroup(vtolGroup).length);
	if (needVtolPads && buildStuff(structures.vtolPads))
	{
		return true;
	}

	return false;
}


//Cobra's unique build decisions
function buildOrders()
{
	if (!findIdleTrucks().length) { return; }
	if (checkUnfinishedStructures()) { return; }
	if (buildBaseStructures()) { return; }
	if (maintenance()) { return; }
	if (buildSpecialStructures()) { return; }
	if (buildBaseStructures2()) { return; }
	if (random(100) < 33 && buildAAForPersonality()) { return; }
	if (buildExtras()) { return; }
	buildDefenses();
}

//Check if a building has modules to be built
function maintenance()
{
	if (!countStruct(structures.gens) || (countStruct(structures.derricks) < 4))
	{
		return false;
	}

	var modList;
	var struct = null;
	var module = "";
	if (mapOilLevel() === "NTW")
	{
		modList = [
			{"mod": "A0ResearchModule1", "amount": 1, "structure": structures.labs},
			{"mod": "A0PowMod1", "amount": 1, "structure": structures.gens},
			{"mod": "A0FacMod1", "amount": 2, "structure": FACTORY},
			{"mod": "A0FacMod1", "amount": 2, "structure": VTOL_FACTORY},
		];
	}
	else
	{
		modList = [
			{"mod": "A0PowMod1", "amount": 1, "structure": structures.gens},
			{"mod": "A0FacMod1", "amount": 1, "structure": FACTORY},
			{"mod": "A0ResearchModule1", "amount": 1, "structure": structures.labs},
			{"mod": "A0FacMod1", "amount": 2, "structure": FACTORY},
			{"mod": "A0FacMod1", "amount": 2, "structure": VTOL_FACTORY},
		];
	}

	for (var i = 0, l = modList.length; i < l; ++i)
	{
		var modObj = modList[i];

		if (isStructureAvailable(modObj.mod))
		{
			if (modObj.structure === VTOL_FACTORY && !isStructureAvailable(structures.vtolFactories))
			{
				//Stop wasting power on upgrading VTOL factories if we don't have them
				//researched yet (from some maps).
				continue;
			}

			var structList = enumStruct(me, modObj.structure).sort(distanceToBase).reverse();
			for (var c = 0, s = structList.length; c < s; ++c)
			{
				if (structList[c].modules < modObj.amount)
				{
					struct = structList[c];
					module = modObj.mod;
					break;
				}
			}
			if (struct !== null)
			{
				break;
			}
		}
	}

	if (mapOilLevel() === "NTW" && module === "A0PowMod1" && getRealPower() > 400)
	{
		return false;
	}

	if (((getRealPower() > SUPER_LOW_POWER) || (module === modList[0].mod)) && struct && buildStuff(struct, module))
	{
		return true;
	}

	return false;
}
