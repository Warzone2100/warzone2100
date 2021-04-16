
const standardDefenses = [
	"WallTower-Atmiss",
	"WallTower-HvATrocket",
	"WallTower06",
	"Emplacement-MRL-pit",
	"GuardTower6",
	"GuardTower1",
];
const artilleryDefenses = [
	"Emplacement-HvART-pit",
	"Emplacement-MdART-pit",
	"Emplacement-Rocket06-IDF",
	"Emplacement-MRL-pit",
];

// If positive, there are oil derricks that unused due to lack of power generators.
// If negative, we have too many power generator (usually not a problem in itself).
function numUnusedDerricks()
{
	return countStruct(DERRICK_STAT) - countStruct(POW_GEN_STAT) * 4;
}

function conCanHelp(mydroid, bx, by)
{
	return (mydroid.order !== DORDER_BUILD &&
		mydroid.order !== DORDER_LINEBUILD &&
		mydroid.order !== DORDER_RECYCLE &&
		mydroid.order !== DORDER_DEMOLISH &&
		!droidNeedsRepair(mydroid.id, 80) &&
		droidCanReach(mydroid, bx, by)
	);
}

//Return all trucks that are not doing anything.
function findIdleTrucks(obj)
{
	const builders = enumGroup(baseBuilders);
	const droidlist = [];
	if (!defined(obj))
	{
		obj = BASE;
	}

	for (let i = 0, d = builders.length; i < d; ++i)
	{
		if (conCanHelp(builders[i], obj.x, obj.y))
		{
			droidlist.push(builders[i]);
		}
	}

	return droidlist;
}

// Demolish object.
function demolishThis(object)
{
	let success = false;
	const droidList = findIdleTrucks(object);

	for (let i = 0, d = droidList.length; i < d; ++i)
	{
		if (orderDroidObj(droidList[i], DORDER_DEMOLISH, object))
		{
			success = true;
		}
	}

	return success;
}

// Build something. MaxBlockingTiles is optional.
function grabTrucksAndBuild(structure, maxBlockingTiles)
{
	if (!isStructureAvailable(structure))
	{
		return false;
	}

	if (!defined(maxBlockingTiles))
	{
		maxBlockingTiles = 1;
	}

	const droidList = findIdleTrucks();
	let found = false;

	for (let i = 0, d = droidList.length; i < d; ++i)
	{
		const result = pickStructLocation(droidList[i], structure, BASE.x, BASE.y, maxBlockingTiles);
		if (result)
		{
			//logObj(mydroid, "Construction work");
			if (orderDroidBuild(droidList[i], DORDER_BUILD, structure, result.x, result.y))
			{
				found = true;
			}
		}
	}

	return found;
}

// Help finish building some object that is close to base.
function checkLocalJobs()
{
	const trucks = findIdleTrucks();
	const freeTrucks = trucks.length;
	let success = false;
	let structlist = enumStruct(me).filter(function(obj) {
		return (obj.status !== BUILT &&
			obj.stattype !== RESOURCE_EXTRACTOR &&
			obj.stattype !== DEFENSE &&
			distBetweenTwoPoints(BASE.x, BASE.y, obj.x, obj.y) < HELP_CONSTRUCT_AREA
		);
	});

	if (freeTrucks && structlist.length)
	{
		structlist = structlist.sort(sortByDistToBase);
		for (let j = 0; j < freeTrucks; ++j)
		{
			if (orderDroidObj(trucks[j], DORDER_HELPBUILD, structlist[0]))
			{
				//logObj(trucks[j], "Go help construction");
				success = true;
			}
		}
	}

	return success;
}

