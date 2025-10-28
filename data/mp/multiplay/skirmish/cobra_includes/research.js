
function eventResearched(research, structure, player)
{
	if (LOG_RESEARCH_PATH && allianceExistsBetween(me, player))
	{
		dump(research.name);
	}
}

//updates a research list with whatever is passed to it.
function updateResearchList(stat, len)
{
	if (!isDefined(len))
	{
		len = 0;
	}

	let list = [];

	for (let x = 0, d = stat.length - len; x < d; ++x)
	{
		let st = stat[x];

		if (isDefined(st.res))
		{
			list.push(st.res);
		}
		else
		{
			list.push(st);
		}
	}

	return list;
}

//Initialization of research lists when eventStartLevel is triggered.
//Call this again when manually changing a personality.
function initializeResearchLists()
{
	techlist = subPersonalities[personality].res;
	antiAirTech = updateResearchList(subPersonalities[personality].antiAir.defenses);
	antiAirExtras = updateResearchList(subPersonalities[personality].antiAir.extras);
	extremeLaserTech = updateResearchList(weaponStats.AS.weapons);
	extremeLaserExtra = updateResearchList(weaponStats.AS.extras);
	laserTech = updateResearchList(weaponStats.lasers.weapons);
	laserExtra = updateResearchList(weaponStats.lasers.extras);
	weaponTech = updateResearchList(subPersonalities[personality].primaryWeapon.weapons);
	artilleryTech = updateResearchList(subPersonalities[personality].artillery.weapons);
	artillExtra = updateResearchList(subPersonalities[personality].artillery.extras);
	extraTech = updateResearchList(subPersonalities[personality].primaryWeapon.extras);
	secondaryWeaponTech = updateResearchList(subPersonalities[personality].secondaryWeapon.weapons);
	secondaryWeaponExtra = updateResearchList(subPersonalities[personality].secondaryWeapon.extras);
	defenseTech = updateResearchList(subPersonalities[personality].artillery.defenses);
	standardDefenseTech = updateResearchList(subPersonalities[personality].primaryWeapon.defenses);
	cyborgWeaps = updateResearchList(subPersonalities[personality].primaryWeapon.templates);
	machinegunWeaponTech = updateResearchList(weaponStats.machineguns.weapons);
	machinegunWeaponExtra = updateResearchList(weaponStats.machineguns.extras);
	empWeapons = updateResearchList(weaponStats.nexusTech.weapons);
}

function isPowerResearch(research)
{
	const POWERS = [
		"R-Struc-Power-Upgrade01",
		"R-Struc-Power-Upgrade01b",
		"R-Struc-Power-Upgrade01c",
		"R-Struc-Power-Upgrade02",
		"R-Struc-Power-Upgrade03",
		"R-Struc-Power-Upgrade03a",
	];

	for (let i = 0, len = POWERS.length; i < len; ++i)
	{
		if (research === POWERS[i])
		{
			return true;
		}
	}

	return false;
}

