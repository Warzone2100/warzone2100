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

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "ai.h"
#include "activity.h"
#include "multiint.h"
#include "mission.h"
#include "challenge.h"
#include <algorithm>

std::string ActivitySink::getTeamDescription(const ActivitySink::SkirmishGameInfo& info)
{
	if (!alliancesSetTeamsBeforeGame(info.game.alliance))
	{
		return "";
	}
	std::map<int32_t, size_t> teamIdToCountOfPlayers;
	for (size_t index = 0; index < std::min<size_t>(info.players.size(), (size_t)game.maxPlayers); ++index)
	{
		PLAYER const &p = NetPlay.players[index];
		if (p.ai == AI_CLOSED)
		{
			// closed slot - skip
			continue;
		}
		else if (p.ai == AI_OPEN)
		{
			if (!p.allocated)
			{
				// available slot - count team association
				// (since available slots can have assigned teams)
				teamIdToCountOfPlayers[p.team]++;
			}
			else
			{
				// human player
				teamIdToCountOfPlayers[p.team]++;
			}
		}
		else
		{
			// bot player
			teamIdToCountOfPlayers[p.team]++;
		}
	}
	if (teamIdToCountOfPlayers.size() <= 1)
	{
		// does not have multiple teams
		return "";
	}
	std::string teamDescription;
	for (const auto& team : teamIdToCountOfPlayers)
	{
		if (!teamDescription.empty())
		{
			teamDescription += "v";
		}
		teamDescription += std::to_string(team.second);
	}
	return teamDescription;
}

std::string to_string(const ActivitySink::GameEndReason& reason)
{
	switch (reason)
	{
		case ActivitySink::GameEndReason::WON:
			return "Won";
		case ActivitySink::GameEndReason::LOST:
			return "Lost";
		case ActivitySink::GameEndReason::QUIT:
			return "Quit";
	}
	return ""; // silence warning
}

std::string to_string(const END_GAME_STATS_DATA& stats)
{
	return astringf("numUnits: %u, missionStartedTime: %u, unitsBuilt: %u, unitsLost: %u, unitsKilled: %u", stats.numUnits, stats.missionData.missionStarted, stats.missionData.unitsBuilt, stats.missionData.unitsLost, stats.missionData.unitsKilled);
}

class LoggingActivitySink : public ActivitySink {
public:
	// campaign games
	virtual void startedCampaignMission(const std::string& campaign, const std::string& levelName) override
	{
		debug(LOG_ACTIVITY, "- startedCampaignMission: %s:%s", campaign.c_str(), levelName.c_str());
	}
	virtual void endedCampaignMission(const std::string& campaign, const std::string& levelName, GameEndReason result, END_GAME_STATS_DATA stats, bool cheatsUsed) override
	{
		debug(LOG_ACTIVITY, "- endedCampaignMission: %s:%s; result: %s; stats: (%s)", campaign.c_str(), levelName.c_str(), to_string(result).c_str(), to_string(stats).c_str());
	}

	// challenges
	virtual void startedChallenge(const std::string& challengeName) override
	{
		debug(LOG_ACTIVITY, "- startedChallenge: %s", challengeName.c_str());
	}

	virtual void endedChallenge(const std::string& challengeName, GameEndReason result, const END_GAME_STATS_DATA& stats, bool cheatsUsed) override
	{
		debug(LOG_ACTIVITY, "- endedChallenge: %s; result: %s; stats: (%s)", challengeName.c_str(), to_string(result).c_str(), to_string(stats).c_str());
	}

	virtual void startedSkirmishGame(const SkirmishGameInfo& info) override
	{
		debug(LOG_ACTIVITY, "- startedSkirmishGame: %s", info.game.name);
	}

	virtual void endedSkirmishGame(const SkirmishGameInfo& info, GameEndReason result, const END_GAME_STATS_DATA& stats) override
	{
		debug(LOG_ACTIVITY, "- endedSkirmishGame: %s; result: %s; stats: (%s)", info.game.name, to_string(result).c_str(), to_string(stats).c_str());
	}

