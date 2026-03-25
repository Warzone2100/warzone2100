// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include <memory>
#include <vector>
#include <array>
#include <string>
#include <functional>
#include <unordered_map>
#include <variant>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#include <tl/expected.hpp>

#include "netjoin.h"
#include "lib/framework/wzstring.h"

#include <nlohmann/json_fwd.hpp>

class EcKey;
class MessageWriter;
class WIDGET;

namespace netlobby {

struct NetcodeVer
{
	uint32_t major = 0;
	uint32_t minor = 0;

	bool operator==(const NetcodeVer& other) const
	{
		return major == other.major && minor == other.minor;
	}
};

struct HostDetails
{
	WzString name;
	std::string publicIdentity;
};

struct MapDetails
{
	WzString name;
	std::string hash;
};

struct ModDetails
{
	WzString name;
	WzString hash;
};

struct PlayerCounts
{
	uint32_t current = 0;
	uint32_t max = 0;

public:
	uint32_t availableSlots() const;
};

enum class StructureLimits : uint8_t {
	NO_TANKS   = 1 << 0,
	NO_BORGS   = 1 << 1,
	NO_VTOLS   = 1 << 2
};

struct GameDetails
{
	WzString versionStr;
	NetcodeVer netcodeVer;

	WzString name;
	HostDetails host;
	MapDetails map;
	std::vector<ModDetails> mods;

	PlayerCounts players;
	PlayerCounts spectators;

	uint8_t alliancesMode = 0;
	uint8_t blindMode = 0;

	uint8_t limits = 0;

	bool isPrivate = false;
};

enum class HostJoinOptionValue
{
	BlockAll,
	BlockSuspicious,
	Allow
};

enum class HostJoinOptionType
{
	BotProt,
	ProxyIPs,
	HostingIPs
};

struct HostJoinOptions
{
	HostJoinOptionValue botProt = HostJoinOptionValue::BlockAll;
	HostJoinOptionValue proxyIPs = HostJoinOptionValue::BlockSuspicious;
	HostJoinOptionValue hostingIPs = HostJoinOptionValue::BlockAll;
};

enum class ConnectionMethod
{
	TCP_DIRECT,
	GNS_DIRECT,
};

enum class IPVersion : uint8_t
{
	IPv4 = 1,
	IPv6
};

const char* to_string(IPVersion ip);

struct ConnectionType
{
	ConnectionMethod method;
	optional<IPVersion> ipVersion;
};

struct ConnectionInfo
{
	ConnectionType type;
	std::string host;
	uint16_t port = 0;
};

enum class ConnectionStatus : uint8_t
{
	Submitted = 0,
	Validated,
	Error
};

struct HostConnectionStatusInfo
{
	ConnectionType type;
	ConnectionStatus status;
};

struct HostGameStatus
{
	std::string gameId;
	bool published;
	std::vector<HostConnectionStatusInfo> connectionStatusInfo;
};

struct JoinRequest
{
	std::string joinToken;
	std::string ip;
	std::string playerName;
	std::string playerPublicIdentity;
	ConnectionMethod connectionMethod;
	bool asSpectator;
};

struct CheckJoinRequestResult
{
	bool pass = false;
	std::string error;
};

struct LobbyErrorResolutionData
{
protected:
	LobbyErrorResolutionData();
public:
	virtual ~LobbyErrorResolutionData();
};

typedef std::function<void(std::shared_ptr<LobbyErrorResolutionData> result)> LobbyErrorResolveResultHandler;

struct LobbyError
{
public:
	LobbyError()
	{ }

	LobbyError(const std::string& errCode, const std::string& errMessage)
	: errCode(errCode)
	, errMessage(errMessage)
	{ }

public:
	std::string errCode;
	std::string errMessage;

	bool isResolvableLobbyError() const;

private:
	friend void from_json(const nlohmann::json& j, LobbyError& v);
	friend std::shared_ptr<WIDGET> GetLobbyErrorResolveWidget(const LobbyError& lobbyError, const LobbyErrorResolveResultHandler& resultHandler);

