
// Tank definitions
const TANK_BODY_LIST = [
	"Body14SUP", // dragon
	"Body10MBT", //vengeance
	"Body7ABT", // retribution
	"Body9REC", //Tiger
	"Body6SUPP", // panther
	"Body11ABT", // python
	"Body5REC", // cobra
	"Body1REC", // viper
];
const TANK_PROP_LIST = [
	"HalfTrack", // half-track
	"wheeled01", // wheels
];
const TANK_WEAPON_LIST = [
	"Missile-A-T", // Scourge
	"Rocket-HvyA-T", // Tank-killer
	"Rocket-LtA-T", // Lancer
	"Rocket-MRL", // MRL
	"MG2Mk1", // twin mg
	"MG1Mk1", // mg, initial weapon
];
const TANK_BUNKER_BUSTERS = [
	"Rocket-BB",
];
const TANK_ARTILLERY = [
	"Missile-MdArt", // Seraph
	"Rocket-MRL", // MRL
];
const CYBORG_FLAMERS = [
	"Cyb-Wpn-Thermite",
	"CyborgFlamer01",
];
const CYBORG_ROCKETS = [
	"Cyb-Hvywpn-TK",
	"Cyb-Hvywpn-A-T",
];

// System definitions
const SYSTEM_BODY_LIST = [
	"Body3MBT",  // Retaliation
	"Body4ABT",  // Bug
	"Body1REC",  // Viper
];
const SYSTEM_PROP_LIST = [
	"hover01", // hover
	"wheeled01", // wheels
];

// VTOL definitions
const BOMB_LIST = [
	"Bomb5-VTOL-Plasmite",	// plasmite bomb
	"Bomb4-VTOL-HvyINC",	// thermite bomb
];
const VTOL_ROCKETS = [
	"Missile-VTOL-AT", // scourge
	"Rocket-VTOL-HvyA-T", //tank killer
	"Rocket-VTOL-LtA-T", // lancer
];
var VTOL_BODY_LIST = [
	"Body7ABT", // retribution
	"Body6SUPP", // panther
	"Body8MBT", // scorpion
	"Body5REC", // cobra
	"Body4ABT", // bug
	"Body1REC", // viper
];

function buildAttacker(struct)
{
	const HOVER_CHANCE = 65;
	const WEAPON_CHANCE = 68;
	const EMP_CHANCE = 40;
	const MIN_ATTACK_GSIZE = 11;

	//Choose either artillery or anti-tank.
	var weaponChoice = (random(100) < WEAPON_CHANCE) ? TANK_WEAPON_LIST : TANK_ARTILLERY;
	var secondary = (random(100) < WEAPON_CHANCE) ? TANK_WEAPON_LIST : TANK_ARTILLERY;
	var prop = TANK_PROP_LIST;

	//When dragon is available, try a chance at using EMP-Cannon as secondary.
	if (componentAvailable("Body14SUP") && componentAvailable("EMP-Cannon") && random(100) < EMP_CHANCE)
	{
		secondary = "EMP-Cannon";
	}

	if ((isSeaMap || (random(100) < HOVER_CHANCE)) && componentAvailable("hover01"))
	{
		prop = "hover01";
	}

	if (enumGroup(attackGroup).length > MIN_ATTACK_GSIZE && enumGroup(busterGroup).length < MIN_BUSTERS)
	{
		for (var i = 0; i < TANK_BUNKER_BUSTERS.length; ++i)
		{
			if (componentAvailable(TANK_BUNKER_BUSTERS[i]))
			{
				return buildDroid(struct, "Bunker buster", TANK_BODY_LIST, prop, "", "", TANK_BUNKER_BUSTERS, secondary);
			}
		}
	}
	return buildDroid(struct, "Ranged Attacker", TANK_BODY_LIST, prop, "", "", weaponChoice, secondary);
}

function buildTruck(struct)
{
	return buildDroid(struct, "Constructor", SYSTEM_BODY_LIST, SYSTEM_PROP_LIST, "", "", "Spade1Mk1");
}

function buildCyborg(struct)
{
	// Cyborg templates are special -- their bodies, legs and weapons are linked. We should fix this one day...
	if (!random(2))
	{
		return buildDroid(struct, "Cyborg Flamer", "CyborgLightBody", "CyborgLegs", "", "", CYBORG_FLAMERS);
	}
	return buildDroid(struct, "Cyborg Rocket", "CyborgHeavyBody", "CyborgLegs", "", "", CYBORG_ROCKETS);
}

function buildVTOL(struct)
{
	const WEAPON_CHANCE = 50;
	const EMP_CHANCE = 20;

	var weaponChoice = (random(100) < WEAPON_CHANCE) ? BOMB_LIST : VTOL_ROCKETS;
	if (random(100) < EMP_CHANCE && componentAvailable("Bomb6-VTOL-EMP"))
	{
		weaponChoice = "Bomb6-VTOL-EMP";
	}

	return buildDroid(struct, "VTOL unit", VTOL_BODY_LIST, "V-Tol", "", "", weaponChoice);
}

//Produce droids and potentially research something. Research is done in here
//since we want to prioritize droid production over research.
function produceAndResearch()
{
	if (getRealPower() < MIN_PRODUCTION_POWER)
	{
		return;
	}

	const FAC_LIST = [FACTORY_STAT, VTOL_FACTORY_STAT, CYBORG_FACTORY_STAT];
	var facsVirtual = enumStruct(me, FACTORY_STAT);
	var virtualTrucks = 0;
	var i = 0;
	var x = 0;
	var l = 0;

	//Count the trucks being built so as not to build too many of them.
	for (i = 0, l = facsVirtual.length; i < l; ++i)
	{
		var virDroid = getDroidProduction(facsVirtual[i]);
		if (virDroid !== null)
		{
			if (virDroid.droidType === DROID_CONSTRUCT)
			{
				virtualTrucks += 1;
			}
		}
	}

	for (i = 0; i < 3; ++i)
	{
		var facs = enumStruct(me, FAC_LIST[i]);
		if (FAC_LIST[i] === CYBORG_FACTORY_STAT && isSeaMap === true)
		{
			continue;
		}
		for (x = 0, l = facs.length; x < l; ++x)
		{
			var fc = facs[x];
			if (structureIdle(fc))
			{
				if (FAC_LIST[i] === FACTORY_STAT)
				{
					if (countDroid(DROID_CONSTRUCT) + virtualTrucks < MIN_BASE_TRUCKS + MIN_OIL_TRUCKS)
					{
						buildTruck(fc);
					}
					else
					{
						if (countStruct(POW_GEN_STAT))
						{
							buildAttacker(fc);
						}
					}
				}
				else
				{
					if (!countStruct(POW_GEN_STAT))
					{
						continue;
					}

					if (FAC_LIST[i] === CYBORG_FACTORY_STAT)
					{
						buildCyborg(fc);
					}
					else
					{
						buildVTOL(fc);
					}
				}
			}
		}
	}

	findResearch();
}