	// multiplayer
	virtual void hostingMultiplayerGame(const MultiplayerGameInfo& info) override
	{
		debug(LOG_ACTIVITY, "- hostingMultiplayerGame: %s; isLobbyGame: %s", info.game.name, (info.lobbyGameId != 0) ? "true" : "false");
	}
	virtual void joinedMultiplayerGame(const MultiplayerGameInfo& info) override
	{
		debug(LOG_ACTIVITY, "- joinedMultiplayerGame: %s", info.game.name);
	}
	virtual void updateMultiplayerGameInfo(const MultiplayerGameInfo& info) override
	{
		debug(LOG_ACTIVITY, "- updateMultiplayerGameInfo: (name: %s), (map: %s), maxPlayers: %u, numAvailableSlots: %zu, numHumanPlayers: %u, numAIBotPlayers: %u", info.game.name, info.game.map, info.maxPlayers, (size_t)info.numAvailableSlots, info.numHumanPlayers, info.numAIBotPlayers);
	}
	virtual void startedMultiplayerGame(const MultiplayerGameInfo& info) override
	{
		debug(LOG_ACTIVITY, "- startedMultiplayerGame: %s", info.game.name);
	}
	virtual void endedMultiplayerGame(const MultiplayerGameInfo& info, GameEndReason result, const END_GAME_STATS_DATA& stats) override
	{
		debug(LOG_ACTIVITY, "- endedMultiplayerGame: %s; result: %s; stats: (%s)", info.game.name, to_string(result).c_str(), to_string(stats).c_str());
	}

	// changing settings
	virtual void changedSetting(const std::string& settingKey, const std::string& settingValue) override
	{
		debug(LOG_ACTIVITY, "- changedSetting: %s = %s", settingKey.c_str(), settingValue.c_str());
	}

	// cheats used
	virtual void cheatUsed(const std::string& cheatName) override
	{
		debug(LOG_ACTIVITY, "- cheatUsed: %s", cheatName.c_str());
	}
};

ActivityManager& ActivityManager::instance()
{
	static ActivityManager sharedInstance = ActivityManager();
	return sharedInstance;
}

bool ActivityManager::initialize()
{
	addActivitySink(std::make_shared<LoggingActivitySink>());
	return true;
}

void ActivityManager::shutdown()
{
	// Free up the activity sinks
	activitySinks.clear();
}

void ActivityManager::addActivitySink(std::shared_ptr<ActivitySink> sink)
{
	activitySinks.push_back(sink);
}

void ActivityManager::removeActivitySink(const std::shared_ptr<ActivitySink>& sink)
{
	activitySinks.erase(std::remove(activitySinks.begin(), activitySinks.end(), sink));
}

ActivitySink::GameMode ActivityManager::getCurrentGameMode() const
{
	return currentMode;
}

ActivitySink::GameMode currentGameTypeToMode()
{
	ActivitySink::GameMode mode = ActivitySink::GameMode::CAMPAIGN;
	if (challengeActive)
	{
		mode = ActivitySink::GameMode::CHALLENGE;
	}
	else if (game.type == LEVEL_TYPE::SKIRMISH)
	{
		mode = (NetPlay.bComms) ? ActivitySink::GameMode::MULTIPLAYER : ActivitySink::GameMode::SKIRMISH;
	}
	else if (game.type == LEVEL_TYPE::CAMPAIGN)
	{
		mode = ActivitySink::GameMode::CAMPAIGN;
	}
	return mode;
}

void ActivityManager::startingGame()
{
	ActivitySink::GameMode mode = currentGameTypeToMode();
	bEndedCurrentMission = false;

	currentMode = mode;
}

