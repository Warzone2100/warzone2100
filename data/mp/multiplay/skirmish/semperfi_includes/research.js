
// Research definitions
var fundamentals = [
	"R-Defense-Tower01",	// mg tower
	"R-Wpn-MG2Mk1", // twin mg
	"R-Vehicle-Prop-Halftracks",	// halftracks
	"R-Struc-Power-Upgrade03a", // final power upgrade
	"R-Struc-Research-Upgrade09", // Faster research
	"R-Wpn-Rocket02-MRL", //MRL
	"R-Sys-Sensor-Upgrade03", // increase vision field
];
var lateGameTech = [
	"R-Vehicle-Body14", // dragon body (implies vengeance)
	"R-Sys-Resistance-Circuits", // NEXUS Resistance Circuits
	"R-Wpn-Flamer-Damage09", // even more thermal damage
];
var rocketTech = [
	"R-Defense-Tower06", //mini-rocket pod tower
	"R-Defense-WallTower06", // lancer hardpoint
	"R-Defense-HvyA-Trocket", //Tank-killer rocket and emplacement (implies lancer)
	"R-Wpn-Rocket-ROF03", // Rocket ROF
	"R-Defense-MRL", // mrl emplacement
	"R-Defense-IDFRocket",
	"R-Defense-Sunburst", // sunburst
	"R-Defense-IDFRocket", // ripple rocket battery
	"R-Wpn-Rocket-Damage09", // Rocket Damage
];
var missileTech = [
	"R-Defense-WallTower-A-Tmiss", // Scourge hardpoint
	"R-Defense-MdArtMissile", // Seraph missiles and defense emplacement.
	"R-Wpn-Missile-Accuracy02", // Missile Accuracy
	"R-Wpn-Missile-Damage03", // Missile Damage
	"R-Wpn-Missile-ROF03", // Missile ROF
];
var kineticAlloys = [
	"R-Vehicle-Metals09",
	"R-Cyborg-Metals09",
];
var vtolWeaponry = [
	"R-Wpn-Bomb-Accuracy03", // better bomb accuracy
	"R-Wpn-Bomb05", // plasmite bomb
	"R-Struc-VTOLPad-Upgrade06", // faster vtol rearming pads
];
var startComponents = [
	"R-Vehicle-Body05", // Cobra
	"R-Vehicle-Body11", // Python
	"R-Vehicle-Prop-Hover", //hover
];
var fundamentals2 = [
	"R-Sys-Autorepair-General", // autorepair
	"R-Sys-CBSensor-Tower01", // CB tower
	"R-Struc-Materials04", // better base structure defense.
	"R-Wpn-Flamer-ROF03", // better flamer ROF (implies inferno)
	"R-Wpn-Flamer-Damage06", // better flamer damage.
	"R-Vehicle-Body09", // tiger body
	"R-Vehicle-Armor-Heat04", // some tank thermal armor
	"R-Cyborg-Armor-Heat02", // some cyborg thermal armor
	"R-Struc-VTOLPad-Upgrade02", // get vtol pads
	"R-Wpn-Bomb04", // thermite bomb
	"R-Struc-Factory-Upgrade09", // self-replicating manufacturing
];
var thermalAlloys = [
	"R-Vehicle-Armor-Heat09",
	"R-Cyborg-Armor-Heat09",
];
var antiAirMissiles = [
	"R-Defense-SamSite1", // avenger "P0-AASite-SAM1"
	"R-Defense-SamSite2", // vindicator "P0-AASite-SAM2"
];
var structureDefenseUpgrades = [
	"R-Defense-HvyArtMissile", // Arch-angel Missile emplacement
	"R-Struc-Materials09", // better base structure defense.
	"R-Defense-WallUpgrade12", // stronger walls
];
var empTech = [
	"R-Wpn-Bomb06", // implies EMP-Cannon, uplink, Nexus tower.
	"R-Defense-EMPCannon",
	"R-Defense-EMPMortar",
	"R-Wpn-Mortar-ROF04",
	"R-Wpn-Cannon-ROF06",
	"R-Wpn-Mortar-Acc03",
	"R-Wpn-Cannon-Accuracy02",
];

//This function aims to more cleanly discover available research topics
//with the given list provided.
function evalResearch(lab, list)
{
	var found = pursueResearch(lab, list);
	//Try going a bit deeper.
	if(!found)
	{
		for (var i = 0, l = list.length; i < l; ++i)
		{
			found = pursueResearch(lab, list[i]);
			if (found)
			{
				break;
			}
		}
	}

	return found;
}

function eventResearched(tech, labParam)
{
	const MIN_POWER = -400;
	var labList;
	if (labParam) // check if called with parameter or not
	{
		labList = [];
		labList.push(labParam);
	}
	else
	{
		labList = enumStruct(me, RES_LAB);
	}

	for (var i = 0, r = labList.length; i < r; i++)
	{
		var lab = labList[i];
		if (lab.status == BUILT && structureIdle(lab) && (getRealPower() > MIN_POWER))
		{
			var found = evalResearch(lab, fundamentals);

			if (!found)
				found = evalResearch(lab, startComponents);

			if (!found)
				found = evalResearch(lab, rocketTech);

			if (!found && !random(3))
			{
				if (!isSeaMap && componentAvailable("Cyb-Wpn-Thermite"))
				{
					found = evalResearch(lab, kineticAlloys);
				}
				else
				{
					found = pursueResearch(lab, "R-Vehicle-Metals09");
				}
			}

			if (!found)
				found = pursueResearch(lab, fundamentals2);

			if (!found)
				found = evalResearch(lab, missileTech);

			if (!found)
				found = evalResearch(lab, antiAirMissiles);

			if (!found)
				found = evalResearch(lab, lateGameTech);

			if (!found)
			{
				if (!isSeaMap && componentAvailable("Cyb-Wpn-Thermite"))
				{
					found = evalResearch(lab, thermalAlloys);
				}
				else
				{
					found = pursueResearch(lab, "R-Vehicle-Armor-Heat09");
				}
			}

			if (!found)
				found = evalResearch(lab, vtolWeaponry);

			if (!found)
				found = evalResearch(lab, structureDefenseUpgrades);

			if (!found)
				found = evalResearch(lab, empTech);

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
