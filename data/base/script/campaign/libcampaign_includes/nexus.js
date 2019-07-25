
////////////////////////////////////////////////////////////////////////////////
// Nexus related functionality.
////////////////////////////////////////////////////////////////////////////////

//Play a random laugh.
function camNexusLaugh()
{
	if (camRand(101) < 45)
	{
		var rnd = camRand(3) + 1;
		var snd;

		if (rnd === 0)
		{
			snd = LAUGH1;
		}
		else if (rnd === 1)
		{
			snd = LAUGH2;
		}
		else
		{
			snd = LAUGH3;
		}

		playSound(snd);
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

	var tmp = [];
	if (camRand(100) < 40)
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
			case 0: tmp = enumStruct(player, "A0LightFactory").filter(function(s) { return (s.status === BUILT); }); break;
			case 1: tmp = enumStruct(player, "A0CyborgFactory").filter(function(s) { return (s.status === BUILT); }); break;
			case 2: tmp = enumStruct(player, "A0VTolFactory1").filter(function(s) { return (s.status === BUILT); }); break;
			case 3: tmp = enumStruct(player, "A0ResearchFacility").filter(function(r) { return (r.status === BUILT); }); break;
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

	if (__camLastNexusAttack === 0 || (gameTime > (__camLastNexusAttack + 5000)) && (camRand(100) < 25))
	{
		var obj;
		var steal = true;
		var objects;

		//Try stealing the HQ first.
		if (__camLastNexusAttack === 0)
		{
			objects = enumStruct(player, HQ);
		}
		else
		{
			steal = (camRand(100) < 70) ? true : false;
			objects = tmp;
		}

		__camLastNexusAttack = gameTime;
		if (objects.length === 0)
		{
			return;
		}

		obj = objects[camRand(objects.length)];

		if (steal === true)
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