// Use this to build a defense next to a derrick (that was taken before we got to build there)
// This can be called from eventStructureBuilt() to build a few defenses with a chance.
function scanAndDefendPosition(structure, droid)
{
	if (!droid)
	{
		return;
	}

	const MIN_DEFENSES = 2;
	if (structure && (structure.stattype === FACTORY ||
		structure.stattype === CYBORG_FACTORY ||
		structure.stattype === VTOL_FACTORY ||
		structure.stattype === POWER_GEN ||
		structure.stattype === RESEARCH_LAB ||
		structure.stattype === HQ))
	{
		return; //do not waste time trying to defend basic base structures.
	}

	const chance = ((structure && structure.stattype === RESOURCE_EXTRACTOR) || random(100) < 15);
	const structs = enumRange(droid.x, droid.y, 5, me, false).filter(function (obj) {
		return obj.type === STRUCTURE;
	});
	const defenses = structs.filter(function(obj) {
		return obj.stattype === DEFENSE;
	});
	const enemyDerr = enumRange(droid.x, droid.y, 3, ENEMIES, false).filter(isDerrick);
	//Build a defense structure here.
	if (chance || (defenses.length < MIN_DEFENSES) || (enemyDerr.length > 0 && defenses.length === 0))
	{
		buildDefenses(droid); // Build right where this droid is at.
		return;
	}

	//Try a sensor tower
	if (random(100) < 67 && structs.length < 5)
	{
		let sensor;
		for (let i = SENSOR_TOWERS.length - 1; i > -1; --i)
		{
			const sen = SENSOR_TOWERS[i];
			if (isStructureAvailable(sen))
			{
				sensor = sen;
				break;
			}
		}
		if (defined(sensor))
		{
			const result = pickStructLocation(droid, sensor, droid.x, droid.y, 1);
			if (result)
			{
				orderDroidBuild(droid, DORDER_BUILD, sensor, result.x, result.y);
			}
		}
	}
}

function lookForOil()
{
	const UNSAFE_AREA_RANGE = 7;
	const droids = enumGroup(oilBuilders);
	const oils = enumFeature(-1, OIL_RES_STAT).sort(sortByDistToBase); // grab closer oils first;
	let bestDroid = null;
	let bestDist = 99999;
	let success = false;
	//log("looking for oil... " + oils.length + " available");

	for (let i = 0, oilLen = oils.length; i < oilLen; ++i)
	{
		for (let j = 0, drLen = droids.length; j < drLen; ++j)
		{
			const droid = droids[j];
			const oil = oils[i];
			const dist = distBetweenTwoPoints(droid.x, droid.y, oil.x, oil.y);
			const unsafe = enumRange(oil.x, oil.y, UNSAFE_AREA_RANGE, ENEMIES, false).filter(isUnsafeEnemyObject);
			if (droidCanReach(droid, oil.x, oil.y) &&
				droid.order !== DORDER_BUILD  && // but can snatch from HELPBUILD
				droid.order !== DORDER_LINEBUILD &&
				droid.order !== DORDER_RECYCLE &&
				!droid.busy)
			{
				if (dist < bestDist && unsafe.length === 0)
				{
					bestDroid = droid;
					bestDist = dist;
				}
			}
		}

		if (bestDroid && !ThrottleThis("oil" + oil.y * mapWidth * oil.x, 50000))
		{
			bestDroid.busy = true;
			orderDroidBuild(bestDroid, DORDER_BUILD, DERRICK_STAT, oil.x, oil.y);
			bestDist = 99999;
			bestDroid = null;
			success = true;
		}
	}

	return success;
}

// Build anti air rockets. TODO: demolish obsolete antiair structures.
function buildAntiAir(buildExtras)
{
	if (!defined(buildExtras))
	{
		buildExtras = false;
	}
	const MAX_DEFENSES = countStruct(FACTORY_STAT) * 3;
	const SAM_SITES = ["P0-AASite-SAM2", "P0-AASite-SAM1", "P0-AASite-Sunburst"];
	const antiAirs = enumStruct(me).filter(function(obj) { return obj.canHitAir; }).length;

	if (buildExtras === false && antiAirs > MAX_DEFENSES)
	{
		return false;
	}

	for (let j = 0, s = SAM_SITES.length; j < s; ++j)
	{
		if (grabTrucksAndBuild(SAM_SITES[j], 1))
		{
			return true;
		}
	}

	return false;
}

