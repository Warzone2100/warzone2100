
// Research definitions
const FUNDAMENTALS = [
	"R-Sys-Engineering01",
	"R-Wpn-MG1Mk1",
	"R-Defense-Tower01",
	"R-Wpn-MG2Mk1",
	"R-Wpn-MG-Damage02",
	"R-Struc-PowerModuleMk1",
	"R-Struc-Research-Upgrade01",
	"R-Defense-WallUpgrade02",
	"R-Vehicle-Metals01",
	"R-Cyborg-Metals01",
];
const LATE_GAME_TECH = [
	"R-Vehicle-Body14", // dragon body
	"R-Defense-SamSite1",
	"R-Defense-SamSite2",
	"R-Sys-Resistance-Circuits",
	"R-Wpn-Bomb06", // implies EMP-Cannon and Nexus tower.
	"R-Defense-EMPCannon",
	"R-Defense-EMPMortar",
];
const ROCKET_TECH = [
	"R-Wpn-Rocket-ROF03",
	"R-Wpn-Rocket03-HvAT",
	"R-Cyborg-Hvywpn-TK", //tank killer cyborg
	"R-Defense-WallTower-HvyA-Trocket", //Tank-killer rocket and hardpoint
	"R-Wpn-Rocket-Damage09",
	"R-Defense-IDFRocket", // ripple rocket battery
];
const MISSILE_TECH = [
	"R-Defense-WallTower-A-Tmiss", // Scourge hardpoint
	"R-Cyborg-Hvywpn-A-T", // super scourge cyborg
	"R-Defense-MdArtMissile",
	"R-Wpn-Missile-Accuracy02",
	"R-Wpn-Missile-Damage03",
	"R-Wpn-Missile-ROF03",
	"R-Defense-HvyArtMissile",
];
const KINETIC_ALLOYS = [
	"R-Vehicle-Metals09",
	"R-Cyborg-Metals09",
];
const VTOL_WEAPONRY = [
	"R-Struc-VTOLPad-Upgrade06",
	"R-Wpn-Bomb05", // plasmite bomb
	"R-Wpn-Bomb-Accuracy03",
];
const START_COMPONENTS = [
	"R-Struc-RprFac-Upgrade01",
	"R-Sys-Sensor-Upgrade01", // increase vision field
	"R-Defense-MRL",
	"R-Defense-Tower06",
	"R-Defense-WallTower06", // lancer hardpoint
	"R-Vehicle-Body11", // Python
	"R-Vehicle-Prop-Halftracks",	// halftracks
];
const FUNDAMENTALS2 = [
	"R-Vehicle-Prop-Hover",
	"R-Vehicle-Body09", //tiger
	"R-Sys-Engineering03",
	"R-Sys-Autorepair-General",
	"R-Sys-Sensor-Upgrade03", // increase vision field
	"R-Sys-Sensor-Tower02",
	"R-Struc-RprFac-Upgrade06",
	"R-Struc-Factory-Upgrade09",
	"R-Sys-Sensor-WSTower",
	"R-Sys-Sensor-UpLink",
];
const THERMAL_ALLOYS = [
	"R-Vehicle-Armor-Heat09",
	"R-Cyborg-Armor-Heat09",
];
const STRUCTURE_DEFENSE_UPGRADES = [
	"R-Defense-WallUpgrade12",
	"R-Struc-Materials09",
];
const FLAMER_TECH = [
	"R-Wpn-Flamer-ROF03",
	"R-Wpn-Flamer-Damage09",
];
const ANTI_AIR_TECH = [
	"R-Defense-Sunburst",
	"R-Defense-SamSite1",
	"R-Defense-SamSite2",
];
const POWER_AND_RESEARCH_TECH = [
	"R-Struc-Power-Upgrade03a", // final power upgrade
	"R-Struc-Research-Upgrade09", // Faster research
];

