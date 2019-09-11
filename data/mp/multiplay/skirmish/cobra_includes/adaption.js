//If starting with a low tech level, then disable Machine-guns when the
//personality can design its last primary weapon.
function switchOffMG()
{
	if (turnOffMG || (returnPrimaryAlias() === "mg"))
	{
		removeThisTimer("switchOffMG");
		return;
	}

	if (componentAvailable("Body5REC"))
	{
		turnOffMG = true;
	}
}


//Choose the personality as described in the global subPersonalities.
//When called from chat it will switch to that one directly.
function choosePersonality(chatEvent)
{
	if (!isDefined(chatEvent))
	{
		personality = adaptToMap();
	}
	else
	{
		personality = chatEvent;
		sendChatMessage("Using personality: " + personality, ALLIES);
	}

	initializeResearchLists();
}

//Tell us what our personality is.
function myPersonality()
{
	return personality;
}

//Choose personality based on map oil/ally count or technology. Called from eventStartLevel().
//isDesignable("Howitzer03-Rot") checks if it a T3 match and allows personality AL to be used (must have bases).
function adaptToMap() {
	var choice = "";
	var personal;
	var offset;
	const ALLY_COUNT = playerAlliance(true).length - 1;
	const MAP_OIL_LEVEL = mapOilLevel();
	const T3_MATCH = getMultiTechLevel() === 3;

	if (!startedWithTech && !componentAvailable("hover01") && MAP_OIL_LEVEL === "NTW")
	{
		choice = "AB";
	}
	else if (!T3_MATCH && (((maxPlayers - 1) === 1) || ((MAP_OIL_LEVEL === "LOW") && !ALLY_COUNT)))
	{
		personal = ["AM", "AR", "AB", "AC"];
		choice = personal[random(personal.length)];
	}
	else if ((MAP_OIL_LEVEL === "MEDIUM") || ALLY_COUNT)
	{
		personal = ["AR", "AB", "AC", "AA", "AL"];
		offset = (T3_MATCH && (baseType !== CAMP_CLEAN)) ? 5 : 4;
		choice = personal[random(offset)];
	}
	else
	{
		personal = ["AC", "AA", "AL"];
		offset = (T3_MATCH && (baseType !== CAMP_CLEAN)) ? 3 : 2;
		choice = personal[random(offset)];
	}

	return choice;
}
