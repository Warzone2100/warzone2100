
function eventResearched(research, structure, player)
{
	if (gameTime <= 300)
	{
		return; // Just in case this event gets caught automatically during rules base/tech level research setup.
	}

	if (allianceExistsBetween(me, player))
	{
		if (__LOG_RESEARCH_PATH && isDefined(structure))
		{
			dump(research.name);
		}
	}
	else
	{
		return;
	}

	// Attempt to quickly start another topic if we have enough power.
	if ((player === me) && (highOilMap() || (getRealPower() > __MIN_POWER)))
	{
		cobraDoResearch();
	}

	// One time check to immediately build a HMG tower next to a nearby derrick if an enemy got to it first.
	if ((research.id === "R-Defense-Tower01") &&
		(getMultiTechLevel() === 1) &&
		(baseType === CAMP_CLEAN))
	{
		queue("resDoOilDefend", 100);
	}
}

function resDoOilDefend()
{
	enumGroup(oilGrabberGroup).forEach((obj) => {
		if (conCanHelp(obj, obj.x, obj.y))
		{
			eventDroidIdle(obj);
		}
	});
}

//updates a research list with whatever is passed to it.
function updateResearchList(stat, len)
{
	if (!isDefined(len))
	{
		len = 0;
	}

	const _list = [];

	for (let x = 0, d = stat.length - len; x < d; ++x)
	{
		const _st = stat[x];

		if (isDefined(_st.res))
		{
			_list.push(_st.res);
		}
		else
		{
			_list.push(_st);
		}
	}

	return _list;
}

//Initialization of research lists when eventStartLevel is triggered.
//Call this again when manually changing a personality.
function initializeResearchLists()
{
	techlist = subPersonalities[personality].res;
	antiAirTech = updateResearchList(subPersonalities[personality].antiAir.defenses);
	antiAirExtras = updateResearchList(subPersonalities[personality].antiAir.extras);
	extremeLaserTech = updateResearchList(_WEAPON_STATS.AS.weapons);
	extremeLaserExtra = updateResearchList(_WEAPON_STATS.AS.extras);
	laserTech = updateResearchList(_WEAPON_STATS.lasers.weapons);
	laserExtra = updateResearchList(_WEAPON_STATS.lasers.extras);
	weaponTech = updateResearchList(subPersonalities[personality].primaryWeapon.weapons);
	artilleryTech = updateResearchList(subPersonalities[personality].artillery.weapons);
	artillExtra = updateResearchList(subPersonalities[personality].artillery.extras);
	extraTech = updateResearchList(subPersonalities[personality].primaryWeapon.extras);
	secondaryWeaponTech = updateResearchList(subPersonalities[personality].secondaryWeapon.weapons);
	secondaryWeaponExtra = updateResearchList(subPersonalities[personality].secondaryWeapon.extras);
	defenseTech = updateResearchList(subPersonalities[personality].artillery.defenses);
	standardDefenseTech = updateResearchList(subPersonalities[personality].primaryWeapon.defenses);
	cyborgWeaps = updateResearchList(subPersonalities[personality].primaryWeapon.templates);
	machinegunWeaponTech = updateResearchList(_WEAPON_STATS.machineguns.weapons);
	machinegunWeaponExtra = updateResearchList(_WEAPON_STATS.machineguns.extras);
	empWeapons = updateResearchList(_WEAPON_STATS.nexusTech.weapons);
}

function isPowerResearch(research)
{
	return research.indexOf("R-Struc-Power-Upgrade") !== -1;
}

function checkTopicCurrentlyAvailable(research)
{
	function uncached(research)
	{
		const _availResearch = enumResearch();

		for (let i = 0, len = _availResearch.length; i < len; ++i)
		{
			if (_availResearch[i].id === research)
			{
				return true;
			}
		}

		return false;
	}

	return cacheThis(uncached, [research], "checkTopicCurrentlyAvailable" + me + research, 30000);
}

//This function aims to more cleanly discover available research topics
//with the given list provided. pursueResearch falls short in that it fails to
//acknowledge the availability of an item further into the list if a previous
//one is not completed... so lets help it a bit.
function evalResearch(lab, list, onlyIfCurrentlyAvailable)
{
	if (!isDefined(onlyIfCurrentlyAvailable))
	{
		onlyIfCurrentlyAvailable = false;
	}

	const __sufficientPower = getRealPower() > 2500;

	for (let i = 0, a = list.length; i < a; ++i)
	{
		if (__sufficientPower && isPowerResearch(list[i]))
		{
			//Don't research power upgrades if we have an absurd amount of power.
			continue;
		}

		if (onlyIfCurrentlyAvailable && !checkTopicCurrentlyAvailable(list[i]))
		{
			continue;
		}

		if (pursueResearch(lab, list[i]))
		{
			return true;
		}
	}

	return false;
}

