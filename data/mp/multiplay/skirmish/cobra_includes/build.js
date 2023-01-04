
//Determine if we need a generator.
function needPowerGenerator()
{
	function uncached()
	{
		return ((countStruct(structures.derrick, me) - (countStruct(structures.gen, me) * 4)) > 0);
	}

	return cacheThis(uncached, [], "needPowerGenerator" + me, 8000);
}

function minTruckCount()
{
	function uncached()
	{
		return (highOilMap() ? 3 : 2) * MIN_TRUCKS_PER_GROUP;
	}

	return cacheThis(uncached, [], "minTruckCount" + me, Infinity);
}

//Determine if this is a constructor droid. Specify and optional second paramter
//to find combat engineers.
function isConstruct(objID, countCybEng)
{
	let dr = getObject(DROID, me, objID);
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
	const SAFE_DIST = 20;
	let unfinished = [];
	let stuff = enumStruct(me).filter((obj) => (
		obj.status !== BUILT && obj.stattype !== RESOURCE_EXTRACTOR
	));

	for (let i = 0, l = stuff.length; i < l; ++i)
	{
		let s = stuff[i];
		if (s.stattype === DEFENSE && ((!componentAvailable("hover01") || (distBetweenTwoPoints(MY_BASE.x, MY_BASE.y, s.x, s.y) > SAFE_DIST))))
		{
			continue;
		}

		unfinished.push(s.id);
	}

	return unfinished;
}


//Can a construction droid do something right now.
function conCanHelp(mydroidID, bx, by)
{
	let mydroid = getObject(DROID, me, mydroidID);
	if (mydroid === null)
	{
		return false;
	}
	return (mydroid.order !== DORDER_BUILD &&
		mydroid.order !== DORDER_HELPBUILD &&
		mydroid.order !== DORDER_LINEBUILD &&
		mydroid.order !== DORDER_RECYCLE &&
		!repairDroid(mydroidID) &&
		droidCanReach(mydroid, bx, by)
	);
}

//Return all idle constructs object IDs for a given group.
function findIdleTrucks(group)
{
	let builders;
	let droidlist = [];

	if (!isDefined(group))
	{
		builders = enumGroup(constructGroup);
	}
	else
	{
		builders = enumGroup(group);
	}

	for (let i = 0, s = builders.length; i < s; ++i)
	{
		let con = builders[i];
		if (conCanHelp(con.id, con.x, con.y))
		{
			droidlist.push(con.id);
		}
	}

	return droidlist;
}

//Build a certain number of something
function countAndBuild(stat, count)
{
	if (countStruct(stat, me) < count)
	{
		if (isStructureAvailable(stat) && buildStuff(stat))
		{
			return true;
		}
	}

	return false;
}

// Use this to build a defense next to a derrick (that was taken before we got to build there)
// This can be called from eventStructureBuilt() to build a few defenses with a chance.
function fastDefendSpot(structure, droid)
{
	if (!droid)
	{
		return;
	}

	const MIN_DEFENSES = (gameTime < 900000) ? 1 : 2;
	if (structure && (structure.stattype === FACTORY ||
		structure.stattype === CYBORG_FACTORY ||
		structure.stattype === VTOL_FACTORY ||
		structure.stattype === POWER_GEN ||
		structure.stattype === RESEARCH_LAB ||
		structure.stattype === HQ))
	{
		return; //do not waste time trying to defend basic base structures.
	}

	let chance = (gameTime > 900000 && ((structure && structure.stattype === RESOURCE_EXTRACTOR) || (random(100) < 20)));
	let structs = enumRange(droid.x, droid.y, 7, me, false).filter((obj) => (obj.type === STRUCTURE));
	let defenses = structs.filter((obj) => (obj.stattype === DEFENSE));
	let enemyDerr = enumRange(droid.x, droid.y, 8, ENEMIES, false).filter((obj) => (obj.type === STRUCTURE && obj.stattype === RESOURCE_EXTRACTOR));
	//Build a defense structure here.
	if (chance || (defenses.length < MIN_DEFENSES) || (enemyDerr.length > 0 && defenses.length === 0))
	{
		buildDefenses(droid, true); // Build right where this droid is at.
		return;
	}

	//Try a sensor tower
	if (gameTime > 900000 && random(100) < 67 && structs.length < 5)
	{
		let sensor;
		const TOWERS = [ "Sys-SensoTowerWS", "Sys-SensoTower02" ];
		for (let i = 0, len = TOWERS.length; i < len; ++i)
		{
			let sen = TOWERS[i];
			if (isStructureAvailable(sen))
			{
				sensor = sen;
				break;
			}
		}
		if (isDefined(sensor))
		{
			let result = pickStructLocation(droid, sensor, droid.x, droid.y, 1);
			if (result)
			{
				orderDroidBuild(droid, DORDER_BUILD, sensor, result.x, result.y);
			}
		}
	}
}

