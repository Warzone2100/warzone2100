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
#include "lib/framework/wzapp.h"
#include "lib/framework/crc.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "lib/netplay/netsocket.h"
#include "../activity.h"
#include "../frontend.h"
#include "../multiint.h"
#include "../nethelpers.h"
#include "../notifications.h"
#include "../urlhelpers.h"
#include "../urlrequest.h"
#include "../version.h"
#include <sstream>
#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <limits>
#include <discord_rpc.h>
#include <sodium.h>
#include <EmbeddedJSONSignature.h>
#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

static const char* APPID = "505520727521361941";
static const size_t MAX_DISCORD_STR_LEN = 127; // 128 - 1 (for null terminator)

static bool presenceUpdatesEnabled = true;
static std::unordered_map<std::string, std::chrono::system_clock::time_point> lastDismissedJoinRequestByUserId;
#define WZ_DISCORD_JOIN_SPAM_INTERVAL_SECS 60

#define JOIN_NOTIFICATION_TAG_PREFIX "joinNotify::"
#define JOIN_FIND_AND_CONNECT_TAG  std::string(JOIN_NOTIFICATION_TAG_PREFIX "findandconnect")

static void asyncGetDiscordDefaultUserAvatar(const std::string& discord_user_discriminator, const std::function<void (optional<std::vector<unsigned char>> memoryBuffer)>& callback)
{
	unsigned long user_discriminator = 0;
	try {
		user_discriminator = std::stoul(discord_user_discriminator);
	}
	catch (const std::exception&) {
		// Failed to convert discriminator to unsigned long
		callback(nullopt);
		return;
	}

	std::string user_discriminator_img_str = std::to_string(user_discriminator % 5);

	URLDataRequest urlRequest;
	urlRequest.url = std::string("https://cdn.discordapp.com/embed/avatars/") + urlEncode(user_discriminator_img_str.c_str()) + ".png?size=128";
	urlRequest.onSuccess = [callback](const std::string& url, const HTTPResponseDetails& responseDetails, const std::shared_ptr<MemoryStruct>& data) {
		long httpStatusCode = responseDetails.httpStatusCode();
		if (httpStatusCode != 200)
		{
			wzAsyncExecOnMainThread([httpStatusCode]{
				debug(LOG_WARNING, "Query for default Discord user avatar returned HTTP status code: %ld", httpStatusCode);
			});
			callback(nullopt);
			return;
		}

		if (!data || data->memory == nullptr || data->size == 0)
		{
			// Invalid data response
			callback(nullopt);
			return;
		}

		std::vector<unsigned char> memoryBuffer((unsigned char *)data->memory, ((unsigned char*)data->memory) + data->size);
		callback(memoryBuffer);
	};
	urlRequest.onFailure = [callback](const std::string& url, URLRequestFailureType type, optional<HTTPResponseDetails> transferDetails) {
		callback(nullopt);
	};
	urlRequest.maxDownloadSizeLimit = 4 * 1024 * 1024; // response should never be > 4 MB
	urlRequestData(urlRequest);
}

static void asyncGetDiscordDefaultUserAvatar(const DiscordUser* request, const std::function<void (optional<std::vector<unsigned char>> memoryBuffer)>& callback)
{
	if (request->discriminator == nullptr)
	{
		callback(nullopt);
		return;
	}

	asyncGetDiscordDefaultUserAvatar(request->discriminator, callback);
}

static void asyncGetDiscordUserAvatar(const DiscordUser* request, const std::function<void (optional<std::vector<unsigned char>> memoryBuffer)>& callback)
{
	if (request->avatar == nullptr || strlen(request->avatar) == 0)
	{
		asyncGetDiscordDefaultUserAvatar(request, callback);
		return;
	}

	const std::string discord_user_discriminator = (request->discriminator) ? request->discriminator : "";

	URLDataRequest urlRequest;
	urlRequest.url = std::string("https://cdn.discordapp.com/avatars/") + urlEncode(request->userId) + "/" + urlEncode(request->avatar) + ".png?size=128";
	urlRequest.onSuccess = [callback, discord_user_discriminator](const std::string& url, const HTTPResponseDetails& responseDetails, const std::shared_ptr<MemoryStruct>& data) {
		long httpStatusCode = responseDetails.httpStatusCode();
		if (httpStatusCode != 200)
		{
			wzAsyncExecOnMainThread([httpStatusCode]{
				debug(LOG_WARNING, "Query for Discord user avatar returned HTTP status code: %ld", httpStatusCode);
			});
			// fallback
			asyncGetDiscordDefaultUserAvatar(discord_user_discriminator, callback);
			return;
		}

		if (!data || data->memory == nullptr || data->size == 0)
		{
			// Invalid data response
			// fallback
			asyncGetDiscordDefaultUserAvatar(discord_user_discriminator, callback);
			return;
		}

		std::vector<unsigned char> memoryBuffer((unsigned char *)data->memory, ((unsigned char*)data->memory) + data->size);
		callback(memoryBuffer);
	};
	urlRequest.onFailure = [callback, discord_user_discriminator](const std::string& url, URLRequestFailureType type, optional<HTTPResponseDetails> transferDetails) {
		// fallback
		asyncGetDiscordDefaultUserAvatar(discord_user_discriminator, callback);
	};
	urlRequest.maxDownloadSizeLimit = 4 * 1024 * 1024; // response should never be > 4 MB
	urlRequestData(urlRequest);
}

static std::string getAllianceString(const ActivitySink::MultiplayerGameInfo& info)
{
	switch (info.alliances)
	{
		case ActivitySink::SkirmishGameInfo::AllianceOption::NO_ALLIANCES:
			return "\xf0\x9f\x9a\xab"; // 🚫
		case ActivitySink::SkirmishGameInfo::AllianceOption::ALLIANCES:
			return "\xf0\x9f\x91\x8d"; // 👍
		case ActivitySink::SkirmishGameInfo::AllianceOption::ALLIANCES_TEAMS:
			return "\xf0\x9f\x94\x92\xf0\x9f\x94\xac"; // 🔒🔬
		case ActivitySink::SkirmishGameInfo::AllianceOption::ALLIANCES_UNSHARED:
			return "\xf0\x9f\x94\x92\xf0\x9f\x9a\xab"; // 🔒🚫
	}
	return ""; // silence warning
}