// Funky time magic that seems to yield good times to allow research of non-grey bodies.
function timeToResearchAdvancedBody()
{
	let time = 0;

	switch (getMultiTechLevel())
	{
		case 1:
			time = 600000;
			if (baseType === CAMP_BASE)
			{
				time = 420000;
			}
			else if (baseType === CAMP_WALLS)
			{
				time = 280000;
			}
			break;
		case 2:
			time = 180000;
			break;
		case 3:
			time = 120000;
			break;
		default:
			time = 900000;
	}

	if (playerAlliance(true).length)
	{
		time = Math.floor(time / 2);
	}

	return time;
}

// Checks to make sure Cobra isn't going crazy with too many alloys, engine, body, material upgrades.
// Limits that to two labs at any one given time (except for the initial basic alloys and bodies in the beginning).
function atGenericDefensiveResearchLimit()
{
	if (playerAlliance(true).length)
	{
		return false;
	}

	const __limit = 2;
	const _limitTest = [
		"R-Vehicle-Metals01", "R-Vehicle-Metals02", "R-Vehicle-Metals03",
		"R-Vehicle-Metals04", "R-Vehicle-Metals05", "R-Vehicle-Metals06",
		"R-Vehicle-Metals07", "R-Vehicle-Metals08", "R-Vehicle-Metals09",
		"R-Cyborg-Metals01", "R-Cyborg-Metals02", "R-Cyborg-Metals03",
		"R-Cyborg-Metals04", "R-Cyborg-Metals05", "R-Cyborg-Metals06",
		"R-Cyborg-Metals07", "R-Cyborg-Metals08", "R-Cyborg-Metals09",
		"R-Vehicle-Armor-Heat01", "R-Vehicle-Armor-Heat02", "R-Vehicle-Armor-Heat03",
		"R-Vehicle-Armor-Heat04", "R-Vehicle-Armor-Heat05", "R-Vehicle-Armor-Heat06",
		"R-Vehicle-Armor-Heat07", "R-Vehicle-Armor-Heat08", "R-Vehicle-Armor-Heat09",
		"R-Cyborg-Armor-Heat01", "R-Cyborg-Armor-Heat02", "R-Cyborg-Armor-Heat03",
		"R-Cyborg-Armor-Heat04", "R-Cyborg-Armor-Heat05", "R-Cyborg-Armor-Heat06",
		"R-Cyborg-Armor-Heat07", "R-Cyborg-Armor-Heat08", "R-Cyborg-Armor-Heat09",
		"R-Vehicle-Engine01", "R-Vehicle-Engine02", "R-Vehicle-Engine03",
		"R-Vehicle-Engine04", "R-Vehicle-Engine05", "R-Vehicle-Engine06",
		"R-Vehicle-Engine07", "R-Vehicle-Engine08", "R-Vehicle-Engine09",
		"R-Vehicle-Body01", "R-Vehicle-Body02", "R-Vehicle-Body03",
		"R-Vehicle-Body04", "R-Vehicle-Body05", "R-Vehicle-Body06",
		"R-Vehicle-Body07", "R-Vehicle-Body08", "R-Vehicle-Body09",
		"R-Vehicle-Body10", "R-Vehicle-Body11", "R-Vehicle-Body12",
		"R-Vehicle-Body13", "R-Vehicle-Body14",
		"R-Struc-Materials01", "R-Struc-Materials02", "R-Struc-Materials03",
		"R-Defense-WallUpgrade01", "R-Defense-WallUpgrade02", "R-Defense-WallUpgrade03",
		"R-Defense-WallUpgrade04", "R-Defense-WallUpgrade05", "R-Defense-WallUpgrade06",
		"R-Defense-WallUpgrade07", "R-Defense-WallUpgrade08", "R-Defense-WallUpgrade09",
		"R-Defense-WallUpgrade10", "R-Defense-WallUpgrade11", "R-Defense-WallUpgrade12",
		"R-Sys-Engineering01", "R-Sys-Engineering02", "R-Sys-Engineering03"
	];
	let count = 0;

	for (let i = 0, len = _limitTest.length; i < len; ++i)
	{
		const __test = _limitTest[i];

		if (getResearch(__test, me).started)
		{
			++count;

			if (count == __limit)
			{
				break;
			}
		}
	}

	return count >= __limit;
}

