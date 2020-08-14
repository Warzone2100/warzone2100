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

#include "lib/framework/frame.h"
#include "lib/framework/physfs_ext.h"

#include <3rdparty/json/json.hpp>

#include <ctime>

#include "netreplay.h"
#include "netplay.h"


static PHYSFS_file *replaySaveHandle = nullptr;
static PHYSFS_file *replayLoadHandle = nullptr;

static const uint32_t magicReplayNumber = 0x575A7270;  // "WZrp"

bool NETreplaySaveStart()
{
	time_t aclock;
	time(&aclock);                     // Get time in seconds
	tm *newtime = localtime(&aclock);  // Convert time to struct

	bool isSkirmish = true;

	char filename[256];
	snprintf(filename, sizeof(filename), "replay/%s/%04d%02d%02d_%02d%02d%02d.wzrp", isSkirmish? "skirmish" : "campaign", newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
	replaySaveHandle = PHYSFS_openWrite(filename);  // open the file
	if (replaySaveHandle == nullptr)
	{
		debug(LOG_ERROR, "Could not create replay file %s: %s", filename, WZ_PHYSFS_getLastError());
		return false;
	}

	PHYSFS_writeSBE32(replaySaveHandle, magicReplayNumber);

	// TODO Save map name or map data and game settings and list of players in game and stuff.
	nlohmann::json settings = nlohmann::json::object();

	settings["major"] = NETGetMajorVersion();
	settings["minor"] = NETGetMinorVersion();

	auto data = settings.dump();
	PHYSFS_writeSBE32(replaySaveHandle, data.size());
	WZ_PHYSFS_writeBytes(replaySaveHandle, data.data(), data.size());

	debug(LOG_INFO, "Started writing replay file \"%s\".", filename);
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
		debug(LOG_ERROR, "Could not close replay file: %s", WZ_PHYSFS_getLastError());
		return false;
	}
	replaySaveHandle = nullptr;

	return true;
}

void NETreplaySaveNetMessage(NetMessage const *message, uint8_t player)
{
	if (!replaySaveHandle)
	{
		return;
	}

	if (message->type > GAME_MIN_TYPE && message->type < GAME_MAX_TYPE)
	{
		uint8_t *data = message->rawDataDup();
		WZ_PHYSFS_writeBytes(replaySaveHandle, &player, 1);
		WZ_PHYSFS_writeBytes(replaySaveHandle, data, message->rawLen());
		delete[] data;
	}
}

bool NETreplayLoadStart(std::string const &filename)
{
	auto onFail = [&](char const *reason) {
		debug(LOG_ERROR, "Could not load replay file %s: %s", filename.c_str(), reason);
		if (replayLoadHandle != nullptr)
		{
			PHYSFS_close(replayLoadHandle);
			replayLoadHandle = nullptr;
		}
		return false;
	};

	replayLoadHandle = PHYSFS_openRead(filename.c_str());
	if (replayLoadHandle == nullptr)
	{
		return onFail(WZ_PHYSFS_getLastError());
	}

	int32_t replayNumber = 0;
	PHYSFS_readSBE32(replayLoadHandle, &replayNumber);
	if ((uint32_t)replayNumber != magicReplayNumber)
	{
		return onFail("bad header");
	}

	int32_t dataSize = 0;
	PHYSFS_readSBE32(replayLoadHandle, &dataSize);
	std::string data;
	data.resize((uint32_t)dataSize);
	size_t dataRead = WZ_PHYSFS_readBytes(replayLoadHandle, &data[0], data.size());
	if (dataRead != data.size())
	{
		return onFail("truncated header");
	}

	// TODO Load map name or map data and game settings and list of players in game and stuff.
	nlohmann::json settings = nlohmann::json::parse(data);

	uint32_t major = settings["major"].get<uint32_t>();
	uint32_t minor = settings["minor"].get<uint32_t>();
	if (!NETisCorrectVersion(major, minor))
	{
		return onFail("wrong game version");
	}

	debug(LOG_INFO, "Started reading replay file \"%s\".", filename.c_str());
	return true;
}

bool NETreplayLoadNetMessage(std::unique_ptr<NetMessage> &message, uint8_t &player)
{
	if (!replayLoadHandle)
	{
		return false;
	}

	WZ_PHYSFS_readBytes(replayLoadHandle, &player, 1);

	uint8_t type;
	WZ_PHYSFS_readBytes(replayLoadHandle, &type, 1);

	uint32_t len = 0;
	uint8_t b;
	unsigned n = 0;
	bool rd;
	do
	{
		rd = WZ_PHYSFS_readBytes(replayLoadHandle, &b, 1);
	} while (decode_uint32_t(b, len, n++));

	if (!rd)
	{
		return false;
	}

	message = std::unique_ptr<NetMessage>(new NetMessage(type));
	message->data.resize(len);
	size_t messageRead = WZ_PHYSFS_readBytes(replayLoadHandle, message->data.data(), message->data.size());
	if (messageRead != message->data.size())
	{
		return false;
	}

	return message->type > GAME_MIN_TYPE && message->type < GAME_MAX_TYPE;
}

bool NETreplayLoadStop()
{
	if (!replayLoadHandle)
	{
		return false;
	}

	if (!PHYSFS_close(replayLoadHandle))
	{
		debug(LOG_ERROR, "Could not close replay file: %s", WZ_PHYSFS_getLastError());
		return false;
	}
	replayLoadHandle = nullptr;

	return true;
}
