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
#include "multilobbycommands.h"

#include <utility>
#include <memory>
#include <chrono>
#include <SQLiteCpp/SQLiteCpp.h>

// ////////////////////////////////////////////////////////////////////////////
// STATS STUFF
// ////////////////////////////////////////////////////////////////////////////
static PLAYERSTATS playerStats[MAX_CONNECTED_PLAYERS];

static PLAYERSTATS zeroStats;
static EcKey blindIdentity; // a freshly-generated identity used for the local client in the current blind room

static EcKey hostVerifiedJoinIdentities[MAX_CONNECTED_PLAYERS];


// ////////////////////////////////////////////////////////////////////////////
// Get Player's stats
PLAYERSTATS const &getMultiStats(UDWORD player)
{
	return playerStats[player];
}

bool generateBlindIdentity()
{
	blindIdentity = EcKey::generate();	// Generate new identity
	ASSERT(!blindIdentity.empty(), "Failed to generate new blind identity");
	return !blindIdentity.empty();
}

const EcKey& getLocalSharedIdentity()
{
	if (NetPlay.isHost && realSelectedPlayer >= MAX_PLAYER_SLOTS)
	{
		// If spectator host, send real identity
		return playerStats[realSelectedPlayer].identity;
	}

	if (game.blindMode != BLIND_MODE::NONE)
	{
		// In blind mode, share the blind identity
		return blindIdentity;
	}
	else
	{
		// In regular mode, share the actual identity
		return playerStats[realSelectedPlayer].identity;
	}
}

const EcKey& getVerifiedJoinIdentity(UDWORD player)
{
	ASSERT(player < MAX_CONNECTED_PLAYERS, "Invalid player: %u", player);
	if (NetPlay.isHost)
	{
		return hostVerifiedJoinIdentities[player];
	}
	else
	{
		if (game.blindMode == BLIND_MODE::NONE && ingame.VerifiedIdentity[player])
		{
			return playerStats[player].identity;
		}
		else
		{
			return hostVerifiedJoinIdentities[player];
		}
	}
}

// In blind games, it returns the verified join identity (if executed on the host, or on all clients after the game has ended)
// In regular games, it returns the current player identity
TrueIdentity getTruePlayerIdentity(UDWORD player)
{
	if (game.blindMode != BLIND_MODE::NONE)
	{
		// In blind games, always output the join identity (since the internal shared identity is a random one)
		return {hostVerifiedJoinIdentities[player], (!hostVerifiedJoinIdentities[player].empty()) ? true : false};
	}
	else
	{
		// Otherwise, always return the *current* player identity (which may differ from the identity used to join the game if the player switched to a different profile)
		return {playerStats[player].identity, ingame.VerifiedIdentity[player]};
	}
}

// Should be used when a player identity is to be output (in logs, or otherwise accessible to the user)
const EcKey& getOutputPlayerIdentity(UDWORD player)
{
	return getTruePlayerIdentity(player).identity;
}

static bool generateSessionKeysWithPlayer(uint32_t playerIndex)
{
	if (playerStats[playerIndex].identity.empty())
	{
		NETclearSessionKeys(playerIndex);
		return false;
	}

	// generate session keys
	auto& localIdentity = getLocalSharedIdentity();
	try {
		NETsetSessionKeys(playerIndex, SessionKeys(localIdentity, realSelectedPlayer, playerStats[playerIndex].identity, playerIndex));
	}
	catch (const std::invalid_argument&) {
		NETclearSessionKeys(playerIndex);
		throw;
	}
	return true;
}