function secondaryResPath()
{
	const __len = subPersonalities[personality].primaryWeapon.weapons.length - 1;

	if (componentAvailable(subPersonalities[personality].primaryWeapon.weapons[__len].stat))
	{
		const _cyborgSecondary = updateResearchList(subPersonalities[personality].secondaryWeapon.templates);

		if ((!turnOffCyborgs && _cyborgSecondary.length && pursueResearch(resObj.lab, _cyborgSecondary)) ||
			evalResearch(resObj.lab, secondaryWeaponExtra) ||
			evalResearch(resObj.lab, secondaryWeaponTech))
		{
			return true;
		}
	}
}

function machinegunResPath()
{
	if (resObj.cybCheck && chance(resObj.antiPersonnelChance))
	{
		if (resObj.forceLaser && laserResPath())
		{
			return true;
		}

		if (firstRocketLikeArtilleryAvailable())
		{
			if (evalResearch(resObj.lab, artilleryTech) || evalResearch(resObj.lab, artillExtra))
			{
				return true;
			}
		}
		else if (evalResearch(resObj.lab, machinegunWeaponTech) || evalResearch(resObj.lab, machinegunWeaponExtra))
		{
			return true;
		}
	}

	return false;
}

function genericResPath()
{
	if (subPersonalities[personality].resPath !== "generic")
	{
		return false;
	}


	if (machinegunResPath())
	{
		return true;
	}

	if ((chance(50) && evalResearch(resObj.lab, weaponTech)) ||
		(!turnOffCyborgs && chance(cyborgOnlyGame ? 75 : 50) && evalResearch(resObj.lab, cyborgWeaps)) ||
		(chance(20) && personalityIsRocketMain() && pursueResearch(resObj.lab, "R-Wpn-Rocket03-HvAT")))
	{
		return true;
	}

	if (useVtol && chance(70))
	{
		// Basic VTOL pads tried after Python.
		if ((componentAvailable("Body11ABT") && chance(70) && pursueResearch(resObj.lab, "R-Struc-VTOLPad-Upgrade01")) ||
			(componentAvailable("V-Tol") && chance(subPersonalities[personality].vtolPriority) && evalResearch(resObj.lab, _VTOL_RES)))
		{
			return true;
		}
	}

	if ((chance(33) && evalResearch(resObj.lab, extraTech)) ||
		(useArti && chance(personalityIsRocketMain() ? (componentAvailable("Missile-A-T") ? 50 : 20) : 30) && evalResearch(resObj.lab, artilleryTech)) ||
		(useArti && chance(40) && evalResearch(resObj.lab, artillExtra)))
	{
		return true;
	}

	if (evalResearch(resObj.lab, _SYSTEM_UPGRADES) ||
		evalResearch(resObj.lab, _SENSOR_TECH) ||
		secondaryResPath())
	{
		return true;
	}

	return false;
}

function defensiveResPath()
{
	if (subPersonalities[personality].resPath !== "defensive")
	{
		return false;
	}

	if (((chance(personalityIsRocketMain() ? (componentAvailable("Missile-A-T") ? 60 : 20) : 50) && useArti && evalResearch(resObj.lab, artilleryTech))) ||
		((chance(personalityIsRocketMain() ? 20 : 50) && useArti) && evalResearch(resObj.lab, artillExtra)) ||
		(chance(15) && evalResearch(resObj.lab, _SYSTEM_UPGRADES)) ||
		(chance(15) && evalResearch(resObj.lab, _SENSOR_TECH)))
	{
		return true;
	}

	if (machinegunResPath())
	{
		return true;
	}

	if ((useVtol && chance(60)) &&
		((componentAvailable("Body11ABT") && pursueResearch(resObj.lab, "R-Struc-VTOLPad-Upgrade01")) ||
		(componentAvailable("V-Tol") && chance(subPersonalities[personality].vtolPriority) && evalResearch(resObj.lab, _VTOL_RES))))
	{
		return true; // Basic VTOL pads tried after Python.
	}

	if (evalResearch(resObj.lab, weaponTech) ||
		(!turnOffCyborgs && evalResearch(resObj.lab, cyborgWeaps)) ||
		(personalityIsRocketMain() && pursueResearch(resObj.lab, "R-Wpn-Rocket03-HvAT")) ||
		evalResearch(resObj.lab, extraTech) ||
		secondaryResPath())
	{
		return true;
	}

	return false;
}

