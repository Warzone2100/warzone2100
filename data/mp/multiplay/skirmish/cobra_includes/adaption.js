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
	return getResearch("R-Struc-Research-Upgrade07").done;
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

function playerBodySizeRatio(player)
{
	if (!isDefined(player))
	{
		player = getMostHarmfulPlayer();
	}

	function uncached(player)
	{
		var small = 0;
		var medium = 0;
		var heavy = 0;
		var attackers = enumDroid(player, DROID_WEAPON);
		for (var i = 0, len = attackers.length; i < len; ++i)
		{
			var body = attackers[i].body;

			if (body === "Body1REC" || body === "Body2SUP" || body === "Body4ABT" || body === "Body3MBT")
			{
				++small;
			}
			else if (body === "Body5REC" || body === "Body6SUPP" || body === "Body8MBT" || body === "Body7ABT")
			{
				++medium;
			}
			else if (body === "Body11ABT" || body === "Body9REC" || body === "Body13SUP" || body === "Body14SUP" || body ==="Body12SUP" || body === "Body10MBT")
			{
				++heavy;
			}
		}

		return {
			small: small / (attackers.length + 1),
			medium: medium / (attackers.length + 1),
			heavy: heavy / (attackers.length + 1),
		};
	}

	return cacheThis(uncached, [player], "playerBodySizeRatio" + player, 40000);
}

function playerLandPropRatio(player)
{
	if (!isDefined(player))
	{
		player = getMostHarmfulPlayer();
	}

	function uncached(player)
	{
		var wheel = 0;
		var halftrack = 0;
		var track = 0;
		var hover = 0;
		var attackers = enumDroid(player, DROID_WEAPON);
		for (var i = 0, len = attackers.length; i < len; ++i)
		{
			var prop = attackers[i].propulsion;

			if (prop === "wheeled01")
			{
				++wheel;
			}
			else if (prop === "HalfTrack")
			{
				++halftrack;
			}
			else if (prop === "tracked01")
			{
				++track;
			}
			else if (prop === "hover01")
			{
				++hover;
			}
		}

		return {
			wheel: wheel / (attackers.length + 1),
			halftrack: halftrack / (attackers.length + 1),
			track: track / (attackers.length + 1),
			hover: hover / (attackers.length + 1),
		};
	}

	return cacheThis(uncached, [player], "playerLandPropRatio" + player, 40000);
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
	var highOil = highOilMap();
	var personal;
	var chosen;

	//Map to allow a higher chance for a specific personality to be chosen.
	if (HIGH_TECH_LEVEL || highOil)
	{
		personal = [
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

	//Offensive is better for high oil
	if (highOil)
	{
		subPersonalities[chosen].resPath = "offensive";

		if ((HIGH_TECH_LEVEL || (baseType >= CAMP_BASE)) && random(100) < 33)
		{
			subPersonalities[chosen].resPath = "air";
		}
	}

	return chosen;
}
