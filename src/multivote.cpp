/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2023  Warzone 2100 Project

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

#include "multivote.h"

#include "lib/framework/frame.h"
#include "lib/framework/file.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"

#include "multiplay.h"
#include "multiint.h"
#include "notifications.h"
#include "hci/teamstrategy.h"
#include "ai.h"
#include "wzjsonhelpers.h"
#include "main.h"
#include "stdinreader.h"

#include <array>
#include <unordered_map>
#include <unordered_set>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;


// MARK: -

#define VOTE_TAG                 "voting"
#define VOTE_KICK_TAG_PREFIX     "votekick::"

static uint8_t playerVotes[MAX_PLAYERS];

struct PlayerPreferences
{
public:
	struct BuiltinPreferences
	{
		std::set<ScavType> scavengers;
		std::set<AllianceType> alliances;
		std::set<PowerSetting> power;
		std::set<CampType> base;
		std::set<TechLevel> techLevel;
		uint8_t minPlayers;
		uint8_t maxPlayers;

	private:
		auto tied() const
		{
			return std::tie(scavengers, alliances, power, base, techLevel, minPlayers, maxPlayers);
		}

	public:
		BuiltinPreferences();
		void reset();

		bool operator==(const BuiltinPreferences& o) const
		{
			return tied() == o.tied();
		}
	};

public:
	void reset()
	{
		builtinPreferences.reset();
	}
	void setAllBuiltin(bool pref);

	nlohmann::json prefValuesToJSON();
	bool setPrefValuesFromJSON(const nlohmann::json& j);
	BuiltinPreferences& getBuiltinPreferences() { return builtinPreferences; }
	bool setBuiltinPreferences(BuiltinPreferences&& newPrefs);

	// returns "true" if preference value changed, false if unchanged
	bool setPrefValue(ScavType val, bool pref);
	bool setPrefValue(AllianceType val, bool pref);
	bool setPrefValue(PowerSetting val, bool pref);
	bool setPrefValue(CampType val, bool pref);
	bool setPrefValue(TechLevel val, bool pref);

	// returns the value for the specified preference
	bool getPrefValue(ScavType val) const;
	bool getPrefValue(AllianceType val) const;
	bool getPrefValue(PowerSetting val) const;
	bool getPrefValue(CampType val) const;
	bool getPrefValue(TechLevel val) const;

private:
	template<typename T = uint8_t>
	void setAllPrefValues(T maxVal, bool pref, T minVal = static_cast<T>(0))
	{
		for (uint8_t i = minVal; i <= maxVal; ++i)
		{
			setPrefValue(static_cast<T>(i), pref);
		};
	}
private:
	BuiltinPreferences builtinPreferences;
};

static std::array<PlayerPreferences, MAX_CONNECTED_PLAYERS> playerPreferences;

enum class NetVoteType
{
	LOBBY_SETTING_CHANGE,
	KICK_PLAYER,
	LOBBY_OPTION_PREFERENCES_BUILTIN,
};

struct PendingVoteKick
{
	uint32_t unique_vote_id = 0;
	uint32_t requester_player_id = 0;
	uint32_t target_player_id = 0;
	std::array<optional<bool>, MAX_PLAYERS> votes;
	uint32_t start_time = 0;

public:
	PendingVoteKick(uint32_t unique_vote_id, uint32_t requester_player_id, uint32_t target_player_id)
	: unique_vote_id(unique_vote_id)
	, requester_player_id(requester_player_id)
	, target_player_id(target_player_id)
	{
		if (requester_player_id < votes.size())
		{
			votes[requester_player_id] = true;
		}
		votes[target_player_id] = false;
		start_time = realTime;
	}

public:
	bool setPlayerVote(uint32_t sender, bool vote)
	{
		ASSERT_OR_RETURN(false, sender < votes.size(), "Invalid sender: %" PRIu32, sender);
		if (votes[sender].has_value())
		{
			// ignore double-votes
			return false;
		}
		votes[sender] = vote;
		return true;
	}

	optional<bool> getKickVoteResult();
};

static std::vector<PendingVoteKick> pendingKickVotes;
constexpr uint32_t PENDING_KICK_VOTE_TIMEOUT_MS = 20000;
constexpr uint32_t MIN_INTERVAL_BETWEEN_PLAYER_KICK_VOTES_MS = 60000;
static std::array<optional<uint32_t>, MAX_PLAYERS> lastKickVoteForEachPlayer;

static bool handleVoteKickResult(PendingVoteKick& pendingVote);
static void sendPlayerMultiOptPreferencesBuiltin(uint32_t playerIdx);

// MARK: - PlayerPreferences

PlayerPreferences::BuiltinPreferences::BuiltinPreferences()
{
	minPlayers = 2;
	maxPlayers = MAX_PLAYERS_IN_GUI;
}

void PlayerPreferences::BuiltinPreferences::reset()
{
	scavengers.clear();
	alliances.clear();
	power.clear();
	base.clear();
	techLevel.clear();
	minPlayers = 2;
	maxPlayers = MAX_PLAYERS_IN_GUI;
}

template<typename T = uint8_t>
static bool validateNumToScavTypeEnum(T val)
{
	return val <= static_cast<T>(SCAV_TYPE_MAX);
}

template<typename T = uint8_t>
static bool validateNumToAlliancesTypeEnum(T val)
{
	return val <= static_cast<T>(ALLIANCE_TYPE_MAX);
}

template<typename T = uint8_t>
static bool validateNumToPowerSettingEnum(T val)
{
	return val <= static_cast<T>(POWER_SETTING_MAX);
}

template<typename T = uint8_t>
static bool validateNumToCampTypeEnum(T val)
{
	return val <= static_cast<T>(CAMP_TYPE_MAX);
}

template<typename T = uint8_t>
static bool validateNumToTechLevelEnum(T val)
{
	return val >= static_cast<uint8_t>(TECH_LEVEL_MIN) && val <= static_cast<uint8_t>(TECH_LEVEL_MAX);
}

static bool validateMinMaxPlayersPrefValue(uint8_t val)
{
	return val >= 2 && val <= MAX_PLAYERS_IN_GUI;
}