bool swapPlayerMultiStatsLocal(uint32_t playerIndexA, uint32_t playerIndexB)
{
	if (playerIndexA >= MAX_CONNECTED_PLAYERS || playerIndexB >= MAX_CONNECTED_PLAYERS)
	{
		return false;
	}
	std::swap(playerStats[playerIndexA], playerStats[playerIndexB]);
	std::swap(hostVerifiedJoinIdentities[playerIndexA], hostVerifiedJoinIdentities[playerIndexB]);

	// NOTE: We can't just swap session keys - we have to re-generate to be sure they are correct
	// (since client / server determinism can also be based on the playerIdx relative to the realSelectedPlayer - see SessionKeys constructor)
	if (playerIndexA != realSelectedPlayer && (playerIndexA < MAX_PLAYERS || playerIndexA == NetPlay.hostPlayer))
	{
		try {
			generateSessionKeysWithPlayer(playerIndexA);
		}
		catch (const std::invalid_argument& e) {
			debug(LOG_INFO, "Cannot create session keys: (self: %u), (other: %u, name: \"%s\"), with error: %s", realSelectedPlayer, playerIndexA, getPlayerName(playerIndexA), e.what());
		}
	}
	if (playerIndexB != realSelectedPlayer && (playerIndexB < MAX_PLAYERS || playerIndexB == NetPlay.hostPlayer))
	{
		try {
			generateSessionKeysWithPlayer(playerIndexB);
		}
		catch (const std::invalid_argument& e) {
			debug(LOG_INFO, "Cannot create session keys: (self: %u), (other: %u, name: \"%s\"), with error: %s", realSelectedPlayer, playerIndexB, getPlayerName(playerIndexB), e.what());
		}
	}
	return true;
}

static bool sendMultiStatsInternal(uint32_t playerIndex, optional<uint32_t> recipientPlayerIndex = nullopt, bool sendHostVerifiedJoinIdentity = false)
{
	ASSERT(NetPlay.isHost || playerIndex == realSelectedPlayer, "Huh?");

	NETQUEUE queue;
	if (!recipientPlayerIndex.has_value())
	{
		queue = NETbroadcastQueue();
	}
	else
	{
		queue = NETnetQueue(recipientPlayerIndex.value());
	}
	// Now send it to all other players
	auto w = NETbeginEncode(queue, NET_PLAYER_STATS);
	// Send the ID of the player's stats we're updating
	NETuint32_t(w, playerIndex);

	PLAYERSTATS* pStatsToSend = &playerStats[playerIndex];
	if (game.blindMode != BLIND_MODE::NONE)
	{
		// In blind mode, always send zeroed stats
		pStatsToSend = &zeroStats;
	}

	NETuint32_t(w, pStatsToSend->played);
	NETuint32_t(w, pStatsToSend->wins);
	NETuint32_t(w, pStatsToSend->losses);
	NETuint32_t(w, pStatsToSend->totalKills);
	NETuint32_t(w, pStatsToSend->totalScore);

	EcKey::Key identityPublicKey;
	bool isHostVerifiedIdentity = false;
	// Choose the identity to send
	if (!sendHostVerifiedJoinIdentity || !NetPlay.isHost)
	{
		if (playerIndex == realSelectedPlayer)
		{
			// Local client sending its own identity
			const auto& identity = getLocalSharedIdentity();
			if (!identity.empty())
			{
				identityPublicKey = identity.toBytes(EcKey::Public);
			}
		}
		else
		{
			// Host sending other player details - relay client provided details
			if (!playerStats[playerIndex].identity.empty())
			{
				identityPublicKey = playerStats[playerIndex].identity.toBytes(EcKey::Public);
			}
		}
	}
	else
	{
		// Once game has begun or ended (depending on settings), if we're the host, send the hostVerifiedJoinIdentity
		ASSERT(NetPlay.isHost && !isBlindPlayerInfoState(), "Not time to send host verified identity yet?");
		isHostVerifiedIdentity = true;
		if (!hostVerifiedJoinIdentities[playerIndex].empty())
		{
			identityPublicKey = hostVerifiedJoinIdentities[playerIndex].toBytes(EcKey::Public);
		}
	}

	NETbool(w, isHostVerifiedIdentity);
	NETbytes(w, identityPublicKey);
	NETend(w);

	return true;
}

bool sendMultiStats(uint32_t playerIndex, optional<uint32_t> recipientPlayerIndex /*= nullopt*/)
{
	return sendMultiStatsInternal(playerIndex, recipientPlayerIndex, false);
}

