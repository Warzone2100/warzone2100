//Decide when it is safe for a personality to use the optional machinegun line
//for anti-cyborg measures.
function switchOffMG()
{
	const __cyborgThreat = playerCyborgRatio(getMostHarmfulPlayer()) >= subPersonalities[personality].cyborgThreatPercentage;
	// Will keep using machineguns until the basic laser is available or if the personality
	// doesn't have the first of its primary weapon or artillery line available.
	if ((__cyborgThreat || !havePrimaryOrArtilleryWeapon()) && !componentAvailable("Laser3BEAMMk1"))
	{
		turnOffMG = false;
	}
	else if (!strangeStartSettingOver() || ((getMultiTechLevel() === 1) && gameTime <= 600000))
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
		return enumDroid(player, DROID_CYBORG).length / (countDroid(DROID_ANY, player) + 1);
	}

	return cacheThis(uncached, [player], "playerCyborgRatio" + player, 8000);
}

//Count how many Enemy VTOL units are on the map.
function countEnemyVTOL(player)
{
	function uncached(player)
	{
		const _enemies = isDefined(player) ? [player] : findLivingEnemies();
		let enemyVtolCount = 0;

		for (let x = 0, e = _enemies.length; x < e; ++x)
		{
			const _playerDroids = enumDroid(_enemies[x]);

			for (let c = 0, l = _playerDroids.length; c < l; ++c)
			{
				const __prop = _playerDroids[c].propulsion;

				if (__prop === "V-Tol" || __prop === "Helicopter")
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
		return countEnemyVTOL(player) / (countDroid(DROID_ANY, player) + 1);
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
		return enumStruct(player).length / (countDroid(DROID_ANY, player) + 1);
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
		let small = 0;
		let medium = 0;
		let heavy = 0;
		const _attackers = enumDroid(player, DROID_WEAPON);

		for (let i = 0, len = _attackers.length; i < len; ++i)
		{
			const __body = _attackers[i].body;

			if (__body === "Body1REC" || __body === "Body2SUP" || __body === "Body4ABT" || __body === "Body3MBT")
			{
				++small;
			}
			else if (__body === "Body5REC" || __body === "Body6SUPP" || __body === "Body8MBT" || __body === "Body7ABT")
			{
				++medium;
			}
			else if (__body === "Body11ABT" || __body === "Body9REC" || __body === "Body13SUP" || __body === "Body14SUP" || __body === "Body12SUP" || __body === "Body10MBT")
			{
				++heavy;
			}
		}

		return {
			small: small / (_attackers.length + 1),
			medium: medium / (_attackers.length + 1),
			heavy: heavy / (_attackers.length + 1),
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
		let wheel = 0;
		let halftrack = 0;
		let track = 0;
		let hover = 0;
		const _attackers = enumDroid(player, DROID_WEAPON);

		for (let i = 0, len = _attackers.length; i < len; ++i)
		{
			const __prop = _attackers[i].propulsion;

			if (__prop === "wheeled01")
			{
				++wheel;
			}
			else if (__prop === "HalfTrack")
			{
				++halftrack;
			}
			else if (__prop === "tracked01")
			{
				++track;
			}
			else if (__prop === "hover01")
			{
				++hover;
			}
		}

		return {
			wheel: wheel / (_attackers.length + 1),
			halftrack: halftrack / (_attackers.length + 1),
			track: track / (_attackers.length + 1),
			hover: hover / (_attackers.length + 1),
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
	const __highTechLevel = (getMultiTechLevel() >= 2);
	const __friendCount = playerAlliance(true).length;
	const __highOil = highOilMap();
	let personal;
	let chosen;

	//Map to allow a higher chance for a specific personality to be chosen.
	if (__highTechLevel || __highOil)
	{
		personal = [
			"AR", "AR",
			"AB", "AB", "AB", "AB", "AB", "AB", "AB",
			"AC", "AC", "AC", "AC", "AC", "AC", "AC", "AC", "AC",
			"AA", "AA", "AA", "AA",
			"AV", "AV", "AV", "AV", "AV", "AV"
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
			"AV", "AV", "AV"
		];
	}

	chosen = personal[random(personal.length)];

	//Some personalities should only be chosen if accompanied by a friend
	while ((subPersonalities[chosen].canPlayBySelf === false) && !__friendCount)
	{
		chosen = personal[random(personal.length)];
	}

	//Offensive is better for high oil
	if (__highOil && subPersonalities[chosen].allowAutomaticPersonalityOverride)
	{
		subPersonalities[chosen].resPath = "offensive";

		if ((__highTechLevel || (baseType >= CAMP_BASE)) && chance(33))
		{
			subPersonalities[chosen].resPath = "air";
		}
	}

	return chosen;
}
