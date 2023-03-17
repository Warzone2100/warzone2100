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
 * MultiStat.c
 *
 * Alex Lee , pumpkin studios, EIDOS
 *
 * load / update / store multiplayer statistics for league tables etc...
 */

#include <nlohmann/json.hpp> // Must come before WZ includes

#include "lib/framework/file.h"
#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/i18n.h"
#include "lib/netplay/nettypes.h"

#include "activity.h"
#include "clparse.h"
#include "main.h"
#include "mission.h" // for cheats
#include "multistat.h"
#include "urlrequest.h"
#include "stdinreader.h"

#include <utility>
#include <memory>
#include <chrono>
#include <SQLiteCpp/SQLiteCpp.h>

// ////////////////////////////////////////////////////////////////////////////
// STATS STUFF
// ////////////////////////////////////////////////////////////////////////////
static PLAYERSTATS playerStats[MAX_CONNECTED_PLAYERS];


// ////////////////////////////////////////////////////////////////////////////
// Get Player's stats
PLAYERSTATS const &getMultiStats(UDWORD player)
{
	return playerStats[player];
}

static void NETauto(PLAYERSTATS::Autorating &ar)
{
	NETauto(ar.valid);
	if (ar.valid)
	{
		NETauto(ar.dummy);
		NETauto(ar.star);
		NETauto(ar.medal);
		NETauto(ar.level);
		NETauto(ar.elo);
		NETauto(ar.autohoster);
		NETauto(ar.details);
	}
}

PLAYERSTATS::Autorating::Autorating(nlohmann::json const &json)
{
	try {
		dummy = json["dummy"].get<bool>();
		star[0] = json["star"][0].get<uint8_t>();
		star[1] = json["star"][1].get<uint8_t>();
		star[2] = json["star"][2].get<uint8_t>();
		medal = json["medal"].get<uint8_t>();
		level = json["level"].get<uint8_t>();
		elo = json["elo"].get<std::string>();
		autohoster = json["autohoster"].get<bool>();
		details = json["details"].get<std::string>();
		valid = true;
	} catch (const std::exception &e) {
		debug(LOG_WARNING, "Error parsing rating JSON: %s", e.what());
	}
}

void lookupRatingAsync(uint32_t playerIndex)
{
	if (playerStats[playerIndex].identity.empty())
	{
		return;
	}

	auto hash = playerStats[playerIndex].identity.publicHashString();
	auto key = playerStats[playerIndex].identity.publicKeyHexString();
	if (hash.empty() || key.empty())
	{
		return;
	}

	std::string url = getAutoratingUrl();
	if (url.empty())
	{
		setAutoratingUrl(WZ_DEFAULT_PUBLIC_RATING_LOOKUP_SERVICE_URL);
		url = WZ_DEFAULT_PUBLIC_RATING_LOOKUP_SERVICE_URL;
	}

	if (!getAutoratingEnable())
	{
		return;
	}

	URLDataRequest req;
	req.url = url;
	req.setRequestHeader("WZ-Player-Hash", hash);
	req.setRequestHeader("WZ-Player-Key", key);
	req.setRequestHeader("WZ-Locale", getLanguage());
	debug(LOG_INFO, "Requesting \"%s\" for player %d (%.32s) (%s)", req.url.c_str(), playerIndex, NetPlay.players[playerIndex].name, hash.c_str());
	req.onSuccess = [playerIndex, hash](std::string const &url, HTTPResponseDetails const &response, std::shared_ptr<MemoryStruct> const &data) {
		long httpStatusCode = response.httpStatusCode();
		std::string urlCopy = url;
		if (httpStatusCode != 200 || !data || data->size == 0)
		{
			wzAsyncExecOnMainThread([urlCopy, httpStatusCode] {
				debug(LOG_WARNING, "Failed to retrieve data from \"%s\", got [%ld].", urlCopy.c_str(), httpStatusCode);
			});
			return;
		}

		std::shared_ptr<MemoryStruct> dataCopy = data;
		wzAsyncExecOnMainThread([playerIndex, hash, urlCopy, dataCopy] {
			if (playerStats[playerIndex].identity.publicHashString() != hash)
			{
				debug(LOG_WARNING, "Got data from \"%s\", but player is already gone.", urlCopy.c_str());
				return;
			}
			try {
				playerStats[playerIndex].autorating = nlohmann::json::parse(dataCopy->memory, dataCopy->memory + dataCopy->size);
				playerStats[playerIndex].autoratingFrom = RATING_SOURCE_LOCAL;
				if (playerStats[playerIndex].autorating.valid)
				{
					setMultiStats(playerIndex, playerStats[playerIndex], !NetPlay.isHost);
					netPlayersUpdated = true;
				}
			}
			catch (const std::exception &e) {
				debug(LOG_WARNING, "JSON document from \"%s\" is invalid: %s", urlCopy.c_str(), e.what());
			}
			catch (...) {
				debug(LOG_FATAL, "Unexpected exception parsing JSON \"%s\"", urlCopy.c_str());
			}
		});
	};
	req.onFailure = [](std::string const &url, WZ_DECL_UNUSED URLRequestFailureType type, WZ_DECL_UNUSED optional<HTTPResponseDetails> transferDetails) {
		std::string urlCopy = url;
		wzAsyncExecOnMainThread([urlCopy] {
			debug(LOG_WARNING, "Failure fetching \"%s\".", urlCopy.c_str());
		});
	};
	req.maxDownloadSizeLimit = 4096;
	urlRequestData(req);
}

