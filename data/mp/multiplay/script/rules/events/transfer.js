function eventObjectTransfer(obj, from)
{
	if (obj.player === selectedPlayer || from === selectedPlayer)
	{
		reticuleUpdate(obj, TRANSFER_LIKE_EVENT);
	}
}

function eventTransporterEmbarked(transport)
{
	if (transport.player === selectedPlayer)
	{
		reticuleUpdate(transport, TRANSFER_LIKE_EVENT);
	}
}

function eventTransporterDisembarked(transport)
{
	if (transport.player === selectedPlayer)
	{
		reticuleUpdate(transport, TRANSFER_LIKE_EVENT);
	}
}

function eventTransporterLanded(transport)
{
	if (transport.player === selectedPlayer)
	{
		reticuleUpdate(transport, TRANSFER_LIKE_EVENT);
	}
}