//This function aims to more cleanly discover available research topics
//with the given list provided. pursueResearch falls short in that it fails to
//acknowledge the availability of an item further into the list if a previous
//one is not completed... so lets help it a bit.
function evalResearch(lab, list)
{
	let sufficientPower = getRealPower() > 2500;

	for (let i = 0, a = list.length; i < a; ++i)
	{
		if (sufficientPower && isPowerResearch(list[i]))
		{
			//Don't research power upgrades if we have an absurd amount of power.
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

	if (playerAlliance(true).length > 0)
	{
		time = Math.floor(time / 2);
	}

	return time;
}

// Checks to make sure Cobra isn't going crazy with too many alloys, engine, body, material upgrades.
// Limits that to two labs at any one given time (except for the initial basic alloys and bodies in the beginning).
function atGenericDefensiveResearchLimit()
{
	if (playerAlliance(true).length > 0)
	{
		return false;
	}

	const LIMIT = 2;
	const LIMIT_TESTS = [
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

	for (let i = 0, len = LIMIT_TESTS.length; i < len; ++i)
	{
		let test = LIMIT_TESTS[i];

		if (getResearch(test, me).started)
		{
			++count;

			if (count == LIMIT)
			{
				break;
			}
		}
	}

	return count >= LIMIT;
}

function secondaryResPath()
{
	const len = subPersonalities[personality].primaryWeapon.weapons.length - 1;

	if (componentAvailable(subPersonalities[personality].primaryWeapon.weapons[len].stat))
	{
		const lab = resObj.lab;
		const cyborgSecondary = updateResearchList(subPersonalities[personality].secondaryWeapon.templates);

		if ((!turnOffCyborgs && (cyborgSecondary.length > 0) && pursueResearch(lab, cyborgSecondary)) ||
			evalResearch(lab, secondaryWeaponExtra) ||
			evalResearch(lab, secondaryWeaponTech))
		{
			return true;
		}
	}
}

function genericResPath()
{
	if (subPersonalities[personality].resPath !== "generic")
	{
		return false;
	}

	const lab = resObj.lab;

	if (resObj.cybCheck && chance(resObj.antiPersonnelChance))
	{
		if ((resObj.forceLaser && laserResPath()) ||
			evalResearch(lab, machinegunWeaponTech) ||
			evalResearch(lab, machinegunWeaponExtra))
		{
			return true;
		}
	}

	if ((chance(50) && evalResearch(lab, weaponTech)) ||
		(chance(20) && personalityIsRocketMain() && pursueResearch(lab, "R-Wpn-Rocket03-HvAT")) ||
		(!turnOffCyborgs && chance(cyborgOnlyGame ? 75 : 50) && evalResearch(lab, cyborgWeaps)))
	{
		return true;
	}

	if (useVtol && chance(70))
	{
		// Basic VTOL pads tried after Python.
		if ((componentAvailable("Body11ABT") && chance(70) && pursueResearch(lab, "R-Struc-VTOLPad-Upgrade01")) ||
			(componentAvailable("V-Tol") && chance(subPersonalities[personality].vtolPriority) && evalResearch(lab, VTOL_RES)))
		{
			return true;
		}
	}

	if ((chance(33) && evalResearch(lab, extraTech)) ||
		(useArti && chance(40) && evalResearch(lab, artillExtra)) ||
		(useArti && chance(personalityIsRocketMain() ? (componentAvailable("Missile-A-T") ? 50 : 20) : 30) && evalResearch(lab, artilleryTech)))
	{
		return true;
	}

	if (evalResearch(lab, SYSTEM_UPGRADES) ||
		evalResearch(lab, SENSOR_TECH) ||
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

	const lab = resObj.lab;

	if (((chance(personalityIsRocketMain() ? 20 : 50) && useArti) && evalResearch(lab, artillExtra)) ||
		((chance(personalityIsRocketMain() ? (componentAvailable("Missile-A-T") ? 60 : 20) : 50) && useArti && evalResearch(lab, artilleryTech))) ||
		evalResearch(lab, SYSTEM_UPGRADES) ||
		evalResearch(lab, SENSOR_TECH))
	{
		return true;
	}

	if ((resObj.cybCheck && chance(resObj.antiPersonnelChance)) &&
		((resObj.forceLaser && laserResPath()) ||
		evalResearch(lab, machinegunWeaponTech) ||
		evalResearch(lab, machinegunWeaponExtra)))
	{
		return true;
	}

	if ((useVtol && chance(60)) &&
		((componentAvailable("Body11ABT") && pursueResearch(lab, "R-Struc-VTOLPad-Upgrade01")) ||
		(componentAvailable("V-Tol") && chance(subPersonalities[personality].vtolPriority) && evalResearch(lab, VTOL_RES))))
	{
		return true; // Basic VTOL pads tried after Python.
	}

	if (evalResearch(lab, weaponTech) ||
		(personalityIsRocketMain() && pursueResearch(lab, "R-Wpn-Rocket03-HvAT")) ||
		(!turnOffCyborgs && evalResearch(lab, cyborgWeaps)) ||
		evalResearch(lab, extraTech) ||
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

	const lab = resObj.lab;

	if ((resObj.cybCheck && chance(resObj.antiPersonnelChance)) &&
		((resObj.forceLaser && laserResPath()) ||
		evalResearch(lab, machinegunWeaponTech) ||
		evalResearch(lab, machinegunWeaponExtra)))
	{
		return true;
	}

	if ((chance(40) && evalResearch(lab, weaponTech)) ||
		(!turnOffCyborgs && getResearch("R-Struc-Research-Upgrade04").done && chance(cyborgOnlyGame ? 75 : 30) && evalResearch(lab, cyborgWeaps)) ||
		(chance(60) && evalResearch(lab, extraTech)) ||
		(chance(10) && personalityIsRocketMain() && pursueResearch(lab, "R-Wpn-Rocket03-HvAT")) ||
		(useArti && chance(personalityIsRocketMain() ? (componentAvailable("Missile-A-T") ? 33 : 15) : 10) && evalResearch(lab, artilleryTech)))
	{
		return true;
	}

	if ((useVtol && chance(80)) &&
		((componentAvailable("Body11ABT") && chance(80) && pursueResearch(lab, "R-Struc-VTOLPad-Upgrade01")) ||
		(componentAvailable("V-Tol") && chance(subPersonalities[personality].vtolPriority + 10) && evalResearch(lab, VTOL_RES))))
	{
		return true; // Basic VTOL pads tried after Python.
	}

	if (evalResearch(lab, SYSTEM_UPGRADES) ||
		(useArti && chance(66) && evalResearch(lab, artilleryTech)) ||
		(useArti && chance(50) && evalResearch(lab, artillExtra)))
	{
		return true;
	}

	if (secondaryResPath() ||
		evalResearch(lab, SENSOR_TECH))
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

	const lab = resObj.lab;
	// If for some reason the air personality has VTOL use disabled... enable it.
	if (!useVtol)
	{
		useVtol = true;
	}

	if (evalResearch(lab, SYSTEM_UPGRADES) || evalResearch(lab, VTOL_RES))
	{
		return true;
	}

	if ((resObj.cybCheck && chance(resObj.antiPersonnelChance)) &&
		((resObj.forceLaser && laserResPath()) ||
		evalResearch(lab, machinegunWeaponTech) ||
		evalResearch(lab, machinegunWeaponExtra)))
	{
		return true;
	}

	if (evalResearch(lab, weaponTech) ||
		(personalityIsRocketMain() && pursueResearch(lab, "R-Wpn-Rocket03-HvAT")) ||
		(!turnOffCyborgs && chance(cyborgOnlyGame ? 75 : 50) && evalResearch(lab, cyborgWeaps)) ||
		(chance(50) && evalResearch(lab, extraTech)) ||
		evalResearch(lab, SENSOR_TECH))
	{
		return true;
	}

	if (useArti &&
		((chance(66) && evalResearch(lab, artillExtra)) ||
		(chance(personalityIsRocketMain() ? (componentAvailable("Missile-A-T") ? 50 : 20) : 50) && evalResearch(lab, artilleryTech))))
	{
		return true;
	}

	if (secondaryResPath())
	{
		return true;
	}

	return false;
}

function megaEssentialsResPath()
{
	const lab = resObj.lab;

	if ((forceHover && pursueResearch(lab, "R-Vehicle-Prop-Hover")) ||
		(enemyUsedElectronicWarfare && pursueResearch(lab, "R-Sys-Resistance-Circuits")))
	{
		return true;
	}

	return false;
}

function essentialsResPath()
{
	const lab = resObj.lab;

	if ((resObj.isHighOil && evalResearch(lab, MODULE_RESEARCH)) ||
		evalResearch(lab, MOST_ESSENTIAL) ||
		evalResearch(lab, ESSENTIALS) ||
		evalResearch(lab, techlist))
	{
		return true;
	}

	return false;
}

function structureDefenseResPath()
{
	if (((subPersonalities[personality].resPath === "defensive") ||
		(!resObj.defensiveLimit && chance(subPersonalities[personality].defensePriority))))
	{
		const lab = resObj.lab;

		if ((!resObj.isHighOil || resObj.hasAlly) &&
			(evalResearch(lab, standardDefenseTech) ||
			(useArti && evalResearch(lab, defenseTech)) ||
			evalResearch(lab, DEFENSE_UPGRADES)))
		{
			return true;
		}
	}

	return false;
}

function resistanceResPath()
{
	const lab = resObj.lab;

	if ((resObj.hasAlly && resObj.isHighOil) || chance(resObj.isHighOil ? 45 : 20))
	{
		if ((!cyborgOnlyGame && pursueResearch(lab, "R-Vehicle-Metals03")) ||
			(!turnOffCyborgs && (countStruct(structures.cyborgFactory, me) || cyborgOnlyGame) && pursueResearch(lab, "R-Cyborg-Metals03")))
		{
			return true;
		}

		if (!resObj.defensiveLimit && ((gameTime > timeToResearchAdvancedBody()) || cyborgOnlyGame || resObj.isHighOil))
		{
			if (chance(subPersonalities[personality].alloyPriority + (resObj.isHighOil ? 20 : 0) + ((resObj.hasAlly && resObj.isHighOil) ? 30 : 0)))
			{
				if (!cyborgOnlyGame && getResearch("R-Struc-Research-Upgrade03").done && chance(40) && pursueResearch(lab, "R-Vehicle-Metals05"))
				{
					return true;
				}

				if (!turnOffCyborgs && countStruct(structures.cyborgFactory, me) && chance(cyborgOnlyGame ? 75 : 50))
				{
					if (evalResearch(lab, CYBORG_ARMOR) ||
						(chance(15) && getResearch("R-Cyborg-Metals04").done && evalResearch(lab, CYBORG_ARMOR_THERMAL)))
					{
						return true;
					}
				}

				if (!cyborgOnlyGame || (cyborgOnlyGame && chance(20)))
				{
					if (evalResearch(lab, TANK_ARMOR) ||
						(chance(15) && getResearch("R-Vehicle-Metals04").done && evalResearch(lab, TANK_ARMOR_THERMAL)))
					{
						return true;
					}
				}
			}

			if (getResearch("R-Struc-Research-Upgrade03").done && chance(componentAvailable("Body8MBT") ? 45 : 30))
			{
				return evalResearch(lab, BODY_RESEARCH);
			}
		}
	}

	return false;
}

function antiAirResPath()
{
	if ((getRealPower() > (resObj.isHighOil ? resObj.highOilResPrice : -SUPER_LOW_POWER)) &&
		(countEnemyVTOL() || componentAvailable("V-Tol")))
	{
		const lab = resObj.lab;
		// Prepare the most basic AA defense.
		if ((antiAirTech.length > 0) && pursueResearch(lab, antiAirTech[0]))
		{
			return true;
		}

		if (chance(30) && countEnemyVTOL())
		{
			if (evalResearch(lab, antiAirTech) ||
				evalResearch(lab, antiAirExtras))
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
		let foundLaser = false;
		const lab = resObj.lab;

		if (!turnOffCyborgs)
		{
			foundLaser = pursueResearch(lab, "R-Cyborg-Hvywpn-PulseLsr");
		}
		if (!foundLaser)
		{
			foundLaser = (evalResearch(lab, laserTech) || evalResearch(lab, laserExtra));
		}
		//Rocket/missile AA does not need this. Still uses it if researched.
		if (!foundLaser && (returnAntiAirAlias() !== "rktaa"))
		{
			return pursueResearch(lab, "R-Defense-AA-Laser");
		}
	}

	return false;
}

//Careful not to focus too much on these research topics since offensive capability can be harmed
function specialResPath()
{
	const lab = resObj.lab;

	if (chance((resObj.hasAlly || resObj.isHighOil || (gameTime > 2400000)) ? 60 : 20))
	{
		if ((getResearch("R-Struc-Research-Upgrade02").done && pursueResearch(lab, "R-Vehicle-Body11")) ||
			(chance((resObj.isHighOil || resObj.hasAlly) ? 75 : 33) && evalResearch(lab, ESSENTIALS_2)) ||
			(chance(getResearch("R-Struc-Research-Upgrade04").done ? 40 : 15) && evalResearch(lab, ESSENTIALS_3)))
		{
			return true;
		}
	}

	if (!cyborgOnlyGame && getResearch("R-Struc-Research-Upgrade05").done && chance(15))
	{
		if (pursueResearch(lab, extremeLaserTech) ||
			(componentAvailable("PlasmaHeavy") && chance(70) && evalResearch(lab, FLAMER)) ||
			(componentAvailable("Laser4-PlasmaCannon") && (evalResearch(lab, empWeapons) || evalResearch(lab, extremeLaserExtra))))
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
		isStructureAvailable(structures.lassat) &&
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
	const enemyPlayer = getMostHarmfulPlayer();
	let antiCyborgChance = Math.max(10, Math.floor(playerCyborgRatio(enemyPlayer) * 100));

	if (!startAttacking || (isDefined(scavengerPlayer) && (enemyPlayer === scavengerPlayer)))
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

function research()
{
	if (currently_dead ||
		!countDroid(DROID_CONSTRUCT, me) ||
		!(isDefined(techlist) && isDefined(turnOffCyborgs)))
	{
		return;
	}

	const labList = enumStruct(me, structures.lab).filter((lb) => (
		lb.status === BUILT && structureIdle(lb)
	));

	setAntiCyborgChance();
	resObj.isHighOil = highOilMap();
	resObj.hasAlly = (playerAlliance(true).length > 0);
	resObj.highOilResPrice = -200;
	resObj.cybCheck = (!turnOffMG || cyborgOnlyGame);

	if (!resObj.forceLaser && useLasersForCyborgControl())
	{
		resObj.forceLaser = true;
	}

	for (let i = 0, a = labList.length; i < a; ++i)
	{
		resObj.defensiveLimit = atGenericDefensiveResearchLimit();
		resObj.lab = labList[i];

		if (resExecuteFuncList([megaEssentialsResPath, essentialsResPath, antiAirResPath]))
		{
			continue;
		}

		if (getRealPower() < ((gameTime < 180000) ? MIN_POWER : (resObj.isHighOil ? resObj.highOilResPrice : SUPER_LOW_POWER)))
		{
			continue;
		}

		if (resExecuteFuncList([resistanceResPath, structureDefenseResPath, genericResPath,
			defensiveResPath, offensiveResPath, airResPath, specialResPath, laserResPath,
			finalResPath]))
		{
			continue;
		}
	}
}
