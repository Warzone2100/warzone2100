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
 * Multijoin.c
 *
 * Alex Lee, pumpkin studios, bath,
 *
 * Stuff to handle the comings and goings of players.
 */

#include <physfs.h>
#include <map>
#include <algorithm>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/math_ext.h"
#include "lib/framework/object_list_iteration.h"

#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/netplay/netplay.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"

#include "multijoin.h"

#include "objmem.h"
#include "statsdef.h"
#include "droiddef.h"
#include "game.h"
#include "projectile.h"
#include "droid.h"
#include "map.h"
#include "levels.h"
#include "power.h"
#include "game.h"					// for loading maps
#include "message.h"				// for clearing game messages
#include "order.h"
#include "console.h"
#include "orderdef.h"				// for droid_order_data
#include "hci.h"
#include "component.h"
#include "research.h"
#include "wrappers.h"
#include "intimage.h"
#include "data.h"
#include "activity.h"
#include "main.h"					// for GetGameMode

#include <chrono>

#include "multimenu.h"
#include "multiplay.h"
#include "multirecv.h"
#include "multiint.h"
#include "multistat.h"
#include "multigifts.h"
#include "multivote.h"
#include "qtscript.h"
#include "clparse.h"
#include "multilobbycommands.h"
#include "stdinreader.h"
#include "hci/quickchat.h"
#include "game_world.h"

// ////////////////////////////////////////////////////////////////////////////
// Local Functions

static void resetMultiVisibility(UDWORD player);
void destroyPlayerResources(GameWorld& world, UDWORD player, bool quietly);

//////////////////////////////////////////////////////////////////////////////
/*
** when a remote player leaves an arena game do this!
**
** @param world : the game world to destroy the resources for `player` from
** @param player -- the one we need to clear
** @param quietly -- true means without any visible effects
*/
void clearPlayer(GameWorld& world, UDWORD player, bool quietly)
{
	ASSERT_OR_RETURN(, player < MAX_CONNECTED_PLAYERS, "Invalid player: %" PRIu32 "", player);

	ASSERT(player < NetPlay.playerReferences.size(), "Invalid player: %" PRIu32 "", player);
	NetPlay.playerReferences[player]->disconnect();
	NetPlay.playerReferences[player] = std::make_shared<PlayerReference>(player);

	debug(LOG_NET, "R.I.P. %s (%u). quietly is %s", getPlayerName(player), player, quietly ? "true" : "false");

	ingame.LagCounter[player] = 0;
	ingame.DesyncCounter[player] = 0;
	ingame.JoiningInProgress[player] = false;	// if they never joined, reset the flag
	ingame.DataIntegrity[player] = false;
	ingame.hostChatPermissions[player] = false;
	ingame.lastSentPlayerDataCheck2[player].reset();

	if (player >= MAX_PLAYERS)
	{
		return; // no more to do
	}

	destroyPlayerResources(world, player, quietly);
}

void destroyPlayerResources(GameWorld& world, UDWORD player, bool quietly)
{
	UDWORD			i;

	if (player >= MAX_PLAYERS)
	{
		return; // no more to do
	}

	for (i = 0; i < MAX_PLAYERS; i++)				// remove alliances
	{
		// Never remove a player's self-alliance, as the player can be selected and units added via the debug menu
		// even after they have left, and this would lead to them firing on each other.
		if (i != player)
		{
			alliances[player][i] = ALLIANCE_BROKEN;
			alliances[i][player] = ALLIANCE_BROKEN;
			alliancebits[i] &= ~(1 << player);
			alliancebits[player] &= ~(1 << i);
		}
	}

	debug(LOG_DEATH, "killing off all droids for player %d", player);
	// delete all droids
	mutating_list_iterate(world.objects.droids[player], [quietly, &world](DROID* d)
	{
		if (quietly)			// don't show effects
		{
			killDroid(d, world.objects);
		}
		else				// show effects
		{
			destroyDroid(d, gameTime, world);
		}
		return IterationResult::CONTINUE_ITERATION;
	});

	debug(LOG_DEATH, "killing off all structures for player %d", player);
	// delete all structs
	mutating_list_iterate(world.objects.structures[player], [quietly, &world](STRUCTURE* psStruct)
	{
		// FIXME: look why destroyStruct() doesn't put back the feature like removeStruct() does
		if (quietly || psStruct->pStructureType->type == REF_RESOURCE_EXTRACTOR)		// don't show effects
		{
			removeStruct(psStruct, true, world);
		}
		else			// show effects
		{
			destroyStruct(psStruct, gameTime, world);
		}
		return IterationResult::CONTINUE_ITERATION;
	});

	return;
}

