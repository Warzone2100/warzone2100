
function tryLassatStrike(structureID)
{
	if (!defined(structureID))
	{
		return;
	}

	if (!hasDefinableTarget())
	{
		queue("tryLassatStrike", 5000, structureID);
		return;
	}

	var lassat = getObject(STRUCTURE, me, structureID);

	if (lassat === null)
	{
		return;
	}

	var tar = getObject(targetInfo.what.type, targetInfo.what.player, targetInfo.what.id);

	if (tar !== null)
	{
		activateStructure(lassat, tar);
	}
	else
	{
		queue("tryLassatStrike", 1000, structureID);
		return;
	}
}
