
//updates a research list with whatever is passed to it.
function updateResearchList(stat, len)
{
	if (!isDefined(len))
	{
		len = 0;
	}

	var list = [];
	for (var x = 0, d = stat.length - len; x < d; ++x)
	{
		isDefined(stat[x].res) ? list.push(stat[x].res) : list.push(stat[x]);
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
	extremeLaserTech = updateResearchList(weaponStats.AS.extras);
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

//This function aims to more cleanly discover available research topics
//with the given list provided. pursueResearch falls short in that it fails to
//acknowledge the availability of an item further into the list if a previous
//one is not completed... so lets help it a bit.
function evalResearch(lab, list)
{
	for (var i = 0, a = list.length; i < a; ++i)
	{
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
	var time = 0;

	switch (getMultiTechLevel())
	{
		case 1:
			time = 900000;
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

function research()
{
	if (currently_dead || !countDroid(DROID_CONSTRUCT) || !(isDefined(techlist) && isDefined(turnOffCyborgs)))
	{
		return;
	}

	var labList = enumStruct(me, structures.labs).filter(function(lb) {
		return (lb.status === BUILT && structureIdle(lb));
	});

	var antiCyborgChance = Math.floor(playerCyborgRatio(getMostHarmfulPlayer()) * 100);

	for (var i = 0, a = labList.length; i < a; ++i)
	{
		var lab = labList[i];
		var forceLaser = false;
		var found = false;

		if (forceHover)
			found = pursueResearch(lab, "R-Vehicle-Prop-Hover");

		if (!found)
			found = evalResearch(lab, ESSENTIALS);
		if (!found)
			found = evalResearch(lab, techlist);
		if (!found && random(100) < 20)
			found = evalResearch(lab, ESSENTIALS_2);
		if (!found && random(100) < 10)
			found = evalResearch(lab, ESSENTIALS_3);
		if (!found && componentAvailable("V-Tol"))
			found = evalResearch(lab, VTOL_ESSENTIALS);

		if (!found && getRealPower() > ((gameTime < 180000) ? MIN_POWER : SUPER_LOW_POWER))
		{
			if (gameTime > timeToResearchAdvancedBody() && random(100) < 20)
			{
				found = evalResearch(lab, BODY_RESEARCH_1);

				if (!found)
				{
					if (!turnOffCyborgs && countStruct(CYBORG_FACTORY))
					{
						// just in case cyborg armor was ignored too much. Likely
						// true for personalities without super cyborgs.
						found = pursueResearch(lab, "R-Cyborg-Metals03");
					}
					if (!found)
					{
						found = evalResearch(lab, BODY_RESEARCH_2);
					}
				}
			}

			if (subPersonalities[personality].resPath === "generic")
			{
				if (!found && random(100) < 33)
					found = evalResearch(lab, extraTech);

				if (random(100) < 40 && countEnemyVTOL())
				{
					if (!found)
						found = evalResearch(lab, antiAirTech);
					if (!found)
						found = evalResearch(lab, antiAirExtras);
				}

				if (!turnOffMG)
				{
					if (!found && random(100) < antiCyborgChance)
						found = evalResearch(lab, machinegunWeaponTech);
					if (!found && useArti && random(100) < antiCyborgChance)
						found = evalResearch(lab, machinegunWeaponExtra);
				}
				if (useLasersForCyborgControl() && random(100) < antiCyborgChance)
				{
					found = true;
					forceLaser = true;
				}

				if (!found && !turnOffCyborgs)
					found = evalResearch(lab, cyborgWeaps);
				if (!found && random(100) < 50)
					found = evalResearch(lab, weaponTech);
				if (!found && useArti && random(100) < 50)
					found = evalResearch(lab, artilleryTech);

				if (!found)
					found = evalResearch(lab, SYSTEM_UPGRADES);

				if (!found && (random(100) < subPersonalities[personality].alloyPriority))
				{
					if (!turnOffCyborgs && countStruct(CYBORG_FACTORY) && random(2))
						found = evalResearch(lab, CYBORG_ARMOR);
					if (!found)
						found = evalResearch(lab, TANK_ARMOR);
				}

				if (!found && useVtol && (random(100) < subPersonalities[personality].vtolPriority))
					found = evalResearch(lab, VTOL_RES);

				if (!found)
					found = evalResearch(lab, SENSOR_TECH);
				if (!found && useArti)
					found = evalResearch(lab, artillExtra);


				if (!found && (random(100) < subPersonalities[personality].defensePriority))
				{
					found = evalResearch(lab, standardDefenseTech);
					if (!found && useArti)
						found = evalResearch(lab, defenseTech);
					if (!found)
						found = evalResearch(lab, DEFENSE_UPGRADES);
				}

				var cyborgSecondary = updateResearchList(subPersonalities[personality].secondaryWeapon.templates);
				var len = subPersonalities[personality].primaryWeapon.weapons.length - 1;
				if (componentAvailable(subPersonalities[personality].primaryWeapon.weapons[len].stat))
				{
					if(!found && !turnOffCyborgs && cyborgSecondary.length > 0)
						found = pursueResearch(lab, cyborgSecondary);
					if(!found)
						found = evalResearch(lab, secondaryWeaponExtra);
					if(!found)
						found = evalResearch(lab, secondaryWeaponTech);
				}
			}
			else if (subPersonalities[personality].resPath === "defensive")
			{
				if (random(100) < 40 && countEnemyVTOL())
				{
					if (!found)
						found = evalResearch(lab, antiAirTech);
					if (!found)
						found = evalResearch(lab, antiAirExtras);
				}

				if (!found)
				{
					found = evalResearch(lab, DEFENSE_UPGRADES);
					if (!found && useArti)
						found = evalResearch(lab, defenseTech);
					if (!found)
						found = evalResearch(lab, standardDefenseTech);
				}

				if (!found && random(100) < 50 && useArti)
					found = evalResearch(lab, artillExtra);
				if (!found)
					found = evalResearch(lab, SYSTEM_UPGRADES);
				if (!found)
					found = evalResearch(lab, SENSOR_TECH);
				if (!found && useArti)
					found = evalResearch(lab, artilleryTech);


				if (!found)
				{
					if (!turnOffCyborgs && countStruct(CYBORG_FACTORY) && random(2))
					{
						found = evalResearch(lab, CYBORG_ARMOR);
					}
					if (!found)
						found = evalResearch(lab, TANK_ARMOR);
				}

				if (!found && useVtol && (random(100) < subPersonalities[personality].vtolPriority))
					found = evalResearch(lab, VTOL_RES);

				if (!turnOffMG)
				{
					if (!found && random(100) < antiCyborgChance)
						found = evalResearch(lab, machinegunWeaponTech);
					if (!found && useArti && random(100) < antiCyborgChance)
						found = evalResearch(lab, machinegunWeaponExtra);
				}
				if (useLasersForCyborgControl() && random(100) < antiCyborgChance)
				{
					found = true;
					forceLaser = true;
				}

				if (!found)
					found = evalResearch(lab, extraTech);
				if (!found && !turnOffCyborgs)
					found = evalResearch(lab, cyborgWeaps);
				if (!found)
					found = evalResearch(lab, weaponTech);

				var cyborgSecondary = updateResearchList(subPersonalities[personality].secondaryWeapon.templates);
				var len = subPersonalities[personality].primaryWeapon.weapons.length - 1;
				if (componentAvailable(subPersonalities[personality].primaryWeapon.weapons[len].stat))
				{
					if(!found && !turnOffCyborgs && cyborgSecondary.length > 0)
						found = pursueResearch(lab, cyborgSecondary);
					if(!found)
						found = evalResearch(lab, secondaryWeaponExtra);
					if(!found)
						found = evalResearch(lab, secondaryWeaponTech);
				}
			}
			else if (subPersonalities[personality].resPath === "offensive")
			{
				if (!found)
					found = evalResearch(lab, extraTech);

				if (!turnOffMG)
				{
					if (!found && random(100) < antiCyborgChance)
						found = evalResearch(lab, machinegunWeaponTech);
					if (!found && useArti && random(100) < antiCyborgChance)
						found = evalResearch(lab, machinegunWeaponExtra);
				}
				if (useLasersForCyborgControl() && random(100) < antiCyborgChance)
				{
					found = true;
					forceLaser = true;
				}

				if (!found && !turnOffCyborgs && random(2))
					found = evalResearch(lab, cyborgWeaps);
				if (!found && random(100) < 50)
					found = evalResearch(lab, weaponTech);
				if (!found && useArti && random(100) < 50)
					found = evalResearch(lab, artilleryTech);

				if (!found)
					found = evalResearch(lab, SYSTEM_UPGRADES);

				if (random(100) < 40 && countEnemyVTOL())
				{
					if (!found)
						found = evalResearch(lab, antiAirTech);
					if (!found)
						found = evalResearch(lab, antiAirExtras);
				}

				if (!found && useArti)
					found = evalResearch(lab, artillExtra);
				if (!found && useArti)
					found = evalResearch(lab, artilleryTech);

				if (!found && useVtol && (random(100) < subPersonalities[personality].vtolPriority))
					found = evalResearch(lab, VTOL_RES);

				if (!found && (random(100) < 70))
				{
					if (!turnOffCyborgs && countStruct(CYBORG_FACTORY) && random(2))
					{
						found = evalResearch(lab, CYBORG_ARMOR);
					}
					if (!found)
						found = evalResearch(lab, TANK_ARMOR);
				}

				var cyborgSecondary = updateResearchList(subPersonalities[personality].secondaryWeapon.templates);
				var len = subPersonalities[personality].primaryWeapon.weapons.length - 1;
				if (componentAvailable(subPersonalities[personality].primaryWeapon.weapons[len].stat))
				{
					if(!found && !turnOffCyborgs && cyborgSecondary.length > 0)
						found = pursueResearch(lab, cyborgSecondary);
					if(!found)
						found = evalResearch(lab, secondaryWeaponExtra);
					if(!found)
						found = evalResearch(lab, secondaryWeaponTech);
				}

				if (!found)
					found = evalResearch(lab, SENSOR_TECH);

				if (!found && ((random(100) < subPersonalities[personality].defensePriority)))
				{
					found = evalResearch(lab, standardDefenseTech);
					if (!found && useArti)
						found = evalResearch(lab, defenseTech);
					if (!found)
						found = evalResearch(lab, DEFENSE_UPGRADES);
				}
			}
			else if (subPersonalities[personality].resPath === "air")
			{
				if (!useVtol)
					useVtol = true;

				if (!found && random(100) < antiCyborgChance)
					found = evalResearch(lab, extraTech);
				if (!found && useArti && random(100) < antiCyborgChance)
					found = evalResearch(lab, artilleryTech);

				if (random(100) < 40 && countEnemyVTOL())
				{
					if (!found)
						found = evalResearch(lab, antiAirTech);
					if (!found)
						found = evalResearch(lab, antiAirExtras);
				}

				if (!found)
					found = evalResearch(lab, SYSTEM_UPGRADES);

				if (!found)
					found = evalResearch(lab, VTOL_RES);

				if (!turnOffMG)
				{
					if (!found && random(100) < 50)
						found = evalResearch(lab, machinegunWeaponTech);
					if (!found && useArti && random(100) < 50)
						found = evalResearch(lab, machinegunWeaponExtra);
				}
				if (useLasersForCyborgControl() && random(100) < antiCyborgChance)
				{
					found = true;
					forceLaser = true;
				}

				if (!found && !turnOffCyborgs)
					found = evalResearch(lab, cyborgWeaps);
				if (!found)
					found = evalResearch(lab, weaponTech);

				if (!found)
					found = evalResearch(lab, SENSOR_TECH);

				if (!found && (random(100) < subPersonalities[personality].alloyPriority))
				{
					if (!turnOffCyborgs && countStruct(CYBORG_FACTORY) && random(2))
					{
						found = evalResearch(lab, CYBORG_ARMOR);
					}
					if (!found)
						found = evalResearch(lab, TANK_ARMOR);
				}

				if (!found && useArti)
					found = evalResearch(lab, artillExtra);

				var cyborgSecondary = updateResearchList(subPersonalities[personality].secondaryWeapon.templates);
				var len = subPersonalities[personality].primaryWeapon.weapons.length - 1;
				if (componentAvailable(subPersonalities[personality].primaryWeapon.weapons[len].stat))
				{
					if(!found && !turnOffCyborgs && cyborgSecondary.length > 0)
						found = pursueResearch(lab, cyborgSecondary);
					if(!found)
						found = evalResearch(lab, secondaryWeaponExtra);
					if(!found)
						found = evalResearch(lab, secondaryWeaponTech);
				}


				if (!found && (random(100) < subPersonalities[personality].defensePriority))
				{
					found = evalResearch(lab, standardDefenseTech);
					if (!found && useArti)
						found = evalResearch(lab, defenseTech);
					if (!found)
						found = evalResearch(lab, DEFENSE_UPGRADES);
				}
			}

			if (!found)
				found = evalResearch(lab, empWeapons);

			if (!found)
				found = pursueResearch(lab, "R-Wpn-PlasmaCannon");
			if (componentAvailable("Laser4-PlasmaCannon"))
			{
				if(!found)
					found = evalResearch(lab, extremeLaserTech);
				if(!found)
					found = evalResearch(lab, FLAMER);
			}

			// Lasers
			if (forceLaser || (!found && subPersonalities[personality].useLasers))
			{
				var aa = returnAntiAirAlias();
				var foundLaser = false;

				if (!turnOffCyborgs)
					foundLaser = pursueResearch(lab, "R-Cyborg-Hvywpn-PulseLsr");
				if (!foundLaser)
					foundLaser = evalResearch(lab, laserTech);
				if (!foundLaser)
					foundLaser = evalResearch(lab, laserExtra);
				//Rocket/missile AA does not need this. Still uses it if researched.
				if (!foundLaser && (aa !== "rkta" && aa !== "missa"))
					foundLaser = pursueResearch(lab, "R-Defense-AA-Laser");
			}

			//Very likely going to be done with research by now.
			if ((getMultiTechLevel() === 4) || (!found && componentAvailable("Body14SUP") && componentAvailable("EMP-Cannon") && isStructureAvailable(structures.extras[2])))
			{
				researchComplete = true;
			}
		}
	}
}