static std::string truncateStringIfExceedsLength(const std::string& inputStr, size_t maxLen, std::string ellipsis = "...")
{
	if (inputStr.size() > maxLen)
	{
		size_t truncatedSize = maxLen - ellipsis.size();
		std::string outputStr = inputStr.substr(0, truncatedSize);
		outputStr += ellipsis;
		return outputStr;
	}
	return inputStr;
}

static void displayCantJoinWhileInGameNotification()
{
	WZ_Notification notification;
	notification.duration = 0;
	notification.contentTitle = _("Discord: Can't Join Game");
	notification.contentText = _("You can't join a game while you're in a game. Please save / quit your current game, exit to the main menu, and try again.");
	addNotification(notification, WZ_Notification_Trigger::Immediate());
}

static void joinGameImpl(const std::vector<JoinConnectionDescription>& joinConnectionDetails)
{
	const auto currentGameMode = ActivityManager::instance().getCurrentGameMode();
	if (currentGameMode != ActivitySink::GameMode::MENUS)
	{
		// Can't join a game while already in a game
		cancelOrDismissNotificationsWithTag(JOIN_FIND_AND_CONNECT_TAG);
		debug(LOG_ERROR, "Can't join a game while already in a game / lobby.");
		return;
	}
	// join the game!
	NetPlay.bComms = true; // use network = true
	NETinit(true);
	// Ensure the joinGame has a place to return to
	changeTitleMode(TITLE);
	JoinGameResult result = joinGame(joinConnectionDetails);
	if (result != JoinGameResult::JOINED)
	{
		cancelOrDismissNotificationsWithTag(JOIN_FIND_AND_CONNECT_TAG);
	}
}

static void findAndJoinLobbyGameImpl(const std::string& lobbyAddress, unsigned int lobbyPort, uint32_t lobbyGameId)
{
	WZ_Notification notification;
	notification.duration = 60 * GAME_TICKS_PER_SEC;
	notification.contentTitle = _("Discord: Finding & Connecting to Game");
	std::string contentStr = _("Attempting to find & connect to the game specified by the Discord invite.");
	contentStr += "\n\n";
	contentStr += _("This may take a moment...");
	notification.contentText = contentStr;
	notification.onDisplay = [lobbyAddress, lobbyPort, lobbyGameId](const WZ_Notification&) {
		// once the notification is completely displayed, trigger the lookup & join

		const auto currentGameMode = ActivityManager::instance().getCurrentGameMode();
		if (currentGameMode != ActivitySink::GameMode::MENUS)
		{
			// Can't join a game while in a game - abort
			cancelOrDismissNotificationsWithTag(JOIN_FIND_AND_CONNECT_TAG);
			displayCantJoinWhileInGameNotification();
			return;
		}

		// For now, it is necessary to call `NETinit(true)` before calling findLobbyGame
		// Obviously this wouldn't be a good idea to do *during* a game, hence the check above to
		// ensure we're still in the menus...
		NETinit(true);
		ingame.side = InGameSide::MULTIPLAYER_CLIENT;
		auto joinConnectionDetails = findLobbyGame(lobbyAddress, lobbyPort, lobbyGameId);

		if (joinConnectionDetails.empty())
		{
			cancelOrDismissNotificationsWithTag(JOIN_FIND_AND_CONNECT_TAG);
			debug(LOG_ERROR, "Join code: Failed to find game in the lobby server: %s:%u", lobbyAddress.c_str(), lobbyPort);
			std::string contentText = _("Failed to find game in the lobby server: ");
			contentText += lobbyAddress + ":" + std::to_string(lobbyPort) + "\n\n";
			contentText += _("The game may have already started, or the host may have disbanded the game lobby.");
			WZ_Notification notification;
			notification.duration = 0;
			notification.contentTitle = _("Failed to Find Game");
			notification.contentText = contentText;
			notification.largeIcon = WZ_Notification_Image("images/notifications/exclamation_triangle.png");
			notification.tag = std::string(JOIN_NOTIFICATION_TAG_PREFIX "failedtoconnect");
			addNotification(notification, WZ_Notification_Trigger::Immediate());
			return;
		}

		ActivityManager::instance().willAttemptToJoinLobbyGame(lobbyAddress, lobbyPort, lobbyGameId, joinConnectionDetails);

		joinGameImpl(joinConnectionDetails);
	};
	notification.largeIcon = WZ_Notification_Image("images/notifications/connect_wait.png");
	notification.tag = JOIN_FIND_AND_CONNECT_TAG;
	addNotification(notification, WZ_Notification_Trigger::Immediate());
}

static bool isTrustedLobbyServer(const std::string& lobbyAddress, unsigned int lobbyPort)
{
	return lobbyAddress == "lobby.wz2100.net";
}

