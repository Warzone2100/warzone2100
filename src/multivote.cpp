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
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"

#include "multiplay.h"
#include "multiint.h"
#include "notifications.h"
#include "hci/teamstrategy.h"

#include <array>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;


// MARK: -

#define VOTE_TAG                 "voting"
#define VOTE_KICK_TAG_PREFIX     "votekick::"

static uint8_t playerVotes[MAX_PLAYERS];

enum class NetVoteType
{
	LOBBY_SETTING_CHANGE,
	KICK_PLAYER
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
	NETbeginEncode(NETbroadcastQueue(), NET_VOTE);
	NETuint32_t(&selectedPlayer);
	uint32_t voteID = 0;
	NETuint32_t(&voteID);
	uint8_t voteType = static_cast<uint8_t>(NetVoteType::LOBBY_SETTING_CHANGE);
	NETuint8_t(&voteType);
	NETuint8_t(&currentVote);
	NETend();
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
	playerVotes[player] = (newVote == 1) ? 1 : 0;

	debug(LOG_NET, "total votes: %d/%d", static_cast<int>(getLobbyChangeVoteTotal()), static_cast<int>(NET_numHumanPlayers()));

	// there is no "votes" that disallows map change so assume they are all allowing
	if(newVote == 1) {
		char msg[128] = {0};
		ssprintf(msg, _("%s (%d) allowed map change. Total: %d/%d"), NetPlay.players[player].name, player, static_cast<int>(getLobbyChangeVoteTotal()), static_cast<int>(NET_numHumanPlayers()));
		sendRoomSystemMessage(msg);
	}
}

void sendPlayerKickedVote(uint32_t voteID, uint8_t newVote)
{
	NETbeginEncode(NETbroadcastQueue(), NET_VOTE);
	NETuint32_t(&selectedPlayer);
	NETuint32_t(&voteID);
	uint8_t voteType = static_cast<uint8_t>(NetVoteType::KICK_PLAYER);
	NETuint8_t(&voteType);
	NETuint8_t(&newVote);
	NETend();
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
			outputMsg = astringf(_("A player voted FOR kicking: %s"), getPlayerName(it->target_player_id));
			sendInGameSystemMessage(outputMsg.c_str());
			debug(LOG_INFO, "Player [%" PRIu32 "] %s voted FOR kicking player: %s", sender, getPlayerName(sender), getPlayerName(it->target_player_id));
		}
		else
		{
			if (newVote == 0)
			{
				outputMsg = astringf(_("A player voted AGAINST kicking: %s"), getPlayerName(it->target_player_id));
				sendInGameSystemMessage(outputMsg.c_str());
				debug(LOG_INFO, "Player [%" PRIu32 "] %s voted AGAINST kicking player: %s", sender, getPlayerName(sender), getPlayerName(it->target_player_id));
			}
			else
			{
				outputMsg = astringf(_("A player's client ignored your vote to kick request (too frequent): %s"), getPlayerName(it->target_player_id));
				addConsoleMessage(outputMsg.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE, false); // only display to the host
				debug(LOG_INFO, "Player [%" PRIu32 "] %s ignored vote to kick request for player: %s - (too frequent)", sender, getPlayerName(sender), getPlayerName(it->target_player_id));
			}
		}
	}

	if (handleVoteKickResult(*it))
	{
		// got a result and handled it
		pendingKickVotes.erase(it);
	}
}

bool recvVote(NETQUEUE queue)
{
	ASSERT_HOST_ONLY(return true);

	uint32_t player = MAX_PLAYERS;
	uint32_t voteID = 0;
	uint8_t voteType = 0;
	uint8_t newVote = 0;

	NETbeginDecode(queue, NET_VOTE);
	NETuint32_t(&player);
	NETuint32_t(&voteID);
	NETuint8_t(&voteType);
	NETuint8_t(&newVote);
	NETend();

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Invalid NET_VOTE from player %d: player id = %d", queue.index, static_cast<int>(player));
		return false;
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
	NETbeginEncode(NETbroadcastQueue(), NET_VOTE_REQUEST);
	NETuint32_t(&selectedPlayer);
	NETuint32_t(&targetPlayer);
	NETuint32_t(&voteID);
	uint8_t voteType = static_cast<uint8_t>(type);
	NETuint8_t(&voteType);
	NETend();

	return true;
}

bool recvVoteRequest(NETQUEUE queue)
{
	uint32_t sender = MAX_PLAYERS;
	uint32_t targetPlayer = MAX_PLAYERS;
	uint32_t voteID = 0;
	uint8_t voteType = 0;
	NETbeginDecode(queue, NET_VOTE_REQUEST);
	NETuint32_t(&sender);
	NETuint32_t(&targetPlayer);
	NETuint32_t(&voteID);
	NETuint8_t(&voteType);
	NETend();

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
		std::string outputMsg = astringf(_("The vote to kick player %s succeeded (sufficient votes in favor) - kicking"), getPlayerName(pendingVote.target_player_id));
		sendInGameSystemMessage(outputMsg.c_str());
		std::string logMsg = astringf("kicked %s : %s from the game", getPlayerName(pendingVote.target_player_id), NetPlay.players[pendingVote.target_player_id].IPtextAddress);
		NETlogEntry(logMsg.c_str(), SYNC_FLAG, pendingVote.target_player_id);

		kickPlayer(pendingVote.target_player_id, "The players have voted to kick you from the game.", ERROR_KICKED, false);
	}
	else
	{
		// Vote failed - message all players
		std::string outputMsg = astringf(_("The vote to kick player %s failed (insufficient votes in favor)"), getPlayerName(pendingVote.target_player_id));
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
				std::string outputMsg = astringf(_("The vote to kick player %s failed (insufficient votes before timeout)"), getPlayerName(it->target_player_id));
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
			std::string outputMsg = astringf(_("The vote to kick player %s failed"), getPlayerName(targetPlayer));
			addConsoleMessage(outputMsg.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE, false);
			return false;
		}
	}

	if (lastKickVoteForEachPlayer[targetPlayer].has_value())
	{
		if (realTime - lastKickVoteForEachPlayer[targetPlayer].value() < MIN_INTERVAL_BETWEEN_PLAYER_KICK_VOTES_MS + 5000) // + extra on the sender side
		{
			// Prevent spamming kick votes
			std::string outputMsg = astringf(_("Cannot request vote to kick player %s yet - please wait a bit longer"), getPlayerName(targetPlayer));
			addConsoleMessage(outputMsg.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE, false);
			return false;
		}
	}

	// Store in the list of pending votes
	pendingKickVotes.push_back(pendingVote);

	// Initiate a network vote
	sendVoteRequest(NetVoteType::KICK_PLAYER, pendingVote.unique_vote_id, pendingVote.target_player_id);

	std::string outputMsg = astringf(_("Starting vote to kick player: %s"), getPlayerName(targetPlayer));
	addConsoleMessage(outputMsg.c_str(), DEFAULT_JUSTIFY, SYSTEM_MESSAGE, false);
	debug(LOG_INFO, "Starting vote to kick player: %s", getPlayerName(targetPlayer));

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
