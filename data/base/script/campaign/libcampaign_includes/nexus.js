
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

	const HACK_CHANCE = 25;
	const TARGET_UNIT_CHANCE = 40;
	const HACK_DELAY_TIME = camSecondsToMilliseconds(5); //Allows another hack attempt after X seconds.
	var tmp = [];
	if (camRand(100) < TARGET_UNIT_CHANCE)
	{
		tmp = enumDroid(player).filter(function(d) {
			return !camIsTransporter(d);
		});
	}
	if (tmp.length === 0)
	{
		//Has explicit chances to target factories or research labs.
		switch (camRand(8))
		{
			case 0: tmp = enumStruct(player, FACTORY).filter(function(s) { return (s.status === BUILT); }); break;
			case 1: tmp = enumStruct(player, CYBORG_FACTORY).filter(function(s) { return (s.status === BUILT); }); break;
			case 2: tmp = enumStruct(player, VTOL_FACTORY).filter(function(s) { return (s.status === BUILT); }); break;
			case 3: tmp = enumStruct(player, RESEARCH_LAB).filter(function(r) { return (r.status === BUILT); }); break;
			default: //do nothing
		}
		if (tmp.length === 0)
		{
			tmp = enumStruct(player).filter(function(s) { return (s.status === BUILT); });
		}
	}
	if (tmp.length === 0)
	{
		return;
	}

	if (__camLastNexusAttack === 0 || (gameTime > (__camLastNexusAttack + HACK_DELAY_TIME)) && (camRand(100) < HACK_CHANCE))
	{
		const GIFT_CHANCE = 70; //Else neutralized
		var obj;
		var objects;

		//Try stealing the HQ first.
		if (__camLastNexusAttack === 0)
		{
			objects = enumStruct(player, HQ);
		}
		else
		{
			objects = tmp;
		}

		__camLastNexusAttack = gameTime;
		if (objects.length === 0)
		{
			return;
		}

		obj = objects[camRand(objects.length)];

		if (camRand(100) < GIFT_CHANCE)
		{
			camTrace("Hacking " + obj.name + " at (x,y): " + obj.x + " " + obj.y);
			if (!donateObject(obj, to))
			{
				camSafeRemoveObject(obj, true); //Explode it then.
			}
		}
		else
		{
			camTrace("neutralized " + obj.name + " at (x,y): " + obj.x + " " + obj.y);
			//Nexus neutralize sounds.
			if (obj.player === CAM_HUMAN_PLAYER)
			{
				var snd;
				if (obj.type === STRUCTURE)
				{
					if (obj.stattype === DEFENSE)
					{
						snd = DEFENSE_NEUTRALIZE;
					}
					else
					{
						snd = STRUCTURE_NEUTRALIZE;
					}
				}
				else if (obj.type === DROID)
				{
					snd = UNIT_NEUTRALIZE;
				}

				camSafeRemoveObject(obj, true);

				if (camDef(snd))
				{
					playSound(snd);
				}

				queue("camNexusLaugh", camSecondsToMilliseconds(1.5));
			}
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