static void joinGameFromSecret_v1(const std::string joinSecretStr)
{
	std::vector<JoinConnectionDescription> joinConnectionDetails;

	auto displayErrorNotification = [](const std::string& errorMessage){
		debug(LOG_ERROR, "%s", errorMessage.c_str());
		std::string contentText = _("Failed to join the game from the specified invite link, with error:");
		contentText += "\n" + errorMessage;
		WZ_Notification notification;
		notification.duration = 0;
		notification.contentTitle = _("Failed to Join Game");
		notification.contentText = contentText;
		notification.largeIcon = WZ_Notification_Image("images/notifications/exclamation_triangle.png");
		notification.tag = std::string(JOIN_NOTIFICATION_TAG_PREFIX "failedtojoin");
		addNotification(notification, WZ_Notification_Trigger::Immediate());
		return;
	};

	std::vector<WzString> joinSecretComponents = WzString::fromUtf8(joinSecretStr).split("/");
	if (joinSecretComponents.size() < 3) {
		displayErrorNotification("Join code: Invalid format");
		return;
	}
	if (joinSecretComponents[0] != "v1") {
		displayErrorNotification("Join code: Invalid format version");
		return;
	}

	auto convertStringToUint32 = [](const std::string& inputStr) -> unsigned int
	{
		unsigned int result = 0;
		try {
			unsigned long ulongValue = std::stoul(inputStr);
			if (ulongValue > static_cast<unsigned long>(std::numeric_limits<unsigned int>::max()))
			{
				debug(LOG_WARNING, "Failed to convert string '%s' to unsigned int because of error: value is > std::numeric_limits<unsigned int>::max()", inputStr.c_str());
			}
			result = static_cast<uint32_t>(ulongValue);
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Failed to convert string '%s' to unsigned int because of error: %s", inputStr.c_str(), e.what());
		}
		return result;
	};

	if (joinSecretComponents[1] == "l")
	{
		// lobby game
		// format: "v1/l/<lobbyAddress>:<lobbyPort>/<lobbyGameId>"
		if (joinSecretComponents.size() == 4)
		{
			std::vector<WzString> lobbyDetails = joinSecretComponents[2].split(":");
			if (lobbyDetails.size() != 2)
			{
				displayErrorNotification(astringf("Join code: Invalid format for subcomponent - size: %zu", lobbyDetails.size()));
				return;
			}
			std::string lobbyAddress = lobbyDetails[0].toUtf8();
			unsigned int lobbyPort = convertStringToUint32(lobbyDetails[1].toUtf8());
			uint32_t lobbyGameId = convertStringToUint32(joinSecretComponents[3].toUtf8());
			if (lobbyGameId == 0)
			{
				// Invalid lobby gameId
				displayErrorNotification(astringf("Join code: Invalid lobby gameId: %s", joinSecretComponents[3].toUtf8().c_str()));
				return;
			}

			if (isTrustedLobbyServer(lobbyAddress, lobbyPort))
			{
				// trusted lobby servers can proceed directly to finding and joining the game
				findAndJoinLobbyGameImpl(lobbyAddress, lobbyPort, lobbyGameId);
				return;
			}
			else
			{
				// asynchronously prompt via notification system to trust lobby server before connecting
				std::ostringstream content;
				content << _("The invite link you have followed specifies a non-default lobby server:") << "\n";
				content << " ⇢ " << lobbyAddress << " : " << lobbyPort << "\n";
				content << "\n";
				content << _("Do you wish to trust this lobby server, and continue?") << "\n";

				WZ_Notification notification;
				notification.duration = 0;
				notification.contentTitle = _("Discord: Connect to Untrusted Lobby?");
				notification.contentText = content.str();
				notification.action = WZ_Notification_Action(_("Trust & Connect"), [lobbyAddress, lobbyPort, lobbyGameId](const WZ_Notification&){
					wzAsyncExecOnMainThread([lobbyAddress, lobbyPort, lobbyGameId]{
						findAndJoinLobbyGameImpl(lobbyAddress, lobbyPort, lobbyGameId);
					});
				});
				notification.largeIcon = WZ_Notification_Image("images/notifications/shield_questionmark.png");
				notification.tag = JOIN_NOTIFICATION_TAG_PREFIX "untrusted_lobby_server_prompt";
				addNotification(notification, WZ_Notification_Trigger::Immediate());
				return;
			}
		}
		else
		{
			displayErrorNotification(astringf("Join code: Invalid number of components: %zu", joinSecretComponents.size()));
		}
	}
	else if (joinSecretComponents[1] == "i")
	{
		// direct connection join code
		// format: "v1/i/<uniqueGameStr>/<base64url-network-binary-format-ip-address>:<port>/..."

		// for now, skip the uniqueGameStr
		// joinSecretComponents[2]

		for (size_t i = 3; i < joinSecretComponents.size(); i++)
		{
			// each component should be a "<base64url-network-binary-format-ip-address>:<port>" combo
			std::vector<WzString> connectionDetails = joinSecretComponents[i].split(":");
			if (connectionDetails.size() != 2)
			{
				// Invalid format
				displayErrorNotification(astringf("Join code: Invalid format for subcomponent - size: %zu", connectionDetails.size()));
				return;
			}
			std::vector<unsigned char> ipNetworkBinaryFormat = EmbeddedJSONSignature::b64Decode(b64UrlSafeTob64(connectionDetails[0].toUtf8()));
			std::string ipAddressStr = ipv6_NetBinary_To_AddressString(ipNetworkBinaryFormat);
			if (ipAddressStr.empty())
			{
				ipAddressStr = ipv4_NetBinary_To_AddressString(ipNetworkBinaryFormat);
			}
			if (ipAddressStr.empty())
			{
				// Invalid ip address binary format contents
				displayErrorNotification("Join code: Invalid connecting IP address");
				return;
			}
			uint32_t port = convertStringToUint32(connectionDetails[1].toUtf8());
			if (port == 0)
			{
				// Invalid port provided
				displayErrorNotification("Join code: Invalid connecting port");
				return;
			}
			joinConnectionDetails.push_back(JoinConnectionDescription(ipAddressStr, port));
		}

		if (joinConnectionDetails.empty())
		{
			// no connection details provided...
			displayErrorNotification("Join code: Missing connection details");
			return;
		}

		// Prompt the user to permit direct connection join
		std::ostringstream content;
		content << _("The invite link you have followed specifies a direct connection.") << "\n";
		content << _("It has not been vetted / verified by a lobby server.") << "\n";
		content << "\n";
		content << _("Do you wish to trust this direct connection invite?") << "\n";

		WZ_Notification notification;
		notification.duration = 0;
		notification.contentTitle = _("Discord: Direct Connection Invite");
		notification.contentText = content.str();
		notification.action = WZ_Notification_Action(_("Trust & Connect"), [joinConnectionDetails](const WZ_Notification&){
			wzAsyncExecOnMainThread([joinConnectionDetails]{
				joinGameImpl(joinConnectionDetails);
			});
		});
		notification.largeIcon = WZ_Notification_Image("images/notifications/user_questionmark.png");
		notification.tag = JOIN_NOTIFICATION_TAG_PREFIX "direct_invite_prompt";
		addNotification(notification, WZ_Notification_Trigger::Immediate());
	}
}