bool swapPlayerMultiStatsLocal(uint32_t playerIndexA, uint32_t playerIndexB)
{
	if (playerIndexA >= MAX_CONNECTED_PLAYERS || playerIndexB >= MAX_CONNECTED_PLAYERS)
	{
		return false;
	}
	std::swap(playerStats[playerIndexA], playerStats[playerIndexB]);
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Set Player's stats
// send stats to all players when bLocal is false
bool setMultiStats(uint32_t playerIndex, PLAYERSTATS plStats, bool bLocal)
{
	if (playerIndex >= MAX_CONNECTED_PLAYERS)
	{
		return true;
	}

	// First copy over the data into our local array
	playerStats[playerIndex] = std::move(plStats);

	if (!bLocal && (NetPlay.isHost || playerIndex == realSelectedPlayer))
	{
		// Now send it to all other players
		NETbeginEncode(NETbroadcastQueue(), NET_PLAYER_STATS);
		// Send the ID of the player's stats we're updating
		NETuint32_t(&playerIndex);

		NETauto(playerStats[playerIndex].autorating);

		// Send over the actual stats
		NETuint32_t(&playerStats[playerIndex].played);
		NETuint32_t(&playerStats[playerIndex].wins);
		NETuint32_t(&playerStats[playerIndex].losses);
		NETuint32_t(&playerStats[playerIndex].totalKills);
		NETuint32_t(&playerStats[playerIndex].totalScore);
		NETuint32_t(&playerStats[playerIndex].recentKills);
		NETuint32_t(&playerStats[playerIndex].recentScore);
		NETuint64_t(&playerStats[playerIndex].recentPowerLost);

		EcKey::Key identity;
		if (!playerStats[playerIndex].identity.empty())
		{
			identity = playerStats[playerIndex].identity.toBytes(EcKey::Public);
		}
		NETbytes(&identity);
		NETend();
	}

	return true;
}

void recvMultiStats(NETQUEUE queue)
{
	uint32_t playerIndex;

	NETbeginDecode(queue, NET_PLAYER_STATS);
	// Retrieve the ID number of the player for which we need to
	// update the stats
	NETuint32_t(&playerIndex);

	if (playerIndex >= MAX_CONNECTED_PLAYERS)
	{
		NETend();
		return;
	}


	if (playerIndex != queue.index && queue.index != NetPlay.hostPlayer)
	{
		HandleBadParam("NET_PLAYER_STATS given incorrect params.", playerIndex, queue.index);
		NETend();
		return;
	}

	PLAYERSTATS::Autorating receivedAutorating;
	NETauto(receivedAutorating);
	bool processAutoratingData = false;

	// we don't what to update ourselves, we already know our score (FIXME: rewrite setMultiStats())
	if (!myResponsibility(playerIndex))
	{
		// Retrieve the actual stats
		NETuint32_t(&playerStats[playerIndex].played);
		NETuint32_t(&playerStats[playerIndex].wins);
		NETuint32_t(&playerStats[playerIndex].losses);
		NETuint32_t(&playerStats[playerIndex].totalKills);
		NETuint32_t(&playerStats[playerIndex].totalScore);
		NETuint32_t(&playerStats[playerIndex].recentKills);
		NETuint32_t(&playerStats[playerIndex].recentScore);
		NETuint64_t(&playerStats[playerIndex].recentPowerLost);

		EcKey::Key identity;
		NETbytes(&identity);
		EcKey::Key prevIdentity;
		if (!playerStats[playerIndex].identity.empty())
		{
			prevIdentity = playerStats[playerIndex].identity.toBytes(EcKey::Public);
		}
		playerStats[playerIndex].identity.clear();
		if (!identity.empty())
		{
			playerStats[playerIndex].identity.fromBytes(identity, EcKey::Public);
		}
		else
		{
			debug(LOG_INFO, "Player sent empty identity: (player: %u, name: \"%s\", IP: %s)", playerIndex, NetPlay.players[playerIndex].name, NetPlay.players[playerIndex].IPtextAddress);
		}
		if ((identity != prevIdentity) || identity.empty())
		{
			ingame.PingTimes[playerIndex] = PING_LIMIT;
			ingame.VerifiedIdentity[playerIndex] = false;

			if (!ingame.muteChat[playerIndex])
			{
				// check if the new identity was previously muted
				auto playerOptions = getStoredPlayerOptions(NetPlay.players[playerIndex].name, playerStats[playerIndex].identity);
				if (playerOptions.has_value() && playerOptions.value().mutedTime.has_value())
				{
					ingame.muteChat[playerIndex] = (playerOptions.value().mutedTime.value().time_since_epoch().count() > 0);
				}
			}

			// Output to stdinterface, if enabled
			if (!identity.empty())
			{
				std::string senderPublicKeyB64 = base64Encode(playerStats[playerIndex].identity.toBytes(EcKey::Public));
				std::string senderIdentityHash = playerStats[playerIndex].identity.publicHashString();
				wz_command_interface_output("WZEVENT: player identity UNVERIFIED: %" PRIu32 " %s %s\n", playerIndex, senderPublicKeyB64.c_str(), senderIdentityHash.c_str());
			}
			else
			{
				wz_command_interface_output("WZEVENT: player identity EMPTY: %" PRIu32 "\n", playerIndex);
			}

			processAutoratingData = true;
		}
	}
	else
	{
		processAutoratingData = true;
	}

	if (processAutoratingData)
	{
		if (getAutoratingEnable())
		{
			playerStats[playerIndex].autorating.valid = false;
			playerStats[playerIndex].autoratingFrom = RATING_SOURCE_LOCAL;
			lookupRatingAsync(playerIndex);
		}
		else
		{
			playerStats[playerIndex].autorating = receivedAutorating;
			playerStats[playerIndex].autoratingFrom = RATING_SOURCE_HOST;
		}
	}

	NETend();
}

// ////////////////////////////////////////////////////////////////////////////
// Load Player Stats

static bool loadMultiStatsFile(const std::string& fileName, PLAYERSTATS *st, bool skipLoadingIdentity = false)
{
	char *pFileData = nullptr;
	UDWORD size = 0;

	if (loadFile(fileName.c_str(), &pFileData, &size))
	{
		if (strncmp(pFileData, "WZ.STA.v3", 9) != 0)
		{
			free(pFileData);
			pFileData = nullptr;
			return false; // wrong version or not a stats file
		}

		char identity[1001];
		identity[0] = '\0';
		if (!skipLoadingIdentity)
		{
			sscanf(pFileData, "WZ.STA.v3\n%u %u %u %u %u\n%1000[A-Za-z0-9+/=]",
			   &st->wins, &st->losses, &st->totalKills, &st->totalScore, &st->played, identity);
		}
		else
		{
			sscanf(pFileData, "WZ.STA.v3\n%u %u %u %u %u\n",
				   &st->wins, &st->losses, &st->totalKills, &st->totalScore, &st->played);
		}
		free(pFileData);
		if (identity[0] != '\0')
		{
			st->identity.fromBytes(base64Decode(identity), EcKey::Private);
		}
	}

	return true;
}

bool loadMultiStats(char *sPlayerName, PLAYERSTATS *st)
{
	// preserve current player identity (if loaded)
	EcKey currentIdentity = (st) ? st->identity : EcKey();

	*st = PLAYERSTATS();  // clear in case we don't get to load

	// Prevent an empty player name (where the first byte is a 0x0 terminating char already)
	if (!*sPlayerName)
	{
		strcpy(sPlayerName, _("Player"));
	}

	std::string fileName = std::string(MultiPlayersPath) + sPlayerName + ".sta2";

	debug(LOG_WZ, "loadMultiStats: %s", fileName.c_str());

	// check player .sta2 already exists
	if (PHYSFS_exists(fileName.c_str()))
	{
		if (!loadMultiStatsFile(fileName, st))
		{
			return false;
		}
	}
	else
	{
		// one-time porting of old .sta player files to .sta2
		fileName = std::string(MultiPlayersPath) + sPlayerName + ".sta";
		if (PHYSFS_exists(fileName.c_str()))
		{
			if (!loadMultiStatsFile(fileName, st, true))
			{
				return false;
			}
		}
	}

	if (st->identity.empty())
	{
		if (!currentIdentity.empty())
		{
			st->identity = currentIdentity;  	// Preserve existing identity when creating a new profile
		}
		else
		{
			st->identity = EcKey::generate();	// Generate new identity
		}

		saveMultiStats(sPlayerName, sPlayerName, st);  // Save new profile
	}

	// reset recent scores
	st->recentKills = 0;
	st->recentScore = 0;
	st->recentPowerLost = 0;

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Save Player Stats
bool saveMultiStats(const char *sFileName, const char *sPlayerName, const PLAYERSTATS *st)
{
	if (Cheated)
	{
	    return false;
	}
	char buffer[1000];

	ssprintf(buffer, "WZ.STA.v3\n%u %u %u %u %u\n%s\n",
	         st->wins, st->losses, st->totalKills, st->totalScore, st->played, base64Encode(st->identity.toBytes(EcKey::Private)).c_str());

	std::string fileName = std::string(MultiPlayersPath) + sFileName + ".sta2";

	saveFile(fileName.c_str(), buffer, strlen(buffer));

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// score update functions

// update players damage stats.
void updateMultiStatsDamage(UDWORD attacker, UDWORD defender, UDWORD inflicted)
{
	if (defender == PLAYER_FEATURE)
	{
		// damaging features like skyscrapers does not count
		return;
	}

	ASSERT_OR_RETURN(, attacker < MAX_PLAYERS, "invalid attacker: %" PRIu32 "", attacker);
	ASSERT_OR_RETURN(, defender < MAX_PLAYERS, "invalid defender: %" PRIu32 "", defender);

	if (NetPlay.bComms)
	{
		if ((attacker == scavengerSlot()) || (defender == scavengerSlot()))
		{
			return; // damaging and getting damaged by scavengers does not influence scores in MP games
		}
		playerStats[attacker].totalScore  += 2 * inflicted;
		playerStats[defender].totalScore  -= inflicted;
	}

	playerStats[attacker].recentScore += 2 * inflicted;
	playerStats[defender].recentScore -= inflicted;
}

// update games played.
void updateMultiStatsGames()
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return;
	}
	++playerStats[selectedPlayer].played;
}

// games won
void updateMultiStatsWins()
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return;
	}
	++playerStats[selectedPlayer].wins;
}

