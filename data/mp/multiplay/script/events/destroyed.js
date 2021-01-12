function eventDestroyed(victim)
{
	if (victim.player === selectedPlayer)
	{
		reticuleUpdate(victim, DESTROY_LIKE_EVENT);
	}
}