void ActivityManager::startingSavedGame()
{
	ActivitySink::GameMode mode = currentGameTypeToMode();
	bEndedCurrentMission = false;

	if (mode == ActivitySink::GameMode::SKIRMISH)
	{
		// synthesize an "update multiplay game data" call on skirmish save game load
		ActivityManager::instance().updateMultiplayGameData(game, ingame, false);
	}

	currentMode = mode;

	if (cachedLoadedLevelEvent)
	{
		// process a (delayed) loaded level event
		loadedLevel(cachedLoadedLevelEvent->type, cachedLoadedLevelEvent->levelName);
		delete cachedLoadedLevelEvent;
		cachedLoadedLevelEvent = nullptr;
	}
}

void ActivityManager::loadedLevel(LEVEL_TYPE type, const std::string& levelName)
{
	bEndedCurrentMission = false;

	if (currentMode == ActivitySink::GameMode::MENUS)
	{
		// hit a case where startedGameMode is called *after* loadedLevel, so cache the loadedLevel call
		// (for example, on save game load, the game mode isn't set until the save is loaded)
		ASSERT(cachedLoadedLevelEvent == nullptr, "Missed a cached loaded level event?");
		if (cachedLoadedLevelEvent) delete cachedLoadedLevelEvent;
		cachedLoadedLevelEvent = new LoadedLevelEvent{type, levelName};
		return;
	}

	lastLoadedLevelEvent.type = type;
	lastLoadedLevelEvent.levelName = levelName;

	switch (currentMode)
	{
		case ActivitySink::GameMode::CAMPAIGN:
			for (auto sink : activitySinks) { sink->startedCampaignMission(getCampaignName(), levelName); }
			break;
		case ActivitySink::GameMode::CHALLENGE:
			for (auto sink : activitySinks) { sink->startedChallenge(currentChallengeName()); }
			break;
		case ActivitySink::GameMode::SKIRMISH:
			for (auto sink : activitySinks) { sink->startedSkirmishGame(currentMultiplayGameInfo); }
			break;
		case ActivitySink::GameMode::MULTIPLAYER:
			for (auto sink : activitySinks) { sink->startedMultiplayerGame(currentMultiplayGameInfo); }
			break;
		default:
			debug(LOG_ACTIVITY, "loadedLevel: %s; Unhandled case: %u", levelName.c_str(), (unsigned int)currentMode);
	}
}

void ActivityManager::_endedMission(ActivitySink::GameEndReason result, END_GAME_STATS_DATA stats, bool cheatsUsed)
{
	if (bEndedCurrentMission) return;

	lastLobbyGameJoinAttempt.clear();

	switch (currentMode)
	{
		case ActivitySink::GameMode::CAMPAIGN:
			for (auto sink : activitySinks) { sink->endedCampaignMission(getCampaignName(), lastLoadedLevelEvent.levelName, result, stats, cheatsUsed); }
			break;
		case ActivitySink::GameMode::CHALLENGE:
			for (auto sink : activitySinks) { sink->endedChallenge(currentChallengeName(), result, stats, cheatsUsed); }
			break;
		case ActivitySink::GameMode::SKIRMISH:
			for (auto sink : activitySinks) { sink->endedSkirmishGame(currentMultiplayGameInfo, result, stats); }
			break;
		case ActivitySink::GameMode::MULTIPLAYER:
			for (auto sink : activitySinks) { sink->endedMultiplayerGame(currentMultiplayGameInfo, result, stats); }
			break;
		default:
			debug(LOG_ACTIVITY, "endedMission: Unhandled case: %u", (unsigned int)currentMode);
	}
	bEndedCurrentMission = true;
}

void ActivityManager::completedMission(bool result, END_GAME_STATS_DATA stats, bool cheatsUsed)
{
	_endedMission(result ? ActivitySink::GameEndReason::WON : ActivitySink::GameEndReason::LOST, stats, cheatsUsed);
}

void ActivityManager::quitGame(END_GAME_STATS_DATA stats, bool cheatsUsed)
{
	if (currentMode != ActivitySink::GameMode::MENUS)
	{
		_endedMission(ActivitySink::GameEndReason::QUIT, stats, cheatsUsed);
	}

	currentMode = ActivitySink::GameMode::MENUS;
}