//games lost.
void updateMultiStatsLoses()
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return;
	}
	++playerStats[selectedPlayer].losses;
}

static inline uint32_t calcObjectCost(const BASE_OBJECT *psObj)
{
	switch (psObj->type)
	{
		case OBJ_DROID:
			return calcDroidPower((const DROID *)psObj);
		case OBJ_STRUCTURE:
		{
			auto psStruct = static_cast<const STRUCTURE *>(psObj);
			ASSERT_OR_RETURN(0, psStruct->pStructureType != nullptr, "pStructureType is null?");
			return psStruct->pStructureType->powerToBuild;
		}
		case OBJ_FEATURE:
			return 0;
		default:
			ASSERT(false, "No such supported object type: %d", static_cast<int>(psObj->type));
			break;
	}
	return 0;
}

// update kills
void updateMultiStatsKills(BASE_OBJECT *psKilled, UDWORD player)
{
	if (player >= MAX_PLAYERS)
	{
		return;
	}
	if (NetPlay.bComms)
	{
		if (psKilled != nullptr)
		{
			if (psKilled->player == scavengerSlot())
			{
				return; // killing scavengers does not count in MP games
			}
			if (psKilled->player < MAX_PLAYERS)
			{
				playerStats[psKilled->player].recentPowerLost += static_cast<uint64_t>(calcObjectCost(psKilled));
			}
			++playerStats[player].totalKills;
			++playerStats[player].recentKills;
		}
		return;
	}
	++playerStats[player].recentKills;
}