//Find the closest derrick that is not guarded a defense.
function protectUnguardedDerricks(droid)
{
	let derrs = enumStruct(me, structures.derrick);
	const LEN = derrs.length;
	const MAX_BLOCKING = 8;
	const HIGH_OIL = highOilMap();

	if (droid)
	{
		if (buildStructure(droid, returnDefense(), droid, MAX_BLOCKING))
		{
			return true;
		}

		return false;
	}

	if (LEN > 0)
	{
		let undefended = [];

		for (let i = 0; i < LEN; ++i)
		{
			let found = false;
			let objects = enumRange(derrs[i].x, derrs[i].y, (HIGH_OIL) ? 4 : 8, me, false);

			for (let c = 0, u = objects.length; c < u; ++c)
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
			//Don't defend it ATM if that derrick is surrounded by enemies.
			if (!HIGH_OIL)
			{
				undefended = undefended.filter((obj) => (
					(gameTime < 600000 && distBetweenTwoPoints(obj.x, obj.y, MY_BASE.x, MY_BASE.y) > 9) ||
						enumRange(obj.x, obj.y, 6, ENEMIES, false).length === 0
				)).sort(distanceToBase);
			}
			else
			{
				undefended = undefended.sort(distanceToBase);
			}

			if (undefended.length > 0 && buildStuff(returnDefense(), undefined, undefended[0], MAX_BLOCKING, oilGrabberGroup))
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
	if (!isDefined(stat) || !isStructureAvailable(stat, me))
	{
		return false;
	}
	if (!isDefined(blocking))
	{
		blocking = 0;
	}

	let loc;
	if (droid)
	{
		if (isDefined(defendThis))
		{
			loc = pickStructLocation(droid, stat, defendThis.x, defendThis.y, blocking);
		}
		else
		{
			let tempBasePos = randomOffsetLocation(MY_BASE);
			loc = pickStructLocation(droid, stat, tempBasePos.x, tempBasePos.y, blocking);
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
function buildStuff(struc, module, defendThis, blocking, group)
{
	if (!isDefined(struc))
	{
		return false;
	}

	if (!isDefined(blocking))
	{
		blocking = 0;
	}

	let freeTrucks = findIdleTrucks(isDefined(group) ? group : undefined);

	if (freeTrucks.length > 0)
	{
		let truck = getObject(DROID, me, freeTrucks[0]);
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
function checkUnfinishedStructures(group)
{
	if (!isDefined(group))
	{
		group = constructGroup;
	}

	let structs = unfinishedStructures();
	for (let i = 0, len = structs.length; i < len; ++i)
	{
		let structure = getObject(STRUCTURE, me, structs[i]);
		if (structure === null)
		{
			return false;
		}

		let trucks = findIdleTrucks(group);
		for (let j = 0, len2 = trucks.length; j < len2; ++j)
		{
			let truck = getObject(DROID, me, trucks[j]);
			if (truck === null)
			{
				return false;
			}

			if (orderDroidObj(truck, DORDER_HELPBUILD, structure))
			{
				return true;
			}
		}
	}

	return false;
}

function holdAllOilTrucks()
{
	let oilGrabbers = enumGroup(oilGrabberGroup);

	for (let i = 0, len = oilGrabbers.length; i < len; ++i)
	{
		if (oilGrabbers[i].order !== DORDER_RECYCLE &&
			oilGrabbers[i].order !== DORDER_HOLD)
		{
			orderDroid(oilGrabbers[i], DORDER_HOLD);
		}
	}
}

function skipOilGrabIfEasy()
{
	if (difficulty === EASY)
	{
		let myDerrickCount = enumStruct(me, structures.derrick).filter((obj) => (
			obj.status === BUILT
		)).length;
		let enemies = findLivingEnemies();

		for (let i = 0, len = enemies.length; i < len; ++i)
		{
			if (myDerrickCount >= 5 && myDerrickCount >= countStruct(structures.derrick, enemies[i]))
			{
				holdAllOilTrucks();
				return true;
			}
		}
	}

	return false;
}

function lookForOil()
{
	if (currently_dead)
	{
		return;
	}

	let droids = enumGroup(oilGrabberGroup);
	let oils = enumFeature(ALL_PLAYERS, OIL_RES).sort(distanceToBase);

	if (!forceDerrickBuildDefense && (oils.length < 2))
	{
		//Ok, most oils are already owned so go ahead and defend all derricks from now one
		forceDerrickBuildDefense = true;
	}
	if (forceDerrickBuildDefense && random(100) < 15)
	{
		protectUnguardedDerricks();
	}

	if (oils.length === 0 && highOilMap() && maintenance(oilGrabberGroup))
	{
		return;
	}
	if (skipOilGrabIfEasy())
	{
		return;
	}

	for (let i = 0, oilLen = oils.length; i < oilLen; ++i)
	{
		let bestDroid;
		let bestDist = Infinity;
		let oil = oils[i];

		for (let j = 0, drLen = droids.length; j < drLen; ++j)
		{
			let droid = droids[j];
			let dist = distBetweenTwoPoints(droid.x, droid.y, oil.x, oil.y);
			let unsafe = enumRange(oil.x, oil.y, 6, ENEMIES, false).filter(isUnsafeEnemyObject);

			if (unsafe.length === 0 && bestDist > dist && conCanHelp(droid.id, oil.x, oil.y))
			{
				bestDroid = droid;
				bestDist = dist;
			}
		}

		if (bestDroid && !stopExecution("oil" + oil.y * mapWidth * oil.x, 50000))
		{
			orderDroidBuild(bestDroid, DORDER_BUILD, structures.derrick, oil.x, oil.y);
			return;
		}
	}
}

//Builds an AA site for the personality. It will always use stormbringer AA
//once available.
function buildAAForPersonality()
{
	const VTOL_COUNT = countEnemyVTOL();
	let minAAs = Math.floor(VTOL_COUNT / 2);
	let high = highOilMap();
	if (!high && (getRealPower() >= MIN_BUILD_POWER))
	{
		minAAs = VTOL_COUNT;
	}
	else if (high && (getRealPower() >= SUPER_LOW_POWER))
	{
		minAAs = VTOL_COUNT * 4;
	}

	//Use stormbringer if we have it.
	if (countAndBuild("P0-AASite-Laser", minAAs))
	{
		return true;
	}
	else
	{
		let aaType = subPersonalities[personality].antiAir.defenses;
		for (let i = aaType.length - 1; i >= 0; --i)
		{
			if (countAndBuild(aaType[i].stat, minAAs))
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
	let standardDefenses = subPersonalities[personality].primaryWeapon.defenses;
	let artilleryDefenses = subPersonalities[personality].artillery.defenses;
	let defenses = (type === 0) ? artilleryDefenses.concat(standardDefenses) : standardDefenses.concat(artilleryDefenses);
	let bestDefense;

	//Choose a random electronic warfare defense if possible.
	if (random(100) < ELECTRONIC_CHANCE)
	{
		let avail = 0;
		for (let i = 0, t = ELECTRONIC_DEFENSES.length; i < t; ++i)
		{
			if (isStructureAvailable(ELECTRONIC_DEFENSES[i]))
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

	for (let i = defenses.length - 1; i > -1; --i)
	{
		let def = isDefined(defenses[i].stat);
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

	if (!isDefined(bestDefense))
	{
		if (isStructureAvailable("GuardTower1"))
		{
			return "GuardTower1"; //hmg tower
		}

		return undefined;
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

	let defense = returnDefense(type);

	if (isDefined(defense))
	{
		const MAX_BLOCKING = 8;
		let tempTruckPos = randomOffsetLocation({x: truck.x, y: truck.y});
		let result = pickStructLocation(truck, defense, tempTruckPos.x, tempTruckPos.y, MAX_BLOCKING);
		if (result)
		{
			return orderDroidBuild(truck, DORDER_BUILD, defense, result.x, result.y);
		}
	}

	return false;
}

//Defend a random derrick with a defense. Does NOT include those close to base.
function defendRandomDerrick()
{
	const MAX_DIST = 30;
	let derrs = enumStruct(me, structures.derrick).filter((obj) => (
		distBetweenTwoPoints(MY_BASE.x, MY_BASE.y, obj.x, obj.y) > MAX_DIST
	));

	if (derrs.length > 0)
	{
		const MAX_BLOCKING = 8;
		let derr = derrs[random(derrs.length)];
		let defs = enumRange(derr.x, derr.y, 10, ALLIES, false).filter((obj) => (
			obj.type === STRUCTURE && obj.stattype === DEFENSE
		)).length;

		if ((defs < 4) && buildStuff(returnDefense(), undefined, derr, MAX_BLOCKING, oilGrabberGroup))
		{
			return true;
		}
	}

	return false;
}

// Passing a truck will instruct that truck to pick
// a location to build a defense structure near it.
function buildDefenses(truck, urgent)
{
	let isDefensive = subPersonalities[personality].defensePriority >= 50 || subPersonalities[personality].resPath === "defensive";
	let pow = getRealPower();
	let enoughPower = (pow > SUPER_LOW_POWER || (isDefensive && (pow > SUPER_LOW_POWER - 25)));

	if (!isDefined(urgent))
	{
		urgent = false;
	}

	if (urgent || enoughPower)
	{
		if (truck)
		{
			return buildDefenseNearTruck(truck, 0);
		}

		if (highOilMap())
		{
			if (!defendNTWMap())
			{
				return defendRandomDerrick();
			}
		}
		else
		{
			return defendRandomDerrick();
		}
	}

	return false;
}

//Build the basics when available. Has a different build order if NTW.
function buildBaseStructures()
{
	const GOOD_POWER_LEVEL = getRealPower() > 250;

	if ((getMultiTechLevel() < 4) && forceHover && GOOD_POWER_LEVEL && countAndBuild(structures.lab, 1))
	{
		return true;
	}

	if (!highOilMap())
	{
		if (GOOD_POWER_LEVEL && countAndBuild(structures.factory, 1))
		{
			return true;
		}
		if ((!GOOD_POWER_LEVEL || (getMultiTechLevel() > 1)) && countAndBuild(structures.gen, 1))
		{
			return true;
		}
		if (countAndBuild(structures.factory, 2))
		{
			return true;
		}
		if ((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(structures.lab, 2))
		{
			return true;
		}
		if (countAndBuild(structures.gen, 1))
		{
			return true;
		}
		if (countAndBuild(structures.hq, 1))
		{
			return true;
		}
		if (countAndBuild(structures.gen, 2))
		{
			return true;
		}
		if ((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(structures.lab, 3))
		{
			return true;
		}
		if (needPowerGenerator() && countAndBuild(structures.gen, countStruct(structures.gen, me) + 1))
		{
			return true;
		}
		if (countAndBuild(structures.cyborgFactory, 1))
		{
			return true;
		}
		if (countAndBuild(structures.vtolFactory, 1))
		{
			return true;
		}
		//Build 1 repair facility
		if (countAndBuild(structures.repair, 1))
		{
			return true;
		}
	}
	else
	{
		let haveAllies = (alliancesType === ALLIANCES_TEAMS) && (playerAlliance(true).length > 0);

		if ((!GOOD_POWER_LEVEL || (getMultiTechLevel() > 1)) && countAndBuild(structures.gen, 1))
		{
			return true;
		}
		if (getRealPower() < 550 && countAndBuild(structures.gen, 4))
		{
			return true; //a little fail-safe
		}
		if ((getMultiTechLevel() < 4) && GOOD_POWER_LEVEL && randomResearchLabStart && !researchComplete && countAndBuild(structures.lab, 1))
		{
			return true;
		}
		if (countAndBuild(structures.factory, 2))
		{
			return true;
		}
		if ((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(structures.lab, (haveAllies) ? 2 : 4))
		{
			return true;
		}
		if (countAndBuild(structures.gen, 1))
		{
			return true; //a little fail-safe
		}
		if (countAndBuild(structures.hq, 1))
		{
			return true;
		}
		if (countAndBuild(structures.factory, 3))
		{
			return true;
		}
		if ((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(structures.lab, 5))
		{
			return true;
		}
		if (needPowerGenerator() && countAndBuild(structures.gen, 7))
		{
			return true;
		}
		if (countAndBuild(structures.cyborgFactory, 3))
		{
			return true;
		}
		if (countAndBuild(structures.factory, 5))
		{
			return true;
		}
		if (needPowerGenerator() && countAndBuild(structures.gen, 8))
		{
			return true;
		}
		if (countAndBuild(structures.cyborgFactory, 5))
		{
			return true;
		}
		if (GOOD_POWER_LEVEL && countAndBuild(structures.repair, 3))
		{
			return true;
		}
	}

	if (getMultiTechLevel() > 1 && countStruct(structures.vtolFactory, me) > 0 && countAndBuild(structures.vtolPad, 3))
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

	for (let i = 0; i < subPersonalities[personality].factoryOrder.length; ++i)
	{
		let fac = subPersonalities[personality].factoryOrder[i];

		if ((fac === structures.vtolFactory && !useVtol) || (fac === structures.cyborgFactory && (turnOffCyborgs || forceHover)))
		{
			continue;
		}
		if (fac === structures.vtolFactory && !getResearch("R-Struc-VTOLPad").done)
		{
			continue;
		}

		let derrNum = countStruct(structures.derrick, me);
		let facNum = countStruct(fac, me);
		let allowedAmount = 0;

		if (derrNum >= 20)
		{
			allowedAmount = 5;
		}
		else if (derrNum >= 16)
		{
			allowedAmount = 4;
		}
		else if (derrNum >= 9)
		{
			allowedAmount = 3;
		}
		else if (derrNum >= 6)
		{
			allowedAmount = 2;
		}
		else
		{
			allowedAmount = MIN_FACTORY_COUNT;
		}

		if (facNum < allowedAmount && facNum < MAX_FACTORY_COUNT && countAndBuild(fac, allowedAmount))
		{
			return true;
		}
	}

	return false;
}

function researchBuildOrder()
{
	if (getMultiTechLevel() === 4)
	{
		return false;
	}
	let labs = countStruct(structures.lab, me);
	let seaMap = turnOffCyborgs || forceHover;
	const MAX_LAB_COUNT = 5;

	if (!researchComplete && labs < MAX_LAB_COUNT)
	{
		let amount = 3;
		let derrCount = countStruct(structures.derrick, me);

		if (derrCount >= 10)
		{
			amount = 5;
		}
		else if (derrCount >= 6 || seaMap === true)
		{
			amount = 4;
		}

		if (labs < amount && countAndBuild(structures.lab, amount))
		{
			return true;
		}
	}

	return false;
}

//Build minimum requirements of base structures.
function buildBaseStructures2()
{
	if (!countStruct(structures.gen, me))
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
	if (countAndBuild(structures.uplink, 1))
	{
		return true;
	}
	if (countAndBuild(structures.lassat, 1))
	{
		return true;
	}

	return false;
}

//Build the minimum repairs and any vtol pads.
function buildExtras()
{
	let needVtolPads = countStruct(structures.vtolPad, me) < enumGroup(vtolGroup).length;
	if (needVtolPads && buildStuff(structures.vtolPad))
	{
		return true;
	}

	let gens = countStruct(structures.gen, me);
	if (random(100) < 40 && getRealPower() > SUPER_LOW_POWER && countStruct(structures.repair, me) < 5 && countAndBuild(structures.repair, gens + 1))
	{
		return true;
	}

	return false;
}

function buildNTWPhase2()
{
	if (countAndBuild(structures.cyborgFactory, 5))
	{
		return true;
	}

	// Ignore spam building gens early game on true 40 oil maps
	if (needPowerGenerator() && countAndBuild(structures.gen, countStruct(structures.gen, me) + 1))
	{
		return true;
	}

	if (countAndBuild(structures.repair, 5))
	{
		return true;
	}

	return false;
}

function defendNTWMap()
{
	if (!getResearch("R-Struc-Research-Upgrade07").done)
	{
		return false;
	}

	if (random(100) < 5)
	{
		if (random(100) < 50)
		{
			if (countAndBuild("Sys-SensoTowerWS", Infinity))
			{
				return true;
			}
			else if (countAndBuild("Sys-SensoTower02", Infinity))
			{
				return true;
			}
		}
		if (countAndBuild("Sys-CB-Tower01", Infinity))
		{
			return true;
		}
	}

	let randomChoice;

	if (random(100) < 80 && isStructureAvailable("Emplacement-HvART-pit") && countAndBuild("Emplacement-HvART-pit", Infinity))
	{
		return true;
	}
	if (isStructureAvailable("Emplacement-HeavyPlasmaLauncher") && countAndBuild("Emplacement-HeavyPlasmaLauncher", Infinity))
	{
		return true;
	}
	if (!isStructureAvailable("Emplacement-HvART-pit") && random(100) < 33 && countAndBuild("Emplacement-Rocket06-IDF", Infinity))
	{
		return true;
	}

	switch (random(3))
	{
		case 0: randomChoice = weaponStats.howitzers.defenses.reverse(); break;
		case 1: randomChoice = weaponStats.fireMortars.defenses.reverse(); break;
		default: randomChoice = weaponStats.howitzers.defenses.reverse(); break;
	}

	if (random(100) < 20)
	{
		randomChoice = [{stat: "Emplacement-RotHow"}];
	}

	for (let i = 0, len = randomChoice.length; i < len; ++i)
	{
		if (countAndBuild(randomChoice[i].stat, Infinity))
		{
			return true;
		}
	}

	return false;
}

//Cobra's unique build decisions
function buildOrders()
{
	if (currently_dead) { return; }

	let isNTW = highOilMap();
	let skip = false;

	if (findIdleTrucks(constructGroup).length === 0 && (!isNTW || findIdleTrucks(constructGroupNTWExtra).length === 0)) { return; }

	if (checkUnfinishedStructures(constructGroup)) { skip = true; }
	if (isNTW && checkUnfinishedStructures(constructGroupNTWExtra)) { skip = true; }
	if (skip) { return; }

	if (buildBaseStructures()) { return; }

	if (maintenance(constructGroup)) { skip = true; }
	if (isNTW && maintenance(constructGroupNTWExtra)) { skip = true; }
	if (skip) { return; }

	if (isNTW && buildNTWPhase2()) { return; }

	if (isNTW && random(100) < 20 && buildDefenses(undefined, false)) { return; }
	if (random(100) < 70 && buildAAForPersonality()) { return; }
	if (buildExtras()) { return; }
	if (random(100) < 33 && buildSpecialStructures()) { return; }
	if (buildBaseStructures2()) { return; }

	buildDefenses(undefined, false);
}

//Check if a building has modules to be built
function maintenance(group)
{
	if (!countStruct(structures.gen, me) || (countStruct(structures.derrick, me) < 4))
	{
		return false;
	}
	if (!isDefined(group))
	{
		group = constructGroup;
	}

	let isNTW = highOilMap();
	let minModulePower = (getMultiTechLevel() === 1) ? -SUPER_LOW_POWER : -200;

	let modList;
	let struct = null;
	let module = "";
	if (isNTW)
	{
		modList = [
			{"mod": "A0ResearchModule1", "amount": 1, "structure": structures.lab},
			{"mod": "A0PowMod1", "amount": 1, "structure": structures.gen},
			{"mod": "A0FacMod1", "amount": 2, "structure": structures.factory},
			{"mod": "A0FacMod1", "amount": 2, "structure": structures.vtolFactory},
		];
	}
	else
	{
		if (mapOilLevel() === "LOW")
		{
			modList = [
				{"mod": "A0PowMod1", "amount": 1, "structure": structures.gen},
				{"mod": "A0FacMod1", "amount": 1, "structure": structures.factory},
				{"mod": "A0ResearchModule1", "amount": 1, "structure": structures.lab},
				{"mod": "A0FacMod1", "amount": 2, "structure": structures.factory},
				{"mod": "A0FacMod1", "amount": 2, "structure": structures.vtolFactory},
			];
		}
		else
		{
			modList = [
				{"mod": "A0PowMod1", "amount": 1, "structure": structures.gen},
				{"mod": "A0ResearchModule1", "amount": 1, "structure": structures.lab},
				{"mod": "A0FacMod1", "amount": 2, "structure": structures.factory},
				{"mod": "A0FacMod1", "amount": 2, "structure": structures.vtolFactory},
			];
		}
	}

	if (isNTW && (group === constructGroup))
	{
		modList = modList.reverse();
	}

	for (let i = 0, l = modList.length; i < l; ++i)
	{
		let modObj = modList[i];

		if (isStructureAvailable(modObj.mod))
		{
			if (modObj.structure === structures.vtolFactory && !componentAvailable("V-Tol"))
			{
				//Stop wasting power on upgrading VTOL factories if we don't have them
				//researched yet (from some maps).
				continue;
			}

			let structList = enumStruct(me, modObj.structure).sort(distanceToBase);
			if (group === oilGrabberGroup)
			{
				structList = structList.reverse();
			}

			for (let c = 0, s = structList.length; c < s; ++c)
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

	if (((getRealPower() > minModulePower) ||
		(getMultiTechLevel() > 1 && gameTime > 300000) ||
		(module === "A0PowMod1") ||
		(module === modList[0].mod)) &&
		struct &&
		buildStuff(struct, module, undefined, 0, group))
	{
		return true;
	}

	return false;
}
