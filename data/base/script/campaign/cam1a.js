var timercoming = 0; // start mission timer after first power gen and derrick is built
var lastHitTime = 0;
var scav1group, scav2group, scav3group, scav4group; // premade groups
var cheatmode = false;
var stage = 0;
var numArtifact = 0;

function gameLost()
{
	gameOverMessage(false);
}

// player zero's droid enteres this area
function eventAreaLaunchScavAttack(droid)
{
	stage++;
	var spos = getObject("scav1soundpos");
	playSound("pcv375.ogg", spos.x, spos.y, 0);
	playSound("pcv456.ogg");
	hackAddMessage("MB1A_MSG", MISS_MSG, 0, true);
	hackAddMessage("C1A_OBJ1", PROX_MSG, 0, false);
	hackMarkTiles(); // clear any marked tiles from debugging
	var droids = enumArea("ScavAttack1", ALL_PLAYERS, false);
	// send scavengers on war path if triggered above
	var startpos = getObject("playerBase");
	for (var i = 0; i < droids.length; i++)
	{
		if ((droids[i].player == 7 || droids[i].player == 6) && droids[i].type == DROID)
		{
			orderDroidLoc(droids[i], DORDER_SCOUT, startpos.x, startpos.y);
		}
	}
	if (cheatmode)
	{
		hackMarkTiles("ScavAttack1"); // mark next area
	}
}

// player zero's droid enteres this area
function eventAreaScavAttack1(droid)
{
	stage++;
	hackRemoveMessage("C1A_OBJ1", PROX_MSG, 0);
	hackMarkTiles(); // clear marks
	hackAddMessage("C1A_BASE0", PROX_MSG, 0, false);
}

function eventCheatMode(entered)
{
	cheatmode = entered; // remember this setting
	if (entered)
	{
		if (stage == 0)
		{
			hackMarkTiles("LaunchScavAttack");
		}
		else if (stage == 1)
		{
			hackMarkTiles("ScavAttack1");
		}
		else
		{
			hackMarkTiles(); // clear marks
		}
	}
	else
	{
		hackMarkTiles(); // clear any marked tiles
	}
}

// proceed to next level
function gameWon()
{
	enableResearch("R-Wpn-MG1Mk1"); // bonus research topic on level end
	var bonusTime = getMissionTime();
	if (bonusTime > 0)
	{
		setPowerModifier(125); // 25% bonus to completing fast
		extraPowerTime(bonusTime);
		setPowerModifier(100);
	}
	loadLevel("CAM_1B");
}

// listen to chat messages from player
function eventChat(from, to, message)
{
	if (message == "let me win" && cheatmode)
	{
		enableResearch("R-Wpn-MG-Damage01");
		enableResearch("R-Sys-Engineering01");
		enableResearch("R-Defense-Tower01");
		enableResearch("R-Wpn-Flamer01Mk1");
		enableResearch("R-Wpn-MG1Mk1");
		// TODO, finish some/all of the above
		var artifact = getObject("artifact");
		if (artifact)
		{
			removeObject(artifact);
		}
		queue("gameWon");
	}
	else if (message == "status" && cheatmode)
	{
		console("numArtifact = " + numArtifact);
		console("stage = " + stage);
	}
}

// things that need checking every second
function tick()
{
	// check if game is lost
	var factories = countStruct("A0LightFactory") + countStruct("A0CyborgFactory");
	var droids = countDroid(DROID_CONSTRUCT);
	if (droids == 0 && factories == 0)
	{
		queue("gameLost", 4000); // wait 4 secs before throwing the game
	}
	// check if game is won
	var hostiles = countStruct("A0BaBaFactory", 6) + countStruct("A0BaBaFactory", 7)
	               + countDroid(DROID_ANY, 6) + countDroid(DROID_ANY, 7);
	if (!hostiles && numArtifact >= 4 && stage >= 6)
	{
		queue("gameWon", 6000); // wait 6 secs before giving it
	}
}

function playDelayed374(where)
{
	var spos = getObject("scav2soundpos");
	var spos = getObject(where);
	playSound("pcv374.ogg", spos.x, spos.y, 0);
}

