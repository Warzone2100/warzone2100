/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "debugmappings.h"

#include "lib/netplay/netplay.h"

DebugInputManager::DebugInputManager(const unsigned int maxPlayers)
	: bDoingDebugMappings(false)
	, playerWantsDebugMappings(maxPlayers, false)
{
}

bool DebugInputManager::debugMappingsAllowed() const
{
	return bDoingDebugMappings;
}

bool DebugInputManager::getPlayerWantsDebugMappings(const unsigned int playerIndex) const
{
	ASSERT_OR_RETURN(false, playerIndex < playerWantsDebugMappings.size(), "Tried to get debug mapping status for playerIndex > maxPlayers");

	return playerWantsDebugMappings[playerIndex];
}

void DebugInputManager::setPlayerWantsDebugMappings(const unsigned int playerIndex, const bool bWants)
{
	playerWantsDebugMappings[playerIndex] = bWants;
	bDoingDebugMappings = true;
	for (unsigned int n = 0; n < playerWantsDebugMappings.size(); ++n)
	{
		ASSERT_OR_RETURN(, n < MAX_PLAYERS, "playerWantsDebugMappings has more entries than MAX_PLAYERS");

		const bool bIsEmptySlot = !NetPlay.players[n].allocated;
		const bool bIsSpectatorSlot = NetPlay.players[n].isSpectator;
		const bool bPlayerNWantsDebugMappings = bIsEmptySlot || bIsSpectatorSlot || playerWantsDebugMappings[n];
		bDoingDebugMappings &= bPlayerNWantsDebugMappings;
	}
}
