function eventStructureUpgradeStarted(struct)
{
	if (struct.player === selectedPlayer)
	{
		reticuleUpdate(struct, TRANSFER_LIKE_EVENT);
	}
}