//This function aims to more cleanly discover available research topics
//with the given list provided.
function evalResearch(labID, list)
{
	var lab = getObject(STRUCTURE, me, labID);
	if (lab === null)
	{
		return true;
	}
	for (var i = 0, l = list.length; i < l; ++i)
	{
		if (!getResearch(list[i]).done && pursueResearch(lab, list[i]))
		{
			return true;
		}
	}

	return false;
}

function findResearch(tech, labParam)
{
	if (!countDroid(DROID_CONSTRUCT) || researchDone)
	{
		return; //need construction droids.
	}

	var labList;
	if (labParam) // check if called with parameter or not
	{
		labList = [];
		labList.push(labParam);
	}
	else
	{
		labList = enumStruct(me, RES_LAB_STAT).filter(function(lab) {
			return (lab.status === BUILT && structureIdle(lab));
		});
	}

	for (var i = 0, r = labList.length; i < r; ++i)
	{
		var lab = labList[i];
		var found = evalResearch(lab.id, FUNDAMENTALS);

		// Focus on the hover research for a hover map.
		if (!found && isSeaMap === true)
		{
			found = pursueResearch(lab, "R-Vehicle-Prop-Hover");
		}

		if (!found && getRealPower() > MIN_RESEARCH_POWER)
		{
			found = evalResearch(lab.id, START_COMPONENTS);
			if (!found && random(3) === 0)
			{
				found = evalResearch(lab.id, POWER_AND_RESEARCH_TECH);
			}
			if (!found && enemyHasVtol)
			{
				//Push for anti-air tech if we discover the enemy has VTOLs
				//The side effect is missile research will be prioritized
				//much earlier. Probably not much of an issue.
				found = evalResearch(lab.id, ANTI_AIR_TECH);
				if (!found)
				{
					found = evalResearch(lab.id, MISSILE_TECH);
				}
				if (!found)
				{
					found = evalResearch(lab.id, VTOL_WEAPONRY);
				}
			}
			//If they have vtols then push rocket tech later (for bunker buster at that point).
			if (!found && !enemyHasVtol && random(2) === 0)
			{
				found = evalResearch(lab.id, ROCKET_TECH);
			}
			if (!isSeaMap && countStruct(CYBORG_FACTORY_STAT) > 0 && random(2) === 0 && !found)
			{
				found = evalResearch(lab.id, FLAMER_TECH);
			}
			if (!found && random(3) === 0)
			{
				found = evalResearch(lab.id, STRUCTURE_DEFENSE_UPGRADES);
			}
			if (!found)
			{
				found = evalResearch(lab.id, FUNDAMENTALS2);
			}
			if (!found && random(2) === 0)
			{
				if (!isSeaMap)
				{
					found = evalResearch(lab.id, KINETIC_ALLOYS);
					if (!found && random(2) === 0)
					{
						found = evalResearch(lab.id, THERMAL_ALLOYS);
					}
				}
				else
				{
					found = pursueResearch(lab, "R-Vehicle-Metals09");
					if (!found && random(2) === 0)
					{
						found = pursueResearch(lab, "R-Vehicle-Armor-Heat09");
					}
				}
			}
			if (!enemyHasVtol)
			{
				if (!found)
				{
					found = evalResearch(lab.id, MISSILE_TECH);
				}
				if (!found)
				{
					found = evalResearch(lab.id, VTOL_WEAPONRY);
				}
			}
			else
			{
				if (!found)
				{
					found = evalResearch(lab.id, ROCKET_TECH);
				}
			}
			if (!found)
			{
				found = evalResearch(lab.id, LATE_GAME_TECH);
			}
			//Only research random stuff once we get retribution.
			if (componentAvailable("Body7ABT") && !found)
			{
				// Find a random research item
				var reslist = enumResearch();
				var len = reslist.length;
				if (len > 0)
				{
					var idx = Math.floor(Math.random() * len);
					pursueResearch(lab, reslist[idx].name);
				}
			}
		}
	}
}