function eventGroupLoss(obj, groupid, newsize)
{
	var leftovers;
	if (groupid == scav1group && newsize == 0)
	{
		// eliminated scav base 1
		leftovers = enumArea("scavbase1area");
		hackRemoveMessage("C1A_BASE0", PROX_MSG, 0);
		hackAddMessage("C1A_BASE1", PROX_MSG, 0, false);
		var spos = getObject("scav1soundpos");
		playSound("pcv391.ogg", spos.x, spos.y, 0);
		queue("playDelayed374", 2000, "scav2soundpos");
		stage++;
	}
	else if (groupid == scav2group && newsize == 0)
	{
		// eliminated scav base 2
		leftovers = enumArea("scavbase2area");
		hackRemoveMessage("C1A_BASE1", PROX_MSG, 0);
		hackAddMessage("C1A_BASE2", PROX_MSG, 0, false);
		var spos = getObject("scav2soundpos");
		playSound("pcv392.ogg", spos.x, spos.y, 0);
		queue("playDelayed374", 2000, "scav3soundpos");
		stage++;
	}
	else if (groupid == scav3group && newsize == 0)
	{
		// eliminated scav base 3
		leftovers = enumArea("scavbase3area");
		hackRemoveMessage("C1A_BASE2", PROX_MSG, 0);
		hackAddMessage("C1A_BASE3", PROX_MSG, 0, false);
		var spos = getObject("scav3soundpos");
		playSound("pcv392.ogg", spos.x, spos.y, 0);
		queue("playDelayed374", 2000, "retreat4");
		stage++;
	}
	else if (groupid == scav4group && newsize == 0)
	{
		// eliminated scav base 4
		leftovers = enumArea("scavbase4area");
		hackRemoveMessage("C1A_BASE3", PROX_MSG, 0);
		var spos = getObject("retreat4");
		playSound("pcv392.ogg", spos.x, spos.y, 0);
		stage++;
	}
	// if scav group gone, nuke any leftovers, such as scav walls
	for (var i = 0; leftovers && i < leftovers.length; i++)
	{
		if (((leftovers[i].player == 6 || leftovers[i].player == 7) && leftovers[i].type == STRUCTURE)
		    || (leftovers[i].type == FEATURE && leftovers[i].stattype == BUILDING))
		{
			removeObject(leftovers[i], true); // remove with special effect
		}
	}
}

function addartifact(poslabel, artilabel)
{
	var artpos = getObject(poslabel);
	var artifact = addFeature("Crate", artpos.x, artpos.y);
	addLabel(artifact, artilabel);
}

function eventStartLevel()
{
	var startpos = getObject("startPosition");
	var lz = label("landingZone");

	scav1group = getObject("scavgroup1").id;
	scav2group = getObject("scavgroup2").id;
	scav3group = getObject("scavgroup3").id;
	scav4group = getObject("scavgroup4").id;

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, 0);
	setPower(1300);

	// allow to build stuff
	setStructureLimits("A0PowerGenerator", 5, 0);
	setStructureLimits("A0ResourceExtractor", 200, 0);
	setStructureLimits("A0ResearchFacility", 5, 0);
	setStructureLimits("A0LightFactory", 5, 0);
	setStructureLimits("A0CommandCentre", 1, 0);
	enableStructure("A0CommandCentre", 0);
	enableStructure("A0PowerGenerator", 0);
	enableStructure("A0ResourceExtractor", 0);
	enableStructure("A0ResearchFacility", 0);
	enableStructure("A0LightFactory", 0);

	makeComponentAvailable("MG1Mk1", me);	// needs to be done this way so doesn't enable rest of tree!
	completeResearch("R-Vehicle-Body01", me);
	completeResearch("R-Sys-Spade1Mk1", me);
	completeResearch("R-Vehicle-Prop-Wheels", me);

	// give player briefing
	hackAddMessage("CMB1_MSG", CAMP_MSG, 0, false);

	setReinforcementTime(-1);
	setMissionTime(-1);

	// Add artifacts
	addartifact("artifact4pos", "artifact1");
	addartifact("artifact1pos", "artifact2");
	addartifact("artifact3pos", "artifact3");
	addartifact("artifact2pos", "artifact4");

	setTimer("tick", 1000);
}

function eventStructureBuilt(structure, droid)
{
	if (structure.stattype == POWER_GEN)
	{
		timercoming++;
	}
	else if (structure.stattype == RESOURCE_EXTRACTOR)
	{
		timercoming++;
	}
	if (timercoming == 2)
	{
		setMissionTime(3600);
	}
}

// Called when a human droid moves close to a crate.
function eventPickup(feature, droid)
{
	if (feature.stattype != ARTIFACT)
	{
		return; // not interested!
	}
	playSound("pcv352.ogg", feature.x, feature.y, feature.z);
	var lab = getLabel(feature);
	removeObject(feature); // artifacts are not self-removing...
	if (lab == "artifact1") // first artifact
	{
		enableResearch("R-Wpn-MG-Damage01");
		numArtifact++;
	}
	else if (lab == "artifact2") // second artifact
	{
		enableResearch("R-Sys-Engineering01");
		numArtifact++;
	}
	else if (lab == "artifact3") // third artifact
	{
		enableResearch("R-Defense-Tower01");
		numArtifact++;
	}
	else if (lab == "artifact4") // final artifact
	{
		enableResearch("R-Wpn-Flamer01Mk1");
		numArtifact++;
	}
	else
	{
		debug("Bad artifact found in cam1a!");
	}
}

// /////////////////////////////////////////////////////////////////
// WARNING MESSAGES
// Base Under Attack
// FIXME -- if this is present in every script, put it in rules.js instead?
function eventAttacked(victimObj, attackerObj)
{
	if (gameTime > lastHitTime + 5000)
	{
		lastHitTime = gameTime;
		if (victimObj.type == STRUCTURE)
		{
			playSound("pcv337.ogg", victimObj.x, victimObj.y, victimObj.z);	// show position if still alive
		}
		else
		{
			playSound("pcv399.ogg", victimObj.x, victimObj.y, victimObj.z);
		}
	}
}
