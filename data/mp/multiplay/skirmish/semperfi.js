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
const powModule = "A0PowMod1";
const facModule = "A0FacMod1";
const resModule = "A0ResearchModule1";

// -- globals

var attackGroup;
var vtolGroup;
var attackRun = 0;
var researchDone = false;
var fundamentalsTriggered = false; // avoid triggering it multiple times

// --- utility functions

function log(message)
{
	if (me == selectedPlayer)
	{
		console(message);
	}
	//dump(me + "@" + gameTime + " : " + message);
}

function logObj(obj, message)
{
	if (obj.selected)
	{
		console(message);
	}
	//dump(me + "@" + gameTime + " [" + obj.name + " id=" + obj.id + "] > " + message);
}

function buildAttacker(struct)
{
	// For now, only build long range stuff, easier to handle for an AI. Reusing bodies from
	// the VTOLs -- reconsider this.
	var bodylist = [
		"Body14SUP", // dragon
		"Body7ABT", // retribution
		"Body6SUPP", // panther
		"Body8MBT", // scorpion
		"Body5REC", // cobra
		"Body1REC", // viper
	];
	var proplist = [
		"HalfTrack", // half-track
		"wheeled01", // wheels
	];
	var weaplist = [
		"Missile-MdArt", // Seraph
		"Missile-A-T", // Scourge
		"Rocket-HvyA-T", // Tank-killer
		"Rocket-LtA-T", // Lancer
		"Rocket-MRL", // MRL
		"Rocket-Pod", // mini-pod
		"MG3Mk1", // heavy mg
		"MG2Mk1", // twin mg
		"MG1Mk1", // mg, initial weapon
	];
	if (!buildDroid(struct, "Ranged Attacker", bodylist, proplist, null, null, weaplist, weaplist))
	{
		dbg("Failed to construct new attacker");
	}
}

function buildTruck(struct)
{
	var bodylist = [
		"Body7ABT", // retribution
		"Body6SUPP", // panther
		"Body8MBT", // scorpion
		"Body5REC", // cobra
		"Body1REC", // viper
	];
	var proplist = [
		"hover01", // hover
		"wheeled01", // wheels
	];
	if (!buildDroid(struct, "Constructor", bodylist, proplist, null, null, "Spade1Mk1"))
	{
		log("Failed to construct new truck");
	}
}

function buildCyborg(struct)
{
	// Cyborg templates are special -- their bodies, legs and weapons are linked. We should fix this one day...
	if (!buildDroid(struct, "Cyborg Thermite", "Cyb-Bod-Thermite", "CyborgLegs", null, null, "Cyb-Wpn-Thermite"))
	{
		if (!buildDroid(struct, "Cyborg Flamer", "CyborgFlamerGrd", "CyborgLegs", null, null, "CyborgFlamer01"))
		{
			if (!buildDroid(struct, "Cyborg MG", "CyborgChain1Ground", "CyborgLegs", null, null, "CyborgChaingun"))
			{
				log("Failed to construct new cyborg");
			}
		}
	}
}

function buildVTOL(struct)
{
	var bomblist = [
		"Bomb3-VTOL-LtINC",	// phosphor bomb
		"Bomb4-VTOL-HvyINC",	// thermite bomb
		"Bomb5-VTOL-Plasmite",	// plasmite bomb
	];
	var bodylist = [
		    "Body7ABT", // retribution
		    "Body8MBT", // scorpion
		    "Body5REC", // cobra
		    "Body4ABT", // bug
		    "Body1REC", // viper
	];
	if (!buildDroid(struct, "Bomber", bodylist, "V-Tol", null, null, bomblist))
	{
		log("Failed to construct new VTOL");
	}
}

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
				logObj(mydroid, "added to build list");
				droidlist.push(mydroid);
			}
			else if (currDist < closestDist)
			{
				logObj(mydroid, "is closest to build");
				closestDroid = mydroid;	// record this droid as being closest so far
				closestDist = currDist;
			}
		}
		else logObj(mydroid, "cannot help, sorry; order is " + mydroid.order + " and can reach? " + conCanHelp(mydroid, startPositions[me].x, startPositions[me].y));
	}
	if (droidlist.length == 0 && closestDroid) droidlist.push(closestDroid);
	for (i = 0; i < droidlist.length; i++)
	{
		mydroid = droidlist[i];
		var result = pickStructLocation(mydroid, bstats, startPositions[me].x, startPositions[me].y, maxBlockingTiles);
		if (result)
		{
			logObj(mydroid, "Construction work");
			orderDroidBuild(mydroid, DORDER_BUILD, bstats, result.x, result.y);
			found = true;
		}
		else
		{
			logObj(mydroid, "Pick struct location failed");
			log("Pick struct location failed for droid " + mydroid.id);
		}
	}
	return found;
}

