function eventStructureBuilt(struct, droid)
{
	if (struct.player === selectedPlayer)
	{
		reticuleUpdate(struct, CREATE_LIKE_EVENT);
	}
}