function offensiveResPath()
{
	if (subPersonalities[personality].resPath !== "offensive")
	{
		return false;
	}

	if (machinegunResPath())
	{
		return true;
	}

	if ((chance(40) && evalResearch(resObj.lab, weaponTech)) ||
		(!turnOffCyborgs && getResearch("R-Struc-Research-Upgrade04").done && chance(cyborgOnlyGame ? 75 : 30) && evalResearch(resObj.lab, cyborgWeaps)) ||
		(chance(10) && personalityIsRocketMain() && pursueResearch(resObj.lab, "R-Wpn-Rocket03-HvAT")) ||
		(chance(60) && evalResearch(resObj.lab, extraTech)) ||
		(useArti && chance(personalityIsRocketMain() ? (componentAvailable("Missile-A-T") ? 33 : 15) : 10) && evalResearch(resObj.lab, artilleryTech)))
	{
		return true;
	}

	if ((useVtol && chance(80)) &&
		((componentAvailable("Body11ABT") && chance(80) && pursueResearch(resObj.lab, "R-Struc-VTOLPad-Upgrade01")) ||
		(componentAvailable("V-Tol") && chance(subPersonalities[personality].vtolPriority + 10) && evalResearch(resObj.lab, _VTOL_RES))))
	{
		return true; // Basic VTOL pads tried after Python.
	}

	if (evalResearch(resObj.lab, _SYSTEM_UPGRADES) ||
		(useArti && chance(66) && evalResearch(resObj.lab, artilleryTech)) ||
		(useArti && chance(50) && evalResearch(resObj.lab, artillExtra)))
	{
		return true;
	}

	if (secondaryResPath() || evalResearch(resObj.lab, _SENSOR_TECH))
	{
		return true;
	}

	return false;
}

function airResPath()
{
	if (subPersonalities[personality].resPath !== "air")
	{
		return false;
	}

	// If for some reason the air personality has VTOL use disabled... enable it.
	if (!useVtol)
	{
		useVtol = true;
	}

	if (evalResearch(resObj.lab, _SYSTEM_UPGRADES) || evalResearch(resObj.lab, _VTOL_RES))
	{
		return true;
	}

	if (machinegunResPath())
	{
		return true;
	}

	if (evalResearch(resObj.lab, weaponTech) ||
		(!turnOffCyborgs && chance(cyborgOnlyGame ? 75 : 50) && evalResearch(resObj.lab, cyborgWeaps)) ||
		(chance(50) && evalResearch(resObj.lab, extraTech)) ||
		(personalityIsRocketMain() && pursueResearch(resObj.lab, "R-Wpn-Rocket03-HvAT")))
	{
		return true;
	}

	if (useArti &&
		((chance(personalityIsRocketMain() ? (componentAvailable("Missile-A-T") ? 50 : 20) : 50) && evalResearch(resObj.lab, artilleryTech)) ||
		(chance(66) && evalResearch(resObj.lab, artillExtra))))
	{
		return true;
	}

	if (secondaryResPath() || evalResearch(resObj.lab, _SENSOR_TECH))
	{
		return true;
	}

	return false;
}

function megaEssentialsResPath()
{
	if ((forceHover && pursueResearch(resObj.lab, "R-Vehicle-Prop-Hover")) ||
		(enemyUsedElectronicWarfare && pursueResearch(resObj.lab, "R-Sys-Resistance-Circuits")))
	{
		return true;
	}

	return false;
}

function essentialsResPath()
{
	if ((resObj.isHighOil && evalResearch(resObj.lab, _MODULE_RESEARCH)) ||
		evalResearch(resObj.lab, _MOST_ESSENTIAL) ||
		evalResearch(resObj.lab, _ESSENTIALS) ||
		evalResearch(resObj.lab, techlist))
	{
		return true;
	}
	if (chance((resObj.hasAlly || resObj.isHighOil || (gameTime > 2400000)) ? 60 : 20))
	{
		if ((getResearch("R-Struc-Research-Upgrade02").done && pursueResearch(resObj.lab, "R-Vehicle-Body11")) ||
			(chance((resObj.isHighOil || resObj.hasAlly) ? 75 : 33) && evalResearch(resObj.lab, _ESSENTIALS_2)) ||
			(chance(getResearch("R-Struc-Research-Upgrade04").done ? 40 : 15) && evalResearch(resObj.lab, _ESSENTIALS_3)))
		{
			return true;
		}
	}

	return false;
}