static uint8_t clampMinMaxPlayersPrefValue(uint8_t val)
{
	return std::min<uint8_t>(std::max<uint8_t>(val, 2), MAX_PLAYERS_IN_GUI);
}

inline void to_json(nlohmann::json& j, const PlayerPreferences::BuiltinPreferences& builtinPreferences) {
	j = nlohmann::json::object();
	if (!builtinPreferences.scavengers.empty())
	{
		j["scavs"] = builtinPreferences.scavengers;
	}
	if (!builtinPreferences.alliances.empty())
	{
		j["allies"] = builtinPreferences.alliances;
	}
	if (!builtinPreferences.power.empty())
	{
		j["power"] = builtinPreferences.power;
	}
	if (!builtinPreferences.base.empty())
	{
		j["base"] = builtinPreferences.base;
	}
	if (!builtinPreferences.techLevel.empty())
	{
		j["tech"] = builtinPreferences.techLevel;
	}
	auto playersObj = nlohmann::json::object();
	playersObj["min"] = builtinPreferences.minPlayers;
	playersObj["max"] = builtinPreferences.maxPlayers;
	j["players"] = std::move(playersObj);
}

template<typename T>
std::set<T> setUint8ToSetEnumT(std::set<uint8_t>&& val, std::function<bool (uint8_t)> validateValueFunc)
{
	auto result = std::set<T>();
	for (auto it = val.begin(); it != val.end(); ++it)
	{
		if (!validateValueFunc(*it))
		{
			continue;
		}
		else
		{
			result.insert(static_cast<T>(*it));
		}
	}
	return result;
}

inline void from_json(const nlohmann::json& j, PlayerPreferences::BuiltinPreferences& builtinPreferences) {
	auto it = j.find("scavs");
	if (it != j.end())
	{
		builtinPreferences.scavengers = setUint8ToSetEnumT<ScavType>(it->get<std::set<uint8_t>>(), validateNumToScavTypeEnum<uint8_t>);
	}
	it = j.find("allies");
	if (it != j.end())
	{
		builtinPreferences.alliances = setUint8ToSetEnumT<AllianceType>(it->get<std::set<uint8_t>>(), validateNumToAlliancesTypeEnum<uint8_t>);
	}
	it = j.find("power");
	if (it != j.end())
	{
		builtinPreferences.power = setUint8ToSetEnumT<PowerSetting>(it->get<std::set<uint8_t>>(), validateNumToPowerSettingEnum<uint8_t>);
	}
	it = j.find("base");
	if (it != j.end())
	{
		builtinPreferences.base = setUint8ToSetEnumT<CampType>(it->get<std::set<uint8_t>>(), validateNumToCampTypeEnum<uint8_t>);
	}
	it = j.find("tech");
	if (it != j.end())
	{
		builtinPreferences.techLevel = setUint8ToSetEnumT<TechLevel>(it->get<std::set<uint8_t>>(), validateNumToTechLevelEnum<uint8_t>);
	}
	it = j.find("players");
	if (it != j.end())
	{
		const auto& playersObj = it.value();
		auto players_it = playersObj.find("min");
		if (players_it != playersObj.end())
		{
			builtinPreferences.minPlayers = clampMinMaxPlayersPrefValue(players_it->get<uint8_t>());
		}
		players_it = playersObj.find("max");
		if (players_it != playersObj.end())
		{
			builtinPreferences.maxPlayers = clampMinMaxPlayersPrefValue(players_it->get<uint8_t>());
		}
	}
}

nlohmann::json PlayerPreferences::prefValuesToJSON()
{
	nlohmann::json result = nlohmann::json(builtinPreferences);
	return result;
}

bool PlayerPreferences::setPrefValuesFromJSON(const nlohmann::json& j)
{
	try {
		builtinPreferences = j.get<PlayerPreferences::BuiltinPreferences>();
	}
	catch (nlohmann::json::exception &e)
	{
		debug(LOG_ERROR, "Failed to load pref json with error: %s", e.what());
		return false;
	}
	return true;
}

bool PlayerPreferences::setBuiltinPreferences(BuiltinPreferences&& newPrefs)
{
	if (builtinPreferences == newPrefs)
	{
		// no change
		return false;
	}
	builtinPreferences = std::move(newPrefs);
	return true;
}

bool PlayerPreferences::getPrefValue(ScavType val) const
{
	return builtinPreferences.scavengers.count(val) != 0;
}

bool PlayerPreferences::setPrefValue(ScavType val, bool pref)
{
	if (pref)
	{
		return builtinPreferences.scavengers.insert(val).second;
	}
	return builtinPreferences.scavengers.erase(val) != 0;
}

bool PlayerPreferences::getPrefValue(AllianceType val) const
{
	return builtinPreferences.alliances.count(val) != 0;
}

bool PlayerPreferences::setPrefValue(AllianceType val, bool pref)
{
	if (pref)
	{
		return builtinPreferences.alliances.insert(val).second;;
	}
	return builtinPreferences.alliances.erase(val) != 0;
}

bool PlayerPreferences::getPrefValue(PowerSetting val) const
{
	return builtinPreferences.power.count(val) != 0;
}

bool PlayerPreferences::setPrefValue(PowerSetting val, bool pref)
{
	if (pref)
	{
		return builtinPreferences.power.insert(val).second;
	}
	return builtinPreferences.power.erase(val) != 0;
}

bool PlayerPreferences::getPrefValue(CampType val) const
{
	return builtinPreferences.base.count(val) != 0;
}

bool PlayerPreferences::setPrefValue(CampType val, bool pref)
{
	if (pref)
	{
		return builtinPreferences.base.insert(val).second;
	}
	return builtinPreferences.base.erase(val) != 0;
}

bool PlayerPreferences::getPrefValue(TechLevel val) const
{
	return builtinPreferences.techLevel.count(val) != 0;
}

bool PlayerPreferences::setPrefValue(TechLevel val, bool pref)
{
	if (pref)
	{
		return builtinPreferences.techLevel.insert(val).second;
	}
	return builtinPreferences.techLevel.erase(val) != 0;
}

