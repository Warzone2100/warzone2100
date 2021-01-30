function eventStructureDemolish(struct, droid)
{
	if (struct.player === selectedPlayer)
	{
		reticuleUpdate(struct, DESTROY_LIKE_EVENT);
	}
}
