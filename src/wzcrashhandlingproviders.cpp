/*
	This file is part of Warzone 2100.
	Copyright (C) 2021  Warzone 2100 Project

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

#include "wzcrashhandlingproviders.h"

#include "lib/framework/wzglobal.h" // required for config.h
#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/wzstring.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/string_ext.h"

#include "version.h"
#include "urlhelpers.h"
#include "activity.h"
#include "modding.h"

/* Crash-handling providers */

#if defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
# define WZ_CRASHHANDLING_PROVIDER
#endif

#if defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
# include <sentry.h>
# include <wz-sentry-config.h>
# if !defined(WZ_CRASHHANDLING_PROVIDER_SENTRY_DSN)
#  define WZ_CRASHHANDLING_PROVIDER_SENTRY_DSN ""
# endif
static bool enabledSentryProvider = false;
# define WZ_SENTRY_MAX_BREADCRUMBS 60
#endif

const size_t tagKeyMaxLength = 32;
const size_t tagValueMaxLength = 200;

#if defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
static bool initCrashHandlingProvider_Sentry(const std::string& platformPrefDir_Input, const std::string& defaultLogFilePath, bool debugCrashHandler)
{
	ASSERT_OR_RETURN(false, !platformPrefDir_Input.empty(), "platformPrefDir must not be empty");
	ASSERT_OR_RETURN(false, !defaultLogFilePath.empty(), "defaultLogFilePath must not be empty");
	if (strlen(WZ_CRASHHANDLING_PROVIDER_SENTRY_DSN) == 0)
	{
		debug(LOG_INFO, "Insufficient configuration to enable - skipping");
		return false;
	}
	sentry_options_t *options = sentry_options_new();
	if (!options)
	{
		return false;
	}
	auto releaseString = version_getBuildIdentifierReleaseString();
	auto environmentString = version_getBuildIdentifierReleaseEnvironment();
	sentry_options_set_dsn(options, WZ_CRASHHANDLING_PROVIDER_SENTRY_DSN);
	sentry_options_set_release(options, releaseString.c_str());
	sentry_options_set_environment(options, environmentString.c_str());
#ifdef DEBUG
	if (debugCrashHandler)
	{
		sentry_options_set_debug(options, 1);
	}
#endif
	// for the temp path, always use a subdirectory of the default platform pref dir
	// Make sure that we have a directory separator at the end of the string
	std::string platformPrefDir = platformPrefDir_Input;
	if (!strEndsWith(platformPrefDir, PHYSFS_getDirSeparator()))
	{
		platformPrefDir += PHYSFS_getDirSeparator();
	}
	std::string crashDbPath = platformPrefDir + ".crash-db";
#if defined(WZ_OS_WIN)
	// On Windows: Convert UTF-8 path to UTF-16 and call sentry_options_set_database_pathw
	std::vector<wchar_t> wUtf16Path;
	if (!win_utf8ToUtf16(crashDbPath.c_str(), wUtf16Path))
	{
		debug(LOG_FATAL, "Unable to convert path string to UTF16 - fatal error");
		abort();
	}
	sentry_options_set_database_pathw(options, wUtf16Path.data());
#else
	sentry_options_set_database_path(options, crashDbPath.c_str());
#endif
	std::string logFileFullPath = platformPrefDir + defaultLogFilePath;
#if defined(WZ_OS_WIN)
	// On Windows: Convert UTF-8 path to UTF-16 and call sentry_options_add_attachmentw
	if (!win_utf8ToUtf16(logFileFullPath.c_str(), wUtf16Path))
	{
		debug(LOG_FATAL, "Unable to convert path string (2) to UTF16 - fatal error");
		abort();
	}
	sentry_options_add_attachmentw(options, wUtf16Path.data());
#else
	sentry_options_add_attachment(options, logFileFullPath.c_str());
#endif
	// limit max breadcrumbs to WZ_SENTRY_MAX_BREADCRUMBS (if default exceeds it)
	size_t maxBreadcrumbs = sentry_options_get_max_breadcrumbs(options);
	if (maxBreadcrumbs > WZ_SENTRY_MAX_BREADCRUMBS)
	{
		sentry_options_set_max_breadcrumbs(options, WZ_SENTRY_MAX_BREADCRUMBS);
	}
	int result = sentry_init(options);
	return result == 0;
}