// May complete asynchronously / prompt the user / etc
static void joinGameFromSecret(const std::string joinSecret)
{
	if (joinSecret.rfind("v1/", 0) == 0)
	{
		// v1 join secret
		joinGameFromSecret_v1(joinSecret);
		return;
	}
	else
	{
		// unknown / unsupported join code version
		std::string firstComponent;
		size_t firstComponentEnd = joinSecret.find("/");
		if (firstComponentEnd != std::string::npos)
		{
			firstComponent = joinSecret.substr(0, firstComponentEnd);
		}
		debug(LOG_ERROR, "Unsupported join code version: \"%s\"", firstComponent.c_str());
		// Display a notification that a new version of WZ may be required
		std::ostringstream content;
		content << _("The invite link you have followed isn't supported by this version of Warzone 2100.") << "\n";
		content << _("A newer version of Warzone 2100 may be required, if available.") << "\n";
		WZ_Notification notification;
		notification.duration = 0;
		notification.contentTitle = _("Discord: Unsupported / Invalid Invite");
		notification.contentText = content.str();
		notification.tag = JOIN_NOTIFICATION_TAG_PREFIX "invite_unsupported_version_message";
		addNotification(notification, WZ_Notification_Trigger::Immediate());
		return;
	}
}

// MARK: - DiscordRPCActivitySink

#define WZ_DISCORD_RICH_PRESENCE_UPDATE_TICKS	3 * GAME_TICKS_PER_SEC

class DiscordRPCActivitySink : public ActivitySink {
private:
	struct DiscordRichPresence_Internal {
		std::string state;   /* max 128 bytes */
		std::string details; /* max 128 bytes */
		int64_t startTimestamp;
		int64_t endTimestamp;
		std::string largeImageKey;  /* max 32 bytes */
		std::string largeImageText; /* max 128 bytes */
		std::string smallImageKey;  /* max 32 bytes */
		std::string smallImageText; /* max 128 bytes */
		std::string partyId;        /* max 128 bytes */
		int partySize;
		int partyMax;
		std::string matchSecret;    /* max 128 bytes */
		std::string joinSecret;     /* max 128 bytes */
		std::string spectateSecret; /* max 128 bytes */
		int8_t instance;

		void clear() {
			state.clear();
			details.clear();
			startTimestamp = 0;
			endTimestamp = 0;
			largeImageKey.clear();
			largeImageText.clear();
			smallImageKey.clear();
			smallImageText.clear();
			partyId.clear();
			partySize = 0;
			partyMax = 0;
			matchSecret.clear();
			joinSecret.clear();
			spectateSecret.clear();
			instance = 0;
		}
	};

	DiscordRichPresence_Internal currentRichPresenceData;
	uint32_t lastDiscordPresenceUpdate = 0;
	bool queuedPresenceUpdate = false;
public:
	DiscordRPCActivitySink()
	{
		setDiscordStatus_InMenus();
		updateDiscordPresence();
	}

	// campaign games
	virtual void startedCampaignMission(const std::string& campaign, const std::string& levelName) override
	{
		cancelOrDismissJoinNotifications();
		int64_t oldStartTimestamp = currentRichPresenceData.startTimestamp;
		currentRichPresenceData.clear();
		currentRichPresenceData.state = _("Playing: Campaign");
		currentRichPresenceData.details = campaign + " - " + levelName;
		currentRichPresenceData.startTimestamp = (oldStartTimestamp != 0) ? oldStartTimestamp : currentDiscordTimestamp();
		currentRichPresenceData.largeImageKey = "wz_icon";
		currentRichPresenceData.smallImageKey = "campaign_sm";
		updateDiscordPresence();
	}
	virtual void endedCampaignMission(const std::string& campaign, const std::string& levelName, GameEndReason result, END_GAME_STATS_DATA stats, bool cheatsUsed) override
	{
		if (result == GameEndReason::QUIT)
		{
			setDiscordStatus_InMenus();
			updateDiscordPresence();
			return;
		}
		currentRichPresenceData.endTimestamp = currentDiscordTimestamp();
		updateDiscordPresence();
	}

	// challenges
	virtual void startedChallenge(const std::string& challengeName) override
	{
		cancelOrDismissJoinNotifications();

		currentRichPresenceData.clear();
		currentRichPresenceData.state = _("Playing: Challenge");
		currentRichPresenceData.details = std::string("\"") + challengeName + "\"";
		currentRichPresenceData.startTimestamp = currentDiscordTimestamp();
		currentRichPresenceData.largeImageKey = "wz_icon";
		updateDiscordPresence();
	}

	virtual void endedChallenge(const std::string& challengeName, GameEndReason result, const END_GAME_STATS_DATA& stats, bool cheatsUsed) override
	{
		setDiscordStatus_InMenus();
		updateDiscordPresence();
	}

	virtual void startedSkirmishGame(const SkirmishGameInfo& info) override
	{
		cancelOrDismissJoinNotifications();

		currentRichPresenceData.clear();
		std::string stateStr = _("Playing: Skirmish");
		std::string teamDescription = ActivitySink::getTeamDescription(info);
		if (!teamDescription.empty())
		{
			// TRANSLATORS: "%s" is a team description, ex. "2v2"
			stateStr = std::string(astringf(_("Playing: %s Skirmish"), teamDescription.c_str()));
		}
		currentRichPresenceData.state = stateStr;
		std::string details = std::string(_("Map:")) + " " + info.game.map;
		if (info.numAIBotPlayers > 0)
		{
			details += std::string(" | ") + _("Bots:") + " " + std::to_string(info.numAIBotPlayers);
		}
		currentRichPresenceData.details = details;

		currentRichPresenceData.startTimestamp = currentDiscordTimestamp();
		currentRichPresenceData.largeImageKey = "wz_icon";
		currentRichPresenceData.smallImageKey = "skirmish_sm";
		updateDiscordPresence();
	}

	virtual void endedSkirmishGame(const SkirmishGameInfo& info, GameEndReason result, const END_GAME_STATS_DATA& stats) override
	{
		setDiscordStatus_InMenus();
		updateDiscordPresence();
	}