static bool destroyMatchingStructs(UDWORD player, std::function<bool (STRUCTURE *)> cmp, bool quietly)
{
	bool destroyedAnyStructs = false;
	mutating_list_iterate(gameWorld.objects.structures[player], [quietly, &cmp, &destroyedAnyStructs](STRUCTURE* psStruct)
	{
		if (cmp(psStruct))
		{
			// FIXME: look why destroyStruct() doesn't put back the feature like removeStruct() does
			if (quietly || psStruct->pStructureType->type == REF_RESOURCE_EXTRACTOR)		// don't show effects
			{
				removeStruct(psStruct, true, gameWorld);
			}
			else			// show effects
			{
				destroyStruct(psStruct, gameTime, gameWorld);
			}
			destroyedAnyStructs = true;
		}
		return IterationResult::CONTINUE_ITERATION;
	});
	return destroyedAnyStructs;
}

// Return a recipient's (specified by `idx`) share of a `total` quantity that is
// divided among `numShares` recipients. Any remainder is dealt out one apiece
// to the lowest indices first, so the shares add back up to exactly `total`.
static int64_t shareOf(int64_t total, size_t numShares, size_t idx)
{
	ASSERT_OR_RETURN(0, idx < numShares, "Share %zu out of range (%zu shares)", idx, numShares);
	ASSERT_OR_RETURN(0, total >= 0, "Tried to share a negative total of %" PRId64 "", total);

	const int64_t n = static_cast<int64_t>(numShares);
	const int64_t remainder = total % n;
	const bool receiveRemainder = static_cast<int64_t>(idx) < remainder;

	return total / n + (receiveRemainder ? 1 : 0);
}

// Should teammates inherit a structure (and its limit) when its owner leaves?
static bool teamInherits(STRUCTURE_TYPE type, bool sharedResearch)
{
	if (!sharedResearch && type == REF_RESEARCH)
	{
		return false;
	}
	if (type == REF_HQ)
	{
		// Inheriting HQ may cause potentially subtle issues due to the implicit
		// assumptions all over that there's only 1 HQ per player on a map
		return false;
	}
	return true;
}

// Give the player's structures and limits to the team. This is done fairly so
// that the team's total allowance for each structure ends up what it would have
// been had the player never left, and nothing handed over becomes unbuildable
// once destroyed.
static void distributeStructuresAndLimits(UDWORD player, const std::vector<uint32_t>& possibleTargets)
{
	const bool sharedResearch = alliancesSharedResearch(game.alliance);
	const size_t numTargets = possibleTargets.size();
	ASSERT_OR_RETURN(, numTargets > 0, "No targets to distribute to");

	// Bucketed up front by stat, because gifting moves structures out of this player's list
	std::map<UDWORD, std::vector<STRUCTURE *>> giftableByStat;
	for (STRUCTURE *psStruct : gameWorld.objects.structures[player])
	{
		if (psStruct && teamInherits(psStruct->pStructureType->type, sharedResearch))
		{
			giftableByStat[psStruct->pStructureType - asStructureStats].push_back(psStruct);
		}
	}

	// Increment each time to rotate the recipient order (reduce bias)
	size_t cursor = 0;

	for (UDWORD stat = 0; stat < numStructureStats; ++stat)
	{
		auto& upgrade = asStructureStats[stat].upgrade;

		// If the structure is not inheritable, do not gift it and
		// do not increase its limit for teammates
		if (!teamInherits(asStructureStats[stat].type, sharedResearch))
		{
			continue;
		}

		// An unlimited leaver has no pool to divide, so a capped recipient can end up over its
		// cap once the structures arrive. That only blocks building more until the count drops.
		const unsigned pool = upgrade[player].limit;
		const bool sharePool = (pool != LOTS_OF);

		auto giftable = giftableByStat.find(stat);
		const std::vector<STRUCTURE *> *structs = (giftable != giftableByStat.end()) ? &giftable->second : nullptr;

		if (!sharePool && !structs)
		{
			continue;	// the common case: nothing capped, and none of it built
		}

		size_t next = 0;
		for (size_t i = 0; i < numTargets; ++i)
		{
			const auto to = possibleTargets[(cursor + i) % numTargets];

			// An unlimited recipient discards its share - the team total is unbounded anyway
			if (sharePool && upgrade[to].limit != LOTS_OF)
			{
				// Held below LOTS_OF, which would otherwise read as "no limit at all"
				const unsigned share = static_cast<unsigned>(shareOf(pool, numTargets, i));
				upgrade[to].limit += std::min(share, LOTS_OF - 1 - upgrade[to].limit);
			}

			if (structs)
			{
				const auto share = static_cast<size_t>(shareOf(static_cast<int64_t>(structs->size()), numTargets, i));
				for (size_t n = 0; n < share; ++n)
				{
					giftSingleStructure((*structs)[next++], static_cast<UBYTE>(to), false);
				}
			}
		}
		ASSERT(!structs || next == structs->size(), "Distributed %zu of %zu structures", next, structs ? structs->size() : 0);

		cursor = (cursor + 1) % numTargets;
	}
}

