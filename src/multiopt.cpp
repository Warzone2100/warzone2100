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
/*
 * multiopt.cpp
 *
 * Alex Lee,97/98, Pumpkin Studios
 *
 * Routines for setting the game options and starting the init process.
 */
#include "lib/framework/frame.h"			// for everything

#include "lib/framework/file.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/frameresource.h"

#include "lib/ivis_opengl/piepalette.h" // for pal_Init()
#include "lib/ivis_opengl/piestate.h"

#include "map.h"
#include "game.h"			// for loading maps
#include "message.h"		// for clearing messages.
#include "main.h"
#include "display3d.h"		// for changing the viewpoint
#include "power.h"
#include "lib/widget/widget.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "hci.h"
#include "configuration.h"			// lobby cfg.
#include "clparse.h"

#include "component.h"
#include "console.h"
#include "multiplay.h"
#include "lib/sound/audio.h"
#include "multijoin.h"
#include "frontend.h"
#include "levels.h"
#include "loadsave.h"
#include "modding.h"
#include "multistat.h"
#include "multiint.h"
#include "multilimit.h"
#include "multigifts.h"
#include "multiint.h"
#include "multirecv.h"
#include "template.h"
#include "activity.h"
#include "warzoneconfig.h"

#define MAX_STRUCTURE_LIMITS 4096 // Set a high (but explicit) maximum for the number of structure limits supported

// send complete game info set!
void sendOptions()
{
	ASSERT_HOST_ONLY(return);
	ASSERT_OR_RETURN(, GetGameMode() != GS_NORMAL, "sendOptions shouldn't be called after the game has started");

	game.modHashes = getModHashList();

	NETbeginEncode(NETbroadcastQueue(), NET_OPTIONS);

	// First send information about the game
	NETuint8_t(reinterpret_cast<uint8_t*>(&game.type));
	NETstring(game.map, 128);
	NETbin(game.hash.bytes, game.hash.Bytes);
	uint32_t modHashesSize = game.modHashes.size();
	NETuint32_t(&modHashesSize);
	for (auto &hash : game.modHashes)
	{
		NETbin(hash.bytes, hash.Bytes);
	}
	NETuint8_t(&game.maxPlayers);
	NETstring(game.name, 128);
	NETuint32_t(&game.power);
	NETuint8_t(&game.base);
	NETuint8_t(&game.alliance);
	NETuint8_t(&game.scavengers);
	NETbool(&game.isMapMod);
	NETuint32_t(&game.techLevel);
	if (game.inactivityMinutes > 0 && game.inactivityMinutes < MIN_MPINACTIVITY_MINUTES)
	{
		debug(LOG_ERROR, "Invalid inactivityMinutes value specified: %" PRIu32 "; resetting to: %" PRIu32, game.inactivityMinutes, static_cast<uint32_t>(MIN_MPINACTIVITY_MINUTES));
		game.inactivityMinutes = MIN_MPINACTIVITY_MINUTES;
	}
	NETuint32_t(&game.inactivityMinutes);

	for (unsigned i = 0; i < MAX_PLAYERS; i++)
	{
		NETint8_t(reinterpret_cast<int8_t*>(&NetPlay.players[i].difficulty));
	}

	// Send the list of who is still joining
	for (unsigned i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		NETbool(&ingame.JoiningInProgress[i]);
	}

	// Same goes for the alliances
	for (unsigned i = 0; i < MAX_PLAYERS; i++)
	{
		for (unsigned j = 0; j < MAX_PLAYERS; j++)
		{
			NETuint8_t(&alliances[i][j]);
		}
	}

	// Send the number of structure limits to expect
	uint32_t numStructureLimits = static_cast<uint32_t>(ingame.structureLimits.size());
	if (numStructureLimits > MAX_STRUCTURE_LIMITS)
	{
		debug(LOG_ERROR, "Number of structure limits (%" PRIu32") exceeds maximum supported - truncating", numStructureLimits);
		numStructureLimits = MAX_STRUCTURE_LIMITS;
	}
	NETuint32_t(&numStructureLimits);
	debug(LOG_NET, "(Host) Structure limits to process on client is %zu", ingame.structureLimits.size());
	// Send the structures changed
	for (auto structLimit : ingame.structureLimits)
	{
		NETuint32_t(&structLimit.id);
		NETuint32_t(&structLimit.limit);
	}
	updateStructureDisabledFlags();
	NETuint8_t(&ingame.flags);

	NETend();

	ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
}