	// multiplayer
	virtual void hostingMultiplayerGame(const MultiplayerGameInfo& info) override;
	virtual void joinedMultiplayerGame(const MultiplayerGameInfo& info) override
	{
		cancelOrDismissNotificationsWithTag(JOIN_FIND_AND_CONNECT_TAG);
		currentRichPresenceData.clear();

		currentRichPresenceData.state = _("In Multiplayer Lobby");
		currentRichPresenceData.startTimestamp = currentDiscordTimestamp();

		setBaseMultiplayerGameInfo(info);
		updateDiscordPresence();
	}
	virtual void updateMultiplayerGameInfo(const MultiplayerGameInfo& info) override
	{
		setBaseMultiplayerGameInfo(info);
		updateDiscordPresence();
	}
	virtual void leftMultiplayerGameLobby(bool wasHost, LOBBY_ERROR_TYPES type) override
	{
		setDiscordStatus_InMenus();
		updateDiscordPresence();
	}
	virtual void startedMultiplayerGame(const MultiplayerGameInfo& info) override
	{
		cancelOrDismissJoinNotifications();
		lastDismissedJoinRequestByUserId.clear();

		currentRichPresenceData.endTimestamp = 0;
		currentRichPresenceData.matchSecret.clear();
		currentRichPresenceData.joinSecret.clear();
		currentRichPresenceData.spectateSecret.clear();

		currentRichPresenceData.state = _("In Multiplayer Game");
		currentRichPresenceData.startTimestamp = currentDiscordTimestamp();

		setBaseMultiplayerGameInfo(info);
		updateDiscordPresence();
	}
	virtual void endedMultiplayerGame(const MultiplayerGameInfo& info, GameEndReason result, const END_GAME_STATS_DATA& stats) override
	{
		lastDismissedJoinRequestByUserId.clear();
		setDiscordStatus_InMenus();
		updateDiscordPresence();
	}

public:

	void processQueuedPresenceUpdate();

private:
	void setJoinInformation(const ActivitySink::MultiplayerGameInfo& info, int64_t startTimestamp);
	void setBaseMultiplayerGameInfo(const ActivitySink::MultiplayerGameInfo& info);
	void cancelOrDismissJoinNotifications()
	{
		cancelOrDismissNotificationIfTag([](const std::string& tag) {
			return (tag.rfind(JOIN_NOTIFICATION_TAG_PREFIX, 0) == 0);
		});
	}
	void updateDiscordPresence();
	void setDiscordStatus_InMenus();
	int64_t currentDiscordTimestamp()
	{
		// NOTE: Only C++20+ guarantees that system_clock's epoch is the Unix epoch, but for earlier versions it's the case for almost all platforms.
		int64_t secondsSinceUnixEpoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		return secondsSinceUnixEpoch;
	}
};

static std::shared_ptr<DiscordRPCActivitySink> discordSink;

static std::string hashAndB64EncodePartyId(const std::string& rawPartyId)
{
	std::vector<unsigned char> hash(crypto_generichash_BYTES, 0);
	crypto_generichash(hash.data(), hash.size(),
					   reinterpret_cast<const unsigned char*>(rawPartyId.data()), rawPartyId.size(),
					   nullptr, 0);

	return EmbeddedJSONSignature::b64Encode(hash);
}

// NOTE: Uses host byte order
std::vector<unsigned char> int64_tToUnsignedCharVector(int64_t value)
{
	std::vector<unsigned char> result(sizeof(int64_t), '\0');
	memcpy(result.data(), &value, sizeof(int64_t));
	return result;
}

void DiscordRPCActivitySink::setJoinInformation(const ActivitySink::MultiplayerGameInfo& info, int64_t startTimestamp)
{
	if (info.lobbyGameId != 0)
	{
		// construct a "lobby game" join secret
		std::string lobbyAddressAndPort = info.lobbyAddress + ":" + std::to_string(info.lobbyPort);

		// join secret must be <= 127 characters
		// support a custom lobby + port if they are <= 64 chars long
		if (lobbyAddressAndPort.size() > 64)
		{
			// lobby address is too long
			debug(LOG_WARNING, "Lobby address exceeds maximum supported length for join code: %s", lobbyAddressAndPort.c_str());
			return;
		}

		std::string joinSecretDetails = lobbyAddressAndPort + "/" + std::to_string(info.lobbyGameId);

		std::string joinSecretStr = std::string("v1/l/") + joinSecretDetails;
		currentRichPresenceData.joinSecret = joinSecretStr;

		// construct a unique party id from the lobby address/port + lobbyGameId
		std::string rawPartyId = std::to_string(info.lobbyGameId) + ":" + info.lobbyAddress + ":" + std::to_string(info.lobbyPort) + ":" + std::to_string(info.lobbyGameId);
		currentRichPresenceData.partyId = hashAndB64EncodePartyId(rawPartyId);
		return;
	}
	else
	{
		// no lobby id / connection
		if (info.isHost)
		{
			// asynchronously try to determine the public ip address(es) for this host
			// to construct a "direct connection" join secret
			std::weak_ptr<DiscordRPCActivitySink> weakDiscordSink(discordSink);
			const auto listeningInterfaces = info.listeningInterfaces;
			getPublicIPAddress([weakDiscordSink, listeningInterfaces, startTimestamp](const std::string& ipv4Address, const std::string& ipv6Address, const std::vector<std::string>& errors) {
				auto pSink = weakDiscordSink.lock();
				if (!pSink)
				{
					return;
				}
				// Convert ip address strings to binary, in network-byte-order format, then base64-encode
				// Append the port as a separate string component
				std::string joinSecretDetails;
				if (!ipv4Address.empty())
				{
					auto ipv4AddressBinaryForm = ipv4_AddressString_To_NetBinary(ipv4Address);
					if (!ipv4AddressBinaryForm.empty())
					{
						joinSecretDetails += b64Tob64UrlSafe(EmbeddedJSONSignature::b64Encode(ipv4AddressBinaryForm));
						joinSecretDetails += std::string(":") + std::to_string(listeningInterfaces.ipv4_port);
					}
				}
				if (!ipv6Address.empty())
				{
					auto ipv6AddressBinaryForm = ipv6_AddressString_To_NetBinary(ipv6Address);
					if (!ipv6AddressBinaryForm.empty())
					{
						if (!joinSecretDetails.empty())
						{
							joinSecretDetails += "/";
						}
						joinSecretDetails += b64Tob64UrlSafe(EmbeddedJSONSignature::b64Encode(ipv6AddressBinaryForm));
						joinSecretDetails += std::string(":") + std::to_string(listeningInterfaces.ipv6_port);
					}
				}

				if (joinSecretDetails.empty())
				{
					// no valid join details available - just exit
					return;
				}

				// construct a unique code to distinguish multiple games that use the same direct connection details
				// (particularly important for ensuring a unique partyId if hosting multiple successive games)
				// for now, base64-encode startTimestamp
				std::string uniqueGameStr = b64Tob64UrlSafe(EmbeddedJSONSignature::b64Encode(int64_tToUnsignedCharVector(startTimestamp)));

				// construct "direct connection" join secret
				std::string joinSecretStr = std::string("v1/i/") + uniqueGameStr + "/" + joinSecretDetails;

				// construct a unique party id from the uniqueGameStr + ip addresses, hashed
				std::string rawPartyId = std::string("direct_connection:") + uniqueGameStr + ":" + ipv4Address + ":" + std::to_string(listeningInterfaces.ipv4_port) + ":" + ipv6Address + ":" + std::to_string(listeningInterfaces.ipv6_port);
				std::string partyIdStr = hashAndB64EncodePartyId(rawPartyId);

				wzAsyncExecOnMainThread([pSink, joinSecretStr, partyIdStr](){
					// Verify that game state is still "in lobby"
					const auto currentGameMode = ActivityManager::instance().getCurrentGameMode();
					if ((currentGameMode != ActivitySink::GameMode::HOSTING_IN_LOBBY) && (currentGameMode != ActivitySink::GameMode::JOINING_IN_LOBBY))
					{
						// game is no longer in lobby
						return;
					}
					pSink->currentRichPresenceData.joinSecret = joinSecretStr;
					pSink->currentRichPresenceData.partyId = partyIdStr;
					// Trigger presence update
					pSink->updateDiscordPresence();
				});
			}, info.listeningInterfaces.IPv4, info.listeningInterfaces.IPv6);
		}
	}
}

