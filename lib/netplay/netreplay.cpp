/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
// ////////////////////////////////////////////////////////////////////////
// Includes
#include "lib/framework/frame.h"

#include <ctime>
#include <physfs.h>

#include "netreplay.h"
#include "netplay.h"

// ////////////////////////////////////////////////////////////////////////
// Logging for debug only
// ////////////////////////////////////////////////////////////////////////

static PHYSFS_file *replaySaveHandle = nullptr;

static const uint32_t magicReplayNumber = 0x575A7270;  // "WZrp"

bool NETreplaySaveStart()
{
	time_t aclock;
	struct tm *newtime;
	static char filename[256] = {'\0'};

	time(&aclock);                 // Get time in seconds
	newtime = localtime(&aclock);  // Convert time to struct

	bool isSkirmish = true;
	snprintf(filename, sizeof(filename), "replay/%s/%04d%02d%02d_%02d%02d%02d.wzrp", isSkirmish? "skirmish" : "campaign", newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
	replaySaveHandle = PHYSFS_openWrite(filename); // open the file
	if (!replaySaveHandle)
	{
		debug(LOG_ERROR, "Could not create replay file %s: %d", filename, PHYSFS_getLastErrorCode());
		return false;
	}

	debug(LOG_INFO, "Started replay file \"%s\".", filename);

	PHYSFS_writeSBE32(replaySaveHandle, magicReplayNumber);
	PHYSFS_writeSBE32(replaySaveHandle, NETGetMajorVersion());
	PHYSFS_writeSBE32(replaySaveHandle, NETGetMinorVersion());

	// TODO Write map name or map data and game settings and list of players in game in some format.

	return true;
}

bool NETreplaySaveStop()
{
	if (!replaySaveHandle)
	{
		return false;
	}

	if (!PHYSFS_close(replaySaveHandle))
	{
		debug(LOG_ERROR, "Could not close replay file: %d", PHYSFS_getLastErrorCode());
		return false;
	}
	replaySaveHandle = nullptr;

	return true;
}

void NETreplaySaveNetMessage(NetMessage const *message, unsigned player)
{
	if (!replaySaveHandle)
	{
		return;
	}

	if (message->type > GAME_MIN_TYPE && message->type < GAME_MAX_TYPE)
	{
		uint8_t *data = message->rawDataDup();
		PHYSFS_writeUBE16(replaySaveHandle, player);
		PHYSFS_writeBytes(replaySaveHandle, data, message->rawLen());
		delete[] data;
	}
}