// --- custom triggers

function buildPowerGenerators()
{
	if (isStructureAvailable(powGen))
	{
		if (!grabTrucksAndBuild(20, powGen, 1))
		{
			log("Needed power generator but could not build one");
		}
	}
}

function checkLocalJobs(truck, structlist)
{
	if (!structlist)
	{
		structlist = enumStruct(me);
	}
	// Find closest unfinished structure
	var bestStruct;
	var bestDist = 99999;
	for (var i = 0; i < structlist.length; i++)
	{
		var struct = structlist[i];
		if (struct.status != BUILT && conCanHelp(truck, struct.x, struct.y))
		{
			var dist = distBetweenTwoPoints(truck.x, truck.y, struct.x, struct.y);
			if (dist < 50 && (dist < bestDist || struct.stattype == POWER_GEN))
			{
				bestDist = dist;
				bestStruct = struct;
			}
		}
	}
	if (bestStruct)
	{
		orderDroidObj(truck, DORDER_HELPBUILD, bestStruct);
		logObj(truck, "Go help construction");
		return true;
	}
	return false;
}

function sortByDistToBase(obj1, obj2)
{
	var dist1 = distBetweenTwoPoints(startPositions[me].x, startPositions[me].y, obj1.x, obj1.y);
	var dist2 = distBetweenTwoPoints(startPositions[me].x, startPositions[me].y, obj2.x, obj2.y);
	return (dist1 - dist2);
}

function lookForOil(droids)
{
	var droids = enumDroid(me, DROID_CONSTRUCT);
	var oils = enumFeature(-1, oilRes);
	var bestDroid = null;
	var bestDist = 99999;
	log("looking for oil... " + oils.length + " available");
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
			}
		}
	}
	return bestDroid;
}

function buildFundamentals()
{
	log("build fundamentals");
	var needPwGen = false;

	fundamentalsTriggered = false;

	// Do we need power generators?
	if (playerPower(me) < 1000 && numUnusedDerricks() > 0)
	{
		needPwGen = true;
		log("More power generators needed");
	}
	if (!needPwGen && playerPower(me) < 500) // check for more income
	{
		if (lookForOil())
		{
			log("Now looking for oil");
			return; // do not build anything else
		}
		log("No oil found");
	}
	// Help build unfinished buildings
	var structlist = enumStruct(me);
	var droids = enumDroid(me, DROID_CONSTRUCT);
	for (var j = 0; j < droids.length; j++)
	{
		if (droids[j].order != DORDER_BUILD)
		{
			checkLocalJobs(droids[j], structlist);
		}
	}
	// If we need power generators, try to queue up production of them with any idle trucks
	if (needPwGen && isStructureAvailable(powGen) && grabTrucksAndBuild(20, powGen, 1))
	{
		return; // exit early
	}
	queue("buildFundamentals2"); // go on to the next level
}

function buildFundamentals2()
{
	log("build fundamentals2");
	var factcount = countStruct(factory);
	// Build as many research labs as factories
	if (!researchDone && isStructureAvailable(resLab))
	{
		if (countStruct(resLab) < factcount && grabTrucksAndBuild(20, resLab, 1))
		{
			return;	// done here
		}
	}
	// Build as many factories as we can afford
	if (playerPower(me) > factcount * 750 && isStructureAvailable(factory) && grabTrucksAndBuild(20, factory, 1))
	{
		return; // done here
	}
	// Build power generator if missing
	if (isStructureAvailable(powGen) && countStruct(powGen) == 0 && grabTrucksAndBuild(20, powGen, 1))
	{
		return;
	}
	// Build HQ if missing
	if (isStructureAvailable(playerHQ) && countStruct(playerHQ) == 0 && grabTrucksAndBuild(20, playerHQ, 1))
	{
		return;
	}
	// Build cyborg factory if we don't have one
	if (isStructureAvailable(cybFactory))
	{
		if (countStruct(cybFactory) == 0 && playerPower(me) > 500 && grabTrucksAndBuild(20, cybFactory, 1))
		{
			return;
		}
	}
	// Build VTOL factory if we don't have one
	if (isStructureAvailable(vtolFactory))
	{
		if (countStruct(vtolFactory) == 0 && playerPower(me) > 750 && grabTrucksAndBuild(20, vtolFactory, 1))
		{
			return;
		}
	}
	log("All fundamental buildings built -- proceed to military stuff");
	// FIXME ... but instead look for oil
	lookForOil();
	// queue("buildDefenses");
}

