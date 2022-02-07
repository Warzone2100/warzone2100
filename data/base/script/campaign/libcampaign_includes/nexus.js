
////////////////////////////////////////////////////////////////////////////////
// Nexus related functionality.
////////////////////////////////////////////////////////////////////////////////

//Play a random laugh.
function camNexusLaugh()
{
	const LAUGH_CHANCE = 45;
	if (camRand(100) < LAUGH_CHANCE)
	{
		const LAUGHS = [LAUGH1, LAUGH2, LAUGH3];
		playSound(LAUGHS[camRand(LAUGHS.length)]);
	}
}

function camAbsorbPlayer(who, to)
{
	if (!camDef(who))
	{
		who = CAM_HUMAN_PLAYER;
	}
	if (!camDef(to))
	{
		to = NEXUS;
	}

	var units = enumDroid(who);

	for (var i = 0, len = units.length; i < len; i++)
	{
		var droid = units[i];
		if (!donateObject(droid, to))
		{
			camSafeRemoveObject(droid, false);
		}
	}

	var structs = enumStruct(who);
	for (var i = 0, len = structs.length; i < len; i++)
	{
		var structure = structs[i];
		if (!donateObject(structure, to))
		{
			camSafeRemoveObject(structure, false);
		}
	}

	camTrace("Player " + who + " has been absorbed by player" + to);
	changePlayerColour(who, to);
}

//Steal a droid or structure from a player.
function camHackIntoPlayer(player, to)
{
	if (__camNexusActivated === false)
	{
		return;
	}

	const GIFT_CHANCE = 70; //Else neutralized
	var target;
	var objects;

	if (!camDef(player))
	{
		player = CAM_HUMAN_PLAYER;
	}
	if (!camDef(to))
	{
		to = NEXUS;
	}
	if (!camDef(__camLastNexusAttack))
	{
		__camLastNexusAttack = 0;
	}

	var objects = __camChooseNexusTarget(player);
	if (objects.length === 0)
	{
		return;
	}

	__camLastNexusAttack = gameTime;
	target = objects[camRand(objects.length)];

	if (camRand(100) < GIFT_CHANCE)
	{
		camTrace("Hacking " + target.name + " at (x,y): " + target.x + " " + target.y);
		//Gift sounds are done in eventObjectTransfer.
		if (!donateObject(target, to))
		{
			camSafeRemoveObject(target, true); //Explode it then.
		}
	}
	else
	{
		camTrace("Neutralized " + target.name + " at (x,y): " + target.x + " " + target.y);
		if (target.player === CAM_HUMAN_PLAYER)
		{
			var sound;
			//Nexus neutralize sounds
			if (target.type === STRUCTURE)
			{
				if (target.stattype === DEFENSE)
				{
					sound = DEFENSE_NEUTRALIZE;
				}
				else
				{
					sound = STRUCTURE_NEUTRALIZE;
				}
			}
			else if (target.type === DROID)
			{
				sound = UNIT_NEUTRALIZE;
			}

			if (camDef(sound))
			{
				playSound(sound);
			}

			camSafeRemoveObject(target, true);
			queue("camNexusLaugh", camSecondsToMilliseconds(1.5));
		}
	}
}

function camSetNexusState(flag)
{
	__camNexusActivated = flag;
}

function camGetNexusState()
{
	return __camNexusActivated;
}

//////////// privates

function __camChooseNexusTarget(player)
{
	if (!camDef(player))
	{
		player = CAM_HUMAN_PLAYER;
	}

	//Try stealing the HQ first.
	if (__camLastNexusAttack === 0)
	{
		__camLastNexusAttack = gameTime;
		return enumStruct(player, HQ);
	}

	const TARGET_UNIT_CHANCE = (getResearch("R-Sys-Resistance-Upgrade01").done) ? 40 : 20;
	var objects = [];

	if (camRand(100) < TARGET_UNIT_CHANCE)
	{
		objects = enumDroid(player).filter(function(d) {
			return !camIsTransporter(d);
		});

		const EXP = {
			rookie: 0,
			green: 4,
			trained: 8,
			regular: 16,
			professional: 32,
			veteran: 64,
			elite: 128,
			special: 256,
			hero: 512,
		};

		//As the player researches more resistance upgrades their higher exp units will become more safe
		//Trucks get a little more safe with each upgrade also.
		objects = objects.filter(function(d) {
			if (__camNextLevel === CAM_GAMMA_OUT)
			{
				return true; //Final mission has a static fail chance to hack everything.
			}
			else if (getResearch("R-Sys-Resistance-Upgrade04").done)
			{
				return false; //Everything is safe
			}
			else if (getResearch("R-Sys-Resistance-Upgrade03").done)
			{
				if (d.droidType === DROID_CONSTRUCT && camRand(100) < 66)
				{
					return false;
				}
				return d.experience < EXP.regular;
			}
			else if (getResearch("R-Sys-Resistance-Upgrade02").done)
			{
				if (d.droidType === DROID_CONSTRUCT && camRand(100) < 50)
				{
					return false;
				}
				return d.experience < EXP.veteran;
			}
			else if (getResearch("R-Sys-Resistance-Upgrade01").done)
			{
				if (d.droidType === DROID_CONSTRUCT && camRand(100) < 20)
				{
					return false;
				}
				return d.experience < EXP.special;
			}
			else
			{
				return true;
			}
		});
	}

	if (objects.length === 0)
	{
		//Has explicit chances to target factories or research labs.
		switch (camRand(8))
		{
			case 0: objects = enumStruct(player, FACTORY).filter(function(s) { return (s.status === BUILT); }); break;
			case 1: objects = enumStruct(player, CYBORG_FACTORY).filter(function(s) { return (s.status === BUILT); }); break;
			case 2: objects = enumStruct(player, VTOL_FACTORY).filter(function(s) { return (s.status === BUILT); }); break;
			case 3: objects = enumStruct(player, RESEARCH_LAB).filter(function(r) { return (r.status === BUILT); }); break;
			default: //do nothing
		}

		if (objects.length === 0)
		{
			objects = enumStruct(player).filter(function(s) { return (s.status === BUILT); });
		}

		objects = objects.filter(function(s) {
			//cam3-ab is way too annoying if Nexus can still take factories after the second resistance upgrade.
			if (getResearch("R-Sys-Resistance-Upgrade02").done &&
				(s.stattype === FACTORY ||
				s.stattype === CYBORG_FACTORY ||
				s.stattype === VTOL_FACTORY))
			{
				return false;
			}

			return true;
		});
	}

	return objects;
}
