/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_ACTIVITY_H__
#define __INCLUDED_SRC_ACTIVITY_H__

#include "lib/framework/crc.h"
#include "scores.h"
#include "multiplay.h"
#include "levels.h"
#include "lib/netplay/netplay.h"
#include <vector>
#include <string>

#include <optional-lite/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

// Subclass ActivitySink to implement a custom handler for higher-level game-state event callbacks.
class ActivitySink
{
public:
	enum class GameMode {
		MENUS,
		TUTORIAL,
		CAMPAIGN,
		CHALLENGE,
		SKIRMISH,
		HOSTING_IN_LOBBY,
		JOINING_IN_PROGRESS, // joined but waiting on game information from host
		JOINING_IN_LOBBY,
		MULTIPLAYER
	};

	enum class GameEndReason {
		WON,
		LOST,
		QUIT
	};
public:
	virtual ~ActivitySink() { }
public:
	// campaign games
	virtual void startedCampaignMission(const std::string& campaign, const std::string& levelName) { }
	virtual void endedCampaignMission(const std::string& campaign, const std::string& levelName, GameEndReason result, END_GAME_STATS_DATA stats, bool cheatsUsed) { }

	// challenges
	virtual void startedChallenge(const std::string& challengeName) { }
	virtual void endedChallenge(const std::string& challengeName, GameEndReason result, const END_GAME_STATS_DATA& stats, bool cheatsUsed) { }

	// skirmish
	struct SkirmishGameInfo
	{
		MULTIPLAYERGAME game;
		uint8_t numAIBotPlayers = 0;
		size_t currentPlayerIdx = 0;			// = the selectedPlayer global for the current client (points to currently controlled player in the players array)
		std::vector<PLAYER> players;

		// information on limits
		bool limit_no_tanks;					///< Flag for tanks disabled
		bool limit_no_cyborgs;					///< Flag for cyborgs disabled
		bool limit_no_vtols;					///< Flag for VTOLs disabled
		bool limit_no_uplink;					///< Flag for Satellite Uplink disabled
		bool limit_no_lassat;					///< Flag for Laser Satellite Command Post disabled
		bool force_structure_limits;			///< Flag to force structure limits
		std::vector<MULTISTRUCTLIMITS> structureLimits;

		// information on alliance settings
		enum class AllianceOption
		{
			NO_ALLIANCES,        // FFA
			ALLIANCES,           // Players can make and break alliances during the game.
			ALLIANCES_TEAMS,     // Alliances are set before the game.
			ALLIANCES_UNSHARED,  // Alliances are set before the game. No shared research.
		};
		AllianceOption alliances;

	public:
		virtual ~SkirmishGameInfo() { }
		// some convenience functions to get data
		const char * gameName() const { return game.name; }
		const char * mapName() const { return game.map; }
		virtual uint8_t numberOfPlayers() const { return numAIBotPlayers + 1; }
		bool hasLimits() const { return limit_no_tanks || limit_no_cyborgs || limit_no_vtols || limit_no_uplink || limit_no_lassat || force_structure_limits; }
	};
	virtual void startedSkirmishGame(const SkirmishGameInfo& info) { }
	virtual void endedSkirmishGame(const SkirmishGameInfo& info, GameEndReason result, const END_GAME_STATS_DATA& stats) { }

	// multiplayer
	struct ListeningInterfaces {
		bool IPv4 = false;
		bool IPv6 = false;
		unsigned int ipv4_port;
		unsigned int ipv6_port;
	};
	struct MultiplayerGameInfo : public SkirmishGameInfo {
		// host information
		std::string hostName;		// host player name
		ListeningInterfaces listeningInterfaces;
		std::string lobbyAddress;
		unsigned int lobbyPort;
		uint32_t lobbyGameId = 0;

		bool isHost;	// whether the current client is the game host
		bool privateGame;			// whether game is password-protected
		uint8_t maxPlayers = 0;
		uint8_t numHumanPlayers = 0;
		uint8_t numAvailableSlots = 0;
		uint8_t numSpectators = 0;
		uint8_t numOpenSpectatorSlots = 0;
	};
	virtual void hostingMultiplayerGame(const MultiplayerGameInfo& info) { }
	virtual void joinedMultiplayerGame(const MultiplayerGameInfo& info) { }
	virtual void updateMultiplayerGameInfo(const MultiplayerGameInfo& info) { }
	virtual void leftMultiplayerGameLobby(bool wasHost, LOBBY_ERROR_TYPES type) { }
	virtual void startedMultiplayerGame(const MultiplayerGameInfo& info) { }
	virtual void endedMultiplayerGame(const MultiplayerGameInfo& info, GameEndReason result, const END_GAME_STATS_DATA& stats) { }

	// changing settings
	virtual void changedSetting(const std::string& settingKey, const std::string& settingValue) { }