function maintenance()
{
	log("Maintenance check");

	var reslist = enumResearch();
	if (reslist.length == 0)
	{
		log("Done researching - salvage unusable buildings");

		// No research left, salvage res lab
		researchDone = true; // and do not rebuild them
		var lablist = enumStruct(me, resLab);
		var builders = enumDroid(me, DROID_CONSTRUCT);
		for (i = 0; i < lablist.length; i++)
		{
			var lab = lablist[i];
			// TODO - find closest truck!
			for (var j = 0; j < builders.length; j++)
			{
				var droid = builders[j];
				if (droid.order != DORDER_DEMOLISH && droid.order != DORDER_BUILD
				    && droid.order != DORDER_LINEBUILD && !droid.busy)
				{
					logObj(droid, "Ordered to salvage research lab");
					orderDroidObj(droid, DORDER_DEMOLISH, lab);
					droid.busy = true;
					break;
				}
			}
		}
	}
	// Check for idle trucks
	if (!fundamentalsTriggered)
	{
		queue("buildFundamentals");
		fundamentalsTriggered = true;
	}
	// Check for idle structures (eg factories, labs...)
	var faclist = enumStruct(me);
	for (var j = 0; j < faclist.length; j++)
	{
		if (structureIdle(faclist[j]))
		{
			eventStructureBuilt(faclist[j], null);
		}
	}
}

// --- game events

function eventResearched(tech, labparam)
{
	if (playerPower(me) < 100)
	{
		return; // wait
	}
	var techlist = [
		"R-Defense-Tower01",	// mg tower
		"R-Struc-PowerModuleMk1",	// power module
		"R-Struc-Factory-Module",	// factory module
		"R-Struc-Research-Upgrade06",	// final research upgrade
	];
	var anyfound = true; // assume success
	var lablist;
	if (labparam) // check if called with parameter or not
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
			var found = pursueResearch(lab, techlist);
			if (!found)
			{
				// Look for missile tech (and test string parameter)
				found = pursueResearch(lab, "R-Wpn-MdArtMissile");
				if (!found)
				{
					// Find a random research item
					var reslist = enumResearch();
					if (reslist.length > 0)
					{
						var idx = Math.floor(Math.random() * reslist.length);
						pursueResearch(lab, reslist[idx].name);
					}
				}
			}
		}
	}
}

function eventStructureBuilt(struct, droid)
{
	if (struct.stattype == RESEARCH_LAB)
	{
		// HACK -- we cannot call eventResearched directly from eventStartLevel for some
		// reason, since then research messages are dropped on the floor... FIXME
		queue("eventResearched");
	}
	else if (struct.stattype == FACTORY || struct.stattype == CYBORG_FACTORY || struct.stattype == VTOL_FACTORY)
	{
		eventDroidBuilt(null, struct);
	}
	else if (struct.stattype == POWER_GEN && droid)
	{
		if (isStructureAvailable(powModule)) // Immediately upgrade it, if possible
		{
			var builders = enumDroid(me, DROID_CONSTRUCT);
			for (i = 0; i < builders.length; i++)
			{
				mydroid = builders[i];
				var currDist = distBetweenTwoPoints(struct.x, struct.y, mydroid.x, mydroid.y);
				if (conCanHelp(mydroid, struct.x, struct.y) && currDist < 20)
				{
					orderDroidBuild(droid, DORDER_BUILD, powModule, struct.x, struct.y);
				}
				return;
			}
		}
	}
	if (!fundamentalsTriggered)
	{
		queue("buildFundamentals"); // see if we should build something else
		fundamentalsTriggered = true;
	}
}