class KnownPlayersDB {
public:
	struct PlayerInfo {
		int64_t local_id;
		std::string name;
		EcKey::Key pk;
	};

	struct PlayerOptions {
		std::string name;
		EcKey::Key pk;
		optional<std::chrono::system_clock::time_point> mutedTime;
		optional<std::chrono::system_clock::time_point> bannedTime;
	};

public:
	// Caller is expected to handle thrown exceptions
	KnownPlayersDB(const std::string& knownPlayersDBPath)
	{
		db = std::unique_ptr<SQLite::Database>(new SQLite::Database(knownPlayersDBPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE));
		db->exec("PRAGMA journal_mode=WAL");
		createKnownPlayersDBTables();
		query_findPlayerIdentityByName = std::unique_ptr<SQLite::Statement>(new SQLite::Statement(*db, "SELECT local_id, name, pk FROM known_players WHERE name = ?"));
		query_insertNewKnownPlayer = std::unique_ptr<SQLite::Statement>(new SQLite::Statement(*db, "INSERT OR IGNORE INTO known_players(name, pk) VALUES(?, ?)"));
		query_updateKnownPlayerKey = std::unique_ptr<SQLite::Statement>(new SQLite::Statement(*db, "UPDATE known_players SET pk = ? WHERE name = ?"));
		query_findPlayerOptionsByPK = std::unique_ptr<SQLite::Statement>(new SQLite::Statement(*db, "SELECT name, muted, banned FROM player_options WHERE pk = ?"));
		query_insertNewPlayerOptions = std::unique_ptr<SQLite::Statement>(new SQLite::Statement(*db, "INSERT OR IGNORE INTO player_options(pk, name, muted, banned) VALUES(?, ?, ?, ?)"));
		query_updatePlayerOptionsMuted = std::unique_ptr<SQLite::Statement>(new SQLite::Statement(*db, "UPDATE player_options SET muted = ? WHERE pk = ? AND name = ?"));
	}

public:
	optional<PlayerInfo> findPlayerIdentityByName(const std::string& name)
	{
		if (name.empty())
		{
			return nullopt;
		}
		cleanupCache();
		const auto i = findPlayerCache.find(name);
		if (i != findPlayerCache.end())
		{
			return i->second.first;
		}
		optional<PlayerInfo> result;
		try {
			query_findPlayerIdentityByName->bind(1, name);
			if (query_findPlayerIdentityByName->executeStep())
			{
				PlayerInfo data;
				data.local_id = query_findPlayerIdentityByName->getColumn(0).getInt64();
				data.name = query_findPlayerIdentityByName->getColumn(1).getString();
				std::string publicKeyb64 = query_findPlayerIdentityByName->getColumn(2).getString();
				data.pk = base64Decode(publicKeyb64);
				result = data;
			}
		}
		catch (const std::exception& e) {
			debug(LOG_ERROR, "Failure to query database for player; error: %s", e.what());
			result = nullopt;
		}
		try {
			query_findPlayerIdentityByName->reset();
		}
		catch (const std::exception& e) {
			debug(LOG_ERROR, "Failed to reset prepared statement; error: %s", e.what());
		}
		// add to the current in-memory cache
		findPlayerCache[name] = std::pair<optional<PlayerInfo>, UDWORD>(result, realTime);
		return result;
	}