void DiscordRPCActivitySink::hostingMultiplayerGame(const MultiplayerGameInfo& info)
{
	lastDismissedJoinRequestByUserId.clear();
	currentRichPresenceData.clear();

	currentRichPresenceData.state = _("In Multiplayer Lobby");
	currentRichPresenceData.startTimestamp = currentDiscordTimestamp();
	setJoinInformation(info, currentRichPresenceData.startTimestamp); // may set values asynchronously, if needed

	setBaseMultiplayerGameInfo(info);
	updateDiscordPresence();
}

void DiscordRPCActivitySink::setBaseMultiplayerGameInfo(const MultiplayerGameInfo& info)
{
	std::string botsCountStr;
	if (info.numAIBotPlayers > 0)
	{
		botsCountStr = std::string(" | ");
		// TRANSLATORS: AI Bots. (Used to describe the number of bot players.)
		// Should be a fairly short string - space is limited. Use an abbreviation / acronym if necessary.
		botsCountStr += _("Bots:");
		botsCountStr += " " + std::to_string(info.numAIBotPlayers);
	}
	// Truncate the map name to keep the details string < 128 chars
	std::string details = truncateStringIfExceedsLength(std::string(_("Map:")) + " " + info.game.map, MAX_DISCORD_STR_LEN - botsCountStr.size());
	details += botsCountStr;
	currentRichPresenceData.details = details;
	if (!info.privateGame)
	{
		currentRichPresenceData.largeImageKey = "wz_icon";
	}
	else
	{
		currentRichPresenceData.largeImageKey = "wz_icon_locked";
	}
	std::string hostNameStr = std::string("[") + truncateStringIfExceedsLength(info.hostName, MAX_DISCORD_STR_LEN / 2) + "]";
	std::string nameAndHostStr = astringf(_("Game Name: \"%s\" by %s"), info.game.name, hostNameStr.c_str());
	currentRichPresenceData.largeImageText = truncateStringIfExceedsLength(nameAndHostStr, MAX_DISCORD_STR_LEN);
	const auto currentGameMode = ActivityManager::instance().getCurrentGameMode();
	if (currentGameMode == ActivitySink::GameMode::MULTIPLAYER)
	{
		currentRichPresenceData.smallImageKey = "multiplayer_sm";
	}
	else
	{
		currentRichPresenceData.smallImageKey = "in_lobby_sm";
	}

	// Expanded Game Info
	// 🔐 | No: Tanks,Cyborgs,VTOLs,Uplink,Lassat | T1P1B2 | Alliances: 🔒🔬 | WZ: 3.3.0
	std::string gameInfoStr;
	if (info.privateGame)
	{
		const std::string passwordEmoji = "\xf0\x9f\x94\x90"; // 🔐
		gameInfoStr += passwordEmoji + " | ";
	}
	if (info.hasLimits())
	{
		std::vector<std::string> limits;
		if (info.limit_no_tanks)					///< Flag for tanks disabled
		{
			limits.push_back("Tanks");
		}
		if (info.limit_no_cyborgs)					///< Flag for cyborgs disabled
		{
			limits.push_back("Cyborgs");
		}
		if (info.limit_no_vtols)					///< Flag for VTOLs disabled
		{
			limits.push_back("VTOLs");
		}
		if (info.limit_no_uplink)					///< Flag for Satellite Uplink disabled
		{
			limits.push_back("Uplink");
		}
		if (info.limit_no_lassat)					///< Flag for Laser Satellite Command Post disabled
		{
			limits.push_back("Lassat");
		}
		if (!limits.empty())
		{
			// TRANSLATORS: "No:" to signify limits - example usage: "No: Tanks,VTOLs"
			// Should be a short string - space is limited. Use an abbreviation / symbol if necessary.
			gameInfoStr += _("No:");
			gameInfoStr += " ";
			for (size_t i = 0; i < limits.size(); i++)
			{
				if (i > 0)
				{
					gameInfoStr += ",";
				}
				gameInfoStr += limits[i];
			}
			gameInfoStr += std::string(" | ");
		}
	}
	gameInfoStr += "T" + std::to_string(info.game.techLevel) + "P" + std::to_string(info.game.power) + "B" + std::to_string(info.game.base);
	gameInfoStr += std::string(" | ");
	// TRANSLATORS: Should be a fairly short string - space is limited. Use an abbreviation if necessary.
	gameInfoStr += _("Alliances:");
	gameInfoStr += " " + getAllianceString(info);
	if (gameInfoStr.size() < MAX_DISCORD_STR_LEN)
	{
		std::string versionStr = std::string(" | WZ:") + " " + version_getVersionString();
		gameInfoStr += truncateStringIfExceedsLength(versionStr, MAX_DISCORD_STR_LEN - gameInfoStr.size()); // truncate version if too long;
	}
	currentRichPresenceData.smallImageText = truncateStringIfExceedsLength(gameInfoStr, MAX_DISCORD_STR_LEN); // Limits data, Version of WZ, etc

	currentRichPresenceData.partySize = info.maxPlayers - info.numAvailableSlots;
	currentRichPresenceData.partyMax = info.maxPlayers;
}

