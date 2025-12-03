/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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

#pragma once

#include "lib/widget/widget.h"
#include "lib/widget/form.h"

#include <unordered_set>
#include <unordered_map>
#include <chrono>

#define FOREACH_QUICKCHATMSG(MSG) \
	/* LOBBY ONLY */ \
	MSG(LOBBY_GREETING_WAVE) \
	MSG(LOBBY_GREETING_WELCOME) \
	MSG(LOBBY_DESIRE_TO_SPECTATE) \
	MSG(LOBBY_DESIRE_TO_PLAY) \
	MSG(LOBBY_DESIRE_TO_SWITCH_TEAMS) \
	MSG(LOBBY_REQUEST_CHECK_READY) \
	\
	/* ALL (lobby + in-game global *and* team) */ \
	\
	MSG(REQUEST_PLEASE_WAIT) \
	MSG(REQUEST_LETS_GO) \
	\
	MSG(NOTICE_ALMOST_READY) \
	MSG(NOTICE_READY) \
	MSG(NOTICE_BRB) \
	MSG(NOTICE_I_AM_BACK) \
	\
	MSG(REACTIONS_YES) \
	MSG(REACTIONS_NO) \
	MSG(REACTIONS_MAYBE) \
	MSG(REACTIONS_NOT_YET) \
	MSG(REACTIONS_SOON) \
	MSG(REACTIONS_THANK_YOU) \
	MSG(REACTIONS_NO_PROBLEM) \
	\
	/* In-game reactions (global + team) */ \
	MSG(REACTIONS_WELL_PLAYED) \
	MSG(REACTIONS_CENSORED) \
	MSG(REACTIONS_SORRY_DO_NOT_UNDERSTAND_PLEASE_USE_QUICK_CHAT) \
	\
	/* Global Taunts */ \
	MSG(REACTIONS_TAUNT_GET_READY) \
	MSG(REACTIONS_TAUNT_YOURE_GOING_TO_REGRET_THAT) \
	MSG(REACTIONS_TAUNT_BARELY_A_SCRATCH) \
	\
	/* Team communication */ \
	MSG(TEAM_REQUEST_ATTACK_NOW) \
	MSG(TEAM_REQUEST_GROUP_UP) \
	MSG(TEAM_REQUEST_SPLIT_UP) \
	MSG(TEAM_REQUEST_FOCUS_ATTACKS_WHERE_MARKED) \
	MSG(TEAM_REQUEST_RETREAT) \
	MSG(TEAM_REQUEST_NEED_HELP) \
	MSG(TEAM_REQUEST_LEFT_SIDE) \
	MSG(TEAM_REQUEST_RIGHT_SIDE) \
	MSG(TEAM_REQUEST_TRANSFER_UNITS) \
	MSG(TEAM_REQUEST_TRANSFER_REBUILD) \
	\
	MSG(TEAM_NOTICE_ATTACKING_NOW) \
	MSG(TEAM_NOTICE_ON_MY_WAY) \
	MSG(TEAM_NOTICE_OPPONENTS_COMING) \
	MSG(TEAM_NOTICE_BEING_ATTACKED) \
	MSG(TEAM_NOTICE_RUSHING_OILS) \
	MSG(TEAM_NOTICE_OPPONENTS_RUSHING_OILS) \
	MSG(TEAM_NOTICE_DONT_HAVE_POWER) \
	\
	MSG(TEAM_SUGGESTION_BUILD_MORE_UNITS) \
	MSG(TEAM_SUGGESTION_BUILD_DIFFERENT_UNITS) \
	MSG(TEAM_SUGGESTION_CHECK_TEAM_STRATEGY) \
	MSG(TEAM_SUGGESTION_RESEARCH_DIFFERENT_TECH) \
	MSG(TEAM_SUGGESTION_KEEP_RESEARCH_BUSY) \
	MSG(TEAM_SUGGESTION_BUILD_ANTI_AIR) \
	MSG(TEAM_SUGGESTION_REPAIR_UNITS) \
	MSG(TEAM_SUGGESTION_BUILD_REPAIR_FACILITIES) \
	MSG(TEAM_SUGGESTION_BUILD_POWER_GEN) \
	MSG(TEAM_SUGGESTION_BUILD_CAPTURE_OILS) \
	\
	/* Team reactions */ \
	MSG(REACTIONS_TEAM_THAT_DIDNT_GO_WELL) \
	MSG(REACTIONS_TEAM_I_HAVE_ANOTHER_PLAN) \
	\
	/* End game reactions */ \
	MSG(REACTIONS_ENDGAME_GOOD_GAME) \
	MSG(REACTIONS_ENDGAME_I_GIVE_UP) \
	MSG(REACTIONS_ENDGAME_SORRY_HAVE_TO_LEAVE) \
	\
	/* FROM THIS POINT ON - ONLY INTERNAL MESSAGES! */ \
	/* WZ-generated internal messages - not for users to deliberately send */ \
	MSG(INTERNAL_MSG_DELIVERY_FAILURE_TRY_AGAIN) /* This should always be the first internal message! */ \
	MSG(INTERNAL_LOBBY_NOTICE_MAP_DOWNLOADED) \
	MSG(INTERNAL_ADMIN_ACTION_NOTICE) \
	MSG(INTERNAL_LOCALIZED_LOBBY_NOTICE)