	// Note: May throw on database error!
	void addKnownPlayerIdentity(std::string const &name, EcKey const &key, bool overrideCurrentKey)
	{
		if (key.empty())
		{
			return;
		}

		std::string publicKeyb64 = base64Encode(key.toBytes(EcKey::Public));

		// Begin transaction
		SQLite::Transaction transaction(*db);

		query_insertNewKnownPlayer->bind(1, name);
		query_insertNewKnownPlayer->bind(2, publicKeyb64);
		if (query_insertNewKnownPlayer->exec() == 0 && overrideCurrentKey)
		{
			query_updateKnownPlayerKey->bind(1, publicKeyb64);
			query_updateKnownPlayerKey->bind(2, name);
			if (query_updateKnownPlayerKey->exec() == 0)
			{
				debug(LOG_WARNING, "Failed to update known_player (%s)", name.c_str());
			}
			query_updateKnownPlayerKey->reset();
		}
		query_insertNewKnownPlayer->reset();

		// remove from the current in-memory cache
		findPlayerCache.erase(name);

		// Commit transaction
		transaction.commit();
	}

public:
	std::vector<PlayerOptions> findPlayerOptions(const std::string& name, EcKey const &key)
	{
		if (key.empty())
		{
			return {};
		}

		std::string publicKeyb64 = base64Encode(key.toBytes(EcKey::Public));

		std::vector<PlayerOptions> result;
		try {
			query_findPlayerOptionsByPK->bind(1, publicKeyb64);
			while (query_findPlayerOptionsByPK->executeStep())
			{
				PlayerOptions data;
				data.name = query_findPlayerOptionsByPK->getColumn(0).getString();
				data.pk = key.toBytes(EcKey::Public);
				auto mutedTime = std::chrono::seconds(query_findPlayerOptionsByPK->getColumn(1).getInt64());
				if (mutedTime.count() > 0)
				{
					data.mutedTime = std::chrono::system_clock::time_point(mutedTime);
				}
				auto bannedTime = std::chrono::seconds(query_findPlayerOptionsByPK->getColumn(2).getInt64());
				if (bannedTime.count() > 0)
				{
					data.bannedTime = std::chrono::system_clock::time_point(bannedTime);
				}
				if (data.name == name)
				{
					// if we find an exact match (for both pk *and* name), use it!
					result.clear();
					result.push_back(data);
					break;
				}
				result.push_back(data);
			}
		}
		catch (const std::exception& e) {
			debug(LOG_ERROR, "Failure to query database for player; error: %s", e.what());
			result.clear();
		}
		try {
			query_findPlayerOptionsByPK->reset();
		}
		catch (const std::exception& e) {
			debug(LOG_ERROR, "Failed to reset prepared statement; error: %s", e.what());
		}

		return result;
	}