void ActivityManager::preSystemShutdown()
{
	// Synthesize appropriate events, as needed
	// For example, may need to synthesize a "quitGame" event if the user quit directly from window menus, etc
	if (currentMode != ActivitySink::GameMode::MENUS)
	{
		// quitGame was never generated - synthesize it
		ActivityManager::instance().quitGame(collectEndGameStatsData(), Cheated);
	}
}

void ActivityManager::beginLoadingSettings()
{
	bIsLoadingConfiguration = true;
}

void ActivityManager::changedSetting(const std::string& settingKey, const std::string& settingValue)
{
	if (bIsLoadingConfiguration) return;

	for (auto sink : activitySinks) { sink->changedSetting(settingKey, settingValue); }
}

void ActivityManager::endLoadingSettings()
{
	bIsLoadingConfiguration = false;
}

// cheats used
void ActivityManager::cheatUsed(const std::string& cheatName)
{
	for (auto sink : activitySinks) { sink->cheatUsed(cheatName); }
}

// called when a joinable multiplayer game is hosted
// lobbyGameId is 0 if the lobby can't be contacted / the game is not registered with the lobby
void ActivityManager::hostGame(const char *SessionName, const char *PlayerName, const char *lobbyAddress, unsigned int lobbyPort, const ActivitySink::ListeningInterfaces& listeningInterfaces, uint32_t lobbyGameId /*= 0*/)
{
	currentMode = ActivitySink::GameMode::HOSTING_IN_LOBBY;

	// updateMultiplayGameData should have already been called with the main details before this function is called

	currentMultiplayGameInfo.hostName = PlayerName;
	currentMultiplayGameInfo.listeningInterfaces = listeningInterfaces;
	currentMultiplayGameInfo.lobbyAddress = (lobbyAddress != nullptr) ? lobbyAddress : std::string();
	currentMultiplayGameInfo.lobbyPort = lobbyPort;
	currentMultiplayGameInfo.lobbyGameId = lobbyGameId;
	currentMultiplayGameInfo.isHost = true;

	for (auto sink : activitySinks) { sink->hostingMultiplayerGame(currentMultiplayGameInfo); }
}

void ActivityManager::hostGameLobbyServerDisconnect()
{
	if (currentMode != ActivitySink::GameMode::HOSTING_IN_LOBBY)
	{
		debug(LOG_ACTIVITY, "Unexpected call to hostGameLobbyServerDisconnect - currentMode (%u) - ignoring", (unsigned int)currentMode);
		return;
	}

	if (currentMultiplayGameInfo.lobbyGameId == 0)
	{
		debug(LOG_ACTIVITY, "Unexpected call to hostGameLobbyServerDisconnect - prior lobbyGameId is %u - ignoring", currentMultiplayGameInfo.lobbyGameId);
		return;
	}

	// The lobby server has disconnected the host (us)
	// Hence any prior lobbyGameId, etc is now invalid
	currentMultiplayGameInfo.lobbyAddress.clear();
	currentMultiplayGameInfo.lobbyPort = 0;
	currentMultiplayGameInfo.lobbyGameId = 0;

	// Inform the ActivitySinks
	// Trigger a new hostingMultiplayerGame event
	for (auto sink : activitySinks) { sink->hostingMultiplayerGame(currentMultiplayGameInfo); }
}

void ActivityManager::hostLobbyQuit()
{
	if (currentMode != ActivitySink::GameMode::HOSTING_IN_LOBBY)
	{
		debug(LOG_ACTIVITY, "Unexpected call to hostLobbyQuit - currentMode (%u) - ignoring", (unsigned int)currentMode);
		return;
	}
	currentMode = ActivitySink::GameMode::MENUS;

	// Notify the ActivitySink that we've left the game lobby
	for (auto sink : activitySinks) { sink->leftMultiplayerGameLobby(true, getLobbyError()); }
}

