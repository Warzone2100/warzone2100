
// Tank definitions
const TANK_BODY_LIST = [
	"Body14SUP", // dragon
	"Body10MBT", //vengeance
	"Body7ABT", // retribution
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

	//Choose either artillery or anti-tank.
	var weaponChoice = (random(101) < WEAPON_CHANCE) ? TANK_WEAPON_LIST : TANK_ARTILLERY;
	var secondary = (random(101) < WEAPON_CHANCE) ? TANK_WEAPON_LIST : TANK_ARTILLERY;
	var prop = TANK_PROP_LIST;

	//When dragon is available, try a chance at useing EMP-Cannon as secondary.
	if (componentAvailable("Body14SUP") && componentAvailable("EMP-Cannon") && random(101) < EMP_CHANCE)
	{
		secondary = "EMP-Cannon";
	}

	if ((isSeaMap || (random(101) < HOVER_CHANCE)) && componentAvailable("hover01"))
	{
		prop = "hover01";
	}

	if (getRealPower() > MIN_POWER)
	{
		buildDroid(struct, "Ranged Attacker", TANK_BODY_LIST, prop, "", "", weaponChoice, secondary);
	}
}

function buildTruck(struct)
{
	buildDroid(struct, "Constructor", SYSTEM_BODY_LIST, SYSTEM_PROP_LIST, "", "", "Spade1Mk1");
}

function buildCyborg(struct)
{
	// Cyborg templates are special -- their bodies, legs and weapons are linked. We should fix this one day...
	if (getRealPower() > MIN_POWER)
	{
		if (!random(2))
		{
			buildDroid(struct, "Cyborg Flamer", "CyborgLightBody", "CyborgLegs", "", "", CYBORG_FLAMERS);
		}
		else
		{
			buildDroid(struct, "Cyborg Rocket", "CyborgHeavyBody", "CyborgLegs", "", "", CYBORG_ROCKETS);
		}
	}
}

function buildVTOL(struct)
{
	const WEAPON_CHANCE = 50;
	const EMP_CHANCE = 20;

	var weaponChoice = (random(101) < WEAPON_CHANCE) ? BOMB_LIST : VTOL_ROCKETS;
	if (random(101) < EMP_CHANCE && componentAvailable("Bomb6-VTOL-EMP"))
	{
		weaponChoice = "Bomb6-VTOL-EMP";
	}

	if (getRealPower() > MIN_POWER)
	{
		buildDroid(struct, "VTOL unit", VTOL_BODY_LIST, "V-Tol", "", "", weaponChoice);
	}
}

//Produce droids.
function produce()
{
	const FAC_LIST = [FACTORY, VTOL_FACTORY, CYBORG_FACTORY];

	var facs = enumStruct(me, FACTORY);
	var virtualTrucks = 0;

	//Count the trucks being built so as not to build too many of them.
	for (var i = 0, l = facs.length; i < l; ++i)
	{
		var virDroid = getDroidProduction(facs[i]);
		if (virDroid !== null)
		{
			if (virDroid.droidType === DROID_CONSTRUCT)
			{
				virtualTrucks += 1;
			}
		}
	}

	for (var i = 0; i < 3; ++i)
	{
		var facType = FAC_LIST[i];
		var fac = enumStruct(me, facType);
		if (!(isSeaMap && (facType === CYBORG_FACTORY)))
		{
			for (var x = 0, l = fac.length; x < l; ++x)
			{
				const FC = fac[x];
				if (defined(FC) && structureIdle(FC))
				{
					if (facType === FACTORY)
					{
						if (countDroid(DROID_CONSTRUCT) + virtualTrucks < MIN_TRUCKS)
						{
							buildTruck(FC);
						}
						else
						{
							if (countStruct(POW_GEN))
							{
								buildAttacker(FC);
							}
						}
					}
					else
					{
						if (!countStruct(POW_GEN))
						{
							continue;
						}

						if (facType === CYBORG_FACTORY)
						{
							buildCyborg(FC);
						}
						else
						{
							buildVTOL(FC);
						}
					}
				}
			}
		}
	}
}