bool sendMultiStatsHostVerifiedIdentities(uint32_t playerIndex)
{
	ASSERT_HOST_ONLY(return false);
	ASSERT_OR_RETURN(false, !isBlindPlayerInfoState(), "Not time to send host verified identities yet");
	return sendMultiStatsInternal(playerIndex, nullopt, true);
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
		sendMultiStats(playerIndex);

		if (playerIndex == realSelectedPlayer)
		{
			// need to clear and re-generate any session keys for communication between us and other players
			NETclearSessionKeys();
			auto& localIdentity = getLocalSharedIdentity();
			if (localIdentity.hasPrivate())
			{
				for (uint8_t i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
				{
					if (i == realSelectedPlayer) { continue; }
					if (i >= MAX_PLAYERS && i != NetPlay.hostPlayer)
					{
						// Don't bother creating SessionKeys with non-host spectator slots
						continue;
					}
					if (playerStats[i].identity.empty())
					{
						continue;
					}
					try {
						NETsetSessionKeys(i, SessionKeys(localIdentity, realSelectedPlayer, playerStats[i].identity, i));
					}
					catch (const std::invalid_argument& e) {
						debug(LOG_INFO, "One or both identities can't be used for session keys (self: %u, other: %u), with error: %s", realSelectedPlayer, i, e.what());
					}
				}
			}
			else
			{
				ASSERT(false, "Local identity is missing key pair?");
			}
		}
	}

	return true;
}

bool clearPlayerMultiStats(uint32_t playerIndex)
{
	setMultiStats(playerIndex, PLAYERSTATS(), true); // local only
	if (NetPlay.isHost)
	{
		hostVerifiedJoinIdentities[playerIndex].clear();
	}
	return true;
}

bool sendMultiStatsScoreUpdates(uint32_t playerIndex)
{
	if (game.blindMode != BLIND_MODE::NONE)
	{
		// No-op if in blind mode
		return false;
	}
	if (NetPlay.isHost || playerIndex == realSelectedPlayer)
	{
		return sendMultiStats(playerIndex);
	}
	return false;
}