// called when attempting to join a lobby game
void ActivityManager::willAttemptToJoinLobbyGame(const std::string& lobbyAddress, unsigned int lobbyPort, uint32_t lobbyGameId, const std::vector<JoinConnectionDescription>& connections)
{
	lastLobbyGameJoinAttempt.lobbyAddress = lobbyAddress;
	lastLobbyGameJoinAttempt.lobbyPort = lobbyPort;
	lastLobbyGameJoinAttempt.lobbyGameId = lobbyGameId;
	lastLobbyGameJoinAttempt.connections = connections;
}

// called when an attempt to join fails
void ActivityManager::joinGameFailed(const std::vector<JoinConnectionDescription>& connection_list)
{
	lastLobbyGameJoinAttempt.clear();
}

// called when joining a multiplayer game
void ActivityManager::joinGameSucceeded(const char *host, uint32_t port)
{
	currentMode = ActivitySink::GameMode::JOINING_IN_PROGRESS;
	currentMultiplayGameInfo.isHost = false;

	// If the host and port match information in the lastLobbyGameJoinAttempt.connections,
	// store the lastLobbyGameJoinAttempt lookup info in currentMultiplayGameInfo
	bool joinedLobbyGame = false;
	for (const auto& connection : lastLobbyGameJoinAttempt.connections)
	{
		if ((connection.host == host) && (connection.port == port))
		{
			joinedLobbyGame = true;
			break;
		}
	}
	if (joinedLobbyGame)
	{
		currentMultiplayGameInfo.lobbyAddress = lastLobbyGameJoinAttempt.lobbyAddress;
		currentMultiplayGameInfo.lobbyPort = lastLobbyGameJoinAttempt.lobbyPort;
		currentMultiplayGameInfo.lobbyGameId = lastLobbyGameJoinAttempt.lobbyGameId;
	}
	lastLobbyGameJoinAttempt.clear();

	// NOTE: This is called once the join is accepted, but before all game information has been received from the host
	// Therefore, delay ActivitySink::joinedMultiplayerGame until after we receive the initial game data
}

void ActivityManager::joinedLobbyQuit()
{
	if (currentMode != ActivitySink::GameMode::JOINING_IN_LOBBY)
	{
		if (currentMode != ActivitySink::GameMode::MENUS)
		{
			debug(LOG_ACTIVITY, "Unexpected call to joinedLobbyQuit - currentMode (%u) - ignoring", (unsigned int)currentMode);
		}
		return;
	}
	currentMode = ActivitySink::GameMode::MENUS;

	// Notify the ActivitySink that we've left the game lobby
	for (auto sink : activitySinks) { sink->leftMultiplayerGameLobby(false, getLobbyError()); }
}

