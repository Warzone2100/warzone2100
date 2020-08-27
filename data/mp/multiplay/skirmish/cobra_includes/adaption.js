//Decide when it is safe for a personality to use the optional machinegun line
//for anti-cyborg measures.
function switchOffMG()
{
	var cyborgThreat = playerCyborgRatio(getMostHarmfulPlayer()) >= subPersonalities[personality].cyborgThreatPercentage;
	// Will keep using machineguns until the basic laser is available or if the personality
	// doesn't have the first of its primary weapon or artillery line available.
	if ((cyborgThreat || !havePrimaryOrArtilleryWeapon()) && !componentAvailable("Laser3BEAMMk1"))
	{
		turnOffMG = false;
	}
	else if (getMultiTechLevel() === 1 && gameTime <= 600000)
	{
		turnOffMG = false;
	}
	else
	{
		turnOffMG = true;
	}
}

function useLasersForCyborgControl()
{
	return getResearch("R-Struc-Research-Upgrade08").done;
}

function playerCyborgRatio(player)
{
	if (!isDefined(player))
	{
		player = getMostHarmfulPlayer();
	}

	function uncached(player)
	{
		return enumDroid(player, DROID_CYBORG).length / (enumDroid(player).length + 1);
	}

	return cacheThis(uncached, [player], "playerCyborgRatio" + player, 8000);
}

//Count how many Enemy VTOL units are on the map.
function countEnemyVTOL(player)
{
	function uncached(player)
	{
		var enemies = isDefined(player) ? [player] : findLivingEnemies();
		var enemyVtolCount = 0;

		for (var x = 0, e = enemies.length; x < e; ++x)
		{
			var playerDroids = enumDroid(enemies[x]);
			for (var c = 0, l = playerDroids.length; c < l; ++c)
			{
				var prop = playerDroids[c].propulsion;
				if (prop === "V-Tol" || prop === "Helicopter")
				{
					++enemyVtolCount;
				}
			}
		}

		return enemyVtolCount;
	}

	return cacheThis(uncached, [player], "countEnemyVTOL" + player, 9000);
}

function playerVtolRatio(player)
{
	if (!isDefined(player))
	{
		player = getMostHarmfulPlayer();
	}

	function uncached(player)
	{
		return countEnemyVTOL(player) / (enumDroid(player).length + 1);
	}

	return cacheThis(uncached, [player], "playerVtolRatio" + player, 6000);
}

function playerStructureUnitRatio(player)
{
	if (!isDefined(player))
	{
		player = getMostHarmfulPlayer();
	}

	function uncached(player)
	{
		return enumStruct(player).length / (enumDroid(player).length + 1);
	}

	return cacheThis(uncached, [player], "playerStructureUnitRatio" + player, 30000);
}


//Choose the personality as described in the global subPersonalities.
//When called from chat it will switch to that one directly.
function choosePersonality(chatEvent)
{
	if (!isDefined(chatEvent))
	{
		personality = adaptToMap();
	}
	else
	{
		personality = chatEvent;
		sendChatMessage("Using personality: " + personality, ALLIES);
	}

	initializeResearchLists();
}

//Tell us what our personality is.
function myPersonality()
{
	return personality;
}

//Semi-randomly choose a personality. Called from eventStartLevel().
function adaptToMap()
{
	const HIGH_TECH_LEVEL = getMultiTechLevel() >= 2;
	const FRIEND_COUNT = playerAlliance(true).length;
	var personal;
	var chosen;

	//Map to allow a higher chance for a specific personality to be chosen.
	if (HIGH_TECH_LEVEL || highOilMap())
	{
		personal = [
			"AM", "AM",
			"AR", "AR", "AR", "AR", "AR",
			"AB", "AB", "AB", "AB", "AB", "AB", "AB", "AB", "AB",
			"AC", "AC", "AC", "AC", "AC", "AC", "AC", "AC", "AC",
			"AA", "AA", "AA",
		];
	}
	else
	{
		personal = [
			"AM", "AM", "AM", "AM",
			"AR", "AR", "AR", "AR", "AR",
			"AB", "AB", "AB", "AB", "AB", "AB", "AB",
			"AC", "AC", "AC", "AC", "AC", "AC", "AC",
			"AA", "AA",
		];
	}

	chosen = personal[random(personal.length)];

	//Some personalities should only be chosen if accompanied by a friend
	while ((subPersonalities[chosen].canPlayBySelf === false) && (FRIEND_COUNT === 0))
	{
		chosen = personal[random(personal.length)];
	}

	return chosen;
}