void DiscordRPCActivitySink::setDiscordStatus_InMenus()
{
	currentRichPresenceData.clear();
	currentRichPresenceData.state = _("In Menu");
	currentRichPresenceData.largeImageKey = "wz_icon";
}

void DiscordRPCActivitySink::processQueuedPresenceUpdate()
{
	if (!queuedPresenceUpdate) { return; }
	if (realTime - lastDiscordPresenceUpdate >= WZ_DISCORD_RICH_PRESENCE_UPDATE_TICKS)
	{
		updateDiscordPresence();
	}
}

void DiscordRPCActivitySink::updateDiscordPresence()
{
	if (realTime - lastDiscordPresenceUpdate < WZ_DISCORD_RICH_PRESENCE_UPDATE_TICKS)
	{
		// throttle rich presence updates
		queuedPresenceUpdate = true;
		return;
	}

	if (presenceUpdatesEnabled)
	{
		DiscordRichPresence discordPresence;
		memset(&discordPresence, 0, sizeof(discordPresence));
		discordPresence.state = currentRichPresenceData.state.c_str();   /* max 128 bytes */
		discordPresence.details = currentRichPresenceData.details.c_str(); /* max 128 bytes */
		discordPresence.startTimestamp = currentRichPresenceData.startTimestamp;
		discordPresence.endTimestamp = currentRichPresenceData.endTimestamp;
		discordPresence.largeImageKey = currentRichPresenceData.largeImageKey.c_str();  /* max 32 bytes */
		discordPresence.largeImageText = currentRichPresenceData.largeImageText.c_str(); /* max 128 bytes */
		discordPresence.smallImageKey = currentRichPresenceData.smallImageKey.c_str();  /* max 32 bytes */
		discordPresence.smallImageText = currentRichPresenceData.smallImageText.c_str(); /* max 128 bytes */
		discordPresence.partyId = currentRichPresenceData.partyId.c_str();        /* max 128 bytes */
		discordPresence.partySize = currentRichPresenceData.partySize;
		discordPresence.partyMax = currentRichPresenceData.partyMax;
		discordPresence.matchSecret = currentRichPresenceData.matchSecret.c_str();    /* max 128 bytes */
		discordPresence.joinSecret = currentRichPresenceData.joinSecret.c_str();     /* max 128 bytes */
		discordPresence.spectateSecret = currentRichPresenceData.spectateSecret.c_str(); /* max 128 bytes */
		discordPresence.instance = currentRichPresenceData.instance;
		Discord_UpdatePresence(&discordPresence);
	}
	else
	{
		Discord_ClearPresence();
	}

	queuedPresenceUpdate = false;
	lastDiscordPresenceUpdate = realTime;
}

// MARK: - Discord Event Handlers

static void handleDiscordReady(const DiscordUser* connectedUser)
{
	if (!connectedUser) { return; }
	std::string userId = (connectedUser->userId) ? connectedUser->userId : "";
	std::string username = (connectedUser->username) ? connectedUser->username : "";
	std::string discriminator = (connectedUser->discriminator) ? connectedUser->discriminator : "";
	debug(LOG_INFO, "Discord: connected to user %s#%s - %s", username.c_str(), discriminator.c_str(), userId.c_str());
}

static void handleDiscordDisconnected(int errcode, const char* message)
{
	debug(LOG_INFO, "Discord: disconnected (%d: %s)", errcode, message);
}

static void handleDiscordError(int errcode, const char* message)
{
	debug(LOG_ERROR, "Discord: error (%d: %s)", errcode, message);
}

static void handleDiscordJoin(const char* secret)
{
	if (!secret) { return; }
	debug(LOG_INFO, "Discord: join");

	const auto currentGameMode = ActivityManager::instance().getCurrentGameMode();
	if (currentGameMode != ActivitySink::GameMode::MENUS)
	{
		// For now, display an error - can't join a game while already in a game
		// It's a little complicated to properly save/quit the current game automatically from here...
		// (Plus we'd need to wait for a return to the title menus?)
		displayCantJoinWhileInGameNotification();
		return;
	}

	joinGameFromSecret(secret);
}

static void handleDiscordSpectate(const char* secret)
{
	// Not currently supported, because spectators are not supported (yet)
}

