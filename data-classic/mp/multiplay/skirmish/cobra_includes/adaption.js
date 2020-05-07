//Decide when it is safe for a personality to use the optional machinegun line
//for anti-cyborg measures.
function switchOffMG()
{
	var cyborgThreat = playerCyborgRatio(getMostHarmfulPlayer()) >= subPersonalities[personality].cyborgThreatPercentage;
	// Will keep using machineguns until the basic laser is available or if the personality
	// doesn't have the first of its primary weapon line available.
	if ((cyborgThreat || !componentAvailable(subPersonalities[personality].primaryWeapon.weapons[0].stat)) && !componentAvailable("Laser3BEAMMk1"))
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
	return (componentAvailable("Body12SUP") || isStructureAvailable(structures.vtolPads));
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

	return cacheThis(uncached, [player], undefined, 8000);
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

	return cacheThis(uncached, [player], undefined, 9000);
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

	return cacheThis(uncached, [player], undefined, 6000);
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

//Choose personality based on map oil/ally count or technology. Called from eventStartLevel().
function adaptToMap() {
	var choice = "";
	var personal;
	var offset;
	const ALLY_COUNT = playerAlliance(true).length - 1;
	const MAP_OIL_LEVEL = mapOilLevel();
	const HIGH_TECH_LEVEL = getMultiTechLevel() >= 2;

	if (!HIGH_TECH_LEVEL && (((maxPlayers - 1) === 1) || ((MAP_OIL_LEVEL === "LOW") && !ALLY_COUNT)))
	{
		personal = ["AM", "AR", "AB", "AC"];
		choice = personal[random(personal.length)];
	}
	else if ((MAP_OIL_LEVEL === "MEDIUM") || ALLY_COUNT)
	{
		personal = ["AM", "AR", "AB", "AC", "AA", "AL"];
		offset = (HIGH_TECH_LEVEL && (baseType !== CAMP_CLEAN)) ? 6 : 5;
		choice = personal[random(offset)];
	}
	else
	{
		personal = ["AC", "AB", "AA", "AL"];
		offset = (HIGH_TECH_LEVEL && (baseType !== CAMP_CLEAN)) ? 4 : 3;
		choice = personal[random(offset)];
	}

	return choice;
}
