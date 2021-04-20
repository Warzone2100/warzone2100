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

#include "multishare.h"

#include "lib/netplay/netplay.h"
#include "multiplay.h"

void sendUnitShareStatus(uint8_t from, uint8_t to, bool bStatus)
{
	NETbeginEncode(NETgameQueue(selectedPlayer), static_cast<uint8_t>(MESSAGE_TYPES::GAME_UNIT_SHARE));
	NETuint8_t(&from);
	NETuint8_t(&to);
	NETbool(&bStatus);
	NETend();
}

bool recvUnitShareStatus(NETQUEUE queue)
{
	uint8_t from, to;
	bool bStatus;

	NETbeginDecode(queue, static_cast<uint8_t>(MESSAGE_TYPES::GAME_UNIT_SHARE));
	NETuint8_t(&from);
	NETuint8_t(&to);
	NETbool(&bStatus);
	NETend();

	if (!canGiveOrdersFor(queue.index, from))
	{
		return false;
	}

	NetPlay.players[from].setUnitSharingState(to, bStatus);


	return true;
}

void setUnitShareStatus(const unsigned int from, const unsigned int to, const bool bStatus)
{
	syncDebug("Set unit sharing from %d to %d = %s", from, to, bStatus ? "true" : "false");
	sendUnitShareStatus(static_cast<uint8_t>(from), static_cast<uint8_t>(to), bStatus);
}