static void handleDiscordJoinRequest(const DiscordUser* request)
{
	if (!request) { return; }
	std::string userId = (request->userId) ? request->userId : "";
	std::string username = (request->username) ? request->username : "";
	std::string discriminator = (request->discriminator) ? request->discriminator : "";

	if (userId.empty()) { return; }

	// Check application state and only display if we're actually in hosting lobby mode
	const auto currentGameMode = ActivityManager::instance().getCurrentGameMode();
	if (currentGameMode != ActivitySink::GameMode::HOSTING_IN_LOBBY)
	{
		// If not, just respond with "NO", and log it
		Discord_Respond(request->userId, DISCORD_REPLY_NO);
		debug(LOG_INFO, "Auto-rejecting join request from %s#%s - no longer in lobby", request->username, request->discriminator);
		return;
	}

	std::string notificationTag = std::string(JOIN_NOTIFICATION_TAG_PREFIX "join_request::") + userId;
	if (hasNotificationsWithTag(notificationTag))
	{
		// Already has a notification that is either displayed, or queued for display, for a join request from this user
		// For now, log and ignore this call
		debug(LOG_INFO, "Duplicate join request received for user pending approval: %s#%s - ignoring", request->username, request->discriminator);
		return;
	}

	auto it = lastDismissedJoinRequestByUserId.find(userId);
	if (it != lastDismissedJoinRequestByUserId.end() && std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - it->second).count() < WZ_DISCORD_JOIN_SPAM_INTERVAL_SECS)
	{
		// received another join request from a user we just recently ignored - auto-ignore it
		Discord_Respond(request->userId, DISCORD_REPLY_NO);
		debug(LOG_INFO, "Join spam detected from user that was recently rejected: %s#%s - ignoring", request->username, request->discriminator);
		return;
	}

	// Asynchronously request the user's avatar, and then prompt for permission to join
	asyncGetDiscordUserAvatar(request, [userId, username, discriminator, notificationTag](optional<std::vector<unsigned char>> imageData){
		// dispatch back to main thread
		wzAsyncExecOnMainThread([userId, username, discriminator, notificationTag, imageData](){
			// check again for existing notifications (since this is executed at some indeterminate future point)
			if (hasNotificationsWithTag(notificationTag))
			{
				// Already has a notification that is either displayed, or queued for display, for a join request from this user
				// For now, log and ignore this call
				debug(LOG_INFO, "Duplicate join request received for user pending approval: %s#%s - ignoring", username.c_str(), discriminator.c_str());
				return;
			}

			const std::string discordUserString = username + "#" + discriminator;

			std::string content;
			content = astringf(_("Discord user \"%s\" would like to join your game."), discordUserString.c_str(), userId.c_str());
			content += "\n\n";
			content += _("Allow this player to join your game?");

			WZ_Notification notification;
			notification.duration = 0;
			notification.contentTitle = astringf(_("Join Request from %s"), discordUserString.c_str());
			notification.contentText = content;
			notification.action = WZ_Notification_Action(_("Allow"), [userId](const WZ_Notification&){
				Discord_Respond(userId.c_str(), DISCORD_REPLY_YES);
				lastDismissedJoinRequestByUserId.erase(userId);
			});
			notification.onDismissed = [userId](const WZ_Notification&, WZ_Notification_Dismissal_Reason reason){
				if (reason == WZ_Notification_Dismissal_Reason::ACTION_BUTTON_CLICK) { return; }
				Discord_Respond(userId.c_str(), DISCORD_REPLY_NO);
				// store this to prevent join-spamming
				lastDismissedJoinRequestByUserId[userId] = std::chrono::system_clock::now();
			};
			if (imageData.has_value())
			{
				notification.largeIcon = WZ_Notification_Image(imageData.value());
			}
			notification.tag = notificationTag;
			addNotification(notification, WZ_Notification_Trigger::Immediate());
		});
	});
}

// MARK: - Initializing sub-system

static void discordInit()
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	handlers.ready = handleDiscordReady;
	handlers.disconnected = handleDiscordDisconnected;
	handlers.errored = handleDiscordError;
	handlers.joinGame = handleDiscordJoin;
	handlers.spectateGame = handleDiscordSpectate;
	handlers.joinRequest = handleDiscordJoinRequest;
	Discord_Initialize(APPID, &handlers, 1, nullptr);
}

void discordRPCInitialize()
{
	discordInit();
	if (!discordSink)
	{
		discordSink = std::make_shared<DiscordRPCActivitySink>();
		ActivityManager::instance().addActivitySink(discordSink);
	}
}

static uint32_t framesSinceLastProcessedCallbacks = 0;
#define MIN_FRAMES_UNTIL_CALLBACK 30
static uint32_t lastRPCUpdateTime = 0;
static uint32_t remainingInitialFastChecks = 20;

void discordRPCPerFrame()
{
	if (discordSink)
	{
		discordSink->processQueuedPresenceUpdate();
	}

	if (framesSinceLastProcessedCallbacks < MIN_FRAMES_UNTIL_CALLBACK)
	{
		framesSinceLastProcessedCallbacks++;
		return;
	}
	const auto currentGameMode = ActivityManager::instance().getCurrentGameMode();

	uint32_t minUpdateCheckInterval = (2 * GAME_TICKS_PER_SEC);
	if ((currentGameMode == ActivitySink::GameMode::HOSTING_IN_LOBBY) || (remainingInitialFastChecks > 0))
	{
		// Perform more frequent Discord_RunCallbacks() calls:
		//	- when in the game lobby as host (to surface "ask to join" requests faster)
		//	- at application startup (to handle when Discord launches the game with a game join / invite code)
		minUpdateCheckInterval = (GAME_TICKS_PER_SEC / 2);
	}
	else if (currentGameMode == ActivitySink::GameMode::MENUS)
	{
		minUpdateCheckInterval = GAME_TICKS_PER_SEC;
	}

	if ((realTime - lastRPCUpdateTime) < minUpdateCheckInterval)
	{
		return;
	}

#ifdef DISCORD_DISABLE_IO_THREAD
	Discord_UpdateConnection();
#endif

	Discord_RunCallbacks();
	framesSinceLastProcessedCallbacks = 0;
	lastRPCUpdateTime = realTime;
	if (remainingInitialFastChecks > 0)
	{
		--remainingInitialFastChecks;
	}
}

void discordRPCShutdown()
{
	if (discordSink)
	{
		ActivityManager::instance().removeActivitySink(discordSink);
	}
	Discord_ClearPresence();
	Discord_Shutdown();
	discordSink.reset();
}
