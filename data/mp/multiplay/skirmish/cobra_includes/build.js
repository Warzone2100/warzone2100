
//Determine if we need a generator.
function needPowerGenerator()
{
	function uncached()
	{
		return ((countStruct(_STRUCTURES.derrick, me) - (countStruct(_STRUCTURES.gen, me) * 4)) > 0);
	}

	return cacheThis(uncached, [], "needPowerGenerator" + me, 8000);
}

function minTruckCount()
{
	function uncached()
	{
		return (highOilMap() ? 3 : 2) * __MIN_TRUCKS_PER_GROUP;
	}

	return cacheThis(uncached, [], "minTruckCount" + me, Infinity);
}

//Determine if this is a constructor droid. Specify and optional second paramter
//to find combat engineers.
function isConstruct(objID, countCybEng)
{
	const _dr = getObject(DROID, me, objID);

	if (_dr === null)
	{
		return false;
	}

	if ((_dr.droidType === DROID_SENSOR) || (_dr.droidType === DROID_REPAIR) || (_dr.droidType === DROID_COMMAND))
	{
		return false;
	}

	if (!isDefined(countCybEng))
	{
		countCybEng = false;
	}

	return ((_dr.droidType === DROID_CONSTRUCT) || (countCybEng && !isDefined(_dr.weapons[0])));
}

//Returns unfinished structures in the form of IDs.
function unfinishedStructures()
{
	const __safeDist = 20;
	const _unfinished = [];
	const _stuff = enumStruct(me).filter((obj) => (
		obj.status !== BUILT && obj.stattype !== RESOURCE_EXTRACTOR
	));

	for (let i = 0, l = _stuff.length; i < l; ++i)
	{
		const _s = _stuff[i];

		if ((_s.stattype === DEFENSE) &&
			(!strangeStartSettingOver() ||
			(!componentAvailable("hover01") ||
			(distBetweenTwoPoints(_MY_BASE.x, _MY_BASE.y, _s.x, _s.y) > __safeDist))))
		{
			continue;
		}

		_unfinished.push(_s.id);
	}

	return _unfinished;
}


//Can a construction droid do something right now.
function conCanHelp(mydroidID, bx, by)
{
	const _myDroid = getObject(DROID, me, mydroidID);

	if (_myDroid === null)
	{
		return false;
	}

	return (_myDroid.order !== DORDER_BUILD &&
		_myDroid.order !== DORDER_HELPBUILD &&
		_myDroid.order !== DORDER_LINEBUILD &&
		_myDroid.order !== DORDER_RECYCLE &&
		!repairDroid(mydroidID) &&
		droidCanReach(_myDroid, bx, by)
	);
}

