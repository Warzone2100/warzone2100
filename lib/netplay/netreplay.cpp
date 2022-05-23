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

#include <nlohmann/json.hpp> // Must come before WZ includes

#include "lib/framework/frame.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/wzapp.h"
#include "lib/gamelib/gtime.h"

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wcast-align"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-align"
#endif

#include <3rdparty/readerwriterqueue/readerwriterqueue.h>

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

#include <ctime>
#include <memory>

#include "netreplay.h"
#include "netplay.h"

static PHYSFS_file *replaySaveHandle = nullptr;
static PHYSFS_file *replayLoadHandle = nullptr;

static const uint32_t magicReplayNumber = 0x575A7270;  // "WZrp"
static const uint32_t currentReplayFormatVer = 2;
static const size_t DefaultReplayBufferSize = 32768;
static const size_t MaxReplayBufferSize = 2 * 1024 * 1024;

typedef std::vector<uint8_t> SerializedNetMessagesBuffer;
static moodycamel::BlockingReaderWriterQueue<SerializedNetMessagesBuffer> serializedBufferWriteQueue(256);
static SerializedNetMessagesBuffer latestWriteBuffer;
static size_t minBufferSizeToQueue = DefaultReplayBufferSize;
static std::unique_ptr<wz::thread> saveThread;

// This function is run in its own thread! Do not call any non-threadsafe functions!
static void replaySaveThreadFunc(PHYSFS_file *pSaveHandle)
{
	if (pSaveHandle == nullptr)
	{
		return;
	}
	SerializedNetMessagesBuffer item;
	while (true)
	{
		serializedBufferWriteQueue.wait_dequeue(item);
		if (item.empty())
		{
			// end chunk - we're done
			break;
		}
		WZ_PHYSFS_writeBytes(pSaveHandle, item.data(), item.size());
	}
}

bool NETreplaySaveStart(std::string const& subdir, ReplayOptionsHandler const &optionsHandler, int maxReplaysSaved, bool appendPlayerToFilename)
{
	if (NETisReplay())
	{
		// Have already loaded and will be running a replay - don't bother saving another
		debug(LOG_WZ, "Replay loaded - skip recording of new replay");
		return false;
	}

	ASSERT_OR_RETURN(false, !subdir.empty(), "Must provide a valid subdir");

	if (maxReplaysSaved > 0)
	{
		// clean up old replay files
		std::string replayFullDir = "replay/" + subdir;
		WZ_PHYSFS_cleanupOldFilesInFolder(replayFullDir.c_str(), ".wzrp", maxReplaysSaved - 1, [](const char *fileName){
			if (PHYSFS_delete(fileName) == 0)
			{
				debug(LOG_ERROR, "Failed to delete old replay file: %s", fileName);
				return false;
			}
			return true;
		});
	}

	time_t aclock;
	time(&aclock);                     // Get time in seconds
	tm *newtime = localtime(&aclock);  // Convert time to struct

	std::string filename;
	if (appendPlayerToFilename)
	{
		filename = astringf("replay/%s/%04d%02d%02d_%02d%02d%02d_%s_p%u.wzrp", subdir.c_str(), newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec, subdir.c_str(), selectedPlayer);
	}
	else
	{
		filename = astringf("replay/%s/%04d%02d%02d_%02d%02d%02d_%s.wzrp", subdir.c_str(), newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec, subdir.c_str());
	}
	replaySaveHandle = PHYSFS_openWrite(filename.c_str());  // open the file
	if (replaySaveHandle == nullptr)
	{
		debug(LOG_ERROR, "Could not create replay file %s: %s", filename.c_str(), WZ_PHYSFS_getLastError());
		return false;
	}

	WZ_PHYSFS_SETBUFFER(replaySaveHandle, 1024 * 32)//;

	PHYSFS_writeSBE32(replaySaveHandle, magicReplayNumber);

	// Save map name or map data and game settings and list of players in game and stuff.
	nlohmann::json settings = nlohmann::json::object();

	// Save "replay file format version"
	settings["replayFormatVer"] = currentReplayFormatVer;

	// Save Netcode version
	settings["major"] = NETGetMajorVersion();
	settings["minor"] = NETGetMinorVersion();

	// Save desired info from optionsHandler
	nlohmann::json gameOptions = nlohmann::json::object();
	optionsHandler.saveOptions(gameOptions);
	settings["gameOptions"] = gameOptions;

	auto data = settings.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
	PHYSFS_writeUBE32(replaySaveHandle, data.size());
	WZ_PHYSFS_writeBytes(replaySaveHandle, data.data(), data.size());

	// Save extra map data (if present)
	ReplayOptionsHandler::EmbeddedMapData embeddedMapData;
	if (!optionsHandler.saveMap(embeddedMapData))
	{
		// Failed to save map data - just empty it out for now
		embeddedMapData.mapBinaryData.clear();
	}
	PHYSFS_writeUBE32(replaySaveHandle, embeddedMapData.dataVersion);
#if SIZE_MAX > UINT32_MAX
	ASSERT_OR_RETURN(false, embeddedMapData.mapBinaryData.size() <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "Embedded map data is way too big");
#endif
	PHYSFS_writeUBE32(replaySaveHandle, static_cast<uint32_t>(embeddedMapData.mapBinaryData.size()));
	if (!embeddedMapData.mapBinaryData.empty())
	{
		WZ_PHYSFS_writeBytes(replaySaveHandle, embeddedMapData.mapBinaryData.data(), static_cast<uint32_t>(embeddedMapData.mapBinaryData.size()));
	}

	// determine best buffer size
	size_t desiredBufferSize = optionsHandler.desiredBufferSize();
	if (desiredBufferSize == 0)
	{
		// use default
		minBufferSizeToQueue = DefaultReplayBufferSize;
	}
	else if (desiredBufferSize >= MaxReplayBufferSize)
	{
		minBufferSizeToQueue = MaxReplayBufferSize;
	}
	else
	{
		minBufferSizeToQueue = desiredBufferSize;
	}

	debug(LOG_INFO, "Started writing replay file \"%s\".", filename.c_str());

	// Create a background thread and hand off all responsibility for writing to the file handle to it
	ASSERT(saveThread.get() == nullptr, "Failed to release prior thread");
	latestWriteBuffer.reserve(minBufferSizeToQueue);
	if (desiredBufferSize != std::numeric_limits<size_t>::max())
	{
		saveThread = std::unique_ptr<wz::thread>(new wz::thread(replaySaveThreadFunc, replaySaveHandle));
	}
	else
	{
		// don't use a background thread
		saveThread = nullptr;
	}

	return true;
}