bool splitResourcesAmongTeam(UDWORD player)
{
	auto team = NetPlay.players[player].team;

	// Build a list of team members who are still around
	std::vector<uint32_t> possibleTargets;
	for (uint32_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (i != player
			&& i != scavengerSlot()										// ...not scavenger player
			&& NetPlay.players[i].team == team							// ...belonging to the same team
			&& aiCheckAlliances(i, player)								// ...the alliance hasn't been broken
			// && NetPlay.players[i].difficulty != AIDifficulty::DISABLED	// ...not disabled // NOTE: Can't do this check as the host may set difficulty == DISABLED for slots before clients do, leading to sync issues, so for now (instead) check for human players only...
			&& isHumanPlayer(i)											// ... is a human
			&& !NetPlay.players[i].isSpectator							// ... not spectator
			)
		{
			possibleTargets.push_back(i);
		}
	}

	if (possibleTargets.empty())
	{
		// no valid targets for resources...
		return false;
	}

	const size_t numTargets = possibleTargets.size();

	// Distribute power evenly
	const int32_t power = getPower(player);
	for (size_t i = 0; i < numTargets; ++i)
	{
		addPower(possibleTargets[i], static_cast<int32_t>(shareOf(power, numTargets, i)));
	}
	setPower(player, 0);

	// Distribute the player's additional unit limits
	const int maxDroids = getMaxDroids(player);
	const int maxCommanders = getMaxCommanders(player);
	const int maxConstructors = getMaxConstructors(player);
	for (size_t i = 0; i < numTargets; ++i)
	{
		const auto to = possibleTargets[i];
		setMaxDroids(to, getMaxDroids(to) + static_cast<int>(shareOf(maxDroids, numTargets, i)));
		setMaxCommanders(to, getMaxCommanders(to) + static_cast<int>(shareOf(maxCommanders, numTargets, i)));
		setMaxConstructors(to, getMaxConstructors(to) + static_cast<int>(shareOf(maxConstructors, numTargets, i)));
	}

	// Distribute droids between targets as evenly as possible
	struct PlayerItemsReceived
	{
		uint32_t player = 0;
		uint32_t itemsRecv = 0;
	};
	std::vector<PlayerItemsReceived> droidsGiftedPerTarget;
	for (auto to : possibleTargets)
	{
		droidsGiftedPerTarget.push_back(PlayerItemsReceived{to, 0});
	}
	auto incrRecvItem = [&](size_t idx) {
		droidsGiftedPerTarget[idx].itemsRecv += 1;
		std::stable_sort(droidsGiftedPerTarget.begin(), droidsGiftedPerTarget.end(), [](const PlayerItemsReceived& a, const PlayerItemsReceived& b) -> bool {
			return a.itemsRecv < b.itemsRecv;
		});
	};
	mutating_list_iterate(gameWorld.objects.droids[player], [&droidsGiftedPerTarget, &incrRecvItem](DROID* d)
	{
		bool transferredDroid = false;
		if (!isDead(d))
		{
			for (size_t i = 0; i < droidsGiftedPerTarget.size(); ++i)
			{
				if (giftSingleDroid(d, droidsGiftedPerTarget[i].player, false))
				{
					transferredDroid = true;
					incrRecvItem(i);
					break;
				}
				// if we can't gift this droid to this player, try again with the next player in priority queue
			}
		}

		if (!transferredDroid)
		{
			destroyDroid(d, gameTime, gameWorld);
		}
		return IterationResult::CONTINUE_ITERATION;
	});

	distributeStructuresAndLimits(player, possibleTargets);

	if (!alliancesSharedResearch(game.alliance))
	{
		// research centers are not gifted in unshared research mode, so destroy
		// them. Don't let the leaving player's labs keep researching
		destroyMatchingStructs(player, [](STRUCTURE *psStruct) { return psStruct->pStructureType->type == REF_RESEARCH; }, false);
	}

	return true;
}

