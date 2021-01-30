function eventObjectTransfer(obj, from)
{
	if (obj.player === selectedPlayer || from === selectedPlayer)
	{
		reticuleUpdate(obj, TRANSFER_LIKE_EVENT);
	}
}