bool NETreplaySaveStop()
{
	if (!replaySaveHandle)
	{
		return false;
	}

	// v2: Append the "REPLAY_ENDED" message (from hostPlayer)
	auto replayEndedMessage = NetMessage(REPLAY_ENDED);
	latestWriteBuffer.push_back(NetPlay.hostPlayer);
	replayEndedMessage.rawDataAppendToVector(latestWriteBuffer);

	// Queue the last chunk for writing
	if (!latestWriteBuffer.empty())
	{
		serializedBufferWriteQueue.enqueue(std::move(latestWriteBuffer));
	}

	// Then push one empty chunk to signify "we're done!"
	latestWriteBuffer.resize(0);
	serializedBufferWriteQueue.enqueue(std::move(latestWriteBuffer));

	// Wait for writing thread to finish
	if (saveThread)
	{
		saveThread->join();
		saveThread.reset();
	}
	else
	{
		// do the writing now on the main thread
		replaySaveThreadFunc(replaySaveHandle);
	}

	// v2: Write the "end of game info" chunk
	// (this is JSON that is preceded *and* followed by its size - so it should be possible to seek to the end of the file, read the last uint32_t, and then back up and grab the JSON without processing the whole file)
	nlohmann::json endOfGameInfo = nlohmann::json::object();
	endOfGameInfo["gameTimeElapsed"] = gameTime;
	// FUTURE TODO: Could save things like the game results / winners + losers

	auto data = endOfGameInfo.dump();
	PHYSFS_writeUBE32(replaySaveHandle, data.size());
	WZ_PHYSFS_writeBytes(replaySaveHandle, data.data(), data.size());
	PHYSFS_writeUBE32(replaySaveHandle, data.size()); // should also end with the json size for easy reading from end of file

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
		latestWriteBuffer.push_back(player);
		message->rawDataAppendToVector(latestWriteBuffer);

		if (latestWriteBuffer.size() >= minBufferSizeToQueue)
		{
			serializedBufferWriteQueue.enqueue(std::move(latestWriteBuffer));
			latestWriteBuffer = std::vector<uint8_t>();
			latestWriteBuffer.reserve(minBufferSizeToQueue);
		}
	}
}