void handlePlayerLeftInGame(UDWORD player)
{
	ASSERT_OR_RETURN(, player < MAX_CONNECTED_PLAYERS, "Invalid player: %" PRIu32 "", player);

	bool leftWhilePlayer = !NetPlay.players[player].isSpectator;

	ASSERT(player < NetPlay.playerReferences.size(), "Invalid player: %" PRIu32 "", player);
	NetPlay.playerReferences[player]->disconnect();
	NetPlay.playerReferences[player] = std::make_shared<PlayerReference>(player);

	debug(LOG_NET, "R.I.P. %s (%u).", getPlayerName(player), player);

	ingame.LagCounter[player] = 0;
	ingame.DesyncCounter[player] = 0;
	ingame.JoiningInProgress[player] = false;	// if they never joined, reset the flag
	ingame.PendingDisconnect[player] = false;
	ingame.DataIntegrity[player] = false;
	ingame.lastSentPlayerDataCheck2[player].reset();

	if (leftWhilePlayer)
	{
		ingame.playerLeftGameTime[player] = gameTime;
	}

	if (player >= MAX_PLAYERS)
	{
		return; // no more to do
	}

	// Apply the configured leave behavior to their resources.
	PLAYER_LEAVE_MODE mode = game.playerLeaveMode;
	switch (mode)
	{
		case PLAYER_LEAVE_MODE::DESTROY_RESOURCES:
			destroyPlayerResources(gameWorld, player, false);
			break;
		case PLAYER_LEAVE_MODE::SPLIT_WITH_TEAM:
			if (!splitResourcesAmongTeam(player))
			{
				// no valid targets to split resources among
				// instead, destroy the player
				destroyPlayerResources(gameWorld, player, false);
			}
			break;
	}
}

// Reset visibility, so a new player can't see the old stuff!!
static void resetMultiVisibility(UDWORD player)
{
	UDWORD		owned;

	if (player >= MAX_PLAYERS)
	{
		return;
	}

	for (owned = 0 ; owned < MAX_PLAYERS ; owned++)		// for each player
	{
		if (owned != player)								// done reset own stuff..
		{
			//droids
			for (DROID* pDroid : gameWorld.objects.droids[owned])
			{
				pDroid->visible[player] = false;
			}

			//structures
			for (STRUCTURE* pStruct : gameWorld.objects.structures[owned])
			{
				pStruct->visible[player] = false;
			}

		}
	}
	return;
}

static void sendPlayerLeft(uint32_t playerIndex)
{
	ASSERT_OR_RETURN(, NetPlay.isHost, "Only host should call this.");

	uint32_t forcedPlayerIndex = whosResponsible(playerIndex);
	NETQUEUE(*netQueueType)(unsigned) = forcedPlayerIndex != selectedPlayer ? NETgameQueueForced : NETgameQueue;
	auto w = NETbeginEncode(netQueueType(forcedPlayerIndex), GAME_PLAYER_LEFT);
	NETuint32_t(w, playerIndex);
	NETend(w);
}