void PlayerPreferences::setAllBuiltin(bool pref)
{
	setAllPrefValues<ScavType>(SCAV_TYPE_MAX, pref);
	setAllPrefValues<AllianceType>(ALLIANCE_TYPE_MAX, pref);
	setAllPrefValues<PowerSetting>(POWER_SETTING_MAX, pref);
	setAllPrefValues<CampType>(CAMP_TYPE_MAX, pref);
	setAllPrefValues<TechLevel>(TECH_LEVEL_MAX, pref, TECH_LEVEL_MIN);
}

void loadMultiOptionPrefValues(const char *sPlayerName, uint32_t playerIndex)
{
	ASSERT_OR_RETURN(, playerIndex < playerPreferences.size(), "Invalid player idx: %" PRIu32, playerIndex);
	playerPreferences[playerIndex].reset();

	// Stored alongside the profile, with the same filename but different extension
	WzString fileName = WzString(MultiPlayersPath) + sPlayerName + WzString(".multiprefs");
	auto result = wzLoadJsonObjectFromFile(fileName.toUtf8(), true);
	if (!result.has_value())
	{
		return;
	}

	// Load last used prefs set
	const auto& rootObj = result.value();
	auto it = rootObj.find("last");
	if (it != rootObj.end())
	{
		playerPreferences[playerIndex].setPrefValuesFromJSON(it.value());
	}
}

bool saveMultiOptionPrefValues(const char *sPlayerName, uint32_t playerIndex)
{
	ASSERT_OR_RETURN(false, playerIndex < playerPreferences.size(), "Invalid player idx: %" PRIu32, playerIndex);

	// Stored alongside the profile, with the same filename but different extension
	WzString fileName = WzString(MultiPlayersPath) + sPlayerName + ".multiprefs";

	nlohmann::json rootObj = nlohmann::json::object();
	// Load existing file (to preserve any future saved pref sets)
	auto result = wzLoadJsonObjectFromFile(fileName.toUtf8(), true);
	if (result.has_value())
	{
		rootObj = std::move(result.value());
	}
	rootObj["last"] = playerPreferences[playerIndex].prefValuesToJSON();

	auto dumpedJSON = rootObj.dump(4);
	return saveFile(fileName.toUtf8().c_str(), dumpedJSON.data(), dumpedJSON.size());
}

nlohmann::json getMultiOptionPrefValuesJSON(uint32_t playerIndex)
{
	ASSERT_OR_RETURN({}, playerIndex < playerPreferences.size(), "Invalid player idx: %" PRIu32, playerIndex);
	return playerPreferences[playerIndex].prefValuesToJSON();
}

void resetMultiOptionPrefValues(uint32_t player)
{
	ASSERT_OR_RETURN(, player < playerPreferences.size(), "Invalid player idx: %" PRIu32, player);
	playerPreferences[player].reset();

	if (NetPlay.bComms && ingame.side == InGameSide::MULTIPLAYER_CLIENT && !NetPlay.isHost && player == selectedPlayer)
	{
		// Notify host of change
		sendPlayerMultiOptPreferencesBuiltin(selectedPlayer);
	}
}

void resetAllMultiOptionPrefValues()
{
	for (auto& pref : playerPreferences)
	{
		pref.reset();
	}
}

void multiOptionPrefValuesSwap(uint32_t playerIndexA, uint32_t playerIndexB)
{
	if (!NetPlay.isHost) { return; }
	std::swap(playerPreferences[playerIndexA], playerPreferences[playerIndexB]);
}

void sendPlayerMultiOptPreferencesBuiltin()
{
	sendPlayerMultiOptPreferencesBuiltin(selectedPlayer);
}

template <typename T>
void setMultiOptionPrefValueT(T val, bool pref)
{
	auto& myPrefs = playerPreferences[selectedPlayer];
	if (myPrefs.setPrefValue(val, pref))
	{
		if (NetPlay.bComms && ingame.side == InGameSide::MULTIPLAYER_CLIENT && !NetPlay.isHost)
		{
			// Notify host of change
			sendPlayerMultiOptPreferencesBuiltin(selectedPlayer);
		}
	}
}

void setMultiOptionPrefValue(ScavType val, bool pref)
{
	setMultiOptionPrefValueT(val, pref);
}

void setMultiOptionPrefValue(AllianceType val, bool pref)
{
	setMultiOptionPrefValueT(val, pref);
}

void setMultiOptionPrefValue(PowerSetting val, bool pref)
{
	setMultiOptionPrefValueT(val, pref);
}

void setMultiOptionPrefValue(CampType val, bool pref)
{
	setMultiOptionPrefValueT(val, pref);
}

void setMultiOptionPrefValue(TechLevel val, bool pref)
{
	setMultiOptionPrefValueT(val, pref);
}

void setMultiOptionBuiltinPrefValues(bool pref)
{
	auto& myPrefs = playerPreferences[selectedPlayer];
	myPrefs.setAllBuiltin(pref);

	if (NetPlay.bComms && ingame.side == InGameSide::MULTIPLAYER_CLIENT && !NetPlay.isHost)
	{
		// Notify host of change
		sendPlayerMultiOptPreferencesBuiltin(selectedPlayer);
	}
}

template <typename T>
bool getMultiOptionPrefValueT(T val)
{
	const auto& myPrefs = playerPreferences[selectedPlayer];
	return myPrefs.getPrefValue(val);
}

bool getMultiOptionPrefValue(ScavType val)
{
	return getMultiOptionPrefValueT(val);
}

bool getMultiOptionPrefValue(AllianceType val)
{
	return getMultiOptionPrefValueT(val);
}

bool getMultiOptionPrefValue(PowerSetting val)
{
	return getMultiOptionPrefValueT(val);
}

bool getMultiOptionPrefValue(CampType val)
{
	return getMultiOptionPrefValueT(val);
}

bool getMultiOptionPrefValue(TechLevel val)
{
	return getMultiOptionPrefValueT(val);
}

template <typename T>
size_t getMultiOptionPrefValueTotalT(T val, bool playersOnly)
{
	size_t result = 0;
	for (size_t idx = 0; idx < playerPreferences.size(); ++idx)
	{
		if (playersOnly && idx >= MAX_PLAYERS) { break; }
		const auto& prefs = playerPreferences[idx];
		if (prefs.getPrefValue(val)) { ++result; }
	}
	return result;
}