bool multiStatsSetIdentity(uint32_t playerIndex, const EcKey::Key &identity, bool verifiedFromJoin = false)
{
	EcKey::Key prevIdentity;

	if (!playerStats[playerIndex].identity.empty())
	{
		prevIdentity = playerStats[playerIndex].identity.toBytes(EcKey::Public);
	}

	// If game hasn't actually started, process potential identity changes
	if (!ingame.TimeEveryoneIsInGame.has_value())
	{
		playerStats[playerIndex].identity.clear();
		if (!identity.empty())
		{
			if (!playerStats[playerIndex].identity.fromBytes(identity, EcKey::Public))
			{
				debug(LOG_INFO, "Player sent invalid identity: (player: %u, name: \"%s\", IP: %s)", playerIndex, getPlayerName(playerIndex), NetPlay.players[playerIndex].IPtextAddress);
			}
		}
		else
		{
			debug(LOG_INFO, "Player sent empty identity: (player: %u, name: \"%s\", IP: %s)", playerIndex, getPlayerName(playerIndex), NetPlay.players[playerIndex].IPtextAddress);
		}
		if ((identity != prevIdentity) || identity.empty())
		{
			if (GetGameMode() == GS_NORMAL)
			{
				debug(LOG_INFO, "Unexpected identity change after NET_FIREUP for: (player: %u, name: \"%s\", IP: %s)", playerIndex, getPlayerName(playerIndex), NetPlay.players[playerIndex].IPtextAddress);
			}

			ingame.PingTimes[playerIndex] = PING_LIMIT;

			if (verifiedFromJoin && !identity.empty())
			{
				// Record that the identity has been verified
				ingame.VerifiedIdentity[playerIndex] = true;
				if (NetPlay.isHost)
				{
					NetPlay.players[playerIndex].isAdmin = identityMatchesAdmin(playerStats[playerIndex].identity);
					// Do not broadcast player info here, as it's assumed caller will do it
				}

				// Store the verified join identity
				hostVerifiedJoinIdentities[playerIndex].fromBytes(identity, EcKey::Public);

				// Do *not* output to stdinterface here - the join event is still being processed
			}
			else
			{
				ingame.VerifiedIdentity[playerIndex] = false;
				if (NetPlay.isHost)
				{
					// when verified identity is cleared, so is admin status (until new identity is verified)
					if (NetPlay.players[playerIndex].isAdmin)
					{
						NetPlay.players[playerIndex].isAdmin = false;
						NETBroadcastPlayerInfo(playerIndex);
					}
				}

				// Output to stdinterface, if enabled
				if (!identity.empty())
				{
					std::string senderPublicKeyB64 = base64Encode(playerStats[playerIndex].identity.toBytes(EcKey::Public));
					std::string senderIdentityHash = playerStats[playerIndex].identity.publicHashString();
					std::string sendername = getPlayerName(playerIndex);
					std::string senderNameB64 = base64Encode(std::vector<unsigned char>(sendername.begin(), sendername.end()));
					wz_command_interface_output("WZEVENT: player identity UNVERIFIED: %" PRIu32 " %s %s %s %s\n", playerIndex, senderPublicKeyB64.c_str(), senderIdentityHash.c_str(), senderNameB64.c_str(), NetPlay.players[playerIndex].IPtextAddress);
				}
				else
				{
					wz_command_interface_output("WZEVENT: player identity EMPTY: %" PRIu32 "\n", playerIndex);
				}
			}

			if (!ingame.muteChat[playerIndex])
			{
				// check if the new identity was previously muted
				auto playerOptions = getStoredPlayerOptions(NetPlay.players[playerIndex].name, playerStats[playerIndex].identity);
				if (playerOptions.has_value() && playerOptions.value().mutedTime.has_value())
				{
					ingame.muteChat[playerIndex] = (playerOptions.value().mutedTime.value().time_since_epoch().count() > 0);
				}
			}

			if (playerIndex < MAX_PLAYERS || playerIndex == NetPlay.hostPlayer)
			{
				if (!playerStats[playerIndex].identity.empty())
				{
					// generate session keys
					try {
						generateSessionKeysWithPlayer(playerIndex);
					}
					catch (const std::invalid_argument& e) {
						debug(LOG_INFO, "Cannot create session keys: (self: %u), (other: %u, name: \"%s\", IP: %s), with error: %s", realSelectedPlayer, playerIndex, getPlayerName(playerIndex), NetPlay.players[playerIndex].IPtextAddress, e.what());
					}
				}
				else
				{
					NETclearSessionKeys(playerIndex);
				}
			}

			return true;
		}
	}
	else
	{
		// Changing an identity should not happen once a game starts
		if ((identity != prevIdentity) || identity.empty())
		{
			if (!ingame.endTime.has_value())
			{
				ASSERT(false, "Cannot change identity for player %u after game has started", playerIndex);
			}
		}
	}

	return false;
}

bool recvMultiStats(NETQUEUE queue)
{
	uint32_t playerIndex;

	auto r = NETbeginDecode(queue, NET_PLAYER_STATS);
	// Retrieve the ID number of the player for which we need to
	// update the stats
	NETuint32_t(r, playerIndex);

	if (playerIndex >= MAX_CONNECTED_PLAYERS)
	{
		NETend(r);
		return false;
	}


	if (playerIndex != queue.index && queue.index != NetPlay.hostPlayer)
	{
		HandleBadParam("NET_PLAYER_STATS given incorrect params.", playerIndex, queue.index);
		NETend(r);
		return false;
	}

	// we don't what to update ourselves, we already know our score (FIXME: rewrite setMultiStats())
	if (!myResponsibility(playerIndex))
	{
		// Retrieve the actual stats
		NETuint32_t(r, playerStats[playerIndex].played);
		NETuint32_t(r, playerStats[playerIndex].wins);
		NETuint32_t(r, playerStats[playerIndex].losses);
		NETuint32_t(r, playerStats[playerIndex].totalKills);
		NETuint32_t(r, playerStats[playerIndex].totalScore);

		EcKey::Key identity;
		bool isHostVerifiedIdentity = false;
		NETbool(r, isHostVerifiedIdentity);
		NETbytes(r, identity);
		NETend(r);

		if (!isHostVerifiedIdentity)
		{
			multiStatsSetIdentity(playerIndex, identity, false);
		}
		else
		{
			if (queue.index == NetPlay.hostPlayer)
			{
				hostVerifiedJoinIdentities[playerIndex].clear();
				if (!identity.empty() && !hostVerifiedJoinIdentities[playerIndex].fromBytes(identity, EcKey::Public))
				{
					debug(LOG_INFO, "Host sent invalid host-verified join identity for: (player: %u, name: \"%s\")", playerIndex, getPlayerName(playerIndex));
				}
			}
		}
	}
	else
	{
		NETend(r);
	}

	return true;
}