	// In specific circumstances, additional data may be returned as well
	std::shared_ptr<nlohmann::json> resolvableLobbyErrorJsonData;
};

optional<WzString> GetLocalizedLobbyErrorMessage(const LobbyError& lobbyError);

std::shared_ptr<WIDGET> GetLobbyErrorResolveWidget(const LobbyError& lobbyError, const LobbyErrorResolveResultHandler& resultHandler);

class LobbyServerHostingHandlerProtocol : public std::enable_shared_from_this<LobbyServerHostingHandlerProtocol>
{
public:
	struct EventCallbacks
	{
		// May be called at any point once the LobbyServerHostingHandler detects that
		// the lobby created a game listing
		std::function<void (std::string gameId)> lobbyGameCreatedCallback;

		// May be called at any point once the LobbyServerHostingHandler detects that
		// the lobby has published the game listing
		// Used for:
		// - Messaging the UI / sending wzcmd events that the game is actually listed in the lobby
		// - Signaling to the Discord rich activity provider to generate a game token for joining the game via the lobby
		std::function<void (std::string gameId, std::string motd)> lobbyGamePublishedCallback;

		// May be called at any point once the LobbyServerHostingHandler detects that
		// the lobby dropped (or rejected) the game listing
		std::function<void (optional<LobbyError> error)> lobbyGameGoneCallback;
	};
protected:
	LobbyServerHostingHandlerProtocol(EventCallbacks callbacks);
public:
	virtual ~LobbyServerHostingHandlerProtocol();
public:
	// Requests a game listing
	virtual bool createGameListing(const EcKey& hostIdentity, const GameDetails& gameDetails, const HostJoinOptions& joinOptions, const std::vector<ConnectionInfo>& connections) = 0;

	// Writes a response to a lobby connection check request
	enum class LobbyConnectionCheckRequestError
	{
		GameNotListed,
		InvalidRequest,
	};
	typedef ::tl::expected<std::string, LobbyConnectionCheckRequestError> LobbyConnectionCheckResult;
	virtual LobbyConnectionCheckResult respondToLobbyConnectionCheckRequest(const std::string& jsonRequest, const WzString& playerName, const EcKey& playerIdentity) = 0;

	// Queues a game listing update
	virtual void updateGameListing(const EcKey& hostIdentity, const GameDetails& gameDetails, const HostJoinOptions& joinOptions) = 0;

	typedef ::tl::expected<CheckJoinRequestResult, LobbyError> CheckJoinResult;
	typedef std::function<void(CheckJoinResult)> CheckJoinRequestResultHandler;

	// Checks a lobby-originated join request
	virtual bool checkJoinRequest(const JoinRequest& joinRequest, CheckJoinRequestResultHandler resultHandler) = 0;

	// Removes the game listing from the lobby
	virtual void removeGameListing(bool matchStarted) = 0;

	// Handles sending queued updates as well as keep-alives - should be called regularly by the main loop (at least, while in lobby)
	virtual void run() = 0;

	// Fetching info
	virtual bool isConnectedToLobby() const = 0;
	virtual optional<std::string> getCurrentGameId() const = 0;
	virtual optional<bool> isHostJoinOptionSupportedByLobby(HostJoinOptionType optType) const = 0;
protected:
	EventCallbacks callbacks;
};

std::shared_ptr<LobbyServerHostingHandlerProtocol> MakeLobbyHostListingHandler(const std::string& lobbyAddress, LobbyServerHostingHandlerProtocol::EventCallbacks callbacks);

// MARK: - Enumerating game listings

struct GameListing
{
	std::string gameId;
	GameDetails details;
	std::vector<ConnectionType> availableConnectionTypes;
};

struct ListResponse
{
	std::vector<GameListing> games;
	std::string lobbyMOTD;
};

typedef ::tl::expected<ListResponse, LobbyError> ListResult;
typedef std::function<void(ListResult)> CompletionHandlerFunc;

bool EnumerateGames(const std::string& lobbyAddress, CompletionHandlerFunc completionHandlerFunc, optional<size_t> resultsLimit = nullopt, optional<NetcodeVer> matchingVersion = nullopt);

// MARK: - Requesting join details

struct GameJoinDetails
{
	std::vector<ConnectionInfo> connections;
	std::vector<std::string> joinTokens;
};

struct RequestJoinDetailsResults
{
	GameJoinDetails joinDetails;