static void addConsolePlayerLeftMessage(unsigned playerIndex)
{
	if (!NetPlay.isHost && isBlindSimpleLobby(game.blindMode) && (GetGameMode() != GS_NORMAL))
	{
		return;
	}
	if (selectedPlayer != playerIndex)
	{
		std::string msg = astringf(_("%s has Left the Game"), getPlayerName(playerIndex));
		addConsoleMessage(msg.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
}

static void addConsolePlayerJoinMessage(unsigned playerIndex)
{
	if (!NetPlay.isHost && isBlindSimpleLobby(game.blindMode) && (GetGameMode() != GS_NORMAL))
	{
		return;
	}
	if (selectedPlayer != playerIndex)
	{
		std::string msg = astringf(_("%s joined the Game"), getPlayerName(playerIndex));
		if ((game.blindMode != BLIND_MODE::NONE) && NetPlay.isHost && (NetPlay.hostPlayer >= MAX_PLAYER_SLOTS))
		{
			msg += " ";
			msg += astringf(_("(codename: %s)"), getPlayerGenericName(playerIndex));
		}
		addConsoleMessage(msg.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
}

void recvPlayerLeft(NETQUEUE queue)
{
	uint32_t playerIndex = 0;
	auto r = NETbeginDecode(queue, GAME_PLAYER_LEFT);
	NETuint32_t(r, playerIndex);
	NETend(r);

	addConsolePlayerLeftMessage(playerIndex);

	if (whosResponsible(playerIndex) != queue.index)
	{
		return;
	}

	turnOffMultiMsg(true);
	handlePlayerLeftInGame(playerIndex);
	turnOffMultiMsg(false);
	if (!ingame.TimeEveryoneIsInGame.has_value()) // If game hasn't actually started
	{
		clearPlayerMultiStats(playerIndex); // local only
	}
	NetPlay.players[playerIndex].allocated = false;

	NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_DROPPED, playerIndex);
	cancelOrDismissKickVote(playerIndex);

	debug(LOG_INFO, "** player %u has dropped, in-game! (gameTime: %" PRIu32 ")", playerIndex, gameTime);

	// fire script callback to reassign skirmish players.
	if (GetGameMode() == GS_NORMAL)
	{
		triggerEventPlayerLeft(playerIndex);
	}

	ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());

	wz_command_interface_output_room_status_json();
}

// ////////////////////////////////////////////////////////////////////////////
// A remote player has left the game
bool MultiPlayerLeave(UDWORD playerIndex)
{
	if (playerIndex >= MAX_CONNECTED_PLAYERS)
	{
		ASSERT(false, "Bad player number");
		return false;
	}

	if (NetPlay.isHost)
	{
		multiClearHostRequestMoveToPlayer(playerIndex);
		multiSyncResetPlayerChallenge(playerIndex);
		resetMultiOptionPrefValues(playerIndex);
	}

	NETlogEntry("Player leaving game", SYNC_FLAG, playerIndex);
	debug(LOG_NET, "** Player %u [%s], has left the game at game time %u.", playerIndex, getPlayerName(playerIndex), gameTime);

	ingame.muteChat[playerIndex] = false;

	if (wz_command_interface_enabled() && NetPlay.players[playerIndex].allocated)
	{
		// WZEVENT: playerLeft: <playerIdx> <gameTime> <b64pubkey> <hash> <V|?> <b64name> <ip>
		const auto& identity = getOutputPlayerIdentity(playerIndex);
		std::string playerPublicKeyB64 = base64Encode(identity.toBytes(EcKey::Public));
		std::string playerIdentityHash = identity.publicHashString();
		std::string playerVerifiedStatus = (ingame.VerifiedIdentity[playerIndex]) ? "V" : "?";
		std::string playerName = getPlayerName(playerIndex);
		std::string playerNameB64 = base64Encode(std::vector<unsigned char>(playerName.begin(), playerName.end()));
		wz_command_interface_output("WZEVENT: playerLeft: %" PRIu32 " %" PRIu32 " %s %s %s %s %s\n", playerIndex, gameTime, playerPublicKeyB64.c_str(), playerIdentityHash.c_str(), playerVerifiedStatus.c_str(), playerNameB64.c_str(), NetPlay.players[playerIndex].IPtextAddress);
	}

	if (ingame.localJoiningInProgress)
	{
		addConsolePlayerLeftMessage(playerIndex);
		clearPlayer(gameWorld, playerIndex, false);
		clearPlayerMultiStats(playerIndex); // local only
		NetPlay.players[playerIndex].difficulty = AIDifficulty::DISABLED;
	}
	else if (NetPlay.isHost)  // If hosting, and game has started (not in pre-game lobby screen, that is).
	{
		sendPlayerLeft(playerIndex);

		if (bDisplayMultiJoiningStatus) // if still waiting for players to load *or* waiting for game to start...
		{
			auto w = NETbeginEncode(NETbroadcastQueue(), NET_PLAYER_DROPPED);
			NETuint32_t(w, playerIndex);
			NETend(w);
			// only set ingame.JoiningInProgress[player_id] to false
			// when the game starts, it will handle the GAME_PLAYER_LEFT message in their queue properly
			ingame.JoiningInProgress[playerIndex] = false;
			ingame.PendingDisconnect[playerIndex] = true; // used as a UI indicator that a disconnect will be processed in the future
		}
	}

	if (NetPlay.players[playerIndex].wzFiles && NetPlay.players[playerIndex].fileSendInProgress())
	{
		char buf[256];

		ssprintf(buf, _("File transfer has been aborted for %d.") , playerIndex);
		addConsoleMessage(buf, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		debug(LOG_INFO, "=== File has been aborted for %d ===", playerIndex);
		NetPlay.players[playerIndex].wzFiles->clear();
	}

	if (widgGetFromID(psWScreen, IDRET_FORM))
	{
		if (playerIndex < MAX_PLAYERS) // only play audio when *player* slots drop (ignore spectator slots)
		{
			audio_QueueTrack(ID_CLAN_EXIT);
		}
	}

	netPlayersUpdated = true;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// A Remote Player has joined the game.
bool MultiPlayerJoin(UDWORD playerIndex, optional<EcKey::Key> verifiedJoinIdentity)
{
	if (widgGetFromID(psWScreen, IDRET_FORM))	// if ingame.
	{
		audio_QueueTrack(ID_CLAN_ENTER);
	}

	if (widgGetFromID(psWScreen, MULTIOP_PLAYERS))	// if in multimenu.
	{
		if (!multiRequestUp && (NetPlay.isHost || ingame.localJoiningInProgress))
		{
			netPlayersUpdated = true;	// update the player box.
		}
	}

	playerSpamMuteReset(playerIndex);

	if (NetPlay.isHost)		// host responsible for welcoming this player.
	{
		// if we've already received a request from this player don't reallocate.
		if (ingame.JoiningInProgress[playerIndex])
		{
			return true;
		}
		ASSERT(NetPlay.playercount <= MAX_PLAYERS, "Too many players!");
		ASSERT(GetGameMode() != GS_NORMAL, "A player joined after the game started??");

		// setup data for this player, then broadcast it to the other players.
		setupNewPlayer(playerIndex);						// setup all the guff for that player.
		if (verifiedJoinIdentity.has_value())
		{
			multiStatsSetVerifiedIdentityFromJoin(playerIndex, verifiedJoinIdentity.value());
		}
		sendOptions();
		// if skirmish and game full, then kick...
		if (NetPlay.playercount > game.maxPlayers)
		{
			kickPlayer(playerIndex, _("The game is already full."), ERROR_FULL, false);
		}
		// send everyone's stats to the new guy
		{
			int i;

			for (i = 0; i < MAX_CONNECTED_PLAYERS; i++)
			{
				if (NetPlay.players[i].allocated)
				{
					sendMultiStats(i);
				}
			}
		}
		if (lobby_slashcommands_enabled())
		{
			// Inform the new player that this lobby has slash commands enabled.
			sendRoomSystemMessageToSingleReceiver("Lobby slash commands enabled. Type " LOBBY_COMMAND_PREFIX "help to see details.", playerIndex, true);
		}
	}
	addConsolePlayerJoinMessage(playerIndex);
	return true;
}

bool sendDataCheck()
{
	int i = 0;

	auto w = NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_DATA_CHECK);		// only need to send to HOST
	for (i = 0; i < DATA_MAXDATA; i++)
	{
		NETuint32_t(w, DataHash[i]);
	}
	NETend(w);
	debug(LOG_NET, "sent hash to host");
	return true;
}

bool recvDataCheck(NETQUEUE queue)
{
	int i = 0;
	uint32_t player = queue.index;
	uint32_t tempBuffer[DATA_MAXDATA] = {0};

	if (!NetPlay.isHost)				// only host should act
	{
		ASSERT(false, "Host only routine detected for client!");
		return false;
	}

	auto r = NETbeginDecode(queue, NET_DATA_CHECK);
	for (i = 0; i < DATA_MAXDATA; i++)
	{
		NETuint32_t(r, tempBuffer[i]);
	}
	NETend(r);

	if (player >= MAX_CONNECTED_PLAYERS) // invalid player number.
	{
		debug(LOG_ERROR, "invalid player number (%u) detected.", player);
		return false;
	}

	if (whosResponsible(player) != queue.index)
	{
		HandleBadParam("NET_DATA_CHECK given incorrect params.", player, queue.index);
		return false;
	}

	debug(LOG_NET, "** Received NET_DATA_CHECK from player %u", player);

	if (NetPlay.isHost)
	{
		if (memcmp(DataHash, tempBuffer, sizeof(DataHash)))
		{
			char msg[256] = {'\0'};

			for (i = 0; DataHash[i] == tempBuffer[i]; ++i)
			{
			}

			snprintf(msg, sizeof(msg), _("%s (%u) has an incompatible mod, and has been kicked."), getPlayerName(player), player);
			sendInGameSystemMessage(msg);
			addConsoleMessage(msg, LEFT_JUSTIFY, NOTIFY_MESSAGE);

			kickPlayer(player, _("Your data doesn't match the host's!"), ERROR_WRONGDATA, false);
			debug(LOG_ERROR, "%s (%u) has an incompatible mod. ([%d] got %x, expected %x)", getPlayerName(player), player, i, tempBuffer[i], DataHash[i]);

			return false;
		}
		else
		{
			debug(LOG_NET, "DataCheck message received and verified for player %s (slot=%u)", getPlayerName(player), player);
			ingame.DataIntegrity[player] = true;
		}
	}
	return true;
}
// ////////////////////////////////////////////////////////////////////////////
// Setup Stuff for a new player.
void setupNewPlayer(UDWORD player)
{
	ASSERT_HOST_ONLY(return);
	ASSERT_OR_RETURN(, player < MAX_CONNECTED_PLAYERS, "Invalid player: %" PRIu32 "", player);

	ingame.PingTimes[player] = 0;					// Reset ping time
	ingame.LagCounter[player] = 0;
	ingame.DesyncCounter[player] = 0;
	ingame.VerifiedIdentity[player] = false;
	ingame.JoiningInProgress[player] = true;			// Note that player is now joining
	ingame.joinTimes[player] = std::chrono::steady_clock::now();
	ingame.PendingDisconnect[player] = false;
	ingame.DataIntegrity[player] = false;
	ingame.hostChatPermissions[player] = (NetPlay.bComms) ? NETgetDefaultMPHostFreeChatPreference() : true;
	ingame.lastSentPlayerDataCheck2[player].reset();
	ingame.muteChat[player] = false;
	ingame.lastReadyTimes[player].reset();
	if (multiplayPlayersShouldCheckReady())
	{
		ingame.lastNotReadyTimes[player] = ingame.joinTimes[player];
	}
	else
	{
		ingame.lastNotReadyTimes[player].reset();
	}
	ingame.secondsNotReady[player] = 0;
	ingame.playerLeftGameTime[player].reset();
	multiSyncResetPlayerChallenge(player);

	resetMultiVisibility(player);						// set visibility flags.

	setMultiStats(player, getMultiStats(player), true);  // get the players score

	if (selectedPlayer != player)
	{
		char buf[255];
		ssprintf(buf, _("%s is joining the game"), getPlayerName(player));
		addConsoleMessage(buf, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
}


// While not the perfect place for this, it has to do when a HOST joins (hosts) game
// unfortunately, we don't get the message until after the setup is done.
void ShowLobbyStatusMessage(const std::vector<std::string>& msgs)
{
	char buf[250] = { '\0' };
	// when HOST joins the game, show server MOTD message first
	addConsoleMessage(_("Server message:"), DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
	if (!msgs.empty())
	{
		for (const auto& msg : msgs)
		{
			addConsoleMessage(msg.c_str(), DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
		}
	}
	else
	{
		ssprintf(buf, "%s", "Null message");
		addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
	}
	audio_PlayTrack(FE_AUDIO_MESSAGEEND);

}
