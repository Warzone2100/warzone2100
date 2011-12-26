// -- definitions

const oilRes = "OilResource";
const resLab = "A0ResearchFacility";
const powGen = "A0PowerGenerator";
const factory = "A0LightFactory";
const derrick = "A0ResourceExtractor";
const cybFactory = "A0CyborgFactory";
const playerHQ = "A0CommandCentre";
const vtolPad = "A0VtolPad";
const vtolFactory = "A0VTolFactory1";
const sensorTower = "Sys-SensoTower02";

// --- utility functions

function dbgPlr(message)
{
	if (me == selectedPlayer)
	{
		console(message);
	}
}

function dbgObj(obj, message)
{
	if (obj.selected)
	{
		console(message);
	}
}

// If positive, there are oil derricks that unused due to lack of power generators.
// If negative, we have too many power generator (usually not a problem in itself).
function numUnusedDerricks()
{
	var derricklist = enumStruct(me, derrick);
	var powgenlist = enumStruct(me, powGen);
	return derricklist.length - powgenlist.length * 4;
}

function conCanHelp(mydroid, bx, by)
{
	return (mydroid.order != DORDER_HELPBUILD 
	        && mydroid.order != DORDER_BUILD
	        && mydroid.order != DORDER_LINEBUILD
	        && droidCanReach(mydroid, bx, by));
}

function grabTrucksAndBuild(range, bstats, maxBlockingTiles)
{
	var closestDroid, bx, by, mydroid, i;
	var builders = enumDroid(me, DROID_CONSTRUCT);
	var closestDist = 99999;
	var droidlist = [];
	var found = false;

	// Find truck closest to our base
	for (i = 0; i < builders.length; i++)
	{
		mydroid = builders[i];
		var currDist = distBetweenTwoPoints(startPositions[me].x, startPositions[me].y, mydroid.x, mydroid.y);
		if (conCanHelp(mydroid, startPositions[me].x, startPositions[me].y))
		{
			if (currDist < range)
			{
				dbgObj(mydroid, "added to build list");
				droidlist.push(mydroid);
			}
			else if (currDist < closestDist)
			{
				dbgObj(mydroid, "is closest to build");
				closestDroid = mydroid;	// record this droid as being closest so far
				closestDist = currDist;
			}
		}
		else dbgObj(mydroid, "cannot help, sorry; order is " + mydroid.order + " and can reach? " + conCanHelp(mydroid, startPositions[me].x, startPositions[me].y));
	}
	if (droidlist.length == 0 && closestDroid) droidlist.push(closestDroid);
	for (i = 0; i < droidlist.length; i++)
	{
		mydroid = droidlist[i];
		var result = pickStructLocation(mydroid, bstats, startPositions[me].x, startPositions[me].y, maxBlockingTiles);
		if (result)
		{
			dbgObj(mydroid, "Construction work");
			orderDroidStatsLoc(mydroid, DORDER_BUILD, bstats, result.x, result.y);
			found = true;
		}
		else
		{
			dbgObj(mydroid, "Pick struct location failed");
			debug("Pick struct location failed for droid " + mydroid.id);
		}
	}
	return found;
}

// --- custom triggers

function buildPowerGenerators()
{
	if (isStructureAvailable(powGen, me))
	{
		if (!grabTrucksAndBuild(20, powGen, 1))
		{
			dbgPlr("Needed power generator but could not build one");
		}
	}
}

function conDroids()
{

}

function eventResearched(labparam)
{
	var lablist;
	if (labparam)
	{
		lablist = [];
		lablist[0] = labparam;
	}
	else
	{
		lablist = enumStruct(me, resLab);
	}
	for (i = 0; i < lablist.length; i++)
	{
		var lab = lablist[i];
		if (lab.status == BUILT && structureIdle(lab))
		{
			var found = pursueResearch(lab, "R-Struc-Research-Upgrade06");
			if (!found)
			{
				// TBD - find random research
				return;
			}
		}
	}
	// TBD - no research left, salvage res labs
}

function buildFundamentals()
{
	var needPwGen = false;

	// Do we need power generators?
	//if ((playerPower(me) < HIGH_POWER * 2 or numFactories() > 1) and numUnusedDerricks() > 0)
	if (numUnusedDerricks() > 0)
	{
		needPwGen = true;
		dbgPlr("More power generators needed");
	}
	if (needPwGen)
	{
		queue("buildPowerGenerators");
		return;
	}
}

// --- game events

function eventGameInit()
{
}

function eventStructureAttacked(hitStruct, attackerObj)
{
}

function eventStartLevel()
{
	queue("conDroids");
	queue("eventResearched");
	queue("buildFundamentals");
	/*
	if (numFactories() > 1 && isStructureAvailable(defStructs[0], me) && playerData[me].difficulty > MEDIUM)
	{
		dbgPlr("TRUCK RUSH!");
		next("truckRush");
	}
	else
	{
		next("buildFundamentals");
	}*/
}