std::vector<MULTISTRUCTLIMITS> oldStructureLimits;

// ////////////////////////////////////////////////////////////////////////////
// options for a game. (usually recvd in frontend)
// returns: false if the options should be considered invalid and the client should disconnect
bool recvOptions(NETQUEUE queue)
{
	ASSERT_OR_RETURN(true /* silently ignore */, queue.index == NetPlay.hostPlayer, "NET_OPTIONS received from unexpected player: %" PRIu8 " - ignoring", queue.index);
	ASSERT_OR_RETURN(false, GetGameMode() != GS_NORMAL, "NET_OPTIONS received after the game has started??");

	unsigned int i;

	// store prior map / mod info
	MULTIPLAYERGAME priorGameInfo = game;

	debug(LOG_NET, "Receiving options from host");
	NETbeginDecode(queue, NET_OPTIONS);

	// Get general information about the game
	NETuint8_t(reinterpret_cast<uint8_t*>(&game.type));
	NETstring(game.map, 128);
	NETbin(game.hash.bytes, game.hash.Bytes);
	uint32_t modHashesSize;
	NETuint32_t(&modHashesSize);
	ASSERT_OR_RETURN(false, modHashesSize < 1000000, "Way too many mods %u", modHashesSize);
	game.modHashes.resize(modHashesSize);
	for (auto &hash : game.modHashes)
	{
		NETbin(hash.bytes, hash.Bytes);
	}
	NETuint8_t(&game.maxPlayers);
	NETstring(game.name, 128);
	NETuint32_t(&game.power);
	NETuint8_t(&game.base);
	NETuint8_t(&game.alliance);
	NETuint8_t(&game.scavengers);
	NETbool(&game.isMapMod);
	NETuint32_t(&game.techLevel);
	NETuint32_t(&game.inactivityMinutes);
	if (game.inactivityMinutes > 0 && game.inactivityMinutes < MIN_MPINACTIVITY_MINUTES)
	{
		debug(LOG_ERROR, "Invalid inactivityMinutes value specified: %" PRIu32, game.inactivityMinutes);
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETint8_t(reinterpret_cast<int8_t*>(&NetPlay.players[i].difficulty));
	}

	// Send the list of who is still joining
	for (i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		NETbool(&ingame.JoiningInProgress[i]);
	}

	// Alliances
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		unsigned int j;

		for (j = 0; j < MAX_PLAYERS; j++)
		{
			NETuint8_t(&alliances[i][j]);
		}
	}
	netPlayersUpdated = true;

	// Make a copy of old structure limits to see what got changed
	oldStructureLimits = ingame.structureLimits;

	// Free any structure limits we may have in-place
	ingame.structureLimits.clear();

	// Get the number of structure limits to expect
	uint32_t numStructureLimits = 0;
	NETuint32_t(&numStructureLimits);
	debug(LOG_NET, "Host is sending us %u structure limits", numStructureLimits);
	if (numStructureLimits > MAX_STRUCTURE_LIMITS)
	{
		debug(LOG_POPUP, "Number of structure limits (%" PRIu32") exceeds maximum supported. Incompatible host.", numStructureLimits);
		NETend();
		return false;
	}
	// If there were any changes allocate memory for them
	if (numStructureLimits)
	{
		ingame.structureLimits.resize(numStructureLimits);
		// If host have limits changed everyone should load limits too
		// Do not load limits if mods present because mods are not loaded yet
		if (!modHashesSize && !bLimiterLoaded)
		{
			initLoadingScreen(true);
			if (!resLoad("wrf/limiter_data.wrf", 503))
			{
				debug(LOG_INFO, "Unable to load limiter_data during recvOptions!");
			}
			else
			{
				bLimiterLoaded = true;
				closeLoadingScreen();
			}
		}
	}

	int nondefaultlimitsize = 0;
	for (i = 0; i < numStructureLimits; i++)
	{
		NETuint32_t(&ingame.structureLimits[i].id);
		NETuint32_t(&ingame.structureLimits[i].limit);
		if (bLimiterLoaded && ingame.structureLimits[i].id < numStructureStats)
		{
			ASSERT(asStructureStats != nullptr, "numStructureStats > 0, but asStructureStats is null??");
			if (asStructureStats[ingame.structureLimits[i].id].upgrade[0].limit != ingame.structureLimits[i].limit)
			{
				nondefaultlimitsize++;
			}
		}
	}
	NETuint8_t(&ingame.flags);

	NETend();

	// Do not print limits information if we don't have them loaded
	if (bLimiterLoaded)
	{
		// Check if those vectors are different
		bool structurelimitsUpdated = (oldStructureLimits.size() != ingame.structureLimits.size()) || (oldStructureLimits != ingame.structureLimits);
	
		// Notify if structure limits were changed
		if (structurelimitsUpdated)
		{
			if (nondefaultlimitsize)
			{
				addConsoleMessage(astringf(_("Changed structure limits [%d]:"), nondefaultlimitsize).c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
				int changedNum = 1;
				for (i = 0; i < numStructureLimits; i++)
				{
					if (ingame.structureLimits[i].id < numStructureStats)
					{
						if (asStructureStats[ingame.structureLimits[i].id].upgrade[0].limit != ingame.structureLimits[i].limit)
						{
							WzString structname = asStructureStats[ingame.structureLimits[i].id].name;
							std::string tmpConsoleMsgStr;
							if (asStructureStats[ingame.structureLimits[i].id].upgrade[0].limit != LOTS_OF)
							{
								tmpConsoleMsgStr = astringf(_("[%d] Limit [%s]: %u (default: %u)"), changedNum, structname.toUtf8().c_str(), ingame.structureLimits[i].limit, asStructureStats[ingame.structureLimits[i].id].upgrade[0].limit);
							}
							else
							{
								tmpConsoleMsgStr = astringf(_("[%d] Limit [%s]: %u (default: no limit)"), changedNum, structname.toUtf8().c_str(), ingame.structureLimits[i].limit);
							}
							addConsoleMessage(tmpConsoleMsgStr.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
							changedNum++;
						}
					}
					else
					{
						std::string tmpConsoleMsgStr = astringf(_("[%d] Limit that is bigger than numStructureStats (%u): %u"), i, ingame.structureLimits[i].id, ingame.structureLimits[i].limit);
						addConsoleMessage(tmpConsoleMsgStr.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
					}
				}
			}
			else
			{
				addConsoleMessage(_("Limits were reset to default."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
			}
		}
	}
	else
	{
		if (numStructureLimits)
		{
			std::string tmpConsoleMsgStr = astringf(_("Host initialized %u limits, unable to show them due to mods"), numStructureLimits);
			addConsoleMessage(tmpConsoleMsgStr.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		}
	}


	bool bRebuildMapList = strcmp(game.map, priorGameInfo.map) != 0 || game.hash != priorGameInfo.hash || game.modHashes != priorGameInfo.modHashes;
	if (bRebuildMapList)
	{
		debug(LOG_INFO, "Rebuilding map list");
		// clear out the old level list.
		levShutDown();
		levInitialise();
		rebuildSearchPath(mod_multiplay, true);	// MUST rebuild search path for the new maps we just got!
		buildMapList();
	}

	enum class FileRequestResult {
		StartingDownload,
		DownloadInProgress,
		FileExists,
		FailedToOpenFileForWriting
	};
	auto requestFile = [](Sha256 &hash, char const *filename) -> FileRequestResult {
		if (std::any_of(NET_getDownloadingWzFiles().begin(), NET_getDownloadingWzFiles().end(), [&hash](WZFile const &file) { return file.hash == hash; }))
		{
			debug(LOG_INFO, "Already requested file, continue waiting.");
			return FileRequestResult::DownloadInProgress;  // Downloading the file already
		}

		if (!PHYSFS_exists(filename))
		{
			debug(LOG_INFO, "Creating new file %s", filename);
		}
		else if (findHashOfFile(filename) != hash)
		{
			debug(LOG_INFO, "Overwriting old incomplete or corrupt file %s", filename);
		}
		else
		{
			pal_Init(); // Palette could be modded. // Why is this here - isn't there a better place for it?
			return FileRequestResult::FileExists;  // Have the file already.
		}

		PHYSFS_file *pFileHandle = PHYSFS_openWrite(filename);
		if (pFileHandle == nullptr)
		{
			debug(LOG_ERROR, "Failed to open %s for writing: %s", filename, WZ_PHYSFS_getLastError());
			return FileRequestResult::FailedToOpenFileForWriting;
		}

		NET_addDownloadingWZFile(WZFile(pFileHandle, filename, hash));

		// Request the map/mod from the host
		NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_FILE_REQUESTED);
		NETbin(hash.bytes, hash.Bytes);
		NETend();

		return FileRequestResult::StartingDownload;  // Starting download now.
	};

	LEVEL_DATASET *mapData = levFindDataSet(game.map, &game.hash);
	// See if we have the map or not
	if (mapData == nullptr)
	{
		char mapName[256];
		sstrcpy(mapName, game.map);
		removeWildcards(mapName);

		if (strlen(mapName) >= 3 && mapName[strlen(mapName) - 3] == '-' && mapName[strlen(mapName) - 2] == 'T' && unsigned(mapName[strlen(mapName) - 1] - '1') < 3)
		{
			mapName[strlen(mapName) - 3] = '\0';  // Cut off "-T1", "-T2" or "-T3".
		}
		char filename[256];
		ssprintf(filename, "maps/%dc-%s-%s.wz", game.maxPlayers, mapName, game.hash.toString().c_str());  // Wonder whether game.maxPlayers is initialised already?

		auto requestResult = requestFile(game.hash, filename);
		switch (requestResult)
		{
			case FileRequestResult::StartingDownload:
				debug(LOG_INFO, "Map was not found, requesting map %s from host, type %d", game.map, game.isMapMod);
				addConsoleMessage(_("MAP REQUESTED!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
				break;
			case FileRequestResult::DownloadInProgress:
				// do nothing - just wait
				break;
			case FileRequestResult::FileExists:
				debug(LOG_FATAL, "Can't load map %s, even though we downloaded %s", game.map, filename);
				return false;
			case FileRequestResult::FailedToOpenFileForWriting:
				// TODO: How best to handle? Ideally, message + back out of lobby?
				debug(LOG_FATAL, "Failed to open file for writing - unable to download file: %s", filename);
				return false;
		}
	}

	for (Sha256 &hash : game.modHashes)
	{
		char filename[256];
		ssprintf(filename, "mods/downloads/%s", hash.toString().c_str());

		auto requestResult = requestFile(hash, filename);
		switch (requestResult)
		{
			case FileRequestResult::StartingDownload:
				debug(LOG_INFO, "Mod was not found, requesting mod %s from host", hash.toString().c_str());
				addConsoleMessage(_("MOD REQUESTED!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
				break;
			case FileRequestResult::DownloadInProgress:
				// do nothing - just wait
				break;
			case FileRequestResult::FileExists:
				// Mod already exists / downloaded
				break;
			case FileRequestResult::FailedToOpenFileForWriting:
				// TODO: How best to handle? Ideally, message + back out of lobby?
				debug(LOG_FATAL, "Failed to open file for writing - unable to download file: %s", filename);
				return false;
		}
	}

	if (!NetPlay.isHost && !NET_getDownloadingWzFiles().empty())
	{
		// spectators should automatically become not-ready when files remain to be downloaded
		handleAutoReadyRequest();
	}

	if (mapData && CheckForMod(mapData->realFileName))
	{
		char const *str = game.isMapMod ?
			_("Warning, this is a map-mod, it could alter normal gameplay.") :
			_("Warning, HOST has altered the game code, and can't be trusted!");
		addConsoleMessage(str, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
		game.isMapMod = true;
	}
	game.isRandom = mapData && CheckForRandom(mapData->realFileName, mapData->apDataFiles[0]);

	if (mapData)
	{
		loadMapPreview(false);
	}

	ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Host Campaign.
bool hostCampaign(const char *SessionName, char *hostPlayerName, bool spectatorHost, bool skipResetAIs)
{
	debug(LOG_WZ, "Hosting campaign: '%s', player: '%s'", SessionName, hostPlayerName);

	freeMessages();

	if (!NEThostGame(SessionName, hostPlayerName, spectatorHost, static_cast<uint32_t>(game.type), 0, 0, 0, game.maxPlayers))
	{
		return false;
	}

	/* Skip resetting AIs if we are doing autohost */
	if (NetPlay.bComms && !skipResetAIs)
	{
		for (unsigned i = 0; i < MAX_CONNECTED_PLAYERS; i++)
		{
			if (!(i == selectedPlayer && i < MAX_PLAYERS))
			{
				NetPlay.players[i].difficulty = AIDifficulty::DISABLED;
			}
		}
	}

	NetPlay.players[selectedPlayer].ready = false;
	setPlayerName(selectedPlayer, hostPlayerName);

	ingame.localJoiningInProgress = true;
	ingame.JoiningInProgress[selectedPlayer] = true;
	bMultiPlayer = true;
	bMultiMessages = true; // enable messages

	PLAYERSTATS playerStats;
	loadMultiStats(hostPlayerName, &playerStats);
	setMultiStats(selectedPlayer, playerStats, false);
	setMultiStats(selectedPlayer, playerStats, true);
	lookupRatingAsync(selectedPlayer);

	ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Tell the host we are leaving the game 'nicely', (we wanted to) and not
// because we have some kind of error. (dropped or disconnected)
bool sendLeavingMsg()
{
	debug(LOG_NET, "We are leaving 'nicely'");
	NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_PLAYER_LEAVING);
	{
		bool host = NetPlay.isHost;
		uint32_t id = selectedPlayer;

		NETuint32_t(&id);
		NETbool(&host);
	}
	NETend();

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// called in Init.c to shutdown the whole netgame gubbins.
bool multiShutdown()
{
	// shut down netplay lib.
	debug(LOG_MAIN, "shutting down networking");
	NETshutdown();

	debug(LOG_MAIN, "free game data (structure limits)");
	ingame.structureLimits.clear();

	clearDisplayMultiJoiningStatusCache();

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
static bool gameInit()
{
	UDWORD			player;

	// Various sanity checks (for *true* multiplayer)
	for (player = 0; player < MAX_CONNECTED_PLAYERS; player++)
	{
		if (player < MAX_PLAYERS)
		{
			if (NetPlay.players[player].allocated)
			{
				ASSERT(NetPlay.players[player].difficulty == AIDifficulty::HUMAN, "Found an allocated (human) player (%u) with mis-matched difficulty (%d)", player, (int)NetPlay.players[player].difficulty);

			}
		}

		if (bMultiPlayer && NetPlay.bComms)
		{
			if (!NetPlay.players[player].allocated)
			{
				ASSERT(NetPlay.players[player].difficulty != AIDifficulty::HUMAN, "Found a non-human slot (%u) with mis-matched (human) difficulty (%d)", player, (int)NetPlay.players[player].difficulty);
				ASSERT(NetPlay.players[player].ai < 0 || NetPlay.players[player].difficulty != AIDifficulty::DISABLED, "Slot (%u) has AI, but disabled difficulty?", player);
			}
		}
	}

	for (player = 1; player < MAX_CONNECTED_PLAYERS; player++)
	{
		// we want to remove disabled AI & all the other players that don't belong
		if ((NetPlay.players[player].difficulty == AIDifficulty::DISABLED || player >= game.maxPlayers)
			&& player != scavengerPlayer()
			&& !(NetPlay.players[player].isSpectator && NetPlay.players[player].allocated)
			&& !(player < game.maxPlayers && NetPlay.players[player].allocated))
		{
			clearPlayer(player, true);			// do this quietly
			debug(LOG_NET, "removing disabled AI (%d) from map.", player);
		}
	}

	for (auto i = 0; i < NetPlay.players.size(); i++)
	{
		if (NetPlay.players[i].isSpectator)
		{
			// player is starting as a spectator
			makePlayerSpectator(i, true, true);
			if (i == selectedPlayer)
			{
				setPlayerHasLost(true); // set this flag to true so we don't accumulate loss statistics
			}
		}
	}

	unsigned playerCount = 0;
	for (int index = 0; index < game.maxPlayers; ++index)
	{
		playerCount += NetPlay.players[index].ai >= 0 || NetPlay.players[index].allocated;
	}
	debug(LOG_NET, "Player count: %u", playerCount);

	playerResponding();			// say howdy!

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// say hi to everyone else....
void playerResponding()
{
	ingame.startTime = std::chrono::steady_clock::now();
	ingame.localJoiningInProgress = false; // No longer joining.
	ingame.JoiningInProgress[selectedPlayer] = false;

	// Home the camera to the player
	cameraToHome(selectedPlayer, false);

	// Tell the world we're here
	NETbeginEncode(NETbroadcastQueue(), NET_PLAYERRESPONDING);
	NETuint32_t(&selectedPlayer);
	NETend();
}

// ////////////////////////////////////////////////////////////////////////////
//called when the game finally gets fired up.
bool multiGameInit()
{
	UDWORD player;

	for (player = 0; player < MAX_CONNECTED_PLAYERS; player++)
	{
		openchannels[player] = true;								//open comms to this player.
	}

	gameInit();

	return true;
}

////////////////////////////////
// at the end of every game.
bool multiGameShutdown()
{
	debug(LOG_NET, "%s is shutting down.", getPlayerName(selectedPlayer));

	sendLeavingMsg();							// say goodbye

	if (selectedPlayer < MAX_CONNECTED_PLAYERS)
	{
		PLAYERSTATS st = getMultiStats(selectedPlayer);	// save stats

		saveMultiStats(getPlayerName(selectedPlayer), getPlayerName(selectedPlayer), &st);
	}

	// if we terminate the socket too quickly, then, it is possible not to get the leave message
	uint32_t time = wzGetTicks();
	while (wzGetTicks() - time < 1000)
	{
		wzYieldCurrentThread();  // TODO Make a wzDelay() function?
	}
	// close game
	NETclose();
	NETremRedirects();

	ingame.structureLimits.clear();
	ingame.flags = 0;

	ingame.localJoiningInProgress = false; // Clean up
	ingame.localOptionsReceived = false;
	ingame.side = InGameSide::MULTIPLAYER_CLIENT;
	ingame.TimeEveryoneIsInGame = nullopt;
	ingame.startTime = std::chrono::steady_clock::time_point();
	ingame.endTime = nullopt;
	ingame.lastLagCheck = std::chrono::steady_clock::time_point();
	ingame.lastPlayerDataCheck2 = std::chrono::steady_clock::time_point();
	NetPlay.isHost					= false;
	bMultiPlayer					= false;	// Back to single player mode
	bMultiMessages					= false;
	selectedPlayer					= 0;		// Back to use player 0 (single player friendly)

	return true;
}
