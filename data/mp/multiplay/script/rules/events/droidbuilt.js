function eventDroidBuilt(droid, structure)
{
	if (droid.player === selectedPlayer)
	{
		reticuleUpdate(droid, CREATE_LIKE_EVENT);
	}
}