bool NETreplayLoadStart(std::string const &filename, ReplayOptionsHandler& optionsHandler, uint32_t& output_replayFormatVer)
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

	uint32_t dataSize = 0;
	PHYSFS_readUBE32(replayLoadHandle, &dataSize);
	std::string data;
	data.resize(dataSize);
	size_t dataRead = WZ_PHYSFS_readBytes(replayLoadHandle, &data[0], data.size());
	if (dataRead != data.size())
	{
		return onFail("truncated header");
	}

	// Restore map name or map data and game settings and list of players in game and stuff.
	try
	{
		nlohmann::json settings = nlohmann::json::parse(data);

		uint32_t replayFormatVer = settings.at("replayFormatVer").get<uint32_t>();
		output_replayFormatVer = replayFormatVer;
		if (replayFormatVer > currentReplayFormatVer)
		{
			std::string mismatchVersionDescription = _("The replay file format is newer than this version of Warzone 2100 can support.");
			mismatchVersionDescription += "\n\n";
			mismatchVersionDescription += astringf(_("Replay Format Version: %u"), static_cast<unsigned>(replayFormatVer));
			wzDisplayDialog(Dialog_Error, _("Replay File Format Unsupported"), mismatchVersionDescription.c_str());

			std::string failLogStr = "Replay file format is newer than this version of Warzone 2100 can support: " + std::to_string(replayFormatVer);
			return onFail(failLogStr.c_str());
		}

		uint32_t replay_netcodeMajor = settings.at("major").get<uint32_t>();
		uint32_t replay_netcodeMinor = settings.at("minor").get<uint32_t>();
		if (!NETisCorrectVersion(replay_netcodeMajor, replay_netcodeMinor))
		{
			debug(LOG_INFO, "NetCode Version mismatch: (replay file: 0x%" PRIx32 ", 0x%" PRIx32 ") - (current: 0x%" PRIx32 ", 0x%" PRIx32 ")", replay_netcodeMajor, replay_netcodeMinor, NETGetMajorVersion(), NETGetMinorVersion());
			// do not immediately fail out - restoreOptions handles displaying a nicer warning popup
		}

		ReplayOptionsHandler::EmbeddedMapData embeddedMapData;
		if (replayFormatVer >= 2)
		{
			PHYSFS_readUBE32(replayLoadHandle, &embeddedMapData.dataVersion);
			uint32_t binaryDataSize = 0;
			PHYSFS_readUBE32(replayLoadHandle, &binaryDataSize);
			if (binaryDataSize > 0)
			{
				if (binaryDataSize <= optionsHandler.maximumEmbeddedMapBufferSize())
				{
					embeddedMapData.mapBinaryData.resize(binaryDataSize);
					PHYSFS_sint64 binaryDataRead = WZ_PHYSFS_readBytes(replayLoadHandle, embeddedMapData.mapBinaryData.data(), embeddedMapData.mapBinaryData.size());
					if (binaryDataRead != binaryDataSize)
					{
						return onFail("truncated embedded map data");
					}
				}
				else
				{
					// don't even bother trying to load this - it's too big
					// just attempt to skip to where it claims the map data ends
					PHYSFS_sint64 filePos = PHYSFS_tell(replayLoadHandle);
					if (filePos < 0)
					{
						return onFail("error getting current file position");
					}
					PHYSFS_uint64 afterMapDataPos = filePos + binaryDataSize;
					if (PHYSFS_seek(replayLoadHandle, afterMapDataPos) == 0)
					{
						return onFail("failed to seek after map data");
					}
				}
			}
		}

		// Load game options using optionsHandler
		if (!optionsHandler.restoreOptions(settings.at("gameOptions"), std::move(embeddedMapData), replay_netcodeMajor, replay_netcodeMinor))
		{
			return onFail("invalid options");
		}
	}
	catch (const std::exception& e)
	{
		// Failed to parse or find a key in the json
		std::string parseError = std::string("Error parsing info JSON (\"") + e.what() + "\")";
		return onFail(parseError.c_str());
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

	return (message->type > GAME_MIN_TYPE && message->type < GAME_MAX_TYPE) || message->type == REPLAY_ENDED;
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
