
var standardDefenses = [
	"Emplacement-MdART-pit",
	"WallTower-Atmiss",
	"Emplacement-HvyATrocket",
	"WallTower06",
	"Emplacement-MRL-pit",
	"GuardTower6",
];
var longRangeDefenses = [
	"Emplacement-HvART-pit",
	"Emplacement-Rocket06-IDF",
];

// If positive, there are oil derricks that unused due to lack of power generators.
// If negative, we have too many power generator (usually not a problem in itself).
function numUnusedDerricks()
{
	return countStruct(DERRICK) - countStruct(POW_GEN) * 4;
}

function conCanHelp(mydroid, bx, by)
{
	return (mydroid.order !== DORDER_BUILD
		&& mydroid.order !== DORDER_LINEBUILD
		&& mydroid.order !== DORDER_RECYCLE
		&& mydroid.order !== DORDER_DEMOLISH
		&& !droidNeedsRepair(mydroid.id, 80)
		&& droidCanReach(mydroid, bx, by)
	);
}

//Return all trucks that are not doing anything.
function findIdleTrucks(obj)
{
	var builders = enumGroup(baseBuilders);
	var droidlist = [];
	if (!defined(obj))
	{
		obj = BASE;
	}

	for (var i = 0, d = builders.length; i < d; ++i)
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
	var success = false;
	var droidList = findIdleTrucks(object);

	for (var i = 0, d = droidList.length; i < d; ++i)
	{
		if(orderDroidObj(droidList[i], DORDER_DEMOLISH, object))
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

	var droidList = findIdleTrucks();
	var found = false;

	for (var i = 0, d = droidList.length; i < d; ++i)
	{
		var result = pickStructLocation(droidList[i], structure, BASE.x, BASE.y, maxBlockingTiles);
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
	var trucks = findIdleTrucks();
	var freeTrucks = trucks.length;
	var success = false;
	var structlist = enumStruct(me).filter(function(obj) {
		return (obj.status !== BUILT
			&& obj.stattype !== RESOURCE_EXTRACTOR
			&& distBetweenTwoPoints(BASE.x, BASE.y, obj.x, obj.y) < HELP_CONSTRUCT_AREA
		);
	});

	if (freeTrucks && structlist.length)
	{
		structlist = structlist.sort(sortByDistToBase);
		for (var j = 0; j < freeTrucks; ++j)
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

function lookForOil()
{
	var droids = enumGroup(oilBuilders);
	var oils = enumFeature(-1, OIL_RES);
	var bestDroid = null;
	var bestDist = 99999;
	var success = false;
	//log("looking for oil... " + oils.length + " available");

	oils = oils.sort(sortByDistToBase); // grab closer oils first
	for (var i = 0, oilLen = oils.length; i < oilLen; ++i)
	{
		for (var j = 0, drLen = droids.length; j < drLen; ++j)
		{
			var dist = distBetweenTwoPoints(droids[j].x, droids[j].y, oils[i].x, oils[i].y);
			var unsafe = enumRange(oils[i].x, oils[i].y, 9, ENEMIES, false);
			unsafe = unsafe.filter(isUnsafeEnemyObject);
			if (droidCanReach(droids[j], oils[i].x, oils[i].y)
				&& !defined(unsafe[0])
				&& droids[j].order !== DORDER_BUILD  // but can snatch from HELPBUILD
				&& droids[j].order !== DORDER_LINEBUILD
				&& bestDist > dist
				&& !droids[j].busy)
			{
				bestDroid = droids[j];
				bestDist = dist;
			}
		}

		if (bestDroid && !stopExecution("oil" + oils[i].y * mapWidth * oils[i].x, 50000))
		{
			bestDroid.busy = true;
			orderDroidBuild(bestDroid, DORDER_BUILD, DERRICK, oils[i].x, oils[i].y);
			bestDist = 99999;
			bestDroid = null;
			success = true;
		}
	}

	return success;
}

// Build anti air rockets. TODO: demolish obsolete antiair structures.
function buildAntiAir()
{
	const MAX_DEFENSES = countStruct(FACTORY) * 3;
	const SAM_SITES = ["P0-AASite-SAM2", "P0-AASite-SAM1", "P0-AASite-Sunburst"];
	var antiAir = enumStruct(me).filter(function(obj) { return obj.canHitAir; });

	if (antiAir.length > MAX_DEFENSES)
	{
		return false;
	}

	for (var j = 0, s = SAM_SITES.length; j < s; ++j)
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
	const ELECTRONIC_CHANCE = 45;
	var defenses = type === 0 ? standardDefenses : longRangeDefenses;
	var bestDefense;

	if (!defined(type))
	{
		type = random(2);
	}

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

	for (var i = 0, t = defenses.length; i < t; ++i)
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

	var defense = returnDefense(type);

	if (defined(defense))
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
	if (defined(truck))
	{
		return buildDefenseNearTruck(truck, 0);
	}

	if ((gameTime > 210000) && (getRealPower() > MIN_POWER - 30))
	{
		var def = returnDefense();
		if (defined(def))
		{
			return grabTrucksAndBuild(def, 0);
		}
	}

	return false;
}

// Basic base design so as to survive in a no bases match.
function buildBasicBase()
{
	if (!countStruct(FACTORY) && grabTrucksAndBuild(FACTORY, 0))
	{
		return true;
	}

	if (!researchDone)
	{
		if (countStruct(RES_LAB) < 2 && grabTrucksAndBuild(RES_LAB, 0))
		{
			return true;
		}
	}

	// Build HQ if missing
	if (!countStruct(PLAYER_HQ) && grabTrucksAndBuild(PLAYER_HQ, 0))
	{
		return true;
	}

	return false;
}

//Build factories. Attempts to build at least 1 of each factory.
function factoryBuildOrder()
{
	const FAC_ORDER = [FACTORY, CYBORG_FACTORY, VTOL_FACTORY,];
	for (var x = 0; x < 2; ++x)
	{
		var num = !x ? 1 : countStruct(POW_GEN) - 1;
		if (num > 5)
		{
			num = 5;
		}

		for (var i = 0; i < 3; ++i)
		{
			var fac = FAC_ORDER[i];
			if (!(fac === CYBORG_FACTORY && isSeaMap) && countStruct(fac) < num && grabTrucksAndBuild(fac, 0))
			{
				return true;
			}
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

	// If we need power generators, try to queue up production of them with any idle trucks
	if ((!countStruct(POW_GEN) || (numUnusedDerricks() > 0)) && grabTrucksAndBuild(POW_GEN, 0))
	{
		return; // exit early
	}

	if (maintenance())
	{
		return;
	}

	if (countStruct(FACTORY) < 2 && grabTrucksAndBuild(FACTORY, 0))
	{
		return;
	}

	buildFundamentals2(); // go on to the next level
}

function buildFundamentals2()
{
	//log("build fundamentals2");
	if (buildAntiAir())
	{
		return;
	}

	//Build VTOL pads if needed
	var needVtolPads = 2 * countStruct(VTOL_PAD) < enumGroup(vtolGroup).length;
	if (needVtolPads && grabTrucksAndBuild(VTOL_PAD, 2))
	{
		return;
	}

	if (!researchDone && countStruct(RES_LAB) < 4 && grabTrucksAndBuild(RES_LAB, 0))
	{
		return;
	}

	if (!researchDone && countStruct(REPAIR_FACILITY) < 2 && grabTrucksAndBuild(REPAIR_FACILITY, 0))
	{
		return;
	}

	if (getRealPower() < MIN_POWER)
	{
		return;
	}

	if (random(2) && factoryBuildOrder())
	{
		return;
	}

	var res = countStruct(RES_LAB);
	if (!researchDone && res < 5 && res < countStruct(POW_GEN) + 1 && grabTrucksAndBuild(RES_LAB, 0))
	{
		return;
	}

	if (!countStruct(UPLINK) && grabTrucksAndBuild(UPLINK, 0))
	{
		return;
	}

	// Build CB towers or wide spectrum towers in base.
	if (isStructureAvailable(WIDE_SPECTRUM_TOWER))
	{
		if ((countStruct(WIDE_SPECTRUM_TOWER) < 2) && grabTrucksAndBuild(WIDE_SPECTRUM_TOWER, 0))
		{
			return;
		}
	}
	else
	{
		if (countStruct(CB_TOWER) < 2 && grabTrucksAndBuild(CB_TOWER, 0))
		{
			return;
		}
	}

	if (random(2) && countStruct(REPAIR_FACILITY) < 5)
	{
		grabTrucksAndBuild(REPAIR_FACILITY);
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
	var reslist = enumResearch();
	if (!reslist.length)
	{
		//log("Done researching - salvage unusable buildings");
		researchDone = true; // and do not rebuild them
		var lablist = enumStruct(me, RES_LAB);
		for (var i = 0, l = lablist.length; i < l; ++i)
		{
			if(demolishThis(lablist[i]))
			{
				break;
			}
		}
	}
}

// Build modules and check research completion.
function maintenance()
{
	//log("Maintenance check");

	var struct = null;
	var module = "";
	var structList = [];
	var success = false;
	var modList = [
		{"mod": "A0PowMod1", "amount": 1, "structure": POW_GEN},
		{"mod": "A0ResearchModule1", "amount": 1, "structure": RES_LAB},
		{"mod": "A0FacMod1", "amount": 2, "structure": FACTORY},
		{"mod": "A0FacMod1", "amount": 2, "structure": VTOL_FACTORY}
	];

	for (var i = 0, l = modList.length; i < l; ++i)
	{
		if (isStructureAvailable(modList[i].mod))
		{
			structList = enumStruct(me, modList[i].structure).sort(sortByDistToBase).reverse();
			for (var c = 0, s = structList.length; c < s; ++c)
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

	if (struct && (getRealPower() > 60 || module === "A0PowMod1"))
	{
		//log("Found a structure to upgrade");
		var builders = findIdleTrucks(struct);
		for (var j = 0, t = builders.length; j < t; ++j)
		{
			mydroid = builders[j];
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