size_t getMultiOptionPrefValueTotal(ScavType val, bool playersOnly)
{
	return getMultiOptionPrefValueTotalT(val, playersOnly);
}

size_t getMultiOptionPrefValueTotal(AllianceType val, bool playersOnly)
{
	return getMultiOptionPrefValueTotalT(val, playersOnly);
}

size_t getMultiOptionPrefValueTotal(PowerSetting val, bool playersOnly)
{
	return getMultiOptionPrefValueTotalT(val, playersOnly);
}

size_t getMultiOptionPrefValueTotal(CampType val, bool playersOnly)
{
	return getMultiOptionPrefValueTotalT(val, playersOnly);
}

size_t getMultiOptionPrefValueTotal(TechLevel val, bool playersOnly)
{
	return getMultiOptionPrefValueTotalT(val, playersOnly);
}

static constexpr uint32_t MaxSupportedSetMembers = static_cast<uint32_t>(std::numeric_limits<uint8_t>::max());

template<typename T>
bool NETEnumTSet_uint8(MessageReader& r, std::set<T>& val, std::function<bool(uint8_t)> validateValueFunc = nullptr)
{
	bool retVal = true;
	uint32_t numElements = 0;

	val.clear();

	NETuint32_t(r, numElements);

	if (numElements > MaxSupportedSetMembers)
	{
		debug(LOG_NET, "Invalid number of set members: %" PRIu32, numElements);
		// will skip extras in the loop, set return value to false
		retVal = false;
	}
	for (uint32_t i = 0; i < numElements; ++i)
	{
		uint8_t el = 0;
		NETuint8_t(r, el);
		if (i < numElements)
		{
			if (validateValueFunc)
			{
				if (!validateValueFunc(el))
				{
					retVal = false;
					continue;
				}
			}
			val.insert(static_cast<T>(el));
		}
	}
	return retVal;
}

template<typename T>
bool NETEnumTSet_uint8(MessageWriter& w, std::set<T>& val, std::function<bool(uint8_t)> validateValueFunc = nullptr)
{
	bool retVal = true;
	uint32_t numElements = 0;


#if SIZE_MAX > UINT32_MAX
	if (val.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max()))
	{
		numElements = 0;
		NETuint32_t(w, numElements);
		return false;
	}
#endif
	numElements = static_cast<uint32_t>(val.size());
	if (numElements > MaxSupportedSetMembers)
	{
		ASSERT(false, "Invalid number of set members: %" PRIu32, numElements);
		numElements = MaxSupportedSetMembers;
	}

	NETuint32_t(w, numElements);

	size_t i = 0;
	for (auto el : val)
	{
		if (i >= numElements)
		{
			break;
		}
		uint8_t el_uint8 = static_cast<uint8_t>(el);
		NETuint8_t(w, el_uint8);
		++i;
	}
	return retVal;
}

template <typename SerdeContext>
bool NETbuiltinPlayerPreferences(SerdeContext& c, PlayerPreferences::BuiltinPreferences& builtinPrefs)
{
	static_assert(std::is_same<SerdeContext, MessageReader>::value || std::is_same<SerdeContext, MessageWriter>::value,
		"SerdeContext is expected to be either MessageReader or MessageWriter");

	bool retSuccess = true;
	retSuccess = NETEnumTSet_uint8<ScavType>(c, builtinPrefs.scavengers, validateNumToScavTypeEnum<uint8_t>) && retSuccess;
	retSuccess = NETEnumTSet_uint8<AllianceType>(c, builtinPrefs.alliances, validateNumToAlliancesTypeEnum<uint8_t>) && retSuccess;
	retSuccess = NETEnumTSet_uint8<PowerSetting>(c, builtinPrefs.power, validateNumToPowerSettingEnum<uint8_t>) && retSuccess;
	retSuccess = NETEnumTSet_uint8<CampType>(c, builtinPrefs.base, validateNumToCampTypeEnum<uint8_t>) && retSuccess;
	retSuccess = NETEnumTSet_uint8<TechLevel>(c, builtinPrefs.techLevel, validateNumToTechLevelEnum<uint8_t>) && retSuccess;
	NETuint8_t(c, builtinPrefs.minPlayers);
	NETuint8_t(c, builtinPrefs.maxPlayers);
	if (!validateMinMaxPlayersPrefValue(builtinPrefs.minPlayers))
	{
		builtinPrefs.minPlayers = 2;
		retSuccess = false;
	}
	if (!validateMinMaxPlayersPrefValue(builtinPrefs.maxPlayers))
	{
		builtinPrefs.maxPlayers = MAX_PLAYERS_IN_GUI;
		retSuccess = false;
	}
	return retSuccess;
}

// MARK: -

void resetKickVoteData()
{
	pendingKickVotes.clear();
	for (auto& val : lastKickVoteForEachPlayer)
	{
		val.reset();
	}
}

void resetLobbyChangeVoteData()
{
	for (unsigned int i = 0; i < MAX_PLAYERS; ++i)
	{
		playerVotes[i] = 0;
	}
}

void resetLobbyChangePlayerVote(uint32_t player)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Invalid player: %" PRIu32, player);
	playerVotes[player] = 0;
}

void sendLobbyChangeVoteData(uint8_t currentVote)
{
	auto w = NETbeginEncode(NETbroadcastQueue(), NET_VOTE);
	NETuint32_t(w, selectedPlayer);
	uint8_t voteType = static_cast<uint8_t>(NetVoteType::LOBBY_SETTING_CHANGE);
	NETuint8_t(w, voteType);
	NETuint8_t(w, currentVote);
	NETend(w);
}

uint8_t getLobbyChangeVoteTotal()
{
	ASSERT_HOST_ONLY(return true);

	uint8_t total = 0;

	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		if (isHumanPlayer(i))
		{
			if (selectedPlayer == i)
			{
				// always count the host as a "yes" vote.
				playerVotes[i] = 1;
			}
			total += playerVotes[i];
		}
		else
		{
			playerVotes[i] = 0;
		}
	}

	return total;
}