	enum class ConnectivityFailureType
	{
		ERR_COULD_NOT_CONNECT,
		ERR_TIMEOUT,
		ERR_OTHER
	};
	struct ConnectivityFailure
	{
		ConnectivityFailureType type;
		std::string details;
	};

	typedef std::variant<ConnectivityFailure, LobbyError> ErrorVariant;
	std::unordered_map<IPVersion, ErrorVariant> errors;
};

typedef ::tl::expected<RequestJoinDetailsResults, LobbyError> JoinResult;
typedef std::function<void(JoinResult)> RequestJoinResultHandlerFunc;

// The resultHandler is called on the main thread
bool RequestJoinDetails(const std::string& lobbyAddress, const std::string& lobbyGameId, const WzString& playerName, const EcKey& playerIdentity, bool asSpectator, RequestJoinResultHandlerFunc resultHandler, optional<IPVersion> optIpVersion = nullopt, std::shared_ptr<LobbyErrorResolutionData> errorResolution = nullptr);

// MARK: Defaults

namespace detail {

static constexpr char scr_key = 0x5A;

#if 0
// Uncomment to encode a URL at compile-time, to get the transformed bytes.
// Just change temp_raw_url and call tmpDebugPrintTransformedData() somewhere.
//
// Don't forget to set this back to #if 0, and remove the plaintext url from
// temp_raw_url!

#include <utility> // for std::index_sequence

// Unpacks the string_view into the array via indices
template <std::size_t... Is>
constexpr auto encode_impl(std::string_view sv, std::index_sequence<Is...>)
{
	return std::array<char, sizeof...(Is)>{ (static_cast<char>(sv[Is] ^ scr_key))... };
}

// Takes a string_view and returns a constexpr std::array
template <std::size_t N>
constexpr auto encode(std::string_view sv)
{
	return encode_impl(sv, std::make_index_sequence<N>{});
}

inline void tmpDebugPrintTransformedData()
{
	static constexpr std::string_view temp_raw_url = "<set this>"; // Set this as desired
	static constexpr auto hidden_data = encode<temp_raw_url.size()>(temp_raw_url);

	std::printf("Copy these bytes to the encoded array (and set its length to: %zu):\n", hidden_data.size());
	for (auto c : hidden_data)
	{
		std::printf("0x%02x, ", (unsigned char)c);
	}
	std::printf("\n");
}

#endif

template <std::size_t N>
constexpr std::array<char, N> scr_transform(const std::array<char, N>& input)
{
	std::array<char, N> out{};
	for (std::size_t i = 0; i < N; ++i) out[i] = input[i] ^ scr_key;
	return out;
}

using lobby_address_encoded_t = std::array<char, 32>;
static constexpr lobby_address_encoded_t lobbyAddressEncoded = {
	0x32, 0x2e, 0x2e, 0x2a, 0x29, 0x60, 0x75, 0x75, 0x2d, 0x20, 0x36, 0x35, 0x38, 0x38, 0x23, 0x74, 0x2d, 0x20, 0x68, 0x6b, 0x6a, 0x6a, 0x74, 0x34, 0x3f, 0x2e, 0x75, 0x36, 0x35, 0x38, 0x38, 0x23
};

// A template struct to hold the "static" decoded data
template <const lobby_address_encoded_t& Raw>
struct UrlStorage {
	static constexpr auto decoded = scr_transform(Raw);
};

} // namespace detail

// NOTE:
// To try to limit basic scraper bots just grabbing a plaintext URL from the source (and then adding it to web scraper bot lists, sigh),
// the default lobby address is encoded with a simple constexpr transform function.

constexpr std::string_view GetDefaultLobbyAddress()
{
	return { detail::UrlStorage<detail::lobbyAddressEncoded>::decoded.data(), detail::UrlStorage<detail::lobbyAddressEncoded>::decoded.size() };
}

} // namespace netlobby