void multiStatsSetVerifiedIdentityFromJoin(uint32_t playerIndex, const EcKey::Key &identity)
{
	ASSERT_HOST_ONLY(return);
	ASSERT_OR_RETURN(, playerIndex != NetPlay.hostPlayer, "playerIndex is hostPlayer? (%" PRIu32 ")", NetPlay.hostPlayer);
	multiStatsSetIdentity(playerIndex, identity, true);
}

void multiStatsSetVerifiedHostIdentityFromJoin(const EcKey::Key &identity)
{
	ASSERT_OR_RETURN(, NetPlay.isHost || NetPlay.isHostAlive, "Unexpected state when called");
	hostVerifiedJoinIdentities[NetPlay.hostPlayer].fromBytes(identity, EcKey::Public);
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
			if (!st->identity.fromBytes(base64Decode(identity), EcKey::Private))
			{
				debug(LOG_INFO, "Failed to load profile identity");
			}
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

	if (st->identity.empty() || !st->identity.hasPrivate())
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
	st->recentDroidsKilled = 0;
	st->recentDroidsLost = 0;
	st->recentDroidsBuilt = 0;
	st->recentStructuresKilled = 0;
	st->recentStructuresLost = 0;
	st->recentStructuresBuilt = 0;
	st->recentScore = 0;
	st->recentResearchComplete = 0;
	st->recentPowerLost = 0;
	st->recentDroidPowerLost = 0;
	st->recentStructurePowerLost = 0;
	st->recentPowerWon = 0;
	st->recentResearchPerformance = 0;
	st->recentResearchPotential = 0;

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

	if (st->identity.empty())
	{
		debug(LOG_INFO, "Refusing to save profile with empty identity: %s", sFileName);
		return false;
	}

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

void incrementMultiStatsResearchPerformance(UDWORD player)
{
	if (player >= MAX_PLAYERS)
	{
		return;
	}
	// printf("Increment performance, player %d was %ld\n", player, playerStats[player].recentResearchPerformance);
	playerStats[player].recentResearchPerformance += 1;
}

void incrementMultiStatsResearchPotential(UDWORD player)
{
	if (player >= MAX_PLAYERS)
	{
		return;
	}
	// printf("Increment potential, player %d was %ld\n", player, playerStats[player].recentResearchPotential);
	playerStats[player].recentResearchPotential += 1;
}

// update kills
void updateMultiStatsKills(BASE_OBJECT *psKilled, UDWORD player)
{
	if (player >= MAX_PLAYERS)
	{
		return;
	}
	if (bMultiPlayer)
	{
		if (psKilled != nullptr)
		{
			if (psKilled->player == scavengerSlot())
			{
				return; // killing scavengers does not count in MP games
			}
			if (psKilled->player < MAX_PLAYERS)
			{
				uint64_t pwrCost = static_cast<uint64_t>(calcObjectCost(psKilled));
				playerStats[psKilled->player].recentPowerLost += pwrCost;
				playerStats[player].recentPowerWon += pwrCost;

				if (isDroid(psKilled))
				{
					playerStats[psKilled->player].recentDroidPowerLost += pwrCost;
					playerStats[psKilled->player].recentDroidsLost++;
					playerStats[player].recentDroidsKilled++;
				}
				else if (isStructure(psKilled))
				{
					playerStats[psKilled->player].recentStructurePowerLost += pwrCost;
					playerStats[psKilled->player].recentStructuresLost++;
					playerStats[player].recentStructuresKilled++;
				}
			}
			if (NetPlay.bComms)
			{
				++playerStats[player].totalKills;
			}
			++playerStats[player].recentKills;
		}
		return;
	}
	++playerStats[player].recentKills;
}

void updateMultiStatsBuilt(BASE_OBJECT *psBuilt)
{
	if (psBuilt->player >= MAX_PLAYERS)
	{
		return;
	}

	if (isDroid(psBuilt))
	{
		playerStats[psBuilt->player].recentDroidsBuilt++;
	}
	else if (isStructure(psBuilt))
	{
		playerStats[psBuilt->player].recentStructuresBuilt++;
	}
}

void updateMultiStatsResearchComplete(RESEARCH *psResearch, UDWORD player)
{
	if (player >= MAX_PLAYERS)
	{
		return;
	}

	playerStats[player].recentResearchComplete++;
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
		db = std::make_unique<SQLite::Database>(knownPlayersDBPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
		db->exec("PRAGMA journal_mode=WAL");
		createKnownPlayersDBTables();
		query_findPlayerIdentityByName = std::make_unique<SQLite::Statement>(*db, "SELECT local_id, name, pk FROM known_players WHERE name = ?");
		query_insertNewKnownPlayer = std::make_unique<SQLite::Statement>(*db, "INSERT OR IGNORE INTO known_players(name, pk) VALUES(?, ?)");
		query_updateKnownPlayerKey = std::make_unique<SQLite::Statement>(*db, "UPDATE known_players SET pk = ? WHERE name = ?");
		query_findPlayerOptionsByPK = std::make_unique<SQLite::Statement>(*db, "SELECT name, muted, banned FROM player_options WHERE pk = ?");
		query_insertNewPlayerOptions = std::make_unique<SQLite::Statement>(*db, "INSERT OR IGNORE INTO player_options(pk, name, muted, banned) VALUES(?, ?, ?, ?)");
		query_updatePlayerOptionsMuted = std::make_unique<SQLite::Statement>(*db, "UPDATE player_options SET muted = ? WHERE pk = ? AND name = ?");
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

		int64_t mutedTimeValue = static_cast<int64_t>(mutedTime.has_value() ? mutedTime.value().time_since_epoch().count() : 0);

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
			knownPlayersDB = std::make_unique<KnownPlayersDB>(knownPlayersDBPath);
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

void setMultiPlayRecentDroidsKilled(uint32_t player, uint32_t value)
{
	playerStats[player].recentDroidsKilled = value;
}

void setMultiPlayRecentDroidsLost(uint32_t player, uint32_t value)
{
	playerStats[player].recentDroidsLost = value;
}

void setMultiPlayRecentDroidsBuilt(uint32_t player, uint32_t value)
{
	playerStats[player].recentDroidsBuilt = value;
}

void setMultiPlayRecentStructuresKilled(uint32_t player, uint32_t value)
{
	playerStats[player].recentStructuresKilled = value;
}

void setMultiPlayRecentStructuresLost(uint32_t player, uint32_t value)
{
	playerStats[player].recentStructuresLost = value;
}

void setMultiPlayRecentStructuresBuilt(uint32_t player, uint32_t value)
{
	playerStats[player].recentStructuresBuilt = value;
}

void setMultiPlayRecentPowerLost(uint32_t player, uint64_t powerLost)
{
	playerStats[player].recentPowerLost = powerLost;
}

void setMultiPlayRecentDroidPowerLost(uint32_t player, uint64_t powerLost)
{
	playerStats[player].recentDroidPowerLost = powerLost;
}

void setMultiPlayRecentStructurePowerLost(uint32_t player, uint64_t powerLost)
{
	playerStats[player].recentStructurePowerLost = powerLost;
}

void setMultiPlayRecentPowerWon(uint32_t player, uint64_t powerWon)
{
	playerStats[player].recentPowerWon = powerWon;
}

void setMultiPlayRecentResearchComplete(uint32_t player, uint32_t value)
{
	playerStats[player].recentResearchComplete = value;
}

void setMultiPlayRecentResearchPotential(uint32_t player, uint64_t value)
{
	playerStats[player].recentResearchPotential = value;
}

void setMultiPlayRecentResearchPerformance(uint32_t player, uint64_t value)
{
	playerStats[player].recentResearchPerformance = value;
}

void resetRecentScoreData()
{
	for (unsigned int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		playerStats[i].recentKills = 0;
		playerStats[i].recentDroidsKilled = 0;
		playerStats[i].recentDroidsLost = 0;
		playerStats[i].recentDroidsBuilt = 0;
		playerStats[i].recentStructuresKilled = 0;
		playerStats[i].recentStructuresLost = 0;
		playerStats[i].recentStructuresBuilt = 0;
		playerStats[i].recentScore = 0;
		playerStats[i].recentResearchComplete = 0;
		playerStats[i].recentPowerLost = 0;
		playerStats[i].recentDroidPowerLost = 0;
		playerStats[i].recentStructurePowerLost = 0;
		playerStats[i].recentPowerWon = 0;
		playerStats[i].recentResearchPerformance = 0;
		playerStats[i].recentResearchPotential = 0;
		playerStats[i].identity.clear();

		hostVerifiedJoinIdentities[i].clear();
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
	j["recentDroidsKilled"] = p.recentDroidsKilled;
	j["recentDroidsLost"] = p.recentDroidsLost;
	j["recentDroidsBuilt"] = p.recentDroidsBuilt;
	j["recentStructuresKilled"] = p.recentStructuresKilled;
	j["recentStructuresLost"] = p.recentStructuresLost;
	j["recentStructuresBuilt"] = p.recentStructuresBuilt;
	j["recentScore"] = p.recentScore;
	j["recentResearchComplete"] = p.recentResearchComplete;
	j["recentPowerLost"] = p.recentPowerLost;
	j["recentDroidPowerLost"] = p.recentDroidPowerLost;
	j["recentStructurePowerLost"] = p.recentStructurePowerLost;
	j["recentPowerWon"] = p.recentPowerWon;
	j["recentResearchPotential"] = p.recentResearchPotential;
	j["recentResearchPerformance"] = p.recentResearchPerformance;
	j["identity"] = p.identity;
}

template <typename T>
optional<T> optGetJSONValue(const nlohmann::json& j, const std::string& key)
{
	auto it = j.find(key);
	if (it == j.end())
	{
		return nullopt;
	}
	return it->get<T>();
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
	// WZ 4.4.0+:
	k.recentDroidsKilled = optGetJSONValue<uint32_t>(j, "recentDroidsKilled").value_or(0);
	k.recentDroidsLost = optGetJSONValue<uint32_t>(j, "recentDroidsLost").value_or(0);
	k.recentDroidsBuilt = optGetJSONValue<uint32_t>(j, "recentDroidsBuilt").value_or(0);
	k.recentStructuresKilled = optGetJSONValue<uint32_t>(j, "recentStructuresKilled").value_or(0);
	k.recentStructuresLost = optGetJSONValue<uint32_t>(j, "recentStructuresLost").value_or(0);
	k.recentStructuresBuilt = optGetJSONValue<uint32_t>(j, "recentStructuresBuilt").value_or(0);
	k.recentResearchComplete = optGetJSONValue<uint32_t>(j, "recentResearchComplete").value_or(0);
	k.recentDroidPowerLost = optGetJSONValue<uint64_t>(j, "recentDroidPowerLost").value_or(0);
	k.recentStructurePowerLost = optGetJSONValue<uint64_t>(j, "recentStructurePowerLost").value_or(0);
	k.recentPowerWon = optGetJSONValue<uint64_t>(j, "recentPowerWon").value_or(0);
	k.recentResearchPotential = optGetJSONValue<uint64_t>(j, "recentResearchPotential").value_or(0);
	k.recentResearchPerformance = optGetJSONValue<uint64_t>(j, "recentResearchPerformance").value_or(0);
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

bool updateMultiStatsIdentitiesInJSON(nlohmann::json& json, bool useVerifiedJoinIdentity)
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
		auto stats = json.at(idx).get<PLAYERSTATS>();
		stats.identity = (useVerifiedJoinIdentity) ? getVerifiedJoinIdentity(idx) : playerStats[idx].identity;
		json[idx] = stats;
	}

	return true;
}