static void recvLobbyChangeVote(uint32_t player, uint8_t newVote)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "Invalid sender: %" PRIu32, player);

	playerVotes[player] = (newVote == 1) ? 1 : 0;

	debug(LOG_NET, "total votes: %d/%d", static_cast<int>(getLobbyChangeVoteTotal()), static_cast<int>(NET_numHumanPlayers()));

	// there is no "votes" that disallows map change so assume they are all allowing
	if(newVote == 1) {
		char msg[128] = {0};
		ssprintf(msg, _("%s (%d) allowed map change. Total: %d/%d"), getPlayerName(player, true), player, static_cast<int>(getLobbyChangeVoteTotal()), static_cast<int>(NET_numHumanPlayers()));
		sendRoomSystemMessage(msg);
	}
}

void sendPlayerKickedVote(uint32_t voteID, uint8_t newVote)
{
	auto w = NETbeginEncode(NETbroadcastQueue(), NET_VOTE);
	NETuint32_t(w, selectedPlayer);
	uint8_t voteType = static_cast<uint8_t>(NetVoteType::KICK_PLAYER);
	NETuint8_t(w, voteType);
	NETuint32_t(w, voteID);
	NETuint8_t(w, newVote);
	NETend(w);
}

static void recvPlayerKickVote(uint32_t voteID, uint32_t sender, uint8_t newVote)
{
	ASSERT_OR_RETURN(, sender < MAX_PLAYERS, "Invalid sender: %" PRIu32, sender);

	auto it = std::find_if(pendingKickVotes.begin(), pendingKickVotes.end(), [voteID](const PendingVoteKick& a) -> bool {
		return a.unique_vote_id == voteID;
	});
	if (it == pendingKickVotes.end())
	{
		// didn't find the vote? may have already ended
		return;
	}

	bool voteToKick = (newVote == 1);
	if (it->setPlayerVote(sender, voteToKick))
	{
		std::string outputMsg;
		if (voteToKick)
		{
			outputMsg = astringf(_("A player voted FOR kicking: %s"), getPlayerName(it->target_player_id, true));
			sendInGameSystemMessage(outputMsg.c_str());
			debug(LOG_INFO, "Player [%" PRIu32 "] %s voted FOR kicking player: %s", sender, getPlayerName(sender, true), getPlayerName(it->target_player_id, true));
		}
		else
		{
			if (newVote == 0)
			{
				outputMsg = astringf(_("A player voted AGAINST kicking: %s"), getPlayerName(it->target_player_id, true));
				sendInGameSystemMessage(outputMsg.c_str());
				debug(LOG_INFO, "Player [%" PRIu32 "] %s voted AGAINST kicking player: %s", sender, getPlayerName(sender, true), getPlayerName(it->target_player_id, true));
			}
			else
			{
				outputMsg = astringf(_("A player's client ignored your vote to kick request (too frequent): %s"), getPlayerName(it->target_player_id, true));
				addConsoleMessage(outputMsg.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE, false); // only display to the host
				debug(LOG_INFO, "Player [%" PRIu32 "] %s ignored vote to kick request for player: %s - (too frequent)", sender, getPlayerName(sender, true), getPlayerName(it->target_player_id, true));
			}
		}
	}

	if (handleVoteKickResult(*it))
	{
		// got a result and handled it
		pendingKickVotes.erase(it);
	}
}

static void sendPlayerMultiOptPreferencesBuiltin(uint32_t playerIdx)
{
	ASSERT_OR_RETURN(, playerIdx < MAX_CONNECTED_PLAYERS, "Invalid player idx: %" PRIu32, playerIdx);
	ASSERT_OR_RETURN(, whosResponsible(playerIdx) == selectedPlayer || NetPlay.isHost, "Sending unexpected player prefs: %" PRIu32, playerIdx);
	ASSERT_OR_RETURN(, GetGameMode() != GS_NORMAL, "Trying to send multiopt preferences after game started?");

	auto w = NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_VOTE);
	NETuint32_t(w, selectedPlayer);
	uint8_t voteType = static_cast<uint8_t>(NetVoteType::LOBBY_OPTION_PREFERENCES_BUILTIN);
	NETuint8_t(w, voteType);
	NETbuiltinPlayerPreferences(w, playerPreferences[playerIdx].getBuiltinPreferences());
	NETend(w);
}

bool recvPlayerMultiOptPreferencesBuiltin(int32_t sender, PlayerPreferences::BuiltinPreferences&& builtinPreferences)
{
	ASSERT_OR_RETURN(false, sender < MAX_CONNECTED_PLAYERS, "Invalid sender: %" PRIu32, sender);
	auto prefsChanged = playerPreferences[sender].setBuiltinPreferences(std::move(builtinPreferences));
	if (prefsChanged)
	{
		wz_command_interface_output_room_status_json(true);
	}
	return prefsChanged;
}