	// Note: May throw on database error!
	void setPlayerMuted(std::string const &name, EcKey const &key, optional<std::chrono::system_clock::time_point> mutedTime)
	{
		std::string publicKeyb64;
		if (!key.empty())
		{
			publicKeyb64 = base64Encode(key.toBytes(EcKey::Public));
		}

		// Begin transaction
		SQLite::Transaction transaction(*db);

		auto mutedTimeValue = mutedTime.has_value() ? mutedTime.value().time_since_epoch().count() : 0;

		query_insertNewPlayerOptions->bind(1, publicKeyb64);
		query_insertNewPlayerOptions->bind(2, name);
		query_insertNewPlayerOptions->bind(3, mutedTimeValue);
		query_insertNewPlayerOptions->bind(4, -1); // default "never set" is -1, not 0
		if (query_insertNewPlayerOptions->exec() == 0)
		{
			query_updatePlayerOptionsMuted->bind(1, mutedTimeValue);
			query_updatePlayerOptionsMuted->bind(2, publicKeyb64);
			query_updatePlayerOptionsMuted->bind(3, name);
			if (query_updatePlayerOptionsMuted->exec() == 0)
			{
				debug(LOG_WARNING, "Failed to update player_options (%s)", name.c_str());
			}
			query_updatePlayerOptionsMuted->reset();
		}
		query_insertNewPlayerOptions->reset();

		// Commit transaction
		transaction.commit();
	}

private:
	void createKnownPlayersDBTables()
	{
		SQLite::Transaction transaction(*db);
		if (!db->tableExists("known_players"))
		{
			// used to store association of name and known player key
			db->exec("CREATE TABLE known_players (local_id INTEGER PRIMARY KEY, name TEXT UNIQUE, pk TEXT)");
		}
		int dbVersion = db->execAndGet("PRAGMA user_version").getInt();
		// database schema upgrades
		switch (dbVersion)
		{
			case 0:
				// used to store player_options that may be associated with a player identity
				// NOTE: In this case, "name" does *NOT* have a unique constraint
				db->exec("CREATE TABLE player_options (local_id INTEGER PRIMARY KEY, pk TEXT NOT NULL, name TEXT NOT NULL, muted INTEGER, banned INTEGER)");
				db->exec("CREATE UNIQUE INDEX idx_player_options_pk_name ON player_options (pk, name)");
				db->exec("PRAGMA user_version = 1");
				dbVersion = 1;
				// fall-through
			default:
				// done
				break;
		}
		transaction.commit();
	}