function structureDefenseResPath()
{
	const __superDefense = ((subPersonalities[personality].resPath === "defensive") || (subPersonalities[personality].defensePriority >= 75));

	if (__superDefense || resObj.forceDefenseRes || ((chance(2) || !resObj.defensiveLimit) && chance(subPersonalities[personality].defensePriority)))
	{
		if ((!resObj.isHighOil || resObj.hasAlly || __superDefense || resObj.forceDefenseRes) &&
			evalResearch(resObj.lab, standardDefenseTech, chance(resObj.forceDefenseRes ? 0 : 85)) ||
			(useArti && evalResearch(resObj.lab, defenseTech, chance(resObj.forceDefenseRes ? 0 : 85))) ||
			(chance(resObj.forceDefenseRes ? 7 : 100) && evalResearch(resObj.lab, _DEFENSE_UPGRADES, true)))
		{
			if (resObj.forceDefenseRes)
			{
				resObj.forceDefenseRes = false;
			}

			return true;
		}
	}

	if (chance(__superDefense ? 10 : 2))
	{
		resObj.forceDefenseRes = true;
	}

	return false;
}

function resistanceResPath()
{
	if ((resObj.hasAlly && resObj.isHighOil) || chance(resObj.isHighOil ? 45 : 20))
	{
		if ((!cyborgOnlyGame && pursueResearch(resObj.lab, "R-Vehicle-Metals03")) ||
			(!turnOffCyborgs && (countStruct(_STRUCTURES.cyborgFactory, me) || cyborgOnlyGame) && pursueResearch(resObj.lab, "R-Cyborg-Metals03")))
		{
			return true;
		}

		if (!resObj.defensiveLimit && ((gameTime > timeToResearchAdvancedBody()) || cyborgOnlyGame || resObj.isHighOil))
		{
			if (chance(subPersonalities[personality].alloyPriority + (resObj.isHighOil ? 20 : 0) + ((resObj.hasAlly && resObj.isHighOil) ? 30 : 0)))
			{
				if (!cyborgOnlyGame && getResearch("R-Struc-Research-Upgrade03").done && chance(40) && pursueResearch(resObj.lab, "R-Vehicle-Metals05"))
				{
					return true;
				}

				if (!turnOffCyborgs && countStruct(_STRUCTURES.cyborgFactory, me) && chance(cyborgOnlyGame ? 75 : 50))
				{
					if (evalResearch(resObj.lab, _CYBORG_ARMOR) ||
						(chance(15) && getResearch("R-Cyborg-Metals04").done && evalResearch(resObj.lab, _CYBORG_ARMOR_THERMAL)))
					{
						return true;
					}
				}

				if (!cyborgOnlyGame || (cyborgOnlyGame && chance(20)))
				{
					if (evalResearch(resObj.lab, _TANK_ARMOR) ||
						(chance(15) && getResearch("R-Vehicle-Metals04").done && evalResearch(resObj.lab, _TANK_ARMOR_THERMAL)))
					{
						return true;
					}
				}
			}

			if (getResearch("R-Struc-Research-Upgrade03").done && chance(componentAvailable("Body8MBT") ? 45 : 30))
			{
				return evalResearch(resObj.lab, _BODY_RESEARCH);
			}
		}
	}

	return false;
}

function antiAirResPath()
{
	if ((getRealPower() > (resObj.isHighOil ? resObj.highOilResPrice : -__SUPER_LOW_POWER)) &&
		(countEnemyVTOL() || componentAvailable("V-Tol")))
	{
		// Prepare the most basic AA defense.
		if (antiAirTech.length && pursueResearch(resObj.lab, antiAirTech[0]))
		{
			return true;
		}

		if (chance(30) && countEnemyVTOL())
		{
			if (evalResearch(resObj.lab, antiAirTech) ||
				evalResearch(resObj.lab, antiAirExtras))
			{
				return true;
			}
		}
	}

	return false;
}

