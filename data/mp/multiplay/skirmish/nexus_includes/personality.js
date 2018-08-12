
function chooseLandPersonality()
{
	var names = findPersonalityByType("land");

	if (names.length > 0)
	{
		branch = names[random(names.length)];
		return;
	}

	//Going to crash hard then
	debugMessage("chooseLandPersonality. No land personality (must have at least one)");
}

function chooseVtolPersonality()
{
	var names = findPersonalityByType("vtol");

	if (names.length > 0)
	{
		branch = names[random(names.length)];
		return;
	}

	chooseLandPersonality(); //fall back
}

//Return all personalities named that match a certain play style type.
function findPersonalityByType(type)
{
	var personalities = [];

	if (!defined(type))
	{
		debugMessage("findPersonalityByType. No parameter. Using land.");
		type = "land";
	}

	for (var i in nexusBranch)
	{
		if (nexusBranch[i].type === type)
		{
			personalities.push(nexusBranch[i].name);
		}
	}

	return personalities;
}

function isNexusPersonality(msg)
{
	for (var personality in nexusBranch)
	{
		if (nexusBranch[personality].name === msg)
		{
			return true;
		}
	}

	return false;
}
