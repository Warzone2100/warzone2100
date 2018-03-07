
// Research definitions
const FUNDAMENTALS = [
	"R-Wpn-MG2Mk1",
	"R-Wpn-MG-Damage03",
	"R-Vehicle-Prop-Halftracks",	// halftracks
	"R-Struc-Research-Upgrade09", // Faster research
	"R-Struc-Power-Upgrade03a", // final power upgrade
	"R-Defense-MRL", // mrl emplacement
	"R-Defense-Tower06",
	"R-Sys-Autorepair-General",
];
const LATE_GAME_TECH = [
	"R-Sys-Resistance-Circuits",
	"R-Vehicle-Body14", // dragon body
	"R-Struc-Materials09",
];
const ROCKET_TECH = [
	"R-Wpn-Rocket-ROF03",
	"R-Defense-IDFRocket", // ripple rocket battery
	"R-Cyborg-Hvywpn-TK", //tank killer cyborg
	"R-Defense-HvyA-Trocket", //Tank-killer rocket and emplacement
];
const MISSILE_TECH = [
	"R-Defense-WallTower-A-Tmiss", // Scourge hardpoint
	"R-Cyborg-Hvywpn-A-T", // super scourge cyborg
	"R-Defense-MdArtMissile",
	"R-Defense-SamSite1",
	"R-Wpn-Missile-Accuracy02",
	"R-Wpn-Missile-Damage03",
	"R-Wpn-Missile-ROF03",
	"R-Defense-HvyArtMissile",
	"R-Defense-SamSite2",
];
const KINETIC_ALLOYS = [
	"R-Vehicle-Metals09",
	"R-Cyborg-Metals09",
];
const VTOL_WEAPONRY = [
	"R-Struc-VTOLPad-Upgrade06",
	"R-Wpn-Bomb-Accuracy03",
	"R-Wpn-Bomb05", // plasmite bomb
];
const START_COMPONENTS = [
	"R-Defense-WallTower06", // lancer hardpoint
	"R-Sys-Sensor-Upgrade01", // increase vision field
	"R-Cyborg-Metals02",
	"R-Struc-RprFac-Upgrade01",
	"R-Vehicle-Prop-Hover",
	"R-Wpn-Flamer-ROF01", // better flamer ROF
	"R-Wpn-Flamer-Damage03",
	"R-Vehicle-Body11", // Python
];
const FUNDAMENTALS2 = [
	"R-Wpn-Flamer-ROF03", // better flamer ROF
	"R-Defense-WallUpgrade03",
	"R-Vehicle-Body09", //tiger
	"R-Defense-Sunburst",
	"R-Sys-Sensor-Upgrade03", // increase vision field
	"R-Wpn-Rocket-Damage09",
	"R-Wpn-Flamer-Damage09",
	"R-Struc-Factory-Upgrade09",
	"R-Struc-RprFac-Upgrade06",
];
const THERMAL_ALLOYS = [
	"R-Vehicle-Armor-Heat09",
	"R-Cyborg-Armor-Heat09",
];
const STRUCTURE_DEFENSE_UPGRADES = [
	"R-Defense-WallUpgrade12",
];
const EMP_TECH = [
	"R-Wpn-Bomb06", // implies EMP-Cannon, uplink, Nexus tower.
	"R-Defense-EMPCannon",
	"R-Defense-EMPMortar",
];

//This function aims to more cleanly discover available research topics
//with the given list provided.
function evalResearch(lab, list)
{
	for (var i = 0, l = list.length; i < l; ++i)
	{
		if (pursueResearch(lab, list[i]))
		{
			return true;
		}
	}

	return false;
}

function findResearch(tech, labParam)
{
	if (!countDroid(DROID_CONSTRUCT))
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
		labList = enumStruct(me, RES_LAB).filter(function(lab) {
			return (lab.status === BUILT && structureIdle(lab));
		});
	}

	for (var i = 0, r = labList.length; i < r; ++i)
	{
		var lab = labList[i];
		var found = evalResearch(lab, FUNDAMENTALS);
		if (getRealPower() > MIN_POWER)
		{
			if (!found)
				found = evalResearch(lab, START_COMPONENTS);

			if (!found)
				found = evalResearch(lab, ROCKET_TECH);

			if (!random(3) && !found)
				found = evalResearch(lab, STRUCTURE_DEFENSE_UPGRADES);

			if (!found && random(3))
				found = pursueResearch(lab, FUNDAMENTALS2);

			if (!found && !random(4))
			{
				if (!isSeaMap)
				{
					found = evalResearch(lab, KINETIC_ALLOYS);
					if (!found)
						found = evalResearch(lab, THERMAL_ALLOYS);
				}
				else
				{
					found = pursueResearch(lab, "R-Vehicle-Metals09");
					if (!found)
						found = pursueResearch(lab, "R-Vehicle-Armor-Heat09");
				}
			}

			if (!found)
				found = evalResearch(lab, VTOL_WEAPONRY);

			if (!found)
				found = evalResearch(lab, MISSILE_TECH);

			if (!found)
				found = evalResearch(lab, EMP_TECH);


			if (!found)
				found = evalResearch(lab, LATE_GAME_TECH);

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