function laserResPath()
{
	if (resObj.forceLaser || subPersonalities[personality].useLasers)
	{
		if ((!turnOffCyborgs && pursueResearch(resObj.lab, "R-Cyborg-Hvywpn-PulseLsr")) ||
			(evalResearch(resObj.lab, laserTech) || evalResearch(resObj.lab, laserExtra)) ||
			(returnAntiAirAlias() !== "rktaa")) //Rocket/missile AA does not need this. Still uses it if researched.
		{
			return pursueResearch(resObj.lab, "R-Defense-AA-Laser");
		}
	}

	return false;
}

//Careful not to focus too much on these research topics since offensive capability can be harmed
function specialResPath()
{
	//Tiny chance to get one of these early.
	if (getResearch("R-Struc-Research-Upgrade04").done &&
		chance(3) &&
		(evalResearch(resObj.lab, _SYSTEM_UPGRADES) || evalResearch(resObj.lab, _SENSOR_TECH)))
	{
		return true;
	}

	if (!cyborgOnlyGame && getResearch("R-Struc-Research-Upgrade05").done && chance(10))
	{
		if (pursueResearch(resObj.lab, extremeLaserTech) ||
			(componentAvailable("PlasmaHeavy") && chance(70) && evalResearch(resObj.lab, _FLAMER)) ||
			(componentAvailable("Laser4-PlasmaCannon") && (evalResearch(resObj.lab, empWeapons) || (chance(15) && evalResearch(resObj.lab, extremeLaserExtra)))))
		{
			return true;
		}
	}

	return false;
}

//Very likely going to be done with research by now.
function finalResPath()
{
	if (researchComplete)
	{
		return true;
	}

	if ((getMultiTechLevel() === 4) ||
		(componentAvailable("Body14SUP") &&
		componentAvailable("EMP-Cannon") &&
		isStructureAvailable(_STRUCTURES.lassat) &&
		getResearch("R-Defense-WallUpgrade12", me).done &&
		!enumResearch().length))
	{
		researchComplete = true;
		return true;
	}

	return false;
}

function setAntiCyborgChance()
{
	const __enemyPlayer = getMostHarmfulPlayer();
	let antiCyborgChance = Math.max(10, Math.floor(playerCyborgRatio(__enemyPlayer) * 100));

	if (!startAttacking || (isDefined(scavengerPlayer) && (__enemyPlayer === scavengerPlayer)))
	{
		antiCyborgChance = 35;
	}

	resObj.antiPersonnelChance = antiCyborgChance;
}

// Passing an array of functions here will run through them all until one returns true and skips the rest.
function resExecuteFuncList(list)
{
	if (!isDefined(list))
	{
		return false;
	}

	for (let i = 0, len = list.length; i < len; ++i)
	{
		if (list[i]()) // Call function in list.
		{
			return true;
		}
	}

	return false;
}

function cobraDoResearch()
{
	if (currently_dead ||
		!countDroid(DROID_CONSTRUCT, me) ||
		!(isDefined(techlist) && isDefined(turnOffCyborgs)))
	{
		return;
	}

	const _labList = enumStruct(me, _STRUCTURES.lab).filter((lb) => (
		lb.status === BUILT && structureIdle(lb)
	)).sort(function(obj1, obj2) {
		return obj1.modules - obj2.modules;
	}).reverse();

	setAntiCyborgChance();
	resObj.isHighOil = highOilMap();
	resObj.hasAlly = (playerAlliance(true).length > 0);
	resObj.highOilResPrice = -200;
	resObj.cybCheck = (!turnOffMG || cyborgOnlyGame);

	if (!resObj.forceLaser && useLasersForCyborgControl())
	{
		resObj.forceLaser = true;
	}

	for (let i = 0, a = _labList.length; i < a; ++i)
	{
		resObj.defensiveLimit = atGenericDefensiveResearchLimit();
		resObj.lab = _labList[i];

		if (!strangeStartSettingOver() && (i >= 2))
		{
			break;
		}

		if (resExecuteFuncList([megaEssentialsResPath, essentialsResPath, antiAirResPath]))
		{
			continue;
		}

		if (getRealPower() < ((gameTime < 180000) ? __MIN_POWER : (resObj.isHighOil ? resObj.highOilResPrice : __SUPER_LOW_POWER)))
		{
			continue;
		}

		if (resExecuteFuncList([resistanceResPath, structureDefenseResPath, specialResPath,
			genericResPath, defensiveResPath, offensiveResPath, airResPath, laserResPath,
			finalResPath]))
		{
			continue;
		}
	}
}