	// cheats used
	virtual void cheatUsed(const std::string& cheatName) { }

	// loaded mods changed
	virtual void loadedModsChanged(const std::vector<Sha256>& loadedModHashes) { }

public:
	// Helper Functions
	static std::string getTeamDescription(const ActivitySink::SkirmishGameInfo& info);
};

std::string to_string(const ActivitySink::GameEndReason& reason);
std::string to_string(const END_GAME_STATS_DATA& stats);

// Thread-safe class for retrieving and setting ActivityRecord data
class ActivityDBProtocol
{
public:
	virtual ~ActivityDBProtocol();
public:
	virtual std::string getFirstLaunchDate() const = 0;
};

// ActivityManager accepts numerous event callbacks from the core game and synthesizes
// a (more) sensible stream of higher-level event callbacks to subscribed ActivitySinks.
//
// To access the single, global instance of ActivityManager, use ActivityManager::instance()
//
class ActivityManager
{
public:
	void startingGame();
	void startingSavedGame();
	void loadedLevel(LEVEL_TYPE type, const std::string& levelName);
	void completedMission(bool result, END_GAME_STATS_DATA stats, bool cheatsUsed);
	void quitGame(END_GAME_STATS_DATA stats, bool cheatsUsed);
	void preSystemShutdown();

	// changing settings
	void beginLoadingSettings();
	void changedSetting(const std::string& settingKey, const std::string& settingValue);
	void endLoadingSettings();

	// cheats used
	void cheatUsed(const std::string& cheatName);

	// mods reloaded / possibly changed
	void rebuiltSearchPath();

	// called when a joinable multiplayer game is hosted
	// lobbyGameId is 0 if the lobby can't be contacted / the game is not registered with the lobby
	void hostGame(const char *SessionName, const char *PlayerName, const char* lobbyAddress, unsigned int lobbyPort, const ActivitySink::ListeningInterfaces& listeningInterfaces, uint32_t lobbyGameId = 0);
	void hostGameLobbyServerDisconnect();
	void hostLobbyQuit();
	// called when attempting to join a lobby game
	void willAttemptToJoinLobbyGame(const std::string& lobbyAddress, unsigned int lobbyPort, uint32_t lobbyGameId, const std::vector<JoinConnectionDescription>& connections);
	// called when an attempt to join fails
	void joinGameFailed(const std::vector<JoinConnectionDescription>& connection_list);
	// called when joining a multiplayer game
	void joinGameSucceeded(const char *host, uint32_t port);
	void joinedLobbyQuit();
	// for skirmish / multiplayer, provide additional data / state
	void updateMultiplayGameData(const MULTIPLAYERGAME& game, const MULTIPLAYERINGAME& ingame, optional<bool> privateGame);
	// called on the host when the host kicks a player
	void hostKickPlayer(const PLAYER& player, LOBBY_ERROR_TYPES kick_type, const std::string& reason);
	// called on the kicked player when they are kicked by another player
	void wasKickedByPlayer(const PLAYER& kicker, LOBBY_ERROR_TYPES kick_type, const std::string& reason);
public:
	static ActivityManager& instance();
	bool initialize();
	void shutdown();
	void addActivitySink(std::shared_ptr<ActivitySink> sink);
	void removeActivitySink(const std::shared_ptr<ActivitySink>& sink);
	ActivitySink::GameMode getCurrentGameMode() const;
	inline std::shared_ptr<ActivityDBProtocol> getRecord() { return activityDatabase; }
private:
	ActivityManager();
	void _endedMission(ActivitySink::GameEndReason result, END_GAME_STATS_DATA stats, bool cheatsUsed);
private:
	std::vector<std::shared_ptr<ActivitySink>> activitySinks;
	std::shared_ptr<ActivityDBProtocol> activityDatabase;

	// storing current game state, to aide in synthesizing events
	bool bIsLoadingConfiguration = false;
	ActivitySink::GameMode currentMode = ActivitySink::GameMode::MENUS;
	bool bEndedCurrentMission = false;
	ActivitySink::MultiplayerGameInfo currentMultiplayGameInfo;

	struct LoadedLevelEvent {
		LEVEL_TYPE type;
		std::string levelName;
	};
	LoadedLevelEvent* cachedLoadedLevelEvent = nullptr;

	LoadedLevelEvent lastLoadedLevelEvent;

	struct FoundLobbyGameDetails
	{
		std::string lobbyAddress;
		unsigned int lobbyPort;
		uint32_t lobbyGameId;
		std::vector<JoinConnectionDescription> connections;

		void clear()
		{
			lobbyAddress.clear();
			lobbyPort = 0;
			lobbyGameId = 0;
			connections.clear();
		}
	};
	FoundLobbyGameDetails lastLobbyGameJoinAttempt;

	optional<std::vector<Sha256>> lastLoadedMods;
};

#endif // __INCLUDED_SRC_ACTIVITY_H__