//Return all idle constructs object IDs for a given group.
function findIdleTrucks(group)
{
	const _builders = enumGroup((!isDefined(group)) ? constructGroup : group);
	const _droidlist = [];

	for (let i = 0, s = _builders.length; i < s; ++i)
	{
		const _con = _builders[i];

		if (conCanHelp(_con.id, _con.x, _con.y))
		{
			_droidlist.push(_con.id);
		}
	}

	return _droidlist;
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

// Return a hopefully good enough set of coordinates for where to build repair facilities.
function findRepairClusterLocation()
{
	function uncached()
	{
		const _repairStr = enumStruct(me, REPAIR_FACILITY);

		if (!_repairStr.length)
		{
			return _MY_BASE;
		}

		const _avgCoords = {x: 0, y: 0};
		let total = 0;

		_repairStr.forEach((r) => {
			_avgCoords.x += r.x;
			_avgCoords.y += r.y;
			++total;
		});

		_avgCoords.x = _avgCoords.x / total;
		_avgCoords.y = _avgCoords.y / total;

		return _avgCoords;
	}

	return cacheThis(uncached, [], "findRepairClusterLocation" + me, 10000);
}

// Use this to build a defense next to a derrick (that was taken before we got to build there)
// This can be called from eventStructureBuilt() to build a few defenses with a chance.
function fastDefendSpot(structure, droid)
{
	if (!droid)
	{
		return;
	}

	const __minDefenses = (gameTime < 900000) ? 1 : 2;

	if (structure && (structure.stattype === FACTORY ||
		structure.stattype === CYBORG_FACTORY ||
		structure.stattype === VTOL_FACTORY ||
		structure.stattype === POWER_GEN ||
		structure.stattype === RESEARCH_LAB ||
		structure.stattype === HQ))
	{
		return; //do not waste time trying to defend basic base structures.
	}

	const __rndChance = (gameTime > 900000 && ((structure && structure.stattype === RESOURCE_EXTRACTOR) || chance(20)));
	const _structs = enumRange(droid.x, droid.y, 7, me, false).filter((obj) => (obj.type === STRUCTURE));
	const _defenses = _structs.filter((obj) => (obj.stattype === DEFENSE));
	const _enemyDerrLen = enumRange(droid.x, droid.y, 8, ENEMIES, false).filter((obj) => (obj.type === STRUCTURE && obj.stattype === RESOURCE_EXTRACTOR)).length;
	//Build a defense structure here.
	if (__rndChance || (_defenses.length < __minDefenses) || (_enemyDerrLen && !_defenses.length))
	{
		buildDefenses(droid, true); // Build right where this droid is at.
		return;
	}

	//Try a sensor tower
	if (gameTime > 900000 && chance(67) && _structs.length < 5)
	{
		let sensor;
		const __towers = [ "Sys-SensoTowerWS", "Sys-SensoTower02" ];

		for (let i = 0, len = __towers.length; i < len; ++i)
		{
			const __sen = __towers[i];

			if (isStructureAvailable(__sen))
			{
				sensor = __sen;
				break;
			}
		}
		if (isDefined(sensor))
		{
			const _result = pickStructLocation(droid, sensor, droid.x, droid.y, 1);

			if (_result)
			{
				orderDroidBuild(droid, DORDER_BUILD, sensor, _result.x, _result.y);
			}
		}
	}
}

//Find the closest derrick that is not guarded a defense.
function protectUnguardedDerricks(droid)
{
	if (!droid && (gameTime < 440000) && (getMultiTechLevel() === 1) && (baseType === CAMP_CLEAN) && (getRealPower() < 100))
	{
		return;
	}

	const _derrs = enumStruct(me, _STRUCTURES.derrick);
	const __len = _derrs.length;
	const __maxBlocking = 8;
	const __highOil = highOilMap();

	if (droid)
	{
		if (buildStructure(droid, returnDefense(), droid, __maxBlocking))
		{
			return true;
		}

		return false;
	}

	if (__len)
	{
		let undefended = [];

		for (let i = 0; i < __len; ++i)
		{
			let found = false;
			const _objects = enumRange(_derrs[i].x, _derrs[i].y, (__highOil) ? 4 : 8, me, false);

			for (let c = 0, u = _objects.length; c < u; ++c)
			{
				if ((_objects[c].type === STRUCTURE) && (_objects[c].stattype === DEFENSE))
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				undefended.push(_derrs[i]);
			}
		}

		if (undefended.length)
		{
			//Don't defend it ATM if that derrick is surrounded by enemies.
			if (!__highOil)
			{
				undefended = undefended.filter((obj) => (
					(gameTime < 600000 && distBetweenTwoPoints(obj.x, obj.y, _MY_BASE.x, _MY_BASE.y) > 9) ||
						!enumRange(obj.x, obj.y, 6, ENEMIES, false).length
				)).sort(distanceToBase).reverse();
			}
			else
			{
				const _baseArea = cobraBaseArea();
				// Try to avoid building defenses at the base, as it may trap units with lines of resources.
				undefended = undefended.filter((obj) => (
					!(((obj.x >= _baseArea.x1) && (obj.x <= _baseArea.x2)) && ((obj.y >= _baseArea.y1) && (obj.y <= _baseArea.y2)))
				)).sort(distanceToBase).reverse();
			}

			if (undefended.length && buildStuff(returnDefense(), undefined, undefended[0], __maxBlocking, oilGrabberGroup))
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

	if (droid)
	{
		let loc;

		if (isDefined(defendThis))
		{
			loc = pickStructLocation(droid, stat, defendThis.x, defendThis.y, blocking);
		}
		else
		{
			const _tempBasePos = (stat === _STRUCTURES.repair) ? findRepairClusterLocation() : randomOffsetLocation(_MY_BASE);
			loc = pickStructLocation(droid, stat, _tempBasePos.x, _tempBasePos.y, blocking);
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

	const _freeTrucks = findIdleTrucks(isDefined(group) ? group : undefined);

	if (_freeTrucks.length)
	{
		const _truck = getObject(DROID, me, _freeTrucks[0]);

		if (_truck === null)
		{
			return false;
		}

		if (isDefined(module))
		{
			if (orderDroidBuild(_truck, DORDER_BUILD, module, struc.x, struc.y))
			{
				return true;
			}
		}

		if (isDefined(defendThis))
		{
			if (buildStructure(_truck, struc, defendThis, blocking))
			{
				return true;
			}
		}
		else
		{
			if (buildStructure(_truck, struc, undefined, blocking))
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

	const _structs = unfinishedStructures();

	for (let i = 0, len = _structs.length; i < len; ++i)
	{
		const _structure = getObject(STRUCTURE, me, _structs[i]);

		if (_structure === null)
		{
			return false;
		}

		const _trucks = findIdleTrucks(group);

		for (let j = 0, len2 = _trucks.length; j < len2; ++j)
		{
			const _truck = getObject(DROID, me, _trucks[j]);

			if (_truck === null)
			{
				return false;
			}

			if (orderDroidObj(_truck, DORDER_HELPBUILD, _structure))
			{
				return true;
			}
		}
	}

	return false;
}

function holdAllOilTrucks()
{
	const _oilGrabbers = enumGroup(oilGrabberGroup);

	for (let i = 0, len = _oilGrabbers.length; i < len; ++i)
	{
		if (_oilGrabbers[i].order !== DORDER_RECYCLE &&
			_oilGrabbers[i].order !== DORDER_HOLD)
		{
			orderDroid(_oilGrabbers[i], DORDER_HOLD);
		}
	}
}

function skipOilGrabIfEasy()
{
	if (difficulty === EASY)
	{
		const __myDerrickCount = enumStruct(me, _STRUCTURES.derrick).filter((obj) => (
			obj.status === BUILT
		)).length;
		const _enemies = findLivingEnemies();

		for (let i = 0, len = _enemies.length; i < len; ++i)
		{
			if (__myDerrickCount >= 5 && __myDerrickCount >= countStruct(_STRUCTURES.derrick, _enemies[i]))
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

	const _droids = enumGroup(oilGrabberGroup);
	const _oils = enumFeature(ALL_PLAYERS, __OIL_RES).sort(distanceToBase);

	if (!forceDerrickBuildDefense && (_oils.length < 2))
	{
		//Ok, most oils are already owned so go ahead and defend all derricks from now on
		forceDerrickBuildDefense = true;
	}

	if (forceDerrickBuildDefense && chance(15))
	{
		protectUnguardedDerricks();
	}

	if ((!_oils.length && highOilMap() && maintenance(oilGrabberGroup)) || skipOilGrabIfEasy())
	{
		return;
	}

	for (let i = 0, oilLen = _oils.length; i < oilLen; ++i)
	{
		let bestDroid;
		let bestDist = Infinity;
		const _oil = _oils[i];

		for (let j = 0, drLen = _droids.length; j < drLen; ++j)
		{
			const _droid = _droids[j];
			const __dist = distBetweenTwoPoints(_droid.x, _droid.y, _oil.x, _oil.y);
			const __unsafeLen = enumRange(_oil.x, _oil.y, 6, ENEMIES, false).filter(isUnsafeEnemyObject).length;

			if (!__unsafeLen && (bestDist > __dist) && conCanHelp(_droid.id, _oil.x, _oil.y))
			{
				bestDroid = _droid;
				bestDist = __dist;
			}
		}

		if (bestDroid && !stopExecution("oil" + _oil.y * mapWidth * _oil.x, 50000))
		{
			orderDroidBuild(bestDroid, DORDER_BUILD, _STRUCTURES.derrick, _oil.x, _oil.y);
			return;
		}
	}
}

//Builds an AA site for the personality. It will always use stormbringer AA
//once available.
function buildAAForPersonality()
{
	const __vtolCount = countEnemyVTOL();
	let minAAs = Math.floor(__vtolCount / 2);
	const __high = highOilMap();

	if (!__high && (getRealPower() >= __MIN_BUILD_POWER))
	{
		minAAs = __vtolCount;
	}
	else if (__high && (getRealPower() >= __SUPER_LOW_POWER))
	{
		minAAs = __vtolCount * 4;
	}

	//Use stormbringer if we have it.
	if (countAndBuild("P0-AASite-Laser", minAAs))
	{
		return true;
	}
	else
	{
		const _aaType = subPersonalities[personality].antiAir.defenses;

		for (let i = _aaType.length - 1; i >= 0; --i)
		{
			if (countAndBuild(_aaType[i].stat, minAAs))
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

	const __electronicChance = 67;
	const _standardDefenses = subPersonalities[personality].primaryWeapon.defenses;
	const _artilleryDefenses = subPersonalities[personality].artillery.defenses;
	let defenses = ((type === 0) || !strangeStartSettingOver()) ? _artilleryDefenses.concat(_standardDefenses) : _standardDefenses.concat(_artilleryDefenses);
	let bestDefense;

	//Choose a random electronic warfare defense if possible.
	if (chance(__electronicChance))
	{
		let avail = 0;

		for (let i = 0, t = _ELECTRONIC_DEFENSES.length; i < t; ++i)
		{
			if (isStructureAvailable(_ELECTRONIC_DEFENSES[i]))
			{
				avail += 1;
			}
		}

		if (avail > 0)
		{
			defenses = [];
			defenses.push(_ELECTRONIC_DEFENSES[random(avail)]);
		}
	}

	for (let i = defenses.length - 1; i > -1; --i)
	{
		const __def = isDefined(defenses[i].stat);

		if (__def && isStructureAvailable(defenses[i].stat))
		{
			bestDefense = defenses[i].stat;
			break;
		}
		else if (!__def && isStructureAvailable(defenses[i]))
		{
			bestDefense = defenses[i];
			break;
		}
	}

	if (!isDefined(bestDefense))
	{
		if (isStructureAvailable("GuardTower1"))
		{
			if (!resObj.forceDefenseRes && (gameTime > 600000))
			{
				resObj.forceDefenseRes = true;
			}
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

	const __defense = returnDefense(type);

	if (isDefined(__defense))
	{
		const __maxBlocking = 8;
		const _tempTruckPos = randomOffsetLocation({x: truck.x, y: truck.y});
		const _result = pickStructLocation(truck, __defense, _tempTruckPos.x, _tempTruckPos.y, __maxBlocking);

		if (_result)
		{
			return orderDroidBuild(truck, DORDER_BUILD, __defense, _result.x, _result.y);
		}
	}

	return false;
}

//Defend a random derrick with a defense. Does NOT include those close to base.
function defendRandomDerrick()
{
	const __maxDist = 30;
	const _derrs = enumStruct(me, _STRUCTURES.derrick).filter((obj) => (
		distBetweenTwoPoints(_MY_BASE.x, _MY_BASE.y, obj.x, obj.y) > __maxDist
	));

	if (_derrs.length)
	{
		const __maxBlocking = 0;
		const _derr = _derrs[random(_derrs.length)];

		if (buildStuff(returnDefense(), undefined, _derr, __maxBlocking, oilGrabberGroup))
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
	const __isDefensive = (subPersonalities[personality].defensePriority >= 50 || subPersonalities[personality].resPath === "defensive");
	const __pow = getRealPower();
	const __enoughPower = (__pow > __SUPER_LOW_POWER || (__isDefensive && (__pow > __SUPER_LOW_POWER - 25)));

	if (!isDefined(urgent))
	{
		urgent = false;
	}

	if (urgent || __enoughPower)
	{
		if (truck)
		{
			return buildDefenseNearTruck(truck, 0);
		}

		if ((getMultiTechLevel() === 1) && (countStruct(_STRUCTURES.derrick, me) <= 8) && (gameTime < 600000))
		{
			return false; // Avoid defending base early on in T1 if derrick count is low.
		}

		return ((highOilMap() && chance(25) && !defendNTWMap()) ||
			(chance(80) && defendRandomDerrick()) ||
			buildStuff(returnDefense(), undefined, undefined, 0, undefined));
	}

	return false;
}

//Build the basics when available. Has a different build order if NTW.
function buildBaseStructures()
{
	const __goodPowerLevel = getRealPower() > 250;

	if ((getMultiTechLevel() < 4) && forceHover && __goodPowerLevel && countAndBuild(_STRUCTURES.lab, 1))
	{
		return true;
	}

	if (!highOilMap())
	{
		if ((__goodPowerLevel && countAndBuild(_STRUCTURES.factory, 1)) ||
			((!__goodPowerLevel || (getMultiTechLevel() > 1)) && countAndBuild(_STRUCTURES.gen, 1)) ||
			(countAndBuild(_STRUCTURES.factory, 2)) ||
			((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(_STRUCTURES.lab, 2)) ||
			(countAndBuild(_STRUCTURES.gen, 1)) ||
			(countAndBuild(_STRUCTURES.hq, 1)) ||
			(countAndBuild(_STRUCTURES.gen, 2)) ||
			((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(_STRUCTURES.lab, 3)) ||
			(needPowerGenerator() && countAndBuild(_STRUCTURES.gen, countStruct(_STRUCTURES.gen, me) + 1)) ||
			// Micro optimization on low oil no bases so Cobra builds power modules before the first cyborg factory.
			((countStruct(_STRUCTURES.gen, me) <= 3) && (gameTime < 900000) && maintenance(undefined, true)) ||
			(isStructureAvailable("A0PowMod1") && countAndBuild(_STRUCTURES.cyborgFactory, 1)) ||
			(countAndBuild(_STRUCTURES.repair, 1)) ||
			((countStruct(_STRUCTURES.derrick, me) >= 6) && countAndBuild(_STRUCTURES.vtolFactory, 1)))
		{
			return true;
		}
	}
	else
	{
		const __haveAllies = ((alliancesType === ALLIANCES_TEAMS) && playerAlliance(true).length);

		if (((!__goodPowerLevel || (getMultiTechLevel() > 1)) && countAndBuild(_STRUCTURES.gen, 1)) ||
			(getRealPower() < 550 && countAndBuild(_STRUCTURES.gen, 4)) ||
			((getMultiTechLevel() < 4) && __goodPowerLevel && randomResearchLabStart && !researchComplete && countAndBuild(_STRUCTURES.lab, 1)) ||
			(countAndBuild(_STRUCTURES.factory, 2)) ||
			((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(_STRUCTURES.lab, (__haveAllies) ? 2 : 4)) ||
			(countAndBuild(_STRUCTURES.gen, 1)) || //a little fail-safe
			(countAndBuild(_STRUCTURES.hq, 1)) ||
			(countAndBuild(_STRUCTURES.factory, 3)) ||
			((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(_STRUCTURES.lab, 5)) ||
			(needPowerGenerator() && countAndBuild(_STRUCTURES.gen, 7)) ||
			(countAndBuild(_STRUCTURES.cyborgFactory, 3)) ||
			(countAndBuild(_STRUCTURES.factory, 5)) ||
			(needPowerGenerator() && countAndBuild(_STRUCTURES.gen, 8)) ||
			(countAndBuild(_STRUCTURES.cyborgFactory, 5)) ||
			(__goodPowerLevel && countAndBuild(_STRUCTURES.repair, 3)))
		{
			return true;
		}
	}

	if ((getMultiTechLevel() > 1) && countStruct(_STRUCTURES.vtolFactory, me) && countAndBuild(_STRUCTURES.vtolPad, 3))
	{
		return true;
	}

	return false;
}

//Build factories.
function factoryBuildOrder()
{
	const __minFactoryCount = 1;
	const __maxFactoryCount = 5;

	for (let i = 0; i < subPersonalities[personality].factoryOrder.length; ++i)
	{
		const _fac = subPersonalities[personality].factoryOrder[i];

		if ((_fac === _STRUCTURES.vtolFactory && (!useVtol || !getResearch("R-Struc-VTOLPad").done)) ||
			(_fac === _STRUCTURES.cyborgFactory && (turnOffCyborgs || forceHover || !isStructureAvailable("A0PowMod1"))))
		{
			continue;
		}

		const __derrNum = countStruct(_STRUCTURES.derrick, me);
		const __facNum = countStruct(_fac, me);
		let allowedAmount = 0;

		if (__derrNum >= 20)
		{
			allowedAmount = 5;
		}
		else if (__derrNum >= 16)
		{
			allowedAmount = 4;
		}
		else if (__derrNum >= 9)
		{
			allowedAmount = 3;
		}
		else if (__derrNum >= 6)
		{
			allowedAmount = 2;
		}
		else
		{
			allowedAmount = __minFactoryCount;
		}

		if (__facNum < allowedAmount && __facNum < __maxFactoryCount && countAndBuild(_fac, allowedAmount))
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

	const __labs = countStruct(_STRUCTURES.lab, me);
	const __seaMap = (turnOffCyborgs || forceHover);
	const __maxLabCount = 5;

	if (!researchComplete && __labs < __maxLabCount)
	{
		let amount = 3;
		const __derrCount = countStruct(_STRUCTURES.derrick, me);

		if (__derrCount >= 10)
		{
			amount = 5;
		}
		else if ((__derrCount >= 6) || __seaMap)
		{
			amount = 4;
		}

		if (__labs < amount && countAndBuild(_STRUCTURES.lab, amount))
		{
			return true;
		}
	}

	return false;
}

//Build minimum requirements of base structures.
function buildBaseStructures2()
{
	return (!countStruct(_STRUCTURES.gen, me) || researchBuildOrder() || factoryBuildOrder());
}

//Laser satellite/uplink center
function buildSpecialStructures()
{
	return (countAndBuild(_STRUCTURES.uplink, 1) || countAndBuild(_STRUCTURES.lassat, 1));
}

//Build the minimum repairs and any vtol pads.
function buildExtras()
{
	const __maxBlocking = 8;
	const __needVtolPads = (countStruct(_STRUCTURES.vtolPad, me) < enumGroup(vtolGroup).length);

	if (__needVtolPads && buildStuff(_STRUCTURES.vtolPad, undefined, undefined, __maxBlocking, undefined))
	{
		return true;
	}

	const __gens = countStruct(_STRUCTURES.gen, me);

	if (chance(40) && getRealPower() > __SUPER_LOW_POWER && countStruct(_STRUCTURES.repair, me) < 5 && countAndBuild(_STRUCTURES.repair, __gens + 1))
	{
		return true;
	}

	return false;
}

function buildNTWPhase2()
{
	return (countAndBuild(_STRUCTURES.cyborgFactory, 5) ||
		// Ignore spam building gens early game on true 40 oil maps
		(needPowerGenerator() && countAndBuild(_STRUCTURES.gen, countStruct(_STRUCTURES.gen, me) + 1)) ||
		countAndBuild(_STRUCTURES.repair, 5));
}

function defendNTWMap()
{
	if (!getResearch("R-Struc-Research-Upgrade05").done)
	{
		return false;
	}

	if (chance(10))
	{
		if (chance(50))
		{
			if (countAndBuild("Sys-SensoTowerWS", Infinity) || countAndBuild("Sys-SensoTower02", Infinity))
			{
				return true;
			}
		}
		if (countAndBuild("Sys-CB-Tower01", Infinity))
		{
			return true;
		}
	}

	if ((chance(80) && isStructureAvailable("Emplacement-HvART-pit") && countAndBuild("Emplacement-HvART-pit", Infinity)) ||
		(isStructureAvailable("Emplacement-HeavyPlasmaLauncher") && countAndBuild("Emplacement-HeavyPlasmaLauncher", Infinity)) ||
		(!isStructureAvailable("Emplacement-HvART-pit") && chance(33) && countAndBuild("Emplacement-Rocket06-IDF", Infinity)))
	{
		return true;
	}

	let randomChoice;

	switch (random(3))
	{
		case 0: randomChoice = _WEAPON_STATS.howitzers.defenses.reverse(); break;
		case 1: randomChoice = _WEAPON_STATS.fireMortars.defenses.reverse(); break;
		default: randomChoice = _WEAPON_STATS.howitzers.defenses.reverse(); break;
	}

	if (chance(20))
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

function strangeBuildOrder()
{
	if (!noBasesHighTechStart)
	{
		return false;
	}

	const __isNTW = highOilMap();

	if (!__isNTW)
	{
		if ((countAndBuild(_STRUCTURES.factory, 2)) ||
			(countAndBuild(_STRUCTURES.gen, 2)) ||
			(countAndBuild(_STRUCTURES.hq, 1)) || // helps artillery hit stuff near the base
			((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(_STRUCTURES.lab, 1)) ||
			(needPowerGenerator() && countAndBuild(_STRUCTURES.gen, countStruct(_STRUCTURES.gen, me) + 1)) ||
			(maintenance()) ||
			(countAndBuild(_STRUCTURES.cyborgFactory, 1)) ||
			((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(_STRUCTURES.lab, 2)))
		{
			return true;
		}
	}
	else
	{
		if ((countAndBuild(_STRUCTURES.factory, 2)) ||
			(countAndBuild(_STRUCTURES.gen, 3)) ||
			((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(_STRUCTURES.lab, 2)) ||
			(needPowerGenerator() && countAndBuild(_STRUCTURES.gen, 7)) ||
			(countAndBuild(_STRUCTURES.cyborgFactory, 1)) ||
			(countAndBuild(_STRUCTURES.hq, 1)) || // helps artillery hit stuff near the base
			((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(_STRUCTURES.lab, 3)) ||
			(maintenance(constructGroup) || maintenance(constructGroupNTWExtra)) ||
			(countAndBuild(_STRUCTURES.factory, 4)) ||
			(countAndBuild(_STRUCTURES.cyborgFactory, 5)) ||
			(needPowerGenerator() && countAndBuild(_STRUCTURES.gen, 9)) ||
			((getMultiTechLevel() < 4) && !researchComplete && countAndBuild(_STRUCTURES.lab, 5)) ||
			(countAndBuild(_STRUCTURES.factory, 5)))
		{
			return true;
		}
	}

	return false;
}

//Cobra's unique build decisions
function buildOrders()
{
	const __isNTW = highOilMap();
	const __superDefense = (subPersonalities[personality].defensePriority >= 75);

	// No idle truck or dead.
	if (currently_dead || (!findIdleTrucks(constructGroup).length && (!__isNTW || !findIdleTrucks(constructGroupNTWExtra).length)))
	{
		return;
	}

	// Check unfinished structures.
	if ((checkUnfinishedStructures(constructGroup) || (__isNTW && checkUnfinishedStructures(constructGroupNTWExtra))) ||
		// You just know what this means.
		(strangeBuildOrder()) ||
		// Build basic base structures in order.
		buildBaseStructures() ||
		// Build the modules.
		(maintenance(constructGroup) || (__isNTW && maintenance(constructGroupNTWExtra))) ||
		// Build further NTW based structure order.
		(__isNTW && buildNTWPhase2()) ||
		// Final build priority. Defenses and special structures. Max limits for base structures.
		((__superDefense && (gameTime > 300000) && chance(20) && buildDefenses(undefined, false))) ||
		(chance(50) && buildAAForPersonality()) ||
		(chance(50) && buildExtras()) ||
		(chance(33) && buildSpecialStructures()) ||
		buildBaseStructures2())
	{
		return;
	}

	// Spam defenses without any else to do.
	buildDefenses(undefined, false);
}

//Check if a building has modules to be built
function maintenance(group, earlyGen)
{
	if (!countStruct(_STRUCTURES.gen, me) || (strangeStartSettingOver() && (countStruct(_STRUCTURES.derrick, me) < 4)))
	{
		return false;
	}

	if (!isDefined(group))
	{
		group = constructGroup;
	}

	const __isNTW = highOilMap();
	const __minModulePower = ((!__isNTW && (getMultiTechLevel() === 1)) ? -__SUPER_LOW_POWER : -200);
	let modList;
	let struct = null;
	let module = "";

	if (__isNTW)
	{
		modList = [
			{"mod": "A0PowMod1", "amount": (noBasesHighTechStart ? 1 : 0), "structure": _STRUCTURES.gen},
			{"mod": "A0ResearchModule1", "amount": 1, "structure": _STRUCTURES.lab},
			{"mod": "A0PowMod1", "amount": 1, "structure": _STRUCTURES.gen},
			{"mod": "A0FacMod1", "amount": 2, "structure": _STRUCTURES.factory},
			{"mod": "A0FacMod1", "amount": 2, "structure": _STRUCTURES.vtolFactory},
		];
	}
	else
	{
		if (isDefined(earlyGen))
		{
			modList = [
				{"mod": "A0PowMod1", "amount": 1, "structure": _STRUCTURES.gen},
			];
		}
		else if (mapOilLevel() === "LOW")
		{
			modList = [
				{"mod": "A0PowMod1", "amount": 1, "structure": _STRUCTURES.gen},
				{"mod": "A0FacMod1", "amount": 1, "structure": _STRUCTURES.factory},
				{"mod": "A0ResearchModule1", "amount": 1, "structure": _STRUCTURES.lab},
				{"mod": "A0FacMod1", "amount": 2, "structure": _STRUCTURES.factory},
				{"mod": "A0FacMod1", "amount": 2, "structure": _STRUCTURES.vtolFactory},
			];
		}
		else
		{
			modList = [
				{"mod": "A0PowMod1", "amount": 1, "structure": _STRUCTURES.gen},
				{"mod": "A0ResearchModule1", "amount": 1, "structure": _STRUCTURES.lab},
				{"mod": "A0FacMod1", "amount": 2, "structure": _STRUCTURES.factory},
				{"mod": "A0FacMod1", "amount": 2, "structure": _STRUCTURES.vtolFactory},
			];
		}
	}

	if (__isNTW && (group === constructGroup))
	{
		modList = modList.reverse();
	}

	for (let i = 0, l = modList.length; i < l; ++i)
	{
		const _modObj = modList[i];

		if (isStructureAvailable(_modObj.mod))
		{
			if (_modObj.structure === _STRUCTURES.vtolFactory && !componentAvailable("V-Tol"))
			{
				//Stop wasting power on upgrading VTOL factories if we don't have them
				//researched yet (from some maps).
				continue;
			}

			let structList = enumStruct(me, _modObj.structure).sort(distanceToBase);

			if (group === oilGrabberGroup)
			{
				structList = structList.reverse();
			}

			for (let c = 0, s = structList.length; c < s; ++c)
			{
				if (structList[c].modules < _modObj.amount)
				{
					struct = structList[c];
					module = _modObj.mod;
					break;
				}
			}

			if (struct !== null)
			{
				break;
			}
		}
	}

	if (((getRealPower() > __minModulePower) ||
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
