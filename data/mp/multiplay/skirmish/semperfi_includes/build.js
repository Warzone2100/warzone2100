
// If positive, there are oil derricks that unused due to lack of power generators.
// If negative, we have too many power generator (usually not a problem in itself).
function numUnusedDerricks()
{
	return countStruct(derrick) - countStruct(powGen) * 4;
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

	for (var i = 0; i < builders.length; i++)
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

	for (var i = 0; i < droidList.length; i++)
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

	for (var i = 0; i < droidList.length; i++)
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
	var success = false;
	var structlist = enumStruct(me).filter(function(obj) { return ((obj.status != BUILT) && (obj.stattype != RESOURCE_EXTRACTOR)); });

	if (trucks.length > 0 && structlist.length > 0)
	{
		structlist.sort(sortByDistToBase);

		for (var j = 0; j < trucks.length; ++j)
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
	var droids = enumDroid(me, DROID_CONSTRUCT);
	var oils = enumFeature(-1, oilRes);
	var bestDroid = null;
	var bestDist = 99999;
	var success = false;
	//log("looking for oil... " + oils.length + " available");

	if (oils.length > 0)
	{
		oils.sort(sortByDistToBase); // grab closer oils first
		for (var i = 0; i < oils.length; i++)
		{
			for (var j = 0; j < droids.length; j++)
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
				orderDroidBuild(bestDroid, DORDER_BUILD, derrick, oils[i].x, oils[i].y);
				bestDist = 99999;
				bestDroid = null;
				success = true;
			}
		}
	}

	return success;
}

// Build anti air rockets. TODO: demolish obsolete antiair structures.
function buildAntiAir()
{
	const MAX_DEFENSES = countStruct(factory) * 2;
	const MIN_POWER = -200;
	const SAM_SITES = ["P0-AASite-SAM2", "P0-AASite-SAM1", "P0-AASite-Sunburst"];
	var antiAir = enumStruct(me).filter(function(obj) { return obj.canHitAir == true; });

	if ((getRealPower() < MIN_POWER) || (antiAir.length > MAX_DEFENSES))
	{
		return false;
	}

	for (var j = 0; j < SAM_SITES.length; ++j)
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
	if(!defined(type))
	{
		type = random(2);
	}

	var defenses;
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

	var bestDefense;
	for(var i = 0; i < defenses.length; ++i)
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
	if (isStructureAvailable(factory) && (countStruct(factory) == 0) && grabTrucksAndBuild(factory, 0))
	{
		return true;
	}

	if (!researchDone && isStructureAvailable(resLab))
	{
		if ((countStruct(resLab) == 0) && grabTrucksAndBuild(resLab, 0))
		{
			return true;
		}
	}

	// Build HQ if missing
	if (isStructureAvailable(playerHQ) && (countStruct(playerHQ) == 0) && grabTrucksAndBuild(playerHQ, 0))
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
	if ((countStruct(powGen) == 0 || numUnusedDerricks() > 0) && isStructureAvailable(powGen) && grabTrucksAndBuild(powGen, 0))
	{
		return; // exit early
	}

	buildFundamentals2(); // go on to the next level
}

function buildFundamentals2()
{
	//log("build fundamentals2");
	var factcount = countStruct(factory);

	//Build VTOL pads if needed
	var needVtolPads = ((2 * countStruct(vtolPad)) < enumGroup(vtolGroup).length);
	if (needVtolPads && isStructureAvailable(vtolPad) && grabTrucksAndBuild(vtolPad, 2))
	{
		return;
	}

	// Same amount of research labs as factories.
	if (!researchDone && isStructureAvailable(resLab))
	{
		//After ten minutes, build more labs if power is good enough.
		if ((((gameTime > 60000 * 10) && (getRealPower() > -150)) || (countStruct(resLab) < factcount)) && grabTrucksAndBuild(resLab, 0))
		{
			return;	// done here
		}
	}

	// Build as many factories as we can afford
	if ((playerPower(me) > factcount * 750) && isStructureAvailable(factory) && grabTrucksAndBuild(factory, 0))
	{
		return; // done here
	}

	// Build VTOL factory if we don't have one
	if (isStructureAvailable(vtolFactory))
	{
		var vFacs = countStruct(vtolFactory);
		if (((vFacs == 0) || (playerPower(me) > factcount * 550)) && (vFacs < factcount) && grabTrucksAndBuild(vtolFactory, 0))
		{
			return;
		}
	}

	// Build cyborg factory if we have thermite cyborg weapon (unless it is a sea map)
	if (!isSeaMap && isStructureAvailable(cybFactory) && componentAvailable("Cyb-Wpn-Thermite"))
	{
		var cFacs = countStruct(cybFactory);
		if (((cFacs == 0) || (playerPower(me) > factcount * 450)) && (cFacs < factcount) && grabTrucksAndBuild(cybFactory, 0))
		{
			return;
		}
	}

	// Build CB towers in base.
	if (isStructureAvailable(cbTower))
	{
		// Or try building wide spectrum towers.
		if (isStructureAvailable(wideSpecTower))
		{
			if ((getRealPower() > -200) && (countStruct(wideSpecTower) < 2) && grabTrucksAndBuild(wideSpecTower, 1))
			{
				return;
			}
		}
		else
		{
			//Keep maxBlockingTile 0 since there is a possibility the trucks
			//could block the path out of base.
			if ((getRealPower() > -100) && (countStruct(cbTower) < 2) && grabTrucksAndBuild(cbTower, 0))
			{
				return;
			}
		}
	}

	if (!lookForOil() && buildDefenses())
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
		var lablist = enumStruct(me, resLab);
		for (var i = 0; i < lablist.length; i++)
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

	for (var i = 0; i < MODULES.length; ++i)
	{
		if (isStructureAvailable(MODULES[i]) && (struct === null))
		{
			switch(i)
			{
				case 0: { structList = enumStruct(me, powGen);  break; }
				case 1: { structList = enumStruct(me, resLab);  break; }
				case 2: { structList = enumStruct(me, factory);  break; }
				case 3: { structList = enumStruct(me, vtolFactory);  break; }
				default: { break; }
			}

			for (var c = 0; c < structList.length; ++c)
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
		for (var j = 0; j < builders.length; j++)
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