function eventDroidBuilt(droid, struct)
{
	var sensorlist = enumBlips(me);

	if (struct && structureIdle(struct))
	{
		if (struct.stattype == FACTORY && playerPower(me) > 100)
		{
			if (countDroid(DROID_CONSTRUCT) < 5)
			{
				buildTruck(struct);
			}
			else
			{
				buildAttacker(struct);
			}
		}
		else if (struct.stattype == CYBORG_FACTORY)
		{
			buildCyborg(struct);
		}
		else if (struct.stattype == VTOL_FACTORY)
		{
			buildVTOL(struct);
		}
	}
	if (droid)
	{
		if (isVTOL(droid))
		{
			groupAdd(vtolGroup, droid);
		}
		else if (droid.droidType == DROID_WEAPON || droid.droidType == DROID_CYBORG)
		{
			groupAdd(attackGroup, droid);

			// HUUUGE hack here :) -- naive attack code nested up in here, 'cos i'm so lazy
			// Only attack once every four minutes
			if (groupSizes[attackGroup] > 20 && gameTime > attackRun + 4 * 60 * 1000)
			{
				log("-- Military offensive --");

				var attackers = enumGroup(attackGroup);
				// Attack! Find a random enemy, since that is more fun.
				var numEnemies = 0;
				for (var i = 0; i < maxPlayers; i++)
				{
					if (!allianceExistsBetween(me, i))
					{
						numEnemies++; // count 'em, then kill 'em :)
					}
				}
				var selectedEnemy = Math.round(Math.random() * (numEnemies - 1));
				for (var i = 0; i < maxPlayers; i++)
				{
					if (!allianceExistsBetween(me, i) && selectedEnemy === 0)
					{
						for (var j = 0; j < attackers.length; j++)
						{
							orderDroidLoc(attackers[j], DORDER_SCOUT, startPositions[i].x, startPositions[i].y);
							logObj(attackers[j], "sent to attack");
						}
						var vtols = enumGroup(vtolGroup);
						for (var j = 0; j < vtols.length; j++)
						{
							if (vtols[j].armed == 100) // only if fully armed
							{
								orderDroidLoc(vtols[j], DORDER_SCOUT, startPositions[i].x, startPositions[i].y);
								logObj(vtols[j], "sent to attack");
							}
						}
						log("ATTACKING player " + i);
						attackRun = gameTime;
						return;
					}
					selectedEnemy--; // not this enemy, but maybe next one?
				}
			}
		}
		else if (droid.droidType == DROID_CONSTRUCT)
		{
			if (!checkLocalJobs(droid) && !fundamentalsTriggered)
			{
				queue("buildFundamentals");
				fundamentalsTriggered = true;
			}
		}
	}
}

function eventDroidIdle(droid)
{
	// TBD
}

function eventStructureReady(struct)
{
	// TBD
	//activateStructure(struct, target);
}

function eventGameInit()
{
	attackGroup = newGroup();
	vtolGroup = newGroup();
}

function eventAttacked(victim, attacker)
{
	// TBD, for now -- SEND EVERYONE!!!
	if (attacker && victim && victim.type == STRUCTURE && attacker.player != me)
	{
		log("Defend!");
		var i;
		var defenders = enumDroid(me, DROID_WEAPON);
		for (i = 0; i < defenders.length; i++)
		{
			orderDroidObj(defenders[i], DORDER_ATTACK, attacker);
		}
		var cyborgs = enumDroid(me, DROID_CYBORG);
		for (i = 0; i < cyborgs.length; i++)
		{
			orderDroidObj(cyborgs[i], DORDER_ATTACK, attacker);
		}
	}
}

function eventStartLevel()
{
	// Pretend like all buildings were just produced, to initiate productions
	var structlist = enumStruct(me);
	for (var i = 0; i < structlist.length; i++)
	{
		eventStructureBuilt(structlist[i]);
	}
	attackRun = gameTime;

	// Make missing buildings
	if (!fundamentalsTriggered)
	{
		queue("buildFundamentals");
		fundamentalsTriggered = true;
	}

	// Maintenance calls - to fix quirks
	setTimer("maintenance", 1000 * 15); // every 15 seconds, call it to check if anything left to do

	//dump("== level started ==");
}

function eventDroidIdle(droid)
{
	if (droid.droidType == DROID_CONSTRUCT)
	{
		if (!checkLocalJobs(droid) && !fundamentalsTriggered)
		{
			queue("buildFundamentals"); // build something
			fundamentalsTriggered = true;
		}
	}
}

function eventGroupLoss(droid, group, size)
{
	log("lost " + droid.id + " in group " + group + " which is now size " + size);
}

function eventChat(from, to, message)
{
	if (to != me || to == from)
	{
		return; // not for me
	}
	if (message == "donatetruck" && allianceExistsBetween(from, to))
	{
		// donate first truck
		var droids = enumDroid(me, DROID_CONSTRUCT);
		for (var i = 0; i < droids.length; i++)
		{
			donateObject(droids[i], from);
			return;
		}
	}
	else if (message == "donatepower" && allianceExistsBetween(from, to))
	{
		donatePower(playerPower(me) / 2, from);
	}
}