bool recvVote(NETQUEUE queue, bool inLobby)
{
	ASSERT_HOST_ONLY(return false);

	uint32_t player = MAX_PLAYERS;
	uint32_t voteID = 0;
	uint8_t voteType = 0;
	uint8_t newVote = 0;
	PlayerPreferences::BuiltinPreferences builtinPreferences;
	bool senderIsSpectator = (queue.index < NetPlay.players.size()) ? NetPlay.players[queue.index].isSpectator : true;
	bool validPrefs = false;

	auto r = NETbeginDecode(queue, NET_VOTE);
	NETuint32_t(r, player);
	NETuint8_t(r, voteType);

	switch (static_cast<NetVoteType>(voteType))
	{
		case NetVoteType::LOBBY_SETTING_CHANGE:
			NETuint8_t(r, newVote);
			break;
		case NetVoteType::KICK_PLAYER:
			NETuint32_t(r, voteID);
			NETuint8_t(r, newVote);
			break;
		case NetVoteType::LOBBY_OPTION_PREFERENCES_BUILTIN:
			if (inLobby)
			{
				validPrefs = NETbuiltinPlayerPreferences(r, builtinPreferences);
			}
			break;
	}

	NETend(r);

	switch (static_cast<NetVoteType>(voteType))
	{
		case NetVoteType::LOBBY_SETTING_CHANGE:
		case NetVoteType::KICK_PLAYER:
			if (senderIsSpectator)
			{
				// Silently ignore LOBBY_SETTING_CHANGE and KICK_PLAYER votes from spectators
				return false;
			}
			if (player >= MAX_PLAYERS)
			{
				debug(LOG_NET, "Invalid NET_VOTE from player %d: player id = %d", queue.index, static_cast<int>(player));
				return false;
			}
			break;
		case NetVoteType::LOBBY_OPTION_PREFERENCES_BUILTIN:
			if (!inLobby || player >= MAX_CONNECTED_PLAYERS)
			{
				debug(LOG_NET, "Invalid NET_VOTE from player %d: player id = %d", queue.index, static_cast<int>(player));
				return false;
			}
			break;
	}

	if (whosResponsible(player) != queue.index)
	{
		debug(LOG_NET, "Invalid NET_VOTE from player %d: (for player id = %d)", queue.index, static_cast<int>(player));
		return false;
	}

	switch (static_cast<NetVoteType>(voteType))
	{
		case NetVoteType::LOBBY_SETTING_CHANGE:
			recvLobbyChangeVote(player, newVote);
			return true;
		case NetVoteType::KICK_PLAYER:
			recvPlayerKickVote(voteID, player, newVote);
			return true;
		case NetVoteType::LOBBY_OPTION_PREFERENCES_BUILTIN:
			if (!validPrefs)
			{
				debug(LOG_NET, "Invalid NET_VOTE from player %d: (for player id = %d)", queue.index, static_cast<int>(player));
				return false;
			}
			return recvPlayerMultiOptPreferencesBuiltin(player, std::move(builtinPreferences));
	}

	return false;
}

// Show a vote popup to allow changing maps or using the randomization feature.
static void setupLobbyChangeVoteChoice()
{
	//This shouldn't happen...
	if (NetPlay.isHost)
	{
		ASSERT(false, "Host tried to send vote data to themself");
		return;
	}

	if (!hasNotificationsWithTag(VOTE_TAG))
	{
		WZ_Notification notification;
		notification.duration = 0;
		notification.contentTitle = _("Vote");
		notification.contentText = _("Allow host to change map or randomize?");
		notification.action = WZ_Notification_Action(_("Allow"), [](const WZ_Notification&) {
			uint8_t vote = 1;
			sendLobbyChangeVoteData(vote);
		});
		notification.tag = VOTE_TAG;

		addNotification(notification, WZ_Notification_Trigger(GAME_TICKS_PER_SEC * 1));
	}
}