static bool shutdownCrashHandlingProvider_Sentry()
{
	sentry_close();
	return true;
}

static bool crashHandlingProviderSetTag_Sentry(const std::string& key, const std::string& value)
{
	sentry_set_tag(key.c_str(), value.c_str());
	return true;
}

sentry_value_t mapJsonToSentryValue(const nlohmann::json& instance); // forward-declaration

static sentry_value_t mapJsonObjectToSentryValue(const nlohmann::json &obj)
{
	if (!obj.is_object())
	{
		return sentry_value_new_null();
	}
	sentry_value_t value = sentry_value_new_object();
	for (auto it = obj.begin(); it != obj.end(); ++it)
	{
		sentry_value_t prop_val = mapJsonToSentryValue(it.value());
		sentry_value_set_by_key(value, it.key().c_str(), prop_val);
	}
	return value;
}

static sentry_value_t mapJsonArrayToSentryValue(const nlohmann::json &array)
{
	if (!array.is_array())
	{
		return sentry_value_new_null();
	}
	sentry_value_t value = sentry_value_new_list();
	for (int i = 0; i < array.size(); i++)
	{
		sentry_value_append(value, mapJsonToSentryValue(array.at(i)));
	}
	return value;
}

sentry_value_t mapJsonToSentryValue(const nlohmann::json& instance)
{
	switch (instance.type())
	{
		case nlohmann::json::value_t::null:
			return sentry_value_new_null();
		case nlohmann::json::value_t::boolean:
			return sentry_value_new_bool(instance.get<bool>());
		case nlohmann::json::value_t::number_integer:
		case nlohmann::json::value_t::number_unsigned:
			return sentry_value_new_int32(instance.get<int32_t>());
		case nlohmann::json::value_t::number_float:
			return sentry_value_new_double(instance.get<double>());
		case nlohmann::json::value_t::string:
		{
			auto strValue = instance.get<std::string>();
			return sentry_value_new_string(strValue.c_str());
		}
		case nlohmann::json::value_t::array:
			return mapJsonArrayToSentryValue(instance);
		case nlohmann::json::value_t::object:
			return mapJsonObjectToSentryValue(instance);
		case nlohmann::json::value_t::binary:
			debug(LOG_ERROR, "Unexpected binary value type");
			return sentry_value_new_null();
		case nlohmann::json::value_t::discarded:
			return sentry_value_new_null();
	}
	return sentry_value_new_null(); // should never be reached
}

static bool crashHandlingProviderSetContext_Sentry(const std::string& key, const nlohmann::json& contextDictionary)
{
	ASSERT_OR_RETURN(false, contextDictionary.is_object(), "contextDictionary must be an object");
	sentry_value_t value = mapJsonObjectToSentryValue(contextDictionary);
	if (sentry_value_get_type(value) != SENTRY_VALUE_TYPE_OBJECT)
	{
		return false;
	}
	sentry_set_context(key.c_str(), value);
	return true;
}

class SentryCrashHandlerActivitySink : public ActivitySink {
private:
	std::string lastGameState = "/";
private:
	void gameStateChange(const std::string& newGameState, nlohmann::json additionalData = nlohmann::json::object())
	{
		sentry_value_t crumb = sentry_value_new_breadcrumb("navigation", NULL);
		sentry_value_set_by_key(crumb, "category", sentry_value_new_string("navigation"));
		sentry_value_t data;
		if (additionalData.is_object())
		{
			data = mapJsonObjectToSentryValue(additionalData);
		}
		else
		{
			data = sentry_value_new_object();
		}
		sentry_value_set_by_key(data, "from", sentry_value_new_string(lastGameState.c_str()));
		sentry_value_set_by_key(data, "to", sentry_value_new_string(newGameState.c_str()));
		sentry_value_set_by_key(crumb, "data", data);
		sentry_add_breadcrumb(crumb);
		lastGameState = newGameState;
	}
	void gameStateToMenus()
	{
		gameStateChange("/menus");
	}
public:
	SentryCrashHandlerActivitySink()
	{
		gameStateToMenus();
	}

	// navigating main menus
	virtual void navigatedToMenu(const std::string& menuName) override
	{
		gameStateChange(std::string("/menus/") + menuName + "/");
	}

