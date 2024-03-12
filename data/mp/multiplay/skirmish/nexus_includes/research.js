
// Slightly better way of discovering research.
function evalResearch(labID, list)
{
	var lab = getObject(STRUCTURE, me, labID);

	if (lab === null)
	{
		return false;
	}

	for (let i = 0, len = list.length; i < len; ++i)
	{
		if (pursueResearch(lab, list[i]))
		{
			return true;
		}
	}

	return false;
}

function doResearch()
{
	var resLabs = enumStruct(me, BASE_STRUCTURES.labs);

	// don't throw in half of your money on research in T3 no bases when we don't have any income yet
	if (isStructureAvailable("A0FacMod1") && !hasPowerSource(me))
	{
		return;
	}

	for (let i = 0, len = resLabs.length; i < len; ++i)
	{
		var lab = resLabs[i];
		var found = false;

		if (lab.status !== BUILT || !structureIdle(lab) || getRealPower(me) < LOW_POWER)
		{
			continue;
		}

		if ((difficulty === MEDIUM && random(100) < 10) || (difficulty >= HARD))
		{
			found = evalResearch(lab.id, nexusBranch[branch].earlyResearch);

			if (!found)
			{
				found = evalResearch(lab.id, ESSENTIAL_RESEARCH_1);
			}

			if (!found && nexusBranch[branch].type === "vtol" && random(100) < 15)
			{
				found = evalResearch(lab.id, VTOL_RESEARCH);
			}

			if (!found)
			{
				found = evalResearch(lab.id, ESSENTIAL_RESEARCH_2);
			}

			if (!found && random(100) < 5)
			{
				found = evalResearch(lab.id, SPECIAL_STRUCTURE_RESEARCH);
			}
		}

		if (!found && (difficulty === INSANE) && (random(100) < 25))
		{
			found = evalResearch(lab.id, INSANE_RESEARCH);
		}

		//Random research
		if (!found)
		{
			var resList = enumResearch();
			var len2 = resList.length;

			if (len2 > 0)
			{
				var idx = Math.floor(Math.random() * len2);

				pursueResearch(lab, resList[idx].name);
			}
		}
	}
}