// Show a kick vote popup
static void setupKickVoteChoice(uint32_t voteID, uint32_t targetPlayer)
{
	//This shouldn't happen...
	if (NetPlay.isHost)
	{
		ASSERT(false, "Host tried to send vote data to themself?");
		return;
	}

	if (targetPlayer >= MAX_PLAYERS)
	{
		// Invalid targetPlayer
		return;
	}

	bool targetIsActiveAIPlayer = NetPlay.players[targetPlayer].allocated == false && NetPlay.players[targetPlayer].ai >= 0 && !NetPlay.players[targetPlayer].isSpectator;
	if (!NetPlay.players[targetPlayer].allocated && !targetIsActiveAIPlayer)
	{
		// no active player to vote on
		return;
	}

	if (lastKickVoteForEachPlayer[targetPlayer].has_value())
	{
		if (realTime - lastKickVoteForEachPlayer[targetPlayer].value() < MIN_INTERVAL_BETWEEN_PLAYER_KICK_VOTES_MS)
		{
			// Host is spamming kick requests - deny it automatically
			if (targetPlayer != selectedPlayer)
			{
				sendPlayerKickedVote(voteID, 2);
			}
			return;
		}
	}

	if (targetPlayer == selectedPlayer)
	{
		// The vote is for the current player - just display a local console message and return
		addConsoleMessage(_("A vote was started to kick you from the game."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE, false);
		debug(LOG_INFO, "A vote was started to kick you from the game.");
		return;
	}

	std::string notificationTag = VOTE_KICK_TAG_PREFIX + std::to_string(targetPlayer);

	if (hasNotificationsWithTag(notificationTag))
	{
		// dismiss existing notification targeting this player
		cancelOrDismissNotificationsWithTag(notificationTag);
	}

	lastKickVoteForEachPlayer[targetPlayer] = realTime;

	std::string outputMsg = astringf(_("A vote was started to kick %s from the game."), getPlayerName(targetPlayer));
	addConsoleMessage(outputMsg.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE, false);
	debug(LOG_INFO, "A vote was started to kick %s from the game.", getPlayerName(targetPlayer));

	WZ_Notification notification;
	notification.duration = 0;
	const char* pPlayerName = getPlayerName(static_cast<int32_t>(targetPlayer));
	std::string playerDisplayName = (pPlayerName) ? std::string(pPlayerName) : astringf("<player %" PRIi32 ">", targetPlayer);
	notification.contentTitle = astringf(_("Vote To Kick: %s"), playerDisplayName.c_str());
	notification.contentText = astringf(_("Should player %s be kicked from the game?"), playerDisplayName.c_str());
	notification.action = WZ_Notification_Action(_("Yes, Kick Them"), [voteID](const WZ_Notification&) {
		sendPlayerKickedVote(voteID, 1);
	});
	notification.onDismissed = [voteID](const WZ_Notification&, WZ_Notification_Dismissal_Reason reason) {
		if (reason != WZ_Notification_Dismissal_Reason::USER_DISMISSED) { return; }
		sendPlayerKickedVote(voteID, 0);
	};
	notification.tag = notificationTag;

	addNotification(notification, WZ_Notification_Trigger(GAME_TICKS_PER_SEC * 1));
}

static bool sendVoteRequest(NetVoteType type, uint32_t voteID = 0, uint32_t targetPlayer = 0)
{
	ASSERT_HOST_ONLY(return false);

	//setup a vote popup for the clients
	auto w = NETbeginEncode(NETbroadcastQueue(), NET_VOTE_REQUEST);
	NETuint32_t(w, selectedPlayer);
	NETuint32_t(w, targetPlayer);
	NETuint32_t(w, voteID);
	uint8_t voteType = static_cast<uint8_t>(type);
	NETuint8_t(w, voteType);
	NETend(w);

	return true;
}

bool recvVoteRequest(NETQUEUE queue)
{
	uint32_t sender = MAX_PLAYERS;
	uint32_t targetPlayer = MAX_PLAYERS;
	uint32_t voteID = 0;
	uint8_t voteType = 0;
	auto r = NETbeginDecode(queue, NET_VOTE_REQUEST);
	NETuint32_t(r, sender);
	NETuint32_t(r, targetPlayer);
	NETuint32_t(r, voteID);
	NETuint8_t(r, voteType);
	NETend(r);

	if (sender >= MAX_PLAYERS)
	{
		debug(LOG_NET, "Invalid NET_VOTE_REQUEST from player %d: player id = %d", queue.index, static_cast<int>(sender));
		return false;
	}

	if (whosResponsible(sender) != queue.index)
	{
		debug(LOG_NET, "Invalid NET_VOTE_REQUEST from player %d: (for player id = %d)", queue.index, static_cast<int>(sender));
		return false;
	}

	switch (static_cast<NetVoteType>(voteType))
	{
		case NetVoteType::LOBBY_SETTING_CHANGE:
			setupLobbyChangeVoteChoice();
			return true;
		case NetVoteType::KICK_PLAYER:
			setupKickVoteChoice(voteID, targetPlayer);
			return true;
		case NetVoteType::LOBBY_OPTION_PREFERENCES_BUILTIN:
			return false;
	}

	return false;
}

void startLobbyChangeVote()
{
	ASSERT_HOST_ONLY(return);
	sendVoteRequest(NetVoteType::LOBBY_SETTING_CHANGE, 0, NetPlay.hostPlayer);
}

static std::vector<uint32_t> getPlayersWhoCanVoteToKick()
{
	std::vector<uint32_t> connectedHumanPlayers;
	for (int32_t player = 0; player < std::min<int32_t>(game.maxPlayers, MAX_PLAYERS); ++player)
	{
		if (isHumanPlayer(player) // is an active (connected) human player
			&& !NetPlay.players[player].isSpectator // who is *not* a spectator
			)
		{
			connectedHumanPlayers.push_back(static_cast<uint32_t>(player));
		}
	}
	return connectedHumanPlayers;
}

static std::vector<uint32_t> filterPlayersByTeam(const std::vector<uint32_t>& playersToFilter, int32_t specifiedTeam)
{
	std::vector<uint32_t> playersOnSameTeamAsDesired;
	for (auto player : playersToFilter)
	{
		if (checkedGetPlayerTeam(player) != specifiedTeam)
		{
			continue;
		}
		playersOnSameTeamAsDesired.push_back(static_cast<uint32_t>(player));
	}

	return playersOnSameTeamAsDesired;
}

static std::vector<uint32_t> getPlayersNotOnTeam(const std::vector<uint32_t>& playersToFilter, int32_t specifiedTeam)
{
	std::vector<uint32_t> playersNotOnTeam;
	for (auto player : playersToFilter)
	{
		if (checkedGetPlayerTeam(player) == specifiedTeam)
		{
			continue;
		}
		playersNotOnTeam.push_back(static_cast<uint32_t>(player));
	}

	return playersNotOnTeam;
}

optional<bool> PendingVoteKick::getKickVoteResult()
{
	bool targetIsActiveAIPlayer = NetPlay.players[target_player_id].allocated == false && NetPlay.players[target_player_id].ai >= 0 && !NetPlay.players[target_player_id].isSpectator;
	if (!NetPlay.players[target_player_id].allocated && !targetIsActiveAIPlayer)
	{
		// target player has already left / AI has lost
		return false;
	}

	auto eligible_voters = getPlayersWhoCanVoteToKick();

	// Special case:
	// - (If there are only two eligible voters): If they are on separate teams, vote fails immediately. If they are on the *same* team, just allow it (presumably they are against bots or the game would have already ended)
	if (eligible_voters.size() <= 2)
	{
		if (eligible_voters.size() <= 1)
		{
			// If there's only 1 eligible voter, let them kick (presumably kicking an AI)
			return true;
		}
		if (checkedGetPlayerTeam(eligible_voters[0]) == checkedGetPlayerTeam(eligible_voters[1]))
		{
			// If they are on the *same* team, just allow it (presumably they are against bots or the game would have already ended)
			return true;
		}
		// If only two human players, but not on the same team, vote to kick can't succeed (the host can always quit, though)
		return false;
	}

	auto target_player_team = checkedGetPlayerTeam(target_player_id);
	auto target_player_teammembers = filterPlayersByTeam(eligible_voters, target_player_team);
	bool target_player_is_solo = target_player_teammembers.size() <= 1;
	bool team_voteout_still_possible = false;

	// Check if vote reached a team threshold
	if (!target_player_is_solo)
	{
		size_t team_votes_for_kick = 0;
		size_t team_votes_against_kick = 0;
		for (auto player : target_player_teammembers)
		{
			if (votes[player].has_value())
			{
				if (votes[player].value())
				{
					++team_votes_for_kick;
				}
				else
				{
					++team_votes_against_kick;
				}
			}
		}

		// If 50%+ of the target player's team agrees to kick, that's a success
		size_t team_threshold = static_cast<size_t>(ceilf(static_cast<float>(target_player_teammembers.size()) / 2.f));
		if (team_votes_for_kick >= team_threshold)
		{
			return true;
		}

		size_t team_votes_outstanding = target_player_teammembers.size() - (team_votes_for_kick + team_votes_against_kick);
		if (team_votes_for_kick + team_votes_outstanding >= team_threshold)
		{
			team_voteout_still_possible = true;
		}

		// Otherwise, fall-through to overall thresholds
	}

	size_t overall_votes_for_kick = 0;
	size_t overall_votes_against_kick = 0;
	for (auto player : eligible_voters)
	{
		if (votes[player].has_value())
		{
			if (votes[player].value())
			{
				++overall_votes_for_kick;
			}
			else
			{
				++overall_votes_against_kick;
			}
		}
	}

	size_t num_voting = overall_votes_for_kick + overall_votes_against_kick;
	size_t num_not_voting = eligible_voters.size() - num_voting;
	auto players_on_other_teams = getPlayersNotOnTeam(eligible_voters, target_player_team);

	size_t overall_vote_threshold;
	if (target_player_is_solo)
	{
		// If target player is the only human player on their team: 50%+ of all other eligible voters vote to kick
		overall_vote_threshold = std::max<size_t>(static_cast<size_t>(ceilf(static_cast<float>(eligible_voters.size()) / 2.f)), 2);
	}
	else
	{
		// If target player is *not* the only human player on their team, the smaller of:
		// - 2/3 of all eligible voters
		// or
		// - num eligible voters on other teams
		// but must be at least 2
		overall_vote_threshold = std::min<size_t>(static_cast<size_t>(ceilf(static_cast<float>(eligible_voters.size()) * 2.f / 3.f)), players_on_other_teams.size());
		overall_vote_threshold = std::max<size_t>(overall_vote_threshold, 2);
	}

	// Check if vote reached an overall threshold
	if (overall_votes_for_kick >= overall_vote_threshold)
	{
		return true;
	}

	bool overall_voteout_still_possible = (overall_votes_for_kick + num_not_voting >= overall_vote_threshold);
	if ((!overall_voteout_still_possible && !team_voteout_still_possible) || num_not_voting == 0)
	{
		// didn't (and can't) hit any threshold
		return false;
	}

	// waiting for more votes
	return nullopt;
}

// Returns: true if PendingVote has a result and was handled, false if still waiting for results
static bool handleVoteKickResult(PendingVoteKick& pendingVote)
{
	ASSERT_HOST_ONLY(return false);

	auto currentResult = pendingVote.getKickVoteResult();
	if (!currentResult.has_value())
	{
		return false;
	}

	if (currentResult.value())
	{
		std::string outputMsg = astringf(_("The vote to kick player %s succeeded (sufficient votes in favor) - kicking"), getPlayerName(pendingVote.target_player_id, true));
		sendInGameSystemMessage(outputMsg.c_str());
		std::string logMsg = astringf("kicked %s : %s from the game", getPlayerName(pendingVote.target_player_id), NetPlay.players[pendingVote.target_player_id].IPtextAddress);
		NETlogEntry(logMsg.c_str(), SYNC_FLAG, pendingVote.target_player_id);

		kickPlayer(pendingVote.target_player_id, "The players have voted to kick you from the game.", ERROR_KICKED, false);
	}
	else
	{
		// Vote failed - message all players
		std::string outputMsg = astringf(_("The vote to kick player %s failed (insufficient votes in favor)"), getPlayerName(pendingVote.target_player_id, true));
		sendInGameSystemMessage(outputMsg.c_str());
	}
	return true;
}

void processPendingKickVotes()
{
	if (!NetPlay.isHost)
	{
		return;
	}

	auto it = pendingKickVotes.begin();
	while (it != pendingKickVotes.end())
	{
		if (realTime - it->start_time >= PENDING_KICK_VOTE_TIMEOUT_MS)
		{
			// pending vote timed-out

			// double-check if pending vote has a result (this might have changed if other players left)
			if (!handleVoteKickResult(*it))
			{
				// dismiss the pending vote
				std::string outputMsg = astringf(_("The vote to kick player %s failed (insufficient votes before timeout)"), getPlayerName(it->target_player_id, true));
				sendInGameSystemMessage(outputMsg.c_str());
				debug(LOG_INFO, "%s", outputMsg.c_str());
			}

			it = pendingKickVotes.erase(it);
		}
		else
		{
			it++;
		}
	}
}

bool startKickVote(uint32_t targetPlayer, optional<uint32_t> requester_player_id /*= nullopt*/)
{
	ASSERT_HOST_ONLY(return false);
	static uint32_t last_vote_id = 0;

	auto pendingVote = PendingVoteKick(last_vote_id++, requester_player_id.value_or(selectedPlayer), targetPlayer);
	auto currentStatus = pendingVote.getKickVoteResult();
	if (currentStatus.has_value())
	{
		// Vote either isn't possible or is a special case
		if (currentStatus.value())
		{
			kickPlayer(targetPlayer, _("The host has kicked you from the game."), ERROR_KICKED, false);
			return true;
		}
		else
		{
			// message to the requester that player kick vote failed
			std::string outputMsg = astringf(_("The vote to kick player %s failed"), getPlayerName(targetPlayer, true));
			addConsoleMessage(outputMsg.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE, false);
			return false;
		}
	}

	if (lastKickVoteForEachPlayer[targetPlayer].has_value())
	{
		if (realTime - lastKickVoteForEachPlayer[targetPlayer].value() < MIN_INTERVAL_BETWEEN_PLAYER_KICK_VOTES_MS + 5000) // + extra on the sender side
		{
			// Prevent spamming kick votes
			std::string outputMsg = astringf(_("Cannot request vote to kick player %s yet - please wait a bit longer"), getPlayerName(targetPlayer, true));
			addConsoleMessage(outputMsg.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE, false);
			return false;
		}
	}

	// Store in the list of pending votes
	pendingKickVotes.push_back(pendingVote);

	// Initiate a network vote
	sendVoteRequest(NetVoteType::KICK_PLAYER, pendingVote.unique_vote_id, pendingVote.target_player_id);

	std::string outputMsg = astringf(_("Starting vote to kick player: %s"), getPlayerName(targetPlayer, true));
	addConsoleMessage(outputMsg.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE, false);
	debug(LOG_INFO, "Starting vote to kick player: %s", getPlayerName(targetPlayer, true));

	lastKickVoteForEachPlayer[targetPlayer] = realTime;
	return true;
}

void cancelOrDismissVoteNotifications()
{
	cancelOrDismissNotificationsWithTag(VOTE_TAG);
	cancelOrDismissNotificationIfTag([](const std::string& tag) {
		return (tag.rfind(VOTE_KICK_TAG_PREFIX, 0) == 0);
	});
}

void cancelOrDismissKickVote(uint32_t targetPlayer)
{
	cancelOrDismissNotificationsWithTag(std::string(VOTE_KICK_TAG_PREFIX) + std::to_string(targetPlayer));
}