	void cleanupCache()
	{
		const UDWORD CACHE_CLEAN_INTERAL = 10 * GAME_TICKS_PER_SEC;
		if (realTime - lastCacheClean > CACHE_CLEAN_INTERAL)
		{
			findPlayerCache.clear();
			lastCacheClean = realTime;
		}
	}

private:
	std::unique_ptr<SQLite::Database> db; // Must be the first-listed member variable so it is destructed last
	std::unique_ptr<SQLite::Statement> query_findPlayerIdentityByName;
	std::unique_ptr<SQLite::Statement> query_insertNewKnownPlayer;
	std::unique_ptr<SQLite::Statement> query_updateKnownPlayerKey;
	std::unordered_map<std::string, std::pair<optional<PlayerInfo>, UDWORD>> findPlayerCache;
	std::unique_ptr<SQLite::Statement> query_findPlayerOptionsByPK;
	std::unique_ptr<SQLite::Statement> query_insertNewPlayerOptions;
	std::unique_ptr<SQLite::Statement> query_updatePlayerOptionsMuted;
	UDWORD lastCacheClean = 0;
};

static std::unique_ptr<KnownPlayersDB> knownPlayersDB;

void initKnownPlayers()
{
	if (!knownPlayersDB)
	{
		const char *pWriteDir = PHYSFS_getWriteDir();
		ASSERT_OR_RETURN(, pWriteDir, "PHYSFS_getWriteDir returned null");
		std::string knownPlayersDBPath = std::string(pWriteDir) + "/" + "knownPlayers.db";
		try {
			knownPlayersDB = std::unique_ptr<KnownPlayersDB>(new KnownPlayersDB(knownPlayersDBPath));
		}
		catch (std::exception& e) {
			// error loading SQLite database
			debug(LOG_ERROR, "Unable to load or initialize SQLite3 database (%s); error: %s", knownPlayersDBPath.c_str(), e.what());
			return;
		}
	}
}

void shutdownKnownPlayers()
{
	knownPlayersDB.reset();
}

bool isLocallyKnownPlayer(std::string const &name, EcKey const &key)
{
	ASSERT_OR_RETURN(false, knownPlayersDB.operator bool(), "knownPlayersDB is uninitialized");
	if (key.empty())
	{
		return false;
	}
	auto result = knownPlayersDB->findPlayerIdentityByName(name);
	if (!result.has_value())
	{
		return false;
	}
	return result.value().pk == key.toBytes(EcKey::Public);
}

void addKnownPlayer(std::string const &name, EcKey const &key, bool override)
{
	if (key.empty())
	{
		return;
	}
	ASSERT_OR_RETURN(, knownPlayersDB.operator bool(), "knownPlayersDB is uninitialized");

	try {
		knownPlayersDB->addKnownPlayerIdentity(name, key, override);
	}
	catch (const std::exception& e) {
		debug(LOG_ERROR, "Failed to add known_player with error: %s", e.what());
	}
}

optional<StoredPlayerOptions> getStoredPlayerOptions(std::string const &name, EcKey const &key)
{
	ASSERT_OR_RETURN(nullopt, knownPlayersDB.operator bool(), "knownPlayersDB is uninitialized");
	auto results = knownPlayersDB->findPlayerOptions(name, key);
	if (results.empty())
	{
		return nullopt;
	}
	// findPlayerOptions may have returned more than one result, so sort through and find the "newest" for each setting
	StoredPlayerOptions playerOptions;
	for (const auto& result : results)
	{
		if (result.mutedTime.has_value())
		{
			playerOptions.mutedTime = std::max(result.mutedTime.value(), playerOptions.mutedTime.value_or(std::chrono::system_clock::time_point()));
		}
		if (result.bannedTime.has_value())
		{
			playerOptions.bannedTime = std::max(result.bannedTime.value(), playerOptions.bannedTime.value_or(std::chrono::system_clock::time_point()));
		}
	}
	return playerOptions;
}