	// campaign games
	virtual void startedCampaignMission(const std::string& campaign, const std::string& levelName) override
	{
		gameStateChange(std::string("/campaign/") + campaign + "/" + levelName);
	}
	virtual void endedCampaignMission(const std::string& campaign, const std::string& levelName, GameEndReason result, END_GAME_STATS_DATA stats, bool cheatsUsed) override
	{
		if (result == GameEndReason::QUIT)
		{
			gameStateToMenus();
			return;
		}
	}

	// challenges
	virtual void startedChallenge(const std::string& challengeName) override
	{
		nlohmann::json additionalData = nlohmann::json::object();
		additionalData["name"] = challengeName;
		gameStateChange("/challenge", additionalData);
	}

	virtual void endedChallenge(const std::string& challengeName, GameEndReason result, const END_GAME_STATS_DATA& stats, bool cheatsUsed) override
	{
		gameStateToMenus();
	}

	virtual void startedSkirmishGame(const SkirmishGameInfo& info) override
	{
		std::string stateStr = "/skirmish";

		nlohmann::json additionalData = nlohmann::json::object();
		additionalData["map"] = info.game.map;
		additionalData["bots"] = info.numAIBotPlayers;
		additionalData["currentPlayerIdx"] = info.currentPlayerIdx;
		additionalData["isReplay"] = info.isReplay;
		std::string teamDescription = ActivitySink::getTeamDescription(info);
		if (!teamDescription.empty())
		{
			additionalData["teams"] = teamDescription;
		}

		gameStateChange(stateStr, additionalData);
	}

	virtual void endedSkirmishGame(const SkirmishGameInfo& info, GameEndReason result, const END_GAME_STATS_DATA& stats) override
	{
		gameStateToMenus();
	}

	// multiplayer
	virtual void hostingMultiplayerGame(const MultiplayerGameInfo& info) override
	{
		gameStateChange("/multiplayerlobby/host");
	}
	virtual void joinedMultiplayerGame(const MultiplayerGameInfo& info) override
	{
		gameStateChange("/multiplayerlobby/join");
	}
	virtual void updateMultiplayerGameInfo(const MultiplayerGameInfo& info) override
	{
		// currently, no-op
	}
	virtual void leftMultiplayerGameLobby(bool wasHost, LOBBY_ERROR_TYPES type) override
	{
		gameStateToMenus();
	}
	virtual void startedMultiplayerGame(const MultiplayerGameInfo& info) override
	{
		nlohmann::json additionalData = nlohmann::json::object();
		additionalData["isHost"] = info.isHost;
		additionalData["map"] = info.game.map;
		additionalData["bots"] = info.numAIBotPlayers;
		additionalData["settings"] = "T" + std::to_string(info.game.techLevel) + "P" + std::to_string(info.game.power) + "B" + std::to_string(info.game.base);
		additionalData["players"] = info.maxPlayers - info.numAvailableSlots;
		additionalData["maxplayers"] = info.maxPlayers;
		additionalData["currentPlayerIdx"] = info.currentPlayerIdx;
		if (info.currentPlayerIdx < info.players.size())
		{
			additionalData["isSpectator"] = info.players[info.currentPlayerIdx].isSpectator;
		}
		additionalData["isReplay"] = info.isReplay;
		gameStateChange("/multiplayer", additionalData);
	}
	virtual void endedMultiplayerGame(const MultiplayerGameInfo& info, GameEndReason result, const END_GAME_STATS_DATA& stats) override
	{
		gameStateToMenus();
	}

	// loaded mods changed
	virtual void loadedModsChanged(const std::vector<Sha256>& loadedModHashes) override
	{
		auto loadedModList = getLoadedMods();
		if (loadedModList.empty())
		{
			crashHandlingProviderSetTag_Sentry("wz.loaded_mods", "false");
			sentry_remove_context("wz.mods");
			return;
		}
		crashHandlingProviderSetTag_Sentry("wz.loaded_mods", "true");
		nlohmann::json modsInfo = nlohmann::json::object();
		for (size_t idx = 0; idx < loadedModList.size(); idx++)
		{
			nlohmann::json mod = nlohmann::json::object();
			mod["name"] = loadedModList[idx].name;
			if (idx < loadedModHashes.size())
			{
				mod["hash"] = loadedModHashes[idx].toString();
			}
			else
			{
				mod["hash"] = "<no hash available?>";
			}
			modsInfo["mod: " + std::to_string(idx)] = std::move(mod);
		}
		crashHandlingProviderSetContext_Sentry("wz.mods", modsInfo);
	}

