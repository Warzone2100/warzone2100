
// If positive, there are oil derricks that unused due to lack of power generators.
// If negative, we have too many power generator (usually not a problem in itself).
function numUnusedDerricks()
{
	return countStruct(DERRICK) - countStruct(POW_GEN) * 4;
}

function conCanHelp(mydroid, bx, by)
{
	return (mydroid.order != DORDER_HELPBUILD
	        && mydroid.order != DORDER_BUILD
	        && mydroid.order != DORDER_LINEBUILD
		   && mydroid.order != DORDER_RECYCLE
		   && mydroid.order != DORDER_DEMOLISH
	        && droidCanReach(mydroid, bx, by));
}

//Return all trucks that are not doing anything.
function findIdleTrucks()
{
	var builders = enumDroid(me, DROID_CONSTRUCT);
	var droidlist = [];

	for (var i = 0, d = builders.length; i < d; i++)
	{
		if (conCanHelp(builders[i], startPositions[me].x, startPositions[me].y))
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
	var droidList = findIdleTrucks();

	for (var i = 0, d = droidList.length; i < d; i++)
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
	if (!defined(maxBlockingTiles))
	{
		maxBlockingTiles = 1;
	}

	var droidList = findIdleTrucks();
	var found = false;

	for (var i = 0, d = droidList.length; i < d; i++)
	{
		var result = pickStructLocation(droidList[i], structure, startPositions[me].x, startPositions[me].y, maxBlockingTiles);
		if (result)
		{
			//logObj(mydroid, "Construction work");
			orderDroidBuild(droidList[i], DORDER_BUILD, structure, result.x, result.y);
			found = true;
		}
	}

	return found;
}

// Help finish building some object.
function checkLocalJobs()
{
	var trucks = findIdleTrucks();
	var freeTrucks = trucks.length;
	var success = false;
	var structlist = enumStruct(me).filter(function(obj) { return ((obj.status != BUILT) && (obj.stattype != RESOURCE_EXTRACTOR)); });

	if (freeTrucks > 0 && structlist.length > 0)
	{
		structlist.sort(sortByDistToBase);

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
	const EARLY_LIMIT = 1200000;
	var droids = enumDroid(me, DROID_CONSTRUCT);
	var oils = enumFeature(-1, OIL_RES);
	var bestDroid = null;
	var bestDist = 99999;
	var success = false;
	//log("looking for oil... " + oils.length + " available");

	oils.sort(sortByDistToBase); // grab closer oils first
	for (var i = 0, oilLen = oils.length; i < oilLen; i++)
	{
		for (var j = 0, drLen = droids.length; j < drLen; j++)
		{
			var dist = distBetweenTwoPoints(droids[j].x, droids[j].y, oils[i].x, oils[i].y);
			if (droidCanReach(droids[j], oils[i].x, oils[i].y)
				&& safeDest(me, oils[i].x, oils[i].y)
				&& droids[j].order != DORDER_BUILD  // but can snatch from HELPBUILD
				&& droids[j].order != DORDER_LINEBUILD
				&& bestDist > dist
				&& !droids[j].busy)
			{
				bestDroid = droids[j];
				bestDist = dist;
			}
		}

		if (bestDroid)
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
	const MAX_DEFENSES = countStruct(FACTORY) * 2;
	const MIN_POWER = -200;
	const SAM_SITES = ["P0-AASite-SAM2", "P0-AASite-SAM1", "P0-AASite-Sunburst"];
	var antiAir = enumStruct(me).filter(function(obj) { return obj.canHitAir == true; });

	if ((getRealPower() < MIN_POWER) || (antiAir.length > MAX_DEFENSES))
	{
		return false;
	}

	for (var j = 0, s = SAM_SITES.length; j < s; ++j)
	{
		if (isStructureAvailable(SAM_SITES[j]))
		{
			if (grabTrucksAndBuild(SAM_SITES[j], 1))
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
	const ELECTRONIC_CHANCE = 45;
	var defenses;
	var bestDefense;

	if(!defined(type))
	{
		type = random(2);
	}

	if(type == 0)
	{
		defenses = [
			"Emplacement-MdART-pit",
			"WallTower-Atmiss",
			"Emplacement-HvyATrocket",
			"WallTower06",
			"Emplacement-MRL-pit",
			"GuardTower6",
		];
	}
	else
	{
		defenses = [
			"Emplacement-HvART-pit",
			"Emplacement-Rocket06-IDF",
		];
	}

	//Choose a random electronic warfare defense if possible.
	if (random(101) < ELECTRONIC_CHANCE)
	{
		var avail = 0;
		for(var i = 0, t = ELECTRONIC_DEFENSES.length; i < t; ++i)
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

	for(var i = 0, t = defenses.length; i < t; ++i)
	{
		if(isStructureAvailable(defenses[i]))
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
	if(!defined(type))
	{
		type = 0;
	}

	var defense = returnDefense(type);

	if(defined(defense))
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

	if(defined(truck))
	{
		return buildDefenseNearTruck(truck);
	}

	var def = returnDefense(1); // Build artillery in base.
	if (defined(def) && (getRealPower() > -200))
	{
		return grabTrucksAndBuild(def, 1);
	}

	return false;
}

// Basic base design so as to survive in a no bases match.
function buildBasicBase()
{
	if (isStructureAvailable(FACTORY) && (countStruct(FACTORY) == 0) && grabTrucksAndBuild(FACTORY, 0))
	{
		return true;
	}

	if (!researchDone && isStructureAvailable(RES_LAB))
	{
		if ((countStruct(RES_LAB) == 0) && grabTrucksAndBuild(RES_LAB, 0))
		{
			return true;
		}
	}

	// Build HQ if missing
	if (isStructureAvailable(PLAYER_HQ) && (countStruct(PLAYER_HQ) == 0) && grabTrucksAndBuild(PLAYER_HQ, 0))
	{
		return true;
	}

	return false;
}

function buildFundamentals()
{
	//log("build fundamentals");

	if(buildBasicBase())
	{
		return;
	}

	if(lookForOil())
	{
		return;
	}

	// Help build unfinished buildings
	if(checkLocalJobs())
	{
		return;
	}

	if (maintenance())
	{
		return;
	}

	// If we need power generators, try to queue up production of them with any idle trucks
	if ((countStruct(POW_GEN) == 0 || numUnusedDerricks() > 0) && isStructureAvailable(POW_GEN) && grabTrucksAndBuild(POW_GEN, 0))
	{
		return; // exit early
	}

	buildFundamentals2(); // go on to the next level
}

function buildFundamentals2()
{
	//log("build fundamentals2");
	var factcount = countStruct(FACTORY);

	//Build VTOL pads if needed
	var needVtolPads = ((2 * countStruct(VTOL_PAD)) < enumGroup(vtolGroup).length);
	if (needVtolPads && isStructureAvailable(VTOL_PAD) && grabTrucksAndBuild(VTOL_PAD, 2))
	{
		return;
	}

	// Same amount of research labs as factories.
	if (!researchDone && isStructureAvailable(RES_LAB))
	{
		//After ten minutes, build more labs if power is good enough.
		if ((((gameTime > 60000 * 10) && (getRealPower() > -150)) || (countStruct(RES_LAB) < factcount)) && grabTrucksAndBuild(RES_LAB, 0))
		{
			return;	// done here
		}
	}

	// Build as many factories as we can afford
	if ((playerPower(me) > factcount * 750) && isStructureAvailable(FACTORY) && grabTrucksAndBuild(FACTORY, 0))
	{
		return; // done here
	}

	// Build a VTOL factory if we don't have one
	if (isStructureAvailable(VTOL_FACTORY))
	{
		var vFacs = countStruct(VTOL_FACTORY);
		if (((vFacs == 0) || (playerPower(me) > factcount * 550)) && (vFacs < factcount) && grabTrucksAndBuild(VTOL_FACTORY, 0))
		{
			return;
		}
	}

	// Build cyborg factory if we have thermite cyborg weapon (unless it is a sea map)
	if (!isSeaMap && isStructureAvailable(CYBORG_FACTORY) && componentAvailable("Cyb-Wpn-Thermite"))
	{
		var cFacs = countStruct(CYBORG_FACTORY);
		if (((cFacs == 0) || (playerPower(me) > factcount * 450)) && (cFacs < factcount) && grabTrucksAndBuild(CYBORG_FACTORY, 0))
		{
			return;
		}
	}

	// Build CB towers in base.
	if (isStructureAvailable(CB_TOWER))
	{
		// Or try building wide spectrum towers.
		if (isStructureAvailable(WIDE_SPECTRUM_TOWER))
		{
			if ((getRealPower() > -200) && (countStruct(WIDE_SPECTRUM_TOWER) < 2) && grabTrucksAndBuild(WIDE_SPECTRUM_TOWER, 1))
			{
				return;
			}
		}
		else
		{
			//Keep maxBlockingTile 0 since there is a possibility the trucks
			//could block the path out of base.
			if ((getRealPower() > -100) && (countStruct(CB_TOWER) < 2) && grabTrucksAndBuild(CB_TOWER, 0))
			{
				return;
			}
		}
	}

	if (isStructureAvailable(UPLINK) && (countStruct(UPLINK) == 0) && grabTrucksAndBuild(UPLINK, 0))
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
	var reslist = enumResearch();
	if (!reslist.length)
	{
		//log("Done researching - salvage unusable buildings");
		researchDone = true; // and do not rebuild them
		var lablist = enumStruct(me, RES_LAB);
		for (var i = 0, l = lablist.length; i < l; i++)
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

	const MODULES = ["A0PowMod1", "A0ResearchModule1", "A0FacMod1", "A0FacMod1"];
	const MOD_NUMBER = [1, 1, 2, 2]; //Number of modules paired with list above
	var struct = null, module = "", structList = [];
	var success = false;

	for (var i = 0, m = MODULES.length; i < m; ++i)
	{
		if (isStructureAvailable(MODULES[i]) && (struct === null))
		{
			switch(i)
			{
				case 0: { structList = enumStruct(me, POW_GEN);  break; }
				case 1: { structList = enumStruct(me, RES_LAB);  break; }
				case 2: { structList = enumStruct(me, FACTORY);  break; }
				case 3: { structList = enumStruct(me, VTOL_FACTORY);  break; }
				default: { break; }
			}

			for (var c = 0, s = structList.length; c < s; ++c)
			{
				if (structList[c].modules < MOD_NUMBER[i])
				{
					struct = structList[c];
					module = MODULES[i];
					break;
				}
			}
		}
		else
		{
			break;
		}
	}

	if (struct)
	{
		//log("Found a structure to upgrade");
		var builders = enumDroid(me, DROID_CONSTRUCT);
		for (var j = 0, t = builders.length; j < t; j++)
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

	if(checkResearchCompletion())
	{
		success = true;
	}

	return success;
}