// type refers to either a hardpoint like structure or an artillery emplacement.
// returns undefined if no structure it can build can be built.
function returnDefense(type)
{
	if (!defined(type))
	{
		type = random(2);
	}

	const ELECTRONIC_CHANCE = 45;
	let defenses;
	let bestDefense;

	if (type === 0 || !isStructureAvailable(artilleryDefenses[artilleryDefenses.length - 1]))
	{
		defenses = standardDefenses;
	}
	else
	{
		defenses = artilleryDefenses;
	}

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

	for (let i = 0, t = defenses.length; i < t; ++i)
	{
		if (isStructureAvailable(defenses[i]))
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
	if (!defined(type))
	{
		type = 0;
	}

	const defense = returnDefense(type);

	if (defined(defense))
	{
		const result = pickStructLocation(truck, defense, truck.x, truck.y, 1);
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
	if (defined(truck))
	{
		return buildDefenseNearTruck(truck, 0);
	}

	if (gameTime > 210000 && getRealPower() > MIN_BUILD_POWER)
	{
		const def = returnDefense();
		if (defined(def))
		{
			return grabTrucksAndBuild(def, 0);
		}
	}

	return false;
}

// If we need power generators, try to queue up production of them with any idle trucks
function buildPowerGen()
{
	return ((!countStruct(POW_GEN_STAT) || (numUnusedDerricks() > 0)) && grabTrucksAndBuild(POW_GEN_STAT, 0));
}

//WzScript semperfi swaps the roles of all the trucks to be oil grabbers after building one power generator.
function changeTruckRoleOnce()
{
	if (truckRoleSwapped === true)
	{
		return false;
	}

	const completeGen = enumStruct(me, POW_GEN_STAT).filter(function(obj) {
		return obj.status === BUILT;
	});

	if (completeGen.length > 0)
	{
		enumDroid(me).forEach(function(droid) {
			if (droid.droidType === DROID_CONSTRUCT && droid.group === baseBuilders)
			{
				orderDroid(droid, DORDER_STOP);
				eventDroidBuilt(droid, null);
			}
		});
		truckRoleSwapped = true;
		return true;
	}

	return false;
}

// Basic base design so as to survive in a no bases match.
function buildBasicBase()
{
	if (countStruct(FACTORY_STAT) < 2 && grabTrucksAndBuild(FACTORY_STAT, 0))
	{
		return true;
	}

	//If we start on T2/T3 no bases then build a few power generators early.
	if (getMultiTechLevel() > 1 && baseType == CAMP_CLEAN && countStruct(POW_GEN_STAT) < 2 && grabTrucksAndBuild(POW_GEN_STAT, 0))
	{
		return true;
	}

	if (!researchDone && countStruct(RES_LAB_STAT) < 2 && grabTrucksAndBuild(RES_LAB_STAT, 0))
	{
		return true;
	}

	if (countStruct(POW_GEN_STAT) < 2 && grabTrucksAndBuild(POW_GEN_STAT, 0))
	{
		return true;
	}

	// Build HQ if missing
	if (countStruct(PLAYER_HQ_STAT) === 0 && grabTrucksAndBuild(PLAYER_HQ_STAT, 0))
	{
		return true;
	}

	if (!researchDone && countStruct(RES_LAB_STAT) < 3 && grabTrucksAndBuild(RES_LAB_STAT, 0))
	{
		return true;
	}

	if (buildPowerGen())
	{
		return true;
	}

	return false;
}

//Build factories. Attempts to build at least 1 of each factory.
function factoryBuildOrder()
{
	const FAC_ORDER = [FACTORY_STAT, VTOL_FACTORY_STAT, CYBORG_FACTORY_STAT,];
	for (let x = 0; x < 2; ++x)
	{
		let num = 1;
		if (x > 0)
		{
			const derrNum = countStruct(DERRICK_STAT);
			if (derrNum >= 20)
			{
				num = 5;
			}
			else if (derrNum >= 16)
			{
				num = 4;
			}
			else if (derrNum >= 12)
			{
				num = 3;
			}
			else if (derrNum >= 8)
			{
				num = 2;
			}
		}

		for (let i = 0; i < 3; ++i)
		{
			const fac = FAC_ORDER[i];
			if (!(fac === CYBORG_FACTORY_STAT && isSeaMap) && countStruct(fac) < num && grabTrucksAndBuild(fac, 0))
			{
				return true;
			}
		}
	}

	return false;
}

// Decide when to build the last few research labs.
function buildResearchLabs()
{
	if (researchDone)
	{
		return false;
	}

	const resCount = countStruct(RES_LAB_STAT);
	if (resCount < 5)
	{
		let amount = 3;
		const derrCount = countStruct(DERRICK_STAT);
		if (derrCount >= 12)
		{
			amount = 5;
		}
		else if (derrCount >= 7)
		{
			amount = 4;
		}
		if (resCount < amount && grabTrucksAndBuild(RES_LAB_STAT, 0))
		{
			return true;
		}
	}

	return false;
}

function buildFundamentals()
{
	//log("build fundamentals");

	// Help build unfinished buildings
	if (checkLocalJobs())
	{
		return;
	}

	if (buildBasicBase())
	{
		return;
	}

	if (changeTruckRoleOnce())
	{
		return;
	}

	if (maintenance())
	{
		return;
	}

	buildFundamentals2(); // go on to the next level
}

function buildFundamentals2()
{
	//log("build fundamentals2");
	if (buildResearchLabs())
	{
		return;
	}

	if (countStruct(REPAIR_FACILITY_STAT) < countStruct(FACTORY_STAT) && grabTrucksAndBuild(REPAIR_FACILITY_STAT, 0))
	{
		return;
	}

	//Build VTOL pads if needed
	const needVtolPads = 2 * countStruct(VTOL_PAD_STAT) < groupSizes[vtolGroup];
	if (needVtolPads && grabTrucksAndBuild(VTOL_PAD_STAT, 2))
	{
		return;
	}

	if (countStruct(UPLINK_STAT) === 0 && grabTrucksAndBuild(UPLINK_STAT, 0))
	{
		return;
	}

	//build minimum anti-air defenses in base
	if (buildAntiAir(false))
	{
		return;
	}

	if (factoryBuildOrder())
	{
		return;
	}

	//Build defenses in base.
	if (random(100) < 10 && buildAntiAir(true))
	{
		return;
	}

	if (buildDefenses())
	{
		return;
	}

	//log("All fundamental buildings built -- proceed to military stuff");
}

// Salvage research labs if there is nothing more to research.
function checkResearchCompletion()
{
	const reslist = enumResearch();
	//Sometimes early in T1 no bases it demolishes a lab because it is researching all
	//available tech. So at least wait until Dragon body is obtained before checking this.
	if (componentAvailable("Body14SUP") && reslist.length === 0)
	{
		//log("Done researching - salvage unusable buildings");
		researchDone = true; // and do not rebuild them
		const labList = enumStruct(me, RES_LAB_STAT);
		for (let i = 0, l = labList.length; i < l; ++i)
		{
			const lab = labList[i];
			if (!structureIdle(lab))
			{
				continue;
			}
			if (demolishThis(lab))
			{
				break;
			}
		}
	}
}

// Build modules and check research completion.
function maintenance()
{
	if (countStruct(POW_GEN_STAT) === 0)
	{
		return false;
	}
	//log("Maintenance check");
	const MIN_POWER_FOR_MODULE = -40;
	let struct = null;
	let module = "";
	let structList = [];
	let success = false;
	const modList = [
		{"mod": "A0PowMod1", "amount": 1, "structure": POW_GEN_STAT},
		{"mod": "A0FacMod1", "amount": 2, "structure": FACTORY_STAT},
		{"mod": "A0ResearchModule1", "amount": 1, "structure": RES_LAB_STAT},
		{"mod": "A0FacMod1", "amount": 2, "structure": VTOL_FACTORY_STAT}
	];

	for (let i = 0, l = modList.length; i < l; ++i)
	{
		if (isStructureAvailable(modList[i].mod))
		{
			structList = enumStruct(me, modList[i].structure).sort(sortByDistToBase);
			for (let c = 0, s = structList.length; c < s; ++c)
			{
				if (structList[c].modules < modList[i].amount)
				{
					struct = structList[c];
					module = modList[i].mod;
					break;
				}
			}
			if (struct !== null)
			{
				break;
			}
		}
	}

	if (struct && ((getRealPower() > MIN_POWER_FOR_MODULE || module === "A0PowMod1") || countStruct(DERRICK_STAT) >= 12))
	{
		//log("Found a structure to upgrade");
		const builders = findIdleTrucks(struct);
		for (let j = 0, t = builders.length; j < t; ++j)
		{
			const mydroid = builders[j];
			if (conCanHelp(mydroid, struct.x, struct.y))
			{
				if (orderDroidBuild(mydroid, DORDER_BUILD, module, struct.x, struct.y))
				{
					success = true;
				}
			}
		}
	}

	if (checkResearchCompletion())
	{
		success = true;
	}

	return success;
}