#define GENERATE_ENUM(ENUM) ENUM,

enum class WzQuickChatMessage : uint32_t
{
	FOREACH_QUICKCHATMSG(GENERATE_ENUM)
	// Always last
	MESSAGE_COUNT
};

constexpr uint32_t WzQuickChatMessage_FIRST_INTERNAL_MSG_VALUE = static_cast<uint32_t>(WzQuickChatMessage::INTERNAL_MSG_DELIVERY_FAILURE_TRY_AGAIN);

const char* to_display_string(WzQuickChatMessage msg);
bool to_WzQuickChatMessage(uint32_t value, WzQuickChatMessage& output);

std::unordered_map<std::string, uint32_t> getWzQuickChatMessageValueMap();

enum class WzQuickChatContext
{
	Lobby,
	InGame
};

enum class WzQuickChatMode
{
	Global,
	Team,
	Cheats,
	EndGame
};

struct WzQuickChatTargeting
{
	bool all = false;
	bool humanTeammates = false;
	bool aiTeammates = false;
	std::unordered_set<uint32_t> specificPlayers;

public:
	static WzQuickChatTargeting targetAll();

public:
	void reset();
	bool noTargets() const;
};

struct WzQuickChatMessageData
{
	uint32_t dataContext = 0;
	uint32_t dataA = 0;
	uint32_t dataB = 0;
};

// Begin: DataContext Enums for specific messages
namespace WzQuickChatDataContexts {

// - INTERNAL_ADMIN_ACTION_NOTICE
namespace INTERNAL_ADMIN_ACTION_NOTICE {
	enum class Context : uint32_t
	{
		Invalid = 0,
		Team,
		Position,
		Color,
		Faction
	};
	WzQuickChatMessageData constructMessageData(Context ctx, uint32_t responsiblePlayerIdx, uint32_t targetPlayerIdx);
} // namespace INTERNAL_ADMIN_ACTION_NOTICE

// - INTERNAL_LOCALIZED_LOBBY_NOTICE
namespace INTERNAL_LOCALIZED_LOBBY_NOTICE {
	enum class Context : uint32_t
	{
		Invalid = 0,
		NotReadyKickWarning,
		NotReadyKicked,
		PlayerShouldCheckReadyNotice
	};
	WzQuickChatMessageData constructMessageData(Context ctx, uint32_t targetPlayerIdx, uint32_t additionalData);
} // namespace INTERNAL_LOCALIZED_LOBBY_NOTICE

} // namespace WzQuickChatDataContexts

std::shared_ptr<W_FORM> createQuickChatForm(WzQuickChatContext context, const std::function<void ()>& onQuickChatSent, optional<WzQuickChatMode> startingPanel = nullopt);

void quickChatInitInGame();

struct NETQUEUE;
void sendQuickChat(WzQuickChatMessage message, uint32_t fromPlayer, WzQuickChatTargeting targeting, optional<WzQuickChatMessageData> messageData = nullopt);
bool recvQuickChat(NETQUEUE queue);

// message throttling, spam prevention
void recordPlayerMessageSent(uint32_t playerIdx);
optional<std::chrono::steady_clock::time_point> playerSpamMutedUntil(uint32_t playerIdx);
void playerSpamMuteNotifyIndexSwap(uint32_t playerIndexA, uint32_t playerIndexB);
void playerSpamMuteReset(uint32_t playerIndex);