void storePlayerMuteOption(std::string const &name, EcKey const &key, bool muted)
{
	ASSERT_OR_RETURN(, knownPlayersDB.operator bool(), "knownPlayersDB is uninitialized");
	optional<std::chrono::system_clock::time_point> mutedTime;
	if (muted)
	{
		mutedTime = std::chrono::system_clock::now();
	}
	try {
		knownPlayersDB->setPlayerMuted(name, key, mutedTime);
	}
	catch (const std::exception& e) {
		debug(LOG_ERROR, "Failed to store player mute option with error: %s", e.what());
	}
}

uint32_t getMultiPlayUnitsKilled(uint32_t player)
{
	return getMultiStats(player).recentKills;
}

void setMultiPlayUnitsKilled(uint32_t player, uint32_t kills)
{
	playerStats[player].recentKills = kills;
}

uint32_t getMultiPlayRecentScore(uint32_t player)
{
	return getMultiStats(player).recentScore;
}

void setMultiPlayRecentScore(uint32_t player, uint32_t score)
{
	playerStats[player].recentScore = score;
}

uint32_t getSelectedPlayerUnitsKilled()
{
	if (ActivityManager::instance().getCurrentGameMode() != ActivitySink::GameMode::CAMPAIGN)
	{
		return getMultiPlayUnitsKilled(selectedPlayer);
	}
	else
	{
		return missionData.unitsKilled;
	}
}

void resetRecentScoreData()
{
	for (unsigned int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		playerStats[i].recentKills = 0;
		playerStats[i].recentScore = 0;
		playerStats[i].recentPowerLost = 0;
		playerStats[i].identity.clear();
		playerStats[i].autorating = PLAYERSTATS::Autorating();
	}
}

// MARK: -

inline void to_json(nlohmann::json& j, const EcKey& k) {
	if (k.empty())
	{
		j = "";
		return;
	}
	auto publicKeyBytes = k.toBytes(EcKey::Public);
	std::string publicKeyB64Str = base64Encode(publicKeyBytes);
	j = publicKeyB64Str;
}

inline void from_json(const nlohmann::json& j, EcKey& k) {
	std::string publicKeyB64Str = j.get<std::string>();
	if (publicKeyB64Str.empty())
	{
		k.clear();
		return;
	}
	EcKey::Key publicKeyBytes = base64Decode(publicKeyB64Str);
	k.fromBytes(publicKeyBytes, EcKey::Public);
}

inline void to_json(nlohmann::json& j, const PLAYERSTATS& p) {
	j = nlohmann::json::object();
	j["played"] = p.played;
	j["wins"] = p.wins;
	j["losses"] = p.losses;
	j["totalKills"] = p.totalKills;
	j["totalScore"] = p.totalScore;
	j["recentKills"] = p.recentKills;
	j["recentScore"] = p.recentScore;
	j["recentPowerLost"] = p.recentPowerLost;
	j["identity"] = p.identity;
}

inline void from_json(const nlohmann::json& j, PLAYERSTATS& k) {
	k.played = j.at("played").get<uint32_t>();
	k.wins = j.at("wins").get<uint32_t>();
	k.losses = j.at("losses").get<uint32_t>();
	k.totalKills = j.at("totalKills").get<uint32_t>();
	k.totalScore = j.at("totalScore").get<uint32_t>();
	k.recentKills = j.at("recentKills").get<uint32_t>();
	k.recentScore = j.at("recentScore").get<uint32_t>();
	k.recentPowerLost = j.at("recentPowerLost").get<uint64_t>();
	k.identity = j.at("identity").get<EcKey>();
}

bool saveMultiStatsToJSON(nlohmann::json& json)
{
	json = nlohmann::json::array();

	for (size_t idx = 0; idx < MAX_CONNECTED_PLAYERS; idx++)
	{
		json.push_back(playerStats[idx]);
	}

	return true;
}

bool loadMultiStatsFromJSON(const nlohmann::json& json)
{
	if (!json.is_array())
	{
		debug(LOG_ERROR, "Expecting an array");
		return false;
	}
	if (json.size() > MAX_CONNECTED_PLAYERS)
	{
		debug(LOG_ERROR, "Array size is too large: %zu", json.size());
		return false;
	}

	for (size_t idx = 0; idx < json.size(); idx++)
	{
		playerStats[idx] = json.at(idx).get<PLAYERSTATS>();
	}

	return true;
}