	// game exit
	virtual void gameExiting() override
	{
		gameStateChange("/shutdown");
	}
};

#endif // defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)

bool initCrashHandlingProvider(const std::string& platformPrefDir, const std::string& defaultLogFilePath, bool debugCrashHandler)
{
#if !defined(WZ_CRASHHANDLING_PROVIDER)
	return false;
#elif defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
	// Sentry crash-handling provider
	ASSERT_OR_RETURN(true, !enabledSentryProvider, "Called more than once");
	enabledSentryProvider = initCrashHandlingProvider_Sentry(platformPrefDir, defaultLogFilePath, debugCrashHandler);
	if (enabledSentryProvider)
	{
		ActivityManager::instance().addActivitySink(std::make_shared<SentryCrashHandlerActivitySink>());
	}
	return enabledSentryProvider;
#else
	#error No available init for crash handling provider
	return false;
#endif
}

bool shutdownCrashHandlingProvider()
{
#if !defined(WZ_CRASHHANDLING_PROVIDER)
	return false;
#elif defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
	// Sentry crash-handling provider
	ASSERT_OR_RETURN(true, enabledSentryProvider, "Already shut-down");
	bool result = shutdownCrashHandlingProvider_Sentry();
	enabledSentryProvider = !result;
	return result;
#else
	// No available shutdown for crash handling provider
	return false;
#endif
}

bool crashHandlingProviderSetTag(const std::string& key, const std::string& value)
{
	ASSERT_OR_RETURN(false, key.size() <= tagKeyMaxLength, "Maximum key length exceeded");
	ASSERT_OR_RETURN(false, value.size() <= tagValueMaxLength, "Maximum value length exceeded");
#if !defined(WZ_CRASHHANDLING_PROVIDER)
	return false;
#elif defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
	// Sentry crash-handling provider
	if (!enabledSentryProvider) { return false; }
	return crashHandlingProviderSetTag_Sentry(key, value);
#else
	// No available setTag for crash handling provider
	return false;
#endif
}

bool crashHandlingProviderSetContext(const std::string& key, const nlohmann::json& contextDictionary)
{
#if !defined(WZ_CRASHHANDLING_PROVIDER)
	return false;
#elif defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
	// Sentry crash-handling provider
	if (!enabledSentryProvider) { return false; }
	return crashHandlingProviderSetContext_Sentry(key, contextDictionary);
#else
	// No available setTag for crash handling provider
	return false;
#endif
}

bool useCrashHandlingProvider(int argc, const char * const *argv, bool& out_debugCrashHandler)
{
#if !defined(WZ_CRASHHANDLING_PROVIDER)
	return false; // use native crash-handling exception handler
#else
	// if compiled with a crash-handling provider, search for "--wz-crash-rpt"
	bool useProvider = true;
	if (argv)
	{
		for (int i = 0; i < argc; ++i)
		{
			if (argv[i] && !strcasecmp(argv[i], "--wz-crash-rpt"))
			{
				useProvider = false;
			}
#if defined(DEBUG)
			else if (argv[i] && !strcasecmp(argv[i], "--wz-debug-crash-handler"))
			{
				out_debugCrashHandler = true;
			}
#endif
		}
	}
	return useProvider;
#endif
}

bool crashHandlingProviderTestCrash()
{
#if !defined(WZ_CRASHHANDLING_PROVIDER)
	return false; // caller should handle its own way
#elif defined(WZ_CRASHHANDLING_PROVIDER_SENTRY)
	// Sentry crash-handling provider
	if (!enabledSentryProvider) { return false; }
# if defined(WZ_OS_WIN)
	RaiseException(0xE000DEAD, EXCEPTION_NONCONTINUABLE, 0, 0);
	return false;
# else
	// future todo: implement for other platforms
	return false;
# endif
#else
	// No available method for this crash handling provider?
	return false;
#endif
}