// for skirmish / multiplayer, provide additional data / state
void ActivityManager::updateMultiplayGameData(const MULTIPLAYERGAME& game, const MULTIPLAYERINGAME& ingame, optional<bool> privateGame)
{
	uint8_t maxPlayers = game.maxPlayers;
	uint8_t numAIBotPlayers = 0;
	uint8_t numHumanPlayers = 0;
	uint8_t numAvailableSlots = 0;

	for (size_t index = 0; index < std::min<size_t>(MAX_PLAYERS, (size_t)game.maxPlayers); ++index)
	{
		PLAYER const &p = NetPlay.players[index];
		if (p.ai == AI_CLOSED)
		{
			--maxPlayers;
		}
		else if (p.ai == AI_OPEN)
		{
			if (!p.allocated)
			{
				++numAvailableSlots;
			}
			else
			{
				++numHumanPlayers;
			}
		}
		else
		{
			if (!p.allocated)
			{
				++numAIBotPlayers;
			}
			else
			{
				++numHumanPlayers;
			}
		}
	}

	ActivitySink::MultiplayerGameInfo::AllianceOption alliances = ActivitySink::MultiplayerGameInfo::AllianceOption::NO_ALLIANCES;
	if (game.alliance == ::AllianceType::ALLIANCES)
	{
		alliances = ActivitySink::MultiplayerGameInfo::AllianceOption::ALLIANCES;
	}
	else if (game.alliance == ::AllianceType::ALLIANCES_TEAMS)
	{
		alliances = ActivitySink::MultiplayerGameInfo::AllianceOption::ALLIANCES_TEAMS;
	}
	else if (game.alliance == ::AllianceType::ALLIANCES_UNSHARED)
	{
		alliances = ActivitySink::MultiplayerGameInfo::AllianceOption::ALLIANCES_UNSHARED;
	}
	currentMultiplayGameInfo.alliances = alliances;

	currentMultiplayGameInfo.maxPlayers = maxPlayers; // accounts for closed slots
	currentMultiplayGameInfo.numHumanPlayers = numHumanPlayers;
	currentMultiplayGameInfo.numAvailableSlots = numAvailableSlots;
	// NOTE: privateGame will currently only be up-to-date for the host
	// for a joined client, it will reflect the passworded state at the time of join
	if (privateGame.has_value())
	{
		currentMultiplayGameInfo.privateGame = privateGame.value();
	}

	currentMultiplayGameInfo.game = game;
	currentMultiplayGameInfo.numAIBotPlayers = numAIBotPlayers;
	currentMultiplayGameInfo.currentPlayerIdx = selectedPlayer;
	currentMultiplayGameInfo.players = NetPlay.players;
	currentMultiplayGameInfo.players.resize(game.maxPlayers);

	currentMultiplayGameInfo.limit_no_tanks = (ingame.flags & MPFLAGS_NO_TANKS) != 0;
	currentMultiplayGameInfo.limit_no_cyborgs = (ingame.flags & MPFLAGS_NO_CYBORGS) != 0;
	currentMultiplayGameInfo.limit_no_vtols = (ingame.flags & MPFLAGS_NO_VTOLS) != 0;
	currentMultiplayGameInfo.limit_no_uplink = (ingame.flags & MPFLAGS_NO_UPLINK) != 0;
	currentMultiplayGameInfo.limit_no_lassat = (ingame.flags & MPFLAGS_NO_LASSAT) != 0;
	currentMultiplayGameInfo.force_structure_limits = (ingame.flags & MPFLAGS_FORCELIMITS) != 0;

	currentMultiplayGameInfo.structureLimits = ingame.structureLimits;

	if (currentMode == ActivitySink::GameMode::JOINING_IN_PROGRESS || currentMode == ActivitySink::GameMode::JOINING_IN_LOBBY)
	{
		currentMultiplayGameInfo.hostName = currentMultiplayGameInfo.players[0].name; // host is always player index 0?
	}

	if (currentMode == ActivitySink::GameMode::HOSTING_IN_LOBBY || currentMode == ActivitySink::GameMode::JOINING_IN_LOBBY)
	{
		for (auto sink : activitySinks) { sink->updateMultiplayerGameInfo(currentMultiplayGameInfo); }
	}
	else if (currentMode == ActivitySink::GameMode::JOINING_IN_PROGRESS)
	{
		// Have now received the initial game data, so trigger ActivitySink::joinedMultiplayerGame
		currentMode = ActivitySink::GameMode::JOINING_IN_LOBBY;
		for (auto sink : activitySinks) { sink->joinedMultiplayerGame(currentMultiplayGameInfo); }
	}
}

// called on the host when the host kicks a player
void ActivityManager::hostKickPlayer(const PLAYER& player, LOBBY_ERROR_TYPES kick_type, const std::string& reason)
{
	/* currently, no-op */
}

// called on the kicked player when they are kicked by another player
void ActivityManager::wasKickedByPlayer(const PLAYER& kicker, LOBBY_ERROR_TYPES kick_type, const std::string& reason)
{
	/* currently, no-op */
}

