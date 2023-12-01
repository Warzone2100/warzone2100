
////////////////////////////////////////////////////////////////////////////////
// Nexus related functionality.
////////////////////////////////////////////////////////////////////////////////

//;; ## camNexusLaugh()
//;;
//;; Play a random NEXUS laugh.
//;;
//;; @returns {void}
//;;
function camNexusLaugh()
{
	const __LAUGH_CHANCE = 45;
	if (camRand(100) < __LAUGH_CHANCE)
	{
		const laughs = [cam_sounds.nexus.laugh1, cam_sounds.nexus.laugh2, cam_sounds.nexus.laugh3];
		playSound(laughs[camRand(laughs.length)]);
	}
}

//;; ## camAbsorbPlayer([who[, to]])
//;;
//;; Completely give all of player `who` droids and structures to player `to`.
//;; Will default to `CAM_HUMAN_PLAYER` and `CAM_NEXUS` respectively.
//;;
//;; @param {number} [who]
//;; @param {number} [to]
//;; @returns {void}
//;;
function camAbsorbPlayer(who, to)
{
	if (!camDef(who))
	{
		who = CAM_HUMAN_PLAYER;
	}
	if (!camDef(to))
	{
		to = CAM_NEXUS;
	}

	const units = enumDroid(who);

	for (let i = 0, len = units.length; i < len; ++i)
	{
		const droid = units[i];
		if (!donateObject(droid, to))
		{
			camSafeRemoveObject(droid, false);
		}
	}

	const structs = enumStruct(who);
	for (let i = 0, len = structs.length; i < len; ++i)
	{
		const structure = structs[i];
		if (!donateObject(structure, to))
		{
			camSafeRemoveObject(structure, false);
		}
	}

	camTrace("Player " + who + " has been absorbed by player" + to);
	changePlayerColour(who, to);
}

//;; ## camHackIntoPlayer([player[, to]])
//;;
//;; Steal a droid or structure from a player if the NEXUS hack state is active.
//;; Will default to `CAM_HUMAN_PLAYER` and `CAM_NEXUS` respectively.
//;;
//;; @param {number} [player]
//;; @param {number} [to]
//;; @returns {void}
//;;
function camHackIntoPlayer(player, to)
{
	if (!camGetNexusState())
	{
		return;
	}

	const __GIFT_CHANCE = 70; //Else neutralized
	let target;
	let objects;

	if (!camDef(player))
	{
		player = CAM_HUMAN_PLAYER;
	}
	if (!camDef(to))
	{
		to = CAM_NEXUS;
	}
	if (!camDef(__camLastNexusAttack))
	{
		__camLastNexusAttack = 0;
	}

	objects = __camChooseNexusTarget(player);
	if (objects.length === 0)
	{
		return;
	}

	__camLastNexusAttack = gameTime;
	target = objects[camRand(objects.length)];

	if ((camRand(100) < __GIFT_CHANCE) && !(target.type === STRUCTURE && target.stattype === WALL))
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
			let sound;
			//Nexus neutralize sounds
			if (target.type === STRUCTURE)
			{
				if (target.stattype === DEFENSE)
				{
					sound = cam_sounds.nexus.defensesNeutralized;
				}
				else
				{
					sound = cam_sounds.nexus.structureNeutralized;
				}
			}
			else if (target.type === DROID)
			{
				sound = cam_sounds.nexus.unitNeutralized;
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

//;; ## camSetNexusState(flag)
//;;
//;; Turn on/off the NEXUS hacking state feature.
//;;
//;; @param {boolean} flag
//;; @returns {void}
//;;
function camSetNexusState(flag)
{
	__camNexusActivated = flag;
}

//;; ## camGetNexusState()
//;;
//;; Returns the activation state of the NEXUS hacking feature.
//;;
//;; @returns {boolean}
//;;
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

	const __TARGET_UNIT_CHANCE = (getResearch(cam_resistance_circuits.first).done) ? 40 : 20;
	let objects = [];

	if (camRand(100) < __TARGET_UNIT_CHANCE)
	{
		objects = enumDroid(player).filter((d) => (!camIsTransporter(d)));

		const exp = {
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
		objects = objects.filter((d) => {
			if (__camNextLevel === CAM_GAMMA_OUT)
			{
				return true; //Final mission has a static fail chance to hack everything.
			}
			else if (getResearch(cam_resistance_circuits.fourth).done)
			{
				return false; //Everything is safe
			}
			else if (getResearch(cam_resistance_circuits.third).done)
			{
				if (d.droidType === DROID_CONSTRUCT && camRand(100) < 66)
				{
					return false;
				}
				return d.experience < exp.regular;
			}
			else if (getResearch(cam_resistance_circuits.second).done)
			{
				if (d.droidType === DROID_CONSTRUCT && camRand(100) < 50)
				{
					return false;
				}
				return d.experience < exp.veteran;
			}
			else if (getResearch(cam_resistance_circuits.first).done)
			{
				if (d.droidType === DROID_CONSTRUCT && camRand(100) < 20)
				{
					return false;
				}
				return d.experience < exp.special;
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
			case 0: objects = enumStruct(player, FACTORY).filter((s) => (s.status === BUILT)); break;
			case 1: objects = enumStruct(player, CYBORG_FACTORY).filter((s) => (s.status === BUILT)); break;
			case 2: objects = enumStruct(player, VTOL_FACTORY).filter((s) => (s.status === BUILT)); break;
			case 3: objects = enumStruct(player, RESEARCH_LAB).filter((r) => (r.status === BUILT)); break;
			default: //do nothing
		}

		if (objects.length === 0)
		{
			objects = enumStruct(player).filter((s) => (s.status === BUILT));
		}

		objects = objects.filter((s) => (
			//cam3-ab is way too annoying if Nexus can still take factories after the second resistance upgrade.
			!(getResearch(cam_resistance_circuits.second).done &&
			(s.stattype === FACTORY || s.stattype === CYBORG_FACTORY || s.stattype === VTOL_FACTORY))
		));
	}

	return objects;
}
