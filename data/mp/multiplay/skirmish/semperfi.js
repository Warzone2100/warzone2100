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
	if (!buildDroid(struct, "Ranged Attacker", bodylist, proplist, "", DROID_WEAPON, weaplist, weaplist))
	{
		debug("Failed to construct new attacker");
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
	if (!buildDroid(struct, "Constructor", bodylist, proplist, "", DROID_CONSTRUCT, "Spade1Mk1"))
	{
		debug("Failed to construct new truck");
	}
}

function buildCyborg(struct)
{
	// Cyborg templates are special -- their bodies, legs and weapons are linked. We should fix this one day...
	if (!buildDroid(struct, "Cyborg Thermite", "Cyb-Bod-Thermite", "CyborgLegs", "", DROID_CYBORG, "Cyb-Wpn-Thermite"))
	{
		if (!buildDroid(struct, "Cyborg Flamer", "CyborgFlamerGrd", "CyborgLegs", "", DROID_CYBORG, "CyborgFlamer01"))
		{
			if (!buildDroid(struct, "Cyborg MG", "CyborgChain1Ground", "CyborgLegs", "", DROID_CYBORG, "CyborgChaingun"))
			{
				debug("Failed to construct new cyborg");
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
	if (!buildDroid(struct, "Bomber", bodylist, "V-Tol", "", DROID_WEAPON, bomblist))
	{
		debug("Failed to construct new VTOL");
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
			orderDroidBuild(mydroid, DORDER_BUILD, bstats, result.x, result.y);
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

function maintenance()
{
	// Salvage unusable buildings?
	var reslist = enumResearch();
	if (reslist.length == 0)
	{
		// No research left, salvage res lab
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
					dbgObj(droid, "Ordered to salvage research lab");
					orderDroidObj(droid, DORDER_DEMOLISH, lab);
					droid.busy = true;
					break;
				}
			}
		}
	}
}

// --- game events

function eventResearched(tech, labparam)
{
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
		if (isStructureAvailable(powModule, me)) // Immediately upgrade it, if possible
		{
			orderDroidBuild(droid, DORDER_BUILD, powModule, struct.x, struct.y);
		}
	}
}

function eventDroidBuilt(droid, struct)
{
	if (struct)
	{
		if (struct.stattype == FACTORY)
		{
			var trucklist = enumDroid(me, DROID_CONSTRUCT);
			if (trucklist.length < 6)
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
			groupAddDroid(vtolGroup, droid);
		}
		else if (droid.droidType == DROID_WEAPON || droid.droidType == DROID_CYBORG)
		{
			groupAddDroid(attackGroup, droid);

			// HUUUGE hack here :) -- naive attack code nested up in here, 'cos i'm so lazy
			// Only attack once every four minutes
			var attackers = enumGroup(attackGroup);
			if (attackers.length > 20 && gameTime > attackRun + 4 * 60 * 1000)
			{
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
							dbgObj(attackers[j], "sent to attack");
						}
						var vtols = enumGroup(vtolGroup);
						for (var j = 0; j < vtols.length; j++)
						{
							orderDroidLoc(vtols[j], DORDER_SCOUT, startPositions[i].x, startPositions[i].y);
							dbgObj(vtols[j], "sent to attack");
						}
						dbgPlr("ATTACKING player " + i);
						attackRun = gameTime;
						return;
					}
					selectedEnemy--; // not this enemy, but maybe next one?
				}
			}
		}
	}
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
		dbgPlr("Defend!");
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
	queue("buildFundamentals");

	// Maintenance calls - to fix quirks
	setTimer("maintenance", 1000 * 60 * 2); // every 2 minutes, call it to check if anything left to do

	/*
	if (numFactories() > 1 && isStructureAvailable(defStructs[0], me) && playerData[me].difficulty > MEDIUM)
	{
		dbgPlr("TRUCK RUSH!");
		queue("truckRush");
	}
	else
	{
		queue("buildFundamentals");
	}*/
}
