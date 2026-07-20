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

#include "netlobby.h"

#include "lib/framework/frame.h" // for `ASSERT`
#include "lib/framework/wzapp.h"
#include "lib/framework/wzstring_json.h"
#include "lib/framework/crc.h"
#include "src/urlrequest.h"
#include "src/version.h"

#include "lib/ivis_opengl/png_util.h"
#include "lib/ivis_opengl/jpeg_util.h"
#include "lib/widget/verifyprompt.h"

#include "lib/netplay/netplay.h"

#include <string_view>
#include <atomic>
#include <re2/re2.h>

#include <nlohmann/json.hpp>

namespace netlobby {

constexpr std::chrono::seconds SERVER_UPDATE_PENDING_PUBLISH_CHECKIN_MIN_INTERVAL = std::chrono::seconds(1);
constexpr std::chrono::seconds SERVER_UPDATE_PENDING_PUBLISH_CHECKIN_MAX_INTERVAL = std::chrono::seconds(5);
constexpr std::chrono::seconds SERVER_UPDATE_MIN_INTERVAL = std::chrono::seconds(8);
constexpr std::chrono::seconds SERVER_UPDATE_MAX_INTERVAL = std::chrono::seconds(30);
constexpr std::chrono::seconds SERVER_UPDATE_NO_SUCCESS_ABANDON_INTERVAL = std::chrono::seconds(120);

constexpr const char* errCodeHostNotAccessible = "HOST_NOT_ACCESSIBLE";
constexpr const char* errCodeHostDisallowedJoinSource = "HOST_DISALLOWED_JOIN_SOURCE";
constexpr const char* errCodeGameNotFound = "GAME_NOT_FOUND";

// MARK: -

LobbyServerHostingHandlerProtocol::LobbyServerHostingHandlerProtocol(LobbyServerHostingHandlerProtocol::EventCallbacks callbacks)
: callbacks(callbacks)
{ }

LobbyServerHostingHandlerProtocol::~LobbyServerHostingHandlerProtocol()
{ }

LobbyErrorResolutionData::LobbyErrorResolutionData()
{ }

LobbyErrorResolutionData::~LobbyErrorResolutionData()
{ }

// MARK: -

uint32_t PlayerCounts::availableSlots() const
{
	if (max < current) { return 0; }
	return max - current;
}

struct HostGameContext
{
	// The lobby game ID
	std::string gameId;

	// A token the host provides to authenticate host actions with the lobby
	std::string hostToken;

	// A token the *lobby* provides to the host when reaching out to attempt & verify connection(s)
	std::string lobbyConnectToken;

public:
	bool isValid() const;
};

bool HostGameContext::isValid() const
{
	return !gameId.empty();
}

// MARK: - Url parsing helpers

struct UrlParts
{
	std::optional<std::string> scheme;
	std::string hostname;
};

UrlParts parse_url(const std::string& url)
{
	// Regex breakdown:
	// ^(?:([^:/?#]+)://)?   -> Group 1: Optional scheme (captured)
	// (?:[^/?#@]+@)?        -> Optional userinfo (username:password@) - NOT captured
	// ([^/?#:]+)            -> Group 2: Hostname (stops at /, ?, #, or : for port)
	static const re2::RE2 url_regex(R"(^(?:([^:/?#]+)://)?(?:[^/?#@]+@)?([^/?#:]+))");

	std::string scheme_str;
	std::string host_str;

	// Because we use the ^ anchor, PartialMatch effectively behaves like a match from the start
	if (re2::RE2::PartialMatch(url, url_regex, &scheme_str, &host_str))
	{
		UrlParts parts;
		if (!scheme_str.empty()) // re2 leaves the string empty if the optional group didn't match
		{
			parts.scheme = scheme_str;
		}
		parts.hostname = host_str;
		return parts;
	}
	return {};
}

optional<std::string> url_string_get_protocol(const std::string& url)
{
	size_t pos = url.find("://"); // Find the position of "://"

	if (pos != std::string::npos) {
		// Extract the part before the delimiter (e.g., "https")
		return url.substr(0, pos);
	} else {
		// delimiter is not found
		return nullopt;
	}
}

static std::string buildLobbyRequestBaseUrl(const std::string& lobbyAddress)
{
	auto protocol = url_string_get_protocol(lobbyAddress);
	if (!protocol.has_value())
	{
		// Default to https if no protocol is specified
		return "https://" + lobbyAddress;
	}
	return lobbyAddress;
}

// Check if the lobbyAddress is trusted / known, with a trusted / known privacy policy (and secure transport)
// Used in various places to simplify matters for alternative lobbies, and to minimize data sent to unknown or insecure servers (with unknown privacy policies)
static inline bool isTrustedLobbyAddress(const std::string& lobbyAddress)
{
	auto parts = parse_url(lobbyAddress);
	if (parts.hostname == "wzlobby.wz2100.net" && parts.scheme.value_or("https") == "https")
	{
		return true;
	}
	return false;
}

// MARK: - LobbyError

void from_json(const nlohmann::json& j, LobbyError& v)
{
	v.errCode = j.at("code").get<std::string>();

	v.errMessage.clear();
	auto it = j.find("message");
	if (it != j.end())
	{
		v.errMessage = it.value().get<std::string>();
	}

	it = j.find("data");
	if (it != j.end())
	{
		v.resolvableLobbyErrorJsonData = std::make_shared<nlohmann::json>(it.value());
	}
}

void to_json(nlohmann::json& j, const LobbyError& v)
{
	j = nlohmann::json::object();
	j["code"] = v.errCode;
	j["message"] = v.errMessage;
}

optional<WzString> GetLocalizedLobbyErrorMessage(const LobbyError& lobbyError)
{
	if (lobbyError.errCode == errCodeHostDisallowedJoinSource)
	{
		WzString retVal = _("This host has disallowed joins from certain hosting providers, VPNs, and/or proxies.");
		retVal += "\n";
		retVal += _("Please try another room or disable any proxies / VPNs and try again.");
		return retVal;
	}
	if (lobbyError.errCode == errCodeGameNotFound)
	{
		WzString retVal = _("Game not found in lobby.");
		retVal += "\n";
		retVal += _("The game may have already started, or the host may have disbanded the game lobby.");
		return retVal;
	}

	return nullopt;
}

bool LobbyError::isResolvableLobbyError() const
{
	if (errCode == "CAPTCHA_REQUIRED")
	{
		return true;
	}
	return false;
}

struct LobbyErrorResolutionJsonData : public LobbyErrorResolutionData
{
public:
	LobbyErrorResolutionJsonData(const nlohmann::json& jsonData)
	: jsonData(jsonData)
	{ }

public:
	nlohmann::json jsonData;
};

// MARK: ImageType

enum class ImageType
{
	PNG,
	JPEG
};

void from_json(const nlohmann::json& j, ImageType& v)
{
	auto strValue = j.get<std::string>();
	if (strValue == "png")
	{
		v = ImageType::PNG;
		return;
	}
	else if  (strValue == "jpg" || strValue == "jpeg")
	{
		v = ImageType::JPEG;
		return;
	}
	else
	{
		throw nlohmann::json::type_error::create(302, "unsupported value for ImageType field: " + strValue, &j);
	}
}

// MARK: B64ImageData

class B64ImageData : public WzVerifyPrompt::IvImageProviderInterface
{
public:
	optional<iV_Image> GetImage() const;
public:
	std::string b64Data;
	ImageType type = ImageType::PNG;
};

optional<iV_Image> B64ImageData::GetImage() const
{
	auto decodedB64Data = base64Decode(b64Data);
	iV_Image ivImage;

	switch (type)
	{
		case ImageType::PNG:
		{
			auto result = iV_loadImage_PNG(decodedB64Data, &ivImage);
			if (!result.noError())
			{
				debug(LOG_ERROR, "Failed to load image from memory buffer: %s", result.text.c_str());
				return nullopt;
			}
			break;
		}
		case ImageType::JPEG:
		{
			auto result = iV_loadImage_JPEG(std::move(decodedB64Data), &ivImage);
			if (!result.noError())
			{
				debug(LOG_ERROR, "Failed to load image from memory buffer: %s", result.text.c_str());
				return nullopt;
			}
			break;
		}
	}

	return ivImage;
}

void from_json(const nlohmann::json& j, B64ImageData& v)
{
	v.b64Data = j.at("data").get<std::string>();
	v.type = j.at("type").get<ImageType>();
}

// MARK: CaptchaVerifyPrompt

struct CaptchaVerifyPrompt
{
	std::string token;
	WzVerifyPrompt::CaptchaVerifyPromptData data;
};

void from_json(const nlohmann::json& j, CaptchaVerifyPrompt& v)
{
	v.token = j.at("token").get<std::string>();

	const auto& data = j.at("data");
	v.data.image = std::make_shared<B64ImageData>(data.at("image").get<B64ImageData>());
}

struct CaptchaVerifyResponse
{
	std::string token;
	WzString result;
};

void to_json(nlohmann::json& j, const CaptchaVerifyResponse& v)
{
	j = nlohmann::json::object();
	j["token"] = v.token;

	auto resp = nlohmann::json::object();
	resp["result"] = v.result;

	j["response"] = std::move(resp);
}

// MARK: SlideVerifyPrompt

struct SlideVerifyPrompt
{
	std::string token;
	WzVerifyPrompt::SlideVerifyPromptData data;
};

void from_json(const nlohmann::json& j, SlideVerifyPrompt& v)
{
	v.token = j.at("token").get<std::string>();

	const auto& data = j.at("data");
	v.data.image = std::make_shared<B64ImageData>(data.at("image").get<B64ImageData>());
	v.data.tile = std::make_shared<B64ImageData>(data.at("tile").get<B64ImageData>());
	v.data.tile_width = data.at("tile_width").get<uint32_t>();
	v.data.tile_height = data.at("tile_height").get<uint32_t>();
	v.data.tile_x = data.at("tile_x").get<uint32_t>();
	v.data.tile_y = data.at("tile_y").get<uint32_t>();
}

struct SlideVerifyResponse
{
	std::string token;
	uint32_t x;
	uint32_t y;
};

void to_json(nlohmann::json& j, const SlideVerifyResponse& v)
{
	j = nlohmann::json::object();
	j["token"] = v.token;

	auto resp = nlohmann::json::object();
	resp["x"] = v.x;
	resp["y"] = v.y;

	j["response"] = std::move(resp);
}

// MARK: GetLobbyErrorResolveWidget

static std::shared_ptr<WIDGET> constructChallengeResolveWidgetFromErrorJson(const nlohmann::json& j, const LobbyErrorResolveResultHandler& resultHandler)
{
	ASSERT_OR_RETURN(nullptr, resultHandler != nullptr, "Null resultHandler");

	try {
		auto it = j.find("type");
		if (it == j.end() || !it.value().is_string())
		{
			debug(LOG_LOBBY, "Missing type field");
			return nullptr;
		}

		auto typeStr = it.value().get<std::string>();
		if (typeStr == "captcha")
		{
			auto verifyPrompt = j.get<CaptchaVerifyPrompt>();
			return WzVerifyPrompt::makeCaptchaWidget(std::move(verifyPrompt.data), [resultHandler, token = verifyPrompt.token](WzString result) {
				CaptchaVerifyResponse resp;
				resp.token = token;
				resp.result = result;
				resultHandler(std::make_shared<LobbyErrorResolutionJsonData>(nlohmann::json(resp)));
			});
		}
		else if ((typeStr == "slide") || (typeStr == "dragdrop"))
		{
			auto verifyPrompt = j.get<SlideVerifyPrompt>();
			return WzVerifyPrompt::makeSlideWidget(std::move(verifyPrompt.data), (typeStr == "dragdrop"), [resultHandler, token = verifyPrompt.token](uint32_t pointX, uint32_t pointY) {
				SlideVerifyResponse resp;
				resp.token = token;
				resp.x = pointX;
				resp.y = pointY;
				resultHandler(std::make_shared<LobbyErrorResolutionJsonData>(nlohmann::json(resp)));
			});
		}
		else
		{
			// unknown type
			debug(LOG_LOBBY, "Unknown type field value: %s", typeStr.c_str());
			return nullptr;
		}
	}
	catch (const std::exception &e) {
		debug(LOG_LOBBY, "Failed parsing json: %s", e.what());
		return nullptr;
	}

	return nullptr;
}

std::shared_ptr<WIDGET> GetLobbyErrorResolveWidget(const LobbyError& lobbyError, const LobbyErrorResolveResultHandler& resultHandler)
{
	// Certain special LobbyErrors might be resolvable by the user

	if (lobbyError.errCode == "CAPTCHA_REQUIRED")
	{
		if (!lobbyError.resolvableLobbyErrorJsonData)
		{
			return nullptr;
		}

		return constructChallengeResolveWidgetFromErrorJson(*lobbyError.resolvableLobbyErrorJsonData.get(), resultHandler);
	}

	return nullptr;
}

// MARK: - JSON conversion

// MARK: Helpers

static std::vector<uint8_t> vecFromWzString(const WzString& str)
{
	return std::vector<uint8_t>(str.toUtf8().begin(), str.toUtf8().end());
}

template<typename T, typename BasicJsonContext, std::enable_if_t<nlohmann::detail::is_basic_json_context<BasicJsonContext>::value, int> = 0>
T get_opt_val_or_json_throw(const optional<T>& optVal, std::string_view valueName, const BasicJsonContext& context)
{
	if (!optVal.has_value())
	{
		throw nlohmann::json::type_error::create(302, "invalid value for " + std::string(valueName) + ": " + std::string(context.type_name()), &context);
	}
	return optVal.value();
}

// MARK: NetcodeVer
// NetcodeVer has special compressed JSON format of:
// [major,minor]

void to_json(nlohmann::ordered_json& j, const NetcodeVer& v)
{
	j = nlohmann::ordered_json::array({v.major, v.minor});
}

void from_json(const nlohmann::json& j, NetcodeVer& v)
{
	v.major = j.at(0).get<uint32_t>();
	v.minor = j.at(1).get<uint32_t>();
}

// MARK: HostDetails

void to_json(nlohmann::ordered_json& j, const HostDetails& v)
{
	j = nlohmann::ordered_json::object();
	j["name"] = v.name;
	j["pk"] = v.publicIdentity;
}

void from_json(const nlohmann::json& j, HostDetails& v)
{
	v.name = j.at("name").get<WzString>();
	v.publicIdentity = j.at("pk").get<std::string>();
}

// MARK: MapDetails
// MapDetails has special compressed JSON format of:
// ["name","hash"]

void to_json(nlohmann::ordered_json& j, const MapDetails& v)
{
	j = nlohmann::ordered_json::array({v.name, v.hash});
}

void from_json(const nlohmann::json& j, MapDetails& v)
{
	v.name = j.at(0).get<WzString>();
	v.hash = j.at(1).get<std::string>();
}

// MARK: ModDetails

void to_json(nlohmann::ordered_json& j, const ModDetails& v)
{
	j = nlohmann::ordered_json::object();
	j["name"] = v.name;
	j["hash"] = v.hash;
}

void from_json(const nlohmann::json& j, ModDetails& v)
{
	v.name = j.at("name").get<WzString>();
	v.hash = j.at("hash").get<WzString>();
}

// MARK: PlayerCounts

void to_json(nlohmann::ordered_json& j, const PlayerCounts& v)
{
	j = nlohmann::ordered_json::object();
	j["cur"] = v.current;
	j["max"] = v.max;
}

void from_json(const nlohmann::json& j, PlayerCounts& v)
{
	v.max = j.at("max").get<uint32_t>();

	v.current = 0;
	auto it = j.find("cur");
	if (it != j.end())
	{
		v.current = it.value().get<uint32_t>();
	}
}

// MARK: GameDetails

void to_json(nlohmann::ordered_json& j, const GameDetails& v)
{
	j = nlohmann::ordered_json::object();

	j["v"] = v.versionStr;
	j["netv"] = v.netcodeVer;

	j["name"] = v.name;
	j["host"] = v.host;
	j["map"] = v.map;
	if (!v.mods.empty())
	{
		j["mods"] = v.mods;
	}
	j["players"] = v.players;
	j["specs"] = v.spectators;
	j["ally"] = v.alliancesMode;
	if (v.blindMode != 0)
	{
		j["blind"] = v.blindMode;
	}
	j["lim"] = v.limits;
	if (v.isPrivate)
	{
		j["private"] = true;
	}
}

static nlohmann::ordered_json serializeGameDetailsChangedFields(const GameDetails& newGameDetails, const GameDetails& lastPersistedGameDetails)
{
	// Start by converting to json
	auto gameDetailsJson = nlohmann::ordered_json(newGameDetails);

	// Then check individual fields and erase them if they are the same as lastPersistedGameDetails
	if (newGameDetails.versionStr == lastPersistedGameDetails.versionStr)
	{
		gameDetailsJson.erase("v");
	}
	if (newGameDetails.netcodeVer == lastPersistedGameDetails.netcodeVer)
	{
		gameDetailsJson.erase("netv");
	}
	if (newGameDetails.name == lastPersistedGameDetails.name)
	{
		gameDetailsJson.erase("name");
	}
	if (newGameDetails.host == lastPersistedGameDetails.host)
	{
		gameDetailsJson.erase("host");
	}
	if (newGameDetails.map == lastPersistedGameDetails.map)
	{
		gameDetailsJson.erase("map");
	}
	if (newGameDetails.mods == lastPersistedGameDetails.mods)
	{
		gameDetailsJson.erase("mods");
	}
	if (newGameDetails.players == lastPersistedGameDetails.players)
	{
		gameDetailsJson.erase("players");
	}
	if (newGameDetails.spectators == lastPersistedGameDetails.spectators)
	{
		gameDetailsJson.erase("specs");
	}
	if (newGameDetails.alliancesMode == lastPersistedGameDetails.alliancesMode)
	{
		gameDetailsJson.erase("ally");
	}
	if (newGameDetails.blindMode == lastPersistedGameDetails.blindMode)
	{
		gameDetailsJson.erase("blind");
	}
	if (newGameDetails.limits == lastPersistedGameDetails.limits)
	{
		gameDetailsJson.erase("lim");
	}
	if (newGameDetails.isPrivate == lastPersistedGameDetails.isPrivate)
	{
		gameDetailsJson.erase("private");
	}

	return gameDetailsJson;
}

void from_json(const nlohmann::json& j, GameDetails& v)
{
	v.versionStr = j.at("v").get<WzString>();
	v.netcodeVer = j.at("netv").get<NetcodeVer>();

	v.name = j.at("name").get<WzString>();
	v.host = j.at("host").get<HostDetails>();
	v.map = j.at("map").get<MapDetails>();

	v.mods.clear();
	auto it = j.find("mods");
	if (it != j.end())
	{
		v.mods = it.value().get<std::vector<ModDetails>>();
	}

	v.players = j.at("players").get<PlayerCounts>();

	v.spectators = PlayerCounts();
	it = j.find("specs");
	if (it != j.end())
	{
		v.spectators = it.value().get<PlayerCounts>();
	}

	v.alliancesMode = 0;
	it = j.find("ally");
	if (it != j.end())
	{
		v.alliancesMode = it.value().get<uint8_t>();
	}

	v.blindMode = 0;
	it = j.find("blind");
	if (it != j.end())
	{
		v.blindMode = it.value().get<uint8_t>();
	}

	v.limits = 0;
	it = j.find("lim");
	if (it != j.end())
	{
		v.limits = it.value().get<uint8_t>();
	}

	v.isPrivate = false;
	it = j.find("private");
	if (it != j.end())
	{
		v.isPrivate = it.value().get<bool>();
	}
}

// MARK: HostJoinOptionValue

const char* to_string(HostJoinOptionValue m)
{
	switch (m)
	{
	case HostJoinOptionValue::BlockAll:
		return "block";
	case HostJoinOptionValue::BlockSuspicious:
		return "blockSus";
	case HostJoinOptionValue::Allow:
		return "allow";
	}
	return ""; // silence compiler warning
}

optional<HostJoinOptionValue> host_join_option_value_from_string(std::string_view s)
{
	if (s == "block")
	{
		return HostJoinOptionValue::BlockAll;
	}
	else if (s == "blockSus")
	{
		return HostJoinOptionValue::BlockSuspicious;
	}
	else if (s == "allow")
	{
		return HostJoinOptionValue::Allow;
	}
	return nullopt;
}

void to_json(nlohmann::ordered_json& j, const HostJoinOptionValue& v)
{
	j = nlohmann::ordered_json(to_string(v));
}

void from_json(const nlohmann::json& j, HostJoinOptionValue& v)
{
	v = get_opt_val_or_json_throw(host_join_option_value_from_string(j.get<std::string>()), "HostJoinOptionValue", j);
}

// MARK: HostJoinOptions

void to_json(nlohmann::ordered_json& j, const HostJoinOptions& v)
{
	j = nlohmann::ordered_json::object();
	j["botProt"] = v.botProt;
	j["proxyIps"] = v.proxyIPs;
	j["hostingIps"] = v.hostingIPs;
}

void from_json(const nlohmann::json& j, HostJoinOptions& v)
{
	v.botProt = j.at("botProt").get<HostJoinOptionValue>();
	v.proxyIPs = j.at("proxyIps").get<HostJoinOptionValue>();
	v.hostingIPs = j.at("hostingIps").get<HostJoinOptionValue>();
}

// MARK: HostGameContext

// Deliberately unimplemented - should never be sending the HostGameContext anywhere directly
//void to_json(nlohmann::ordered_json& j, const HostGameContext& v)
//{
//	// Deliberately unimplemented - should never be sending the HostGameContext anywhere directly
//}

void from_json(const nlohmann::json& j, HostGameContext& v)
{
	v.gameId = j.at("gameId").get<std::string>();
	v.hostToken = j.at("hostToken").get<std::string>();
	v.lobbyConnectToken = j.at("lobbyToken").get<std::string>();
}

// MARK: HostJoinOptionType

optional<HostJoinOptionType> host_join_option_type_from_string(std::string_view s)
{
	if (s == "botProt")
	{
		return HostJoinOptionType::BotProt;
	}
	else if (s == "proxyIps")
	{
		return HostJoinOptionType::ProxyIPs;
	}
	else if (s == "hostingIps")
	{
		return HostJoinOptionType::HostingIPs;
	}
	return nullopt;
}

// MARK: WrappedCreateGameResponse

struct WrappedCreateGameResponse
{
	HostGameContext ctx;
	std::string motd;
	std::unordered_set<HostJoinOptionType> supportedHostJoinOpts;
};

void from_json(const nlohmann::json& j, WrappedCreateGameResponse& v)
{
	v.ctx = j.get<HostGameContext>();

	auto it = j.find("motd");
	if (it != j.end())
	{
		v.motd = it.value().get<std::string>();
	}

	v.supportedHostJoinOpts.clear();
	it = j.find("hostJoinOpts");
	if (it != j.end() && it.value().is_array())
	{
		// Grab any values we know, ignore any we don't
		for (const auto& opt : it.value())
		{
			try {
				auto strVal = opt.get<std::string>();
				auto optType = host_join_option_type_from_string(strVal);
				if (optType.has_value())
				{
					v.supportedHostJoinOpts.insert(optType.value());
				}
			}
			catch (const std::exception&) {
				// silently ignore issues converting
			}
		}
	}
}

// MARK: ConnectionMethod

const char* to_string(ConnectionMethod m)
{
	switch (m)
	{
	case ConnectionMethod::TCP_DIRECT:
		return "tcp";
	case ConnectionMethod::GNS_DIRECT:
		return "gns";
	}
	return "";
}

optional<ConnectionMethod> connection_method_from_string(std::string_view s)
{
	if (s == "tcp")
	{
		return ConnectionMethod::TCP_DIRECT;
	}
	else if (s == "gns")
	{
		return ConnectionMethod::GNS_DIRECT;
	}
	return nullopt;
}

void to_json(nlohmann::ordered_json& j, const ConnectionMethod& v)
{
	j = nlohmann::ordered_json(to_string(v));
}

void from_json(const nlohmann::json& j, ConnectionMethod& v)
{
	v = get_opt_val_or_json_throw(connection_method_from_string(j.get<std::string>()), "ConnectionMethod", j);
}

// MARK: IPVersion

const char* to_string(IPVersion m)
{
	switch (m)
	{
	case IPVersion::IPv4:
		return "ip4";
	case IPVersion::IPv6:
		return "ip6";
	}
	return "";
}

optional<IPVersion> ip_version_from_string(std::string_view s)
{
	if (s == "ip4")
	{
		return IPVersion::IPv4;
	}
	else if (s == "ip6")
	{
		return IPVersion::IPv6;
	}
	return nullopt;
}

// MARK: ConnectionType

void from_json(const nlohmann::json& j, ConnectionType& v)
{
	auto strVal = j.get<std::string>();

	auto delimPos = strVal.find_first_of(':');
	if (delimPos == std::string::npos)
	{
		// assume it's just a ConnectionMethod string, with IPVersion unspecified
		v.ipVersion = nullopt;
		v.method = j.get<ConnectionMethod>();
		return;
	}

	// appears to be a <ConnectionMethod>:<IPVersion> string
	std::string_view sv(strVal);

	// Extract the left part (before the colon)
	std::string_view left = sv.substr(0, delimPos);
	// Extract the right part (after the colon)
	std::string_view right = sv.substr(delimPos + 1);

	v.method = get_opt_val_or_json_throw(connection_method_from_string(left), "ConnectionMethod", j);
	v.ipVersion = ip_version_from_string(right);
}

void to_json(nlohmann::ordered_json& j, const ConnectionType& v)
{
	std::string str = to_string(v.method);

	if (v.ipVersion.has_value())
	{
		str += ":";
		str += to_string(v.ipVersion.value());
	}

	j = nlohmann::ordered_json(str);
}

// MARK: ConnectionInfo

void to_json(nlohmann::ordered_json& j, const ConnectionInfo& v)
{
	j = nlohmann::ordered_json::object();
	j["type"] = v.type;
	j["host"] = v.host;
	j["port"] = v.port;
}

void from_json(const nlohmann::json& j, ConnectionInfo& v)
{
	v.type = j.at("type").get<ConnectionType>();
	v.host = j.at("host").get<std::string>();
	v.port = j.at("port").get<uint16_t>();
}

// MARK: ConnectionStatus

optional<ConnectionStatus> connection_status_from_string(std::string_view s)
{
	if (s == "submitted")
	{
		return ConnectionStatus::Submitted;
	}
	else if (s == "validated")
	{
		return ConnectionStatus::Validated;
	}
	else if (s == "error")
	{
		return ConnectionStatus::Error;
	}
	return nullopt;
}

void from_json(const nlohmann::json& j, ConnectionStatus& v)
{
	v = get_opt_val_or_json_throw(connection_status_from_string(j.get<std::string>()), "ConnectionStatus", j);
}

// MARK: HostConnectionStatusInfo

void from_json(const nlohmann::json& j, HostConnectionStatusInfo& v)
{
	v.type = j.at("type").get<ConnectionType>();
	v.status = j.at("status").get<ConnectionStatus>();
}

// MARK: HostGameStatus

void from_json(const nlohmann::json& j, HostGameStatus& v)
{
	v.gameId = j.at("gameId").get<std::string>();
	v.published = j.at("published").get<bool>();
	v.connectionStatusInfo = j.at("connections").get<std::vector<HostConnectionStatusInfo>>();
}

// MARK: JoinRequest

void to_json(nlohmann::ordered_json& j, const JoinRequest& v)
{
	j = nlohmann::ordered_json::object();
	j["joinToken"] = v.joinToken;
	j["ip"] = v.ip;
	j["name"] = v.playerName;
	j["pk"] = v.playerPublicIdentity;
	j["connMethod"] = v.connectionMethod;
	j["asSpec"] = v.asSpectator;
}


// MARK: CheckJoinRequestResult

void from_json(const nlohmann::json& j, CheckJoinRequestResult& v)
{
	v.pass = j.at("pass").get<bool>();

	v.error.clear();
	auto it = j.find("error");
	if (it != j.end())
	{
		v.error = it.value().get<std::string>();
	}
}

// MARK: RequestAttestation

struct RequestAttestation
{
public:
	RequestAttestation(const WzString& playerName, const EcKey& playerIdentity, const std::string& lobbyAddress, optional<std::string> gameId = nullopt);
public:
	std::string playerNameB64;
	std::string playerPublicIdentity;
	std::string nonceB64;
	std::string secondsSinceEpoch;
	std::string lobbyAddress;
	optional<std::string> gameId;
	std::string signature;
};

RequestAttestation::RequestAttestation(const WzString& playerName, const EcKey& playerIdentity, const std::string& lobbyAddress, optional<std::string> gameId /*= nullopt*/)
: lobbyAddress(lobbyAddress)
, gameId(gameId)
{
	playerNameB64 = base64Encode(vecFromWzString(playerName));
	playerPublicIdentity = base64Encode(playerIdentity.toBytes(EcKey::Public));

	auto nonce = genSecRandomBytes(32);
	nonceB64 = base64Encode(nonce);

	// Get current time in UTC (system_clock)
	const auto now = std::chrono::system_clock::now();
	const auto epoch_duration = now.time_since_epoch();
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch_duration);
	secondsSinceEpoch = std::to_string(seconds.count());

	// Sign a string containing:
	// <random nonce b64><seconds since epoch><playerNameB64><lobbyAddress><gameId, if present>
	std::string strToSign = nonceB64 + secondsSinceEpoch + playerNameB64 + lobbyAddress + gameId.value_or(std::string());

	auto sigBytes = playerIdentity.sign(strToSign.data(), strToSign.size());
	signature = base64Encode(sigBytes);
}

void to_json(nlohmann::ordered_json& j, const RequestAttestation& v)
{
	j = nlohmann::ordered_json::object();
	j["pk"] = v.playerPublicIdentity;
	j["name"] = v.playerNameB64;
	j["n"] = v.nonceB64;
	j["l"] = v.lobbyAddress;
	if (v.gameId.has_value())
	{
		j["gid"] = v.gameId.value();
	}
	j["at"] = v.secondsSinceEpoch;
	j["sig"] = v.signature;
}

// MARK: LobbyConnectCheckRequest

struct LobbyConnectCheckRequest
{
	std::string lobbyConnectToken;
	std::string challenge;
};

void from_json(const nlohmann::json& j, LobbyConnectCheckRequest& v)
{
	v.lobbyConnectToken = j.at("lobbyToken").get<std::string>();
	v.challenge = j.at("challenge").get<std::string>();
}

// MARK: LobbyConnectCheckResponse

struct LobbyConnectCheckResponse
{
public:
	LobbyConnectCheckResponse(const std::string& gameId, const WzString& playerName, const EcKey& playerIdentity, const LobbyConnectCheckRequest& request);
public:
	std::string gameId;
	std::string playerNameB64;
	std::string playerPublicIdentity;
	std::string secondsSinceEpoch;
	std::string signature;
};

LobbyConnectCheckResponse::LobbyConnectCheckResponse(const std::string& gameId, const WzString& playerName, const EcKey& playerIdentity, const LobbyConnectCheckRequest& request)
: gameId(gameId)
{
	playerNameB64 = base64Encode(vecFromWzString(playerName));
	playerPublicIdentity = base64Encode(playerIdentity.toBytes(EcKey::Public));

	// Get current time in UTC (system_clock)
	const auto now = std::chrono::system_clock::now();
	const auto epoch_duration = now.time_since_epoch();
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch_duration);
	secondsSinceEpoch = std::to_string(seconds.count());

	// Sign a string containing:
	// WZLR:<request.Challenge><seconds since epoch><player name b64><gameId>
	std::string strToSign = std::string("WZLR:") + request.challenge + secondsSinceEpoch + playerNameB64 + gameId;

	auto sigBytes = playerIdentity.sign(strToSign.data(), strToSign.size());
	signature = base64Encode(sigBytes);
}

void to_json(nlohmann::ordered_json& j, const LobbyConnectCheckResponse& v)
{
	j = nlohmann::ordered_json::object();
	j["gid"] = v.gameId;
	j["pk"] = v.playerPublicIdentity;
	j["name"] = v.playerNameB64;
	j["at"] = v.secondsSinceEpoch;
	j["sig"] = v.signature;
}

// MARK: -

class LobbyServerHostingHandlerImpl: public LobbyServerHostingHandlerProtocol
{
public:
	LobbyServerHostingHandlerImpl(const std::string& lobbyServerAddress, EventCallbacks callbacks)
	: LobbyServerHostingHandlerProtocol(callbacks)
	, lobbyServerAddress(lobbyServerAddress)
	, trustedLobbyServerAddress(isTrustedLobbyAddress(lobbyServerAddress))
	{ }
	~LobbyServerHostingHandlerImpl();
public:
	// Requests a game listing
	bool createGameListing(const EcKey& hostIdentity, const GameDetails& gameDetails, const HostJoinOptions& joinOptions, const std::vector<ConnectionInfo>& connections) override;

	// Writes a response to a lobby connection check request
	LobbyConnectionCheckResult respondToLobbyConnectionCheckRequest(const std::string& jsonRequest, const WzString& playerName, const EcKey& playerIdentity) override;

	// Queues a game listing update
	void updateGameListing(const EcKey& hostIdentity, const GameDetails& gameDetails, const HostJoinOptions& joinOptions) override;

	// Checks a lobby-originated join request
	bool checkJoinRequest(const JoinRequest& joinRequest, CheckJoinRequestResultHandler resultHandler) override;

	// Removes the game listing from the lobby
	void removeGameListing(bool matchStarted) override;

	// Handles sending queued updates as well as keep-alives - should be called regularly by the main loop (at least, while in lobby)
	void run() override;

	// Fetching info
	bool isConnectedToLobby() const override;
	optional<std::string> getCurrentGameId() const override;
	optional<bool> isHostJoinOptionSupportedByLobby(HostJoinOptionType optType) const override;

private:

	void initializeGameListingCreationData(const GameDetails& gameDetails, const HostJoinOptions& joinOptions, const std::vector<ConnectionInfo>& connections);

	void addConnectionsToGame(const std::vector<ConnectionInfo>& connections);

	void sendListingUpdateImpl();

	void transitionToGameListingGone(bool matchStarted, optional<LobbyError> error, bool skipEventCallback = false);

	UrlRequestResponseHandler buildHostingAPIResponseHandler(std::unordered_set<long> acceptedResponseCodes, std::function<void(const std::string& url, long httpStatusCode, const nlohmann::json&& jsonResponse)> handlerFunc);

	// Builds a request failure handler that treats a request failure as fatal for the lobby game listing
	UrlRequestFailure buildHostingAPIStandardFailureHandler();

	void logAPIFailure(const std::string& url, long httpStatusCode, const nlohmann::json&& jsonData);
	void handleAPIFailure(const std::string& url, long httpStatusCode, const nlohmann::json&& jsonData);
	void handleAPIResponseHandlingFailure(const std::string& url, long httpStatusCode, const std::exception& e);

	void handleHostGameStatus(const HostGameStatus& statusInfo);

	void eventGameCreated(const WrappedCreateGameResponse& resp);
	void eventGamePublished();
	void eventGameListingGone(optional<LobbyError> error, bool skipEventCallback);

private:
	enum class LobbyListingState
	{
		None,
		PendingCreate,
		PendingPublish,
		Listed
	};

private:
	const std::string lobbyServerAddress;
	const bool trustedLobbyServerAddress;
	LobbyListingState state = LobbyListingState::None;

	std::chrono::seconds currPendingPublishCheckinInterval = SERVER_UPDATE_PENDING_PUBLISH_CHECKIN_MIN_INTERVAL;

	GameDetails lastGameDetails;
	HostJoinOptions lastJoinOptions;
	std::vector<ConnectionInfo> publishedConnectionInfo;

	struct PendingUpdateGameDetails
	{
		EcKey hostIdentity;
		GameDetails gameDetails;
		HostJoinOptions joinOptions;
	};
	optional<PendingUpdateGameDetails> pendingUpdate;
	bool awaitingAsyncUpdateResponse = false;
	std::chrono::steady_clock::time_point lastLobbyUpdateAttempt;
	std::chrono::steady_clock::time_point lastSuccessfulLobbyUpdate;
	optional<std::chrono::steady_clock::time_point> lobbyUpdateRateLimitWaitTill = nullopt;

	HostGameContext hostGameCtx;
	std::string createMotd;
	optional<std::unordered_set<HostJoinOptionType>> supportedHostJoinOpts;
};

static const std::string& getWZUserAgent()
{
	static const std::string currentUserAgentString = version_getHTTPUserAgentString();
	return currentUserAgentString;
}

static URLDataRequest buildBaseHostingURLDataRequest()
{
	URLDataRequest urlRequest;

	// We always want direct connections to the lobby server (skipping any locally-configured proxies or redirects)
	urlRequest.noProxy = true;
	urlRequest.noFollowRedirects = true;

	urlRequest.userAgent = getWZUserAgent();

	urlRequest.setRequestHeader("Origin", "wz2100://wz2100.net");
	urlRequest.setRequestHeader("Referer", "https://github.com/Warzone2100/warzone2100?wz_internal=mp.host");
	urlRequest.setRequestHeader("Accept", "application/json");

	urlRequest.maxDownloadSizeLimit = 20 * 1024 * 1024; // response should never be > 20 MB

	return urlRequest;
}

static bool addHostAuthorization(URLDataRequest& request, const HostGameContext& hostGameCtx)
{
	ASSERT_OR_RETURN(false, !hostGameCtx.hostToken.empty(), "Missing HostToken");
	request.setRequestHeader("Authorization", std::string("HostToken ") + hostGameCtx.hostToken);
	return true;
}

static bool LobbyDeleteGameImpl(const std::string& lobbyServerAddress, HostGameContext hostGameCtx, bool matchStarted)
{
	ASSERT_OR_RETURN(false, hostGameCtx.isValid(), "Invalid HostGameContext");

	auto request = buildBaseHostingURLDataRequest();
	addHostAuthorization(request, hostGameCtx);
	request.url = buildLobbyRequestBaseUrl(lobbyServerAddress) + "/api/v1/game/" + hostGameCtx.gameId;
	if (matchStarted)
	{
		request.url += "?started=1";
	}
	request.protocol = InternetProtocol::IPv4;
	request.setDelete();

	request.autoRetryStrategy = URLRequestAutoRetryStrategy::RetryAfter(std::chrono::milliseconds(3000), 3, {HttpRequestConnectionFailure, HttpStatusTooManyRequests});

	urlRequestData(request);

	return true;
}

static void logErrorOnMainThread(const std::string& prefix, const char* error)
{
	std::string errorString = prefix;
	if (error != nullptr)
	{
		errorString += error;
	}
	wzAsyncExecOnMainThread([errorString = std::move(errorString)]() {
		debug(LOG_LOBBY, "%s", errorString.c_str());
	});
}

static void logExceptionOnMainThread(const std::string& prefix, const std::exception& e)
{
	logErrorOnMainThread(prefix, e.what());
}

LobbyServerHostingHandlerImpl::~LobbyServerHostingHandlerImpl()
{
	if (hostGameCtx.isValid())
	{
		debug(LOG_INFO, "Removing game listing due to LobbyServerHostingHandler destruction (unexpected)");
		transitionToGameListingGone(false, nullopt);
	}
}

UrlRequestResponseHandler LobbyServerHostingHandlerImpl::buildHostingAPIResponseHandler(std::unordered_set<long> acceptedResponseCodes, std::function<void(const std::string& url, long httpStatusCode, const nlohmann::json&& jsonResponse)> handlerFunc)
{
	std::weak_ptr<LobbyServerHostingHandlerImpl> weakSelf = std::static_pointer_cast<LobbyServerHostingHandlerImpl>(shared_from_this());
	return [weakSelf, acceptedResponseCodes, handlerFunc](const std::string& url, const HTTPResponseDetails& responseDetails, const std::shared_ptr<MemoryStruct>& data) -> URLRequestHandlingBehavior
	{
		// Parse the (expected) json response
		nlohmann::json jsonData;
		try {
			jsonData = nlohmann::json::parse(data->memory, data->memory + data->size);
		}
		catch (const std::exception &e) {
			jsonData = nlohmann::json();
			logExceptionOnMainThread("Failed to parse JSON response: ", e);
		}

		std::string urlCopy = url;
		long httpStatusCode = responseDetails.httpStatusCode();
		if (acceptedResponseCodes.count(httpStatusCode) != 0)
		{
			// On accepted response code(s) (which may vary by api)
			// call the handlerFunc on the main thread
			wzAsyncExecOnMainThread([weakSelf, handlerFunc, urlCopy, httpStatusCode, jsonData = std::move(jsonData)]() mutable {
				try {
					handlerFunc(urlCopy, httpStatusCode, std::move(jsonData));
				}
				catch (const std::exception &e) {
					// handlerFunc caused an exception - possibly trying to convert json
					// treat this as a failure
					auto strongSelf = weakSelf.lock();
					if (strongSelf == nullptr)
					{
						debug(LOG_LOBBY, "LobbyServerHostingHandler instance is gone");
						return;
					}
					strongSelf->handleAPIResponseHandlingFailure(urlCopy, httpStatusCode, e);
				}
			});
		}
		else
		{
			// Failure response code - call handleAPIFailure on the main thread
			wzAsyncExecOnMainThread([weakSelf, urlCopy, httpStatusCode, jsonData = std::move(jsonData)]() mutable {
				auto strongSelf = weakSelf.lock();
				ASSERT_OR_RETURN(, strongSelf != nullptr, "LobbyServerHostingHandler instance is gone");
				strongSelf->handleAPIFailure(urlCopy, httpStatusCode, std::move(jsonData));
			});
		}
		return URLRequestHandlingBehavior::Done();
	};
}

// Builds a request failure handler that treats a request failure as fatal for the lobby game listing
UrlRequestFailure LobbyServerHostingHandlerImpl::buildHostingAPIStandardFailureHandler()
{
	std::weak_ptr<LobbyServerHostingHandlerImpl> weakSelf = std::static_pointer_cast<LobbyServerHostingHandlerImpl>(shared_from_this());
	return [weakSelf](const std::string& url, URLRequestFailureType type, std::shared_ptr<HTTPResponseDetails> transferDetails) {

		// Failure response - call handleAPIFailure on the main thread
		std::string urlCopy = url;
		long httpStatusCode = 0;
		if (transferDetails)
		{
			httpStatusCode = transferDetails->httpStatusCode();
		}
		wzAsyncExecOnMainThread([weakSelf, urlCopy, httpStatusCode]() {
			auto strongSelf = weakSelf.lock();
			if (strongSelf == nullptr)
			{
				debug(LOG_LOBBY, "LobbyServerHostingHandler instance is gone");
				return;
			}
			strongSelf->handleAPIFailure(urlCopy, httpStatusCode, nlohmann::json());
		});

	};
}

void LobbyServerHostingHandlerImpl::logAPIFailure(const std::string& url, long httpStatusCode, const nlohmann::json&& jsonData)
{
	optional<LobbyError> err;
	try {
		err = jsonData.get<LobbyError>();
	}
	catch (const std::exception&) {
		// Failed to parse response as LobbyError - non-fatal
		// (might not have sent a response besides the httpStatusCode)
	}

	// Log
	if (err.has_value())
	{
		debug(LOG_LOBBY, "Lobby returned error: (%ld) [%s] %s", httpStatusCode, err.value().errCode.c_str(), err.value().errMessage.c_str());
	}
	else
	{
		debug(LOG_LOBBY, "Lobby error: (%ld)", httpStatusCode);
	}
}

void LobbyServerHostingHandlerImpl::handleAPIFailure(const std::string& url, long httpStatusCode, const nlohmann::json&& jsonData)
{
	optional<LobbyError> err;
	try {
		err = jsonData.get<LobbyError>();
	}
	catch (const std::exception&) {
		// Failed to parse response as LobbyError - non-fatal
		// (might not have sent a response besides the httpStatusCode)
	}

	// Log
	if (err.has_value())
	{
		debug(LOG_LOBBY, "Lobby returned error: (%ld) [%s] %s", httpStatusCode, err.value().errCode.c_str(), err.value().errMessage.c_str());
	}
	else
	{
		debug(LOG_LOBBY, "Lobby error: (%ld)", httpStatusCode);
	}

	transitionToGameListingGone(false, err);
}

void LobbyServerHostingHandlerImpl::handleAPIResponseHandlingFailure(const std::string& url, long httpStatusCode, const std::exception& e)
{
	// The API response handler function threw an exception
	// Very likely due to trying to parse the JSON response

	// Log
	debug(LOG_LOBBY, "Lobby response handling failure: (%ld) %s", httpStatusCode, e.what());

	// Treat this as a critical failure, and disconnected lobby listing
	transitionToGameListingGone(false, nullopt);
}

bool LobbyServerHostingHandlerImpl::createGameListing(const EcKey& hostIdentity, const GameDetails& gameDetails, const HostJoinOptions& joinOptions, const std::vector<ConnectionInfo>& connections)
{
	ASSERT_OR_RETURN(false, state == LobbyListingState::None, "create called when lobby listing state is: %d", static_cast<int>(state));
	ASSERT_OR_RETURN(false, !connections.empty(), "create called with no connections?");

	initializeGameListingCreationData(gameDetails, joinOptions, connections);

	auto request = buildBaseHostingURLDataRequest();
	request.url = buildLobbyRequestBaseUrl(lobbyServerAddress) + "/api/v1/game";
	request.protocol = InternetProtocol::IPv4;

	auto hostCreateGameInfo = nlohmann::ordered_json(gameDetails);
	ASSERT_OR_RETURN(false, hostCreateGameInfo.is_object(), "Not an object?");
	hostCreateGameInfo["joinopts"] = joinOptions;

	if (trustedLobbyServerAddress)
	{
		auto attestation = nlohmann::ordered_json(RequestAttestation(gameDetails.host.name, hostIdentity, lobbyServerAddress, nullopt));
		hostCreateGameInfo["attest"] = attestation;
	}

	request.setPost(hostCreateGameInfo.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace));

	request.connectTimeoutMs = 10000;

	request.autoRetryStrategy = URLRequestAutoRetryStrategy::RetryAfter(std::chrono::milliseconds(5000), 2, {HttpRequestConnectionFailure, HttpStatusTooManyRequests});

	std::weak_ptr<LobbyServerHostingHandlerImpl> weakSelf = std::static_pointer_cast<LobbyServerHostingHandlerImpl>(shared_from_this());
	std::string lobbyServerAddressCopy = lobbyServerAddress;
	request.onResponse = buildHostingAPIResponseHandler({200, 201, 202}, [weakSelf, connections, lobbyServerAddressCopy] (const std::string &url, long httpStatusCode, const nlohmann::json &&jsonData) {
		// handle only accepted response codes (called on main thread)
		WrappedCreateGameResponse resp = jsonData.get<WrappedCreateGameResponse>();

		auto strongSelf = weakSelf.lock();
		if (strongSelf == nullptr)
		{
			debug(LOG_LOBBY, "LobbyServerHostingHandler instance is gone");
			// Immediately call DELETE API on gameId
			LobbyDeleteGameImpl(lobbyServerAddressCopy, resp.ctx, false);
			return;
		}

		strongSelf->eventGameCreated(resp);
	});

	request.onFailure = buildHostingAPIStandardFailureHandler();

	if (urlRequestData(request) == nullptr)
	{
		return false;
	}

	state = LobbyListingState::PendingCreate;
	return true;
}

LobbyServerHostingHandlerImpl::LobbyConnectionCheckResult LobbyServerHostingHandlerImpl::respondToLobbyConnectionCheckRequest(const std::string& jsonRequest, const WzString& playerName, const EcKey& playerIdentity)
{
	if (state == LobbyListingState::None || !hostGameCtx.isValid())
	{
		// Game not (yet?) listed
		return ::tl::make_unexpected(LobbyConnectionCheckRequestError::GameNotListed);
	}

	// JSON decode LobbyConnectCheckRequest from jsonString
	LobbyConnectCheckRequest request;
	try {
		nlohmann::json jsonData = nlohmann::json::parse(jsonRequest);
		request = jsonData.get<LobbyConnectCheckRequest>();
	}
	catch (const std::exception &e) {
		debug(LOG_LOBBY, "Failed to parse lobby connection request: %s", e.what());
		return ::tl::make_unexpected(LobbyConnectionCheckRequestError::InvalidRequest);
	}

	// Sanity-checks
	if (request.lobbyConnectToken != hostGameCtx.lobbyConnectToken)
	{
		debug(LOG_LOBBY, "Invalid lobbyConnectToken");
		return ::tl::make_unexpected(LobbyConnectionCheckRequestError::InvalidRequest);
	}
	const auto challengeLength = request.challenge.size();
	if (challengeLength < 32 || challengeLength > 88)
	{
		// Insufficient challenge length
		debug(LOG_LOBBY, "Invalid challenge length: %zu", challengeLength);
		return ::tl::make_unexpected(LobbyConnectionCheckRequestError::InvalidRequest);
	}

	std::string responseString;

	if (trustedLobbyServerAddress)
	{
		// Construct LobbyConnectCheckResponse
		auto response = LobbyConnectCheckResponse(hostGameCtx.gameId, playerName, playerIdentity, request);

		responseString = nlohmann::ordered_json(response).dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace);
	}
	else
	{
		// Send just the gameId
		nlohmann::ordered_json simpleResponse;
		simpleResponse["gid"] = getCurrentGameId().value_or(std::string());

		responseString = simpleResponse.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace);
	}

	return responseString;
}

// Queues a game listing update
void LobbyServerHostingHandlerImpl::updateGameListing(const EcKey& hostIdentity, const GameDetails& gameDetails, const HostJoinOptions& joinOptions)
{
	if (state == LobbyListingState::None)
	{
		return;
	}

	pendingUpdate = {hostIdentity, gameDetails, joinOptions};
}

void LobbyServerHostingHandlerImpl::sendListingUpdateImpl()
{
	auto request = buildBaseHostingURLDataRequest();
	addHostAuthorization(request, hostGameCtx);
	request.url = buildLobbyRequestBaseUrl(lobbyServerAddress) + "/api/v1/game/" + hostGameCtx.gameId;
	request.protocol = InternetProtocol::IPv4;
	request.connectTimeoutMs = std::chrono::duration_cast<std::chrono::milliseconds>(SERVER_UPDATE_MAX_INTERVAL).count();

	optional<PendingUpdateGameDetails> ownedPendingUpdate = std::move(pendingUpdate);
	pendingUpdate.reset();

	if (ownedPendingUpdate.has_value())
	{
		auto hostCreateGameInfo = serializeGameDetailsChangedFields(ownedPendingUpdate.value().gameDetails, lastGameDetails);
		ASSERT_OR_RETURN(, hostCreateGameInfo.is_object(), "Not an object?");

		if (ownedPendingUpdate.value().joinOptions != lastJoinOptions)
		{
			hostCreateGameInfo["joinopts"] = ownedPendingUpdate.value().joinOptions;
		}

		const bool hostIdentityChanged = (lastGameDetails.host.publicIdentity != ownedPendingUpdate.value().gameDetails.host.publicIdentity);
		if (hostIdentityChanged && trustedLobbyServerAddress)
		{
			auto attestation = nlohmann::ordered_json(RequestAttestation(ownedPendingUpdate.value().gameDetails.host.name, ownedPendingUpdate.value().hostIdentity, lobbyServerAddress, nullopt));
			hostCreateGameInfo["attest"] = attestation;
		}

		request.setPatch(hostCreateGameInfo.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace));
	}
	else
	{
		request.setPatch("{}");
	}

	// Don't want the default autoRetryStrategy here because of the asynchronous nature of the requests
	// Specifically: There is no guarantee that a patch request response is returned before another pendingUpdate is queued by the main thread. So we must coordinate retry strategy with the main thread.

	std::weak_ptr<LobbyServerHostingHandlerImpl> weakSelf = std::static_pointer_cast<LobbyServerHostingHandlerImpl>(shared_from_this());
	std::string lobbyServerAddressCopy = lobbyServerAddress;

	request.onResponse = buildHostingAPIResponseHandler({200, 201, 202, 429}, [weakSelf, lobbyServerAddressCopy, ownedPendingUpdate] (const std::string &url, long httpStatusCode, const nlohmann::json &&jsonData) {
		// handle only accepted response codes (called on main thread)

		auto strongSelf = weakSelf.lock();
		if (strongSelf == nullptr)
		{
			debug(LOG_LOBBY, "LobbyServerHostingHandler instance is gone");
			return;
		}

		strongSelf->awaitingAsyncUpdateResponse = false;
		const auto now = std::chrono::steady_clock::now();

		if (httpStatusCode == 429) // 429 Too Many Requests (RFC 6585)
		{
			if (!strongSelf->pendingUpdate.has_value() && ownedPendingUpdate.has_value())
			{
				// No more recent pending update game info
				// Requeue the *same* pendingUpdate
				strongSelf->pendingUpdate = ownedPendingUpdate;
			}

			// Throttle retry
			strongSelf->lobbyUpdateRateLimitWaitTill = now + std::chrono::seconds(5);
			return;
		}

		strongSelf->lastSuccessfulLobbyUpdate = now;
		if (ownedPendingUpdate.has_value())
		{
			strongSelf->lastGameDetails = ownedPendingUpdate.value().gameDetails;
			strongSelf->lastJoinOptions = ownedPendingUpdate.value().joinOptions;
		}

		// Get HostGameStatus from jsonData
		auto hostGameStatus = jsonData.get<HostGameStatus>();
		strongSelf->handleHostGameStatus(hostGameStatus);
	});

	request.onFailure = buildHostingAPIStandardFailureHandler();

	if (!urlRequestData(request))
	{
		// Failed to dispatch request
		debug(LOG_INFO, "Failed to dispatch url request");
	}

	awaitingAsyncUpdateResponse = true;
	lastLobbyUpdateAttempt = std::chrono::steady_clock::now();
}

// Checks a lobby-originated join request
bool LobbyServerHostingHandlerImpl::checkJoinRequest(const JoinRequest& joinRequest, CheckJoinRequestResultHandler resultHandler)
{
	ASSERT_OR_RETURN(false, resultHandler != nullptr, "No result handler");

	if (state == LobbyListingState::None || !hostGameCtx.isValid())
	{
		// Game not (yet?) listed, so can't check join
		return false;
	}

	auto request = buildBaseHostingURLDataRequest();
	addHostAuthorization(request, hostGameCtx);
	request.url = buildLobbyRequestBaseUrl(lobbyServerAddress) + "/api/v1/game/" + hostGameCtx.gameId + "/check-join-request";
	request.protocol = InternetProtocol::IPv4;
	request.connectTimeoutMs = 10000;

	request.autoRetryStrategy = URLRequestAutoRetryStrategy::RetryAfter(std::chrono::milliseconds(3000), 1, {HttpStatusTooManyRequests});

	auto joinRequestJson = nlohmann::ordered_json(joinRequest);
	ASSERT_OR_RETURN(false, joinRequestJson.is_object(), "Not an object?");
	request.setPost(joinRequestJson.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace));

	std::weak_ptr<LobbyServerHostingHandlerImpl> weakSelf = std::static_pointer_cast<LobbyServerHostingHandlerImpl>(shared_from_this());

	request.onResponse = [resultHandler](const std::string& url, const HTTPResponseDetails& responseDetails, const std::shared_ptr<MemoryStruct>& data) -> URLRequestHandlingBehavior
	{
		// Parse the (expected) json response
		nlohmann::json jsonData;
		try {
			jsonData = nlohmann::json::parse(data->memory, data->memory + data->size);
		}
		catch (const std::exception &e) {
			jsonData = nlohmann::json();
			logExceptionOnMainThread("Failed to parse JSON response: ", e);
		}

		long httpStatusCode = responseDetails.httpStatusCode();
		if (httpStatusCode == 200)
		{
			// On accepted response code(s) (which may vary by api)
			// call the resultHandler on the main thread
			wzAsyncExecOnMainThread([resultHandler, jsonData = std::move(jsonData)]() {
				CheckJoinRequestResult result;
				try {
					result = jsonData.get<CheckJoinRequestResult>();
				}
				catch (const std::exception &e) {
					// failed to convert json response
					resultHandler(::tl::make_unexpected(LobbyError("WZ_INTERNAL_INVALID_SERVER_RESPONSE", e.what())));
					return;
				}
				resultHandler(result);
			});
		}
		else
		{
			// Unexpected / failure response code
			wzAsyncExecOnMainThread([resultHandler, httpStatusCode, jsonData = std::move(jsonData)]() {

				optional<LobbyError> err;
				try {
					err = jsonData.get<LobbyError>();
				}
				catch (const std::exception&) {
					// Failed to parse response as LobbyError - non-fatal
					// (might not have sent a response besides the httpStatusCode)
				}

				// Log + call resultHandler
				if (err.has_value())
				{
					debug(LOG_LOBBY, "Lobby returned error: (%ld) [%s] %s", httpStatusCode, err.value().errCode.c_str(), err.value().errMessage.c_str());
					resultHandler(::tl::make_unexpected(err.value()));
				}
				else
				{
					debug(LOG_LOBBY, "Lobby error: (%ld)", httpStatusCode);
					resultHandler(::tl::make_unexpected(LobbyError("GENERIC_ERROR", astringf("Status Code: %ld", httpStatusCode))));
				}
			});
		}
		return URLRequestHandlingBehavior::Done();
	};

	request.onFailure = [resultHandler](const std::string& url, URLRequestFailureType type, std::shared_ptr<HTTPResponseDetails> transferDetails) {

		// Failure response - call resultHandler on the main thread
		std::string urlCopy = url;
		long httpStatusCode = 0;
		if (transferDetails)
		{
			httpStatusCode = transferDetails->httpStatusCode();
		}
		wzAsyncExecOnMainThread([resultHandler, httpStatusCode]() {
			resultHandler(::tl::make_unexpected(LobbyError("SERVER_CONNECTION_FAILURE", astringf("Status Code: %ld", httpStatusCode))));
		});

	};

	// Dispatch query
	if (!urlRequestData(request))
	{
		return false;
	}

	return true;
}

// Removes the current game listing from the lobby
void LobbyServerHostingHandlerImpl::removeGameListing(bool matchStarted)
{
	// skip event callback, since this was deliberately removed from the lobby
	transitionToGameListingGone(matchStarted, nullopt, true);
}

void LobbyServerHostingHandlerImpl::transitionToGameListingGone(bool matchStarted, optional<LobbyError> error, bool skipEventCallback)
{
	if (hostGameCtx.isValid())
	{
		LobbyDeleteGameImpl(lobbyServerAddress, hostGameCtx, matchStarted);
	}
	else
	{
		debug(LOG_LOBBY, "No known game listing to remove - skipping removal");
	}

	eventGameListingGone(error, skipEventCallback);
}

// Handles sending queued updates as well as keep-alives - should be called regularly by the main loop (at least, while in lobby)
void LobbyServerHostingHandlerImpl::run()
{
	if (state == LobbyListingState::None || !hostGameCtx.isValid())
	{
		return; // nothing to do until / unless state is pending or listed
	}

	if (!awaitingAsyncUpdateResponse)
	{
		auto now = std::chrono::steady_clock::now();

		auto secondsSinceLastSuccessfulLobbyUpdate = std::chrono::duration_cast<std::chrono::seconds>(now - lastSuccessfulLobbyUpdate);
		auto secondsSinceLastLobbyUpdateAttempt = std::chrono::duration_cast<std::chrono::seconds>(now - lastLobbyUpdateAttempt);

		if (secondsSinceLastSuccessfulLobbyUpdate >= SERVER_UPDATE_NO_SUCCESS_ABANDON_INTERVAL)
		{
			// Despite trying, we haven't managed to successfully update the server in a while
			// Treat this as a failure and stop trying
			transitionToGameListingGone(false, nullopt);
			return;
		}

		if (lobbyUpdateRateLimitWaitTill.has_value())
		{
			if (now < lobbyUpdateRateLimitWaitTill.value())
			{
				// Wait until lobbyUpdateRateLimitWaitTill passes to try again
				return;
			}
			else
			{
				// clear lobbyUpdateRateLimitWaitTill and proceed as normal
				lobbyUpdateRateLimitWaitTill.reset();
			}
		}

		const auto successfulLobbyUpdateInterval = (state == LobbyListingState::PendingPublish) ? currPendingPublishCheckinInterval : SERVER_UPDATE_MAX_INTERVAL;

		if ((secondsSinceLastSuccessfulLobbyUpdate >= successfulLobbyUpdateInterval)
			|| (pendingUpdate.has_value() && secondsSinceLastLobbyUpdateAttempt >= SERVER_UPDATE_MIN_INTERVAL))
		{
			if (state == LobbyListingState::PendingPublish)
			{
				// increasing delay for initial PendingPublish check-ins (i.e. waiting for lobby to return published status)
				currPendingPublishCheckinInterval = std::min<std::chrono::seconds>(currPendingPublishCheckinInterval + std::chrono::seconds(1), SERVER_UPDATE_PENDING_PUBLISH_CHECKIN_MAX_INTERVAL);
			}
			sendListingUpdateImpl();
		}
	}
}

bool LobbyServerHostingHandlerImpl::isConnectedToLobby() const
{
	return (state != LobbyListingState::None);
}

optional<std::string> LobbyServerHostingHandlerImpl::getCurrentGameId() const
{
	if (!hostGameCtx.isValid())
	{
		return nullopt;
	}
	return hostGameCtx.gameId;
}

optional<bool> LobbyServerHostingHandlerImpl::isHostJoinOptionSupportedByLobby(HostJoinOptionType optType) const
{
	if (!supportedHostJoinOpts.has_value())
	{
		// The server hasn't told us yet - don't know if supported
		return nullopt;
	}

	return supportedHostJoinOpts.value().count(optType) != 0;
}

void LobbyServerHostingHandlerImpl::initializeGameListingCreationData(const GameDetails& gameDetails, const HostJoinOptions& joinOptions, const std::vector<ConnectionInfo>& connections)
{
	lastGameDetails = gameDetails;
	lastJoinOptions = joinOptions;

	publishedConnectionInfo = connections;
}

static InternetProtocol ipVersionToInternetProtocol(IPVersion ver)
{
	switch (ver)
	{
		case IPVersion::IPv4: return InternetProtocol::IPv4;
		case IPVersion::IPv6: return InternetProtocol::IPv6;
	}

	return InternetProtocol::IP_ANY; // silence compiler warning
}

void LobbyServerHostingHandlerImpl::addConnectionsToGame(const std::vector<ConnectionInfo>& connections)
{
	for (auto conn : connections)
	{
		InternetProtocol protocol = ipVersionToInternetProtocol(conn.type.ipVersion.value_or(IPVersion::IPv4));

		auto request = buildBaseHostingURLDataRequest();
		addHostAuthorization(request, hostGameCtx);
		request.url = buildLobbyRequestBaseUrl(lobbyServerAddress) + "/api/v1/game/" + hostGameCtx.gameId + "/connection";
		request.protocol = protocol;

		auto publishConnRequestInfoJson = nlohmann::ordered_json(conn);
		ASSERT_OR_RETURN(, publishConnRequestInfoJson.is_object(), "Not an object?");

		request.setPost(publishConnRequestInfoJson.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace));

		request.connectTimeoutMs = 15000;

		request.autoRetryStrategy = URLRequestAutoRetryStrategy::RetryAfter(std::chrono::milliseconds(3000), 2, {HttpRequestConnectionFailure, HttpStatusTooManyRequests});

		std::weak_ptr<LobbyServerHostingHandlerImpl> weakSelf = std::static_pointer_cast<LobbyServerHostingHandlerImpl>(shared_from_this());

		// Response codes:
		// - 409: May be returned on conflict
		// - 429: May be returned if autoRetryStrategy doesn't succeed
		request.onResponse = buildHostingAPIResponseHandler({200, 201, 202, 406, 409, 429}, [weakSelf, connections] (const std::string &url, long httpStatusCode, const nlohmann::json &&jsonData) {
			// handle only accepted response codes (called on main thread)

			if (httpStatusCode == 406 || httpStatusCode == 409 || httpStatusCode == 429) // 409 Conflict or 429 Too Many Requests
			{
				// 409: Tried to add a duplicate of the same connection type?
				// 429: Lobby is rate-limiting adding this connection (and autoRetryStrategy didn't succeed)
				// (Failing to add a single connection doesn't necessarily mean the whole listing has failed)
				// Just log it
				debug(LOG_LOBBY, "Received status code: %ld", httpStatusCode);
				return;
			}

			// Waiting on the server to test the connection here - status is likely pending

			// Get HostGameStatus from jsonData
			auto hostGameStatus = jsonData.get<HostGameStatus>();
			auto strongSelf = weakSelf.lock();
			if (strongSelf == nullptr)
			{
				debug(LOG_LOBBY, "LobbyServerHostingHandler instance is gone");
				return;
			}
			strongSelf->handleHostGameStatus(hostGameStatus);
		});

		request.onFailure = [protocol](const std::string& url, URLRequestFailureType type, std::shared_ptr<HTTPResponseDetails> transferDetails) {
			// Failing to add a single connection doesn't necessarily mean the whole listing has failed
			// Just log (on main thread)
			logErrorOnMainThread(astringf("AddConnection request failure (protocol: %s, failureType: %s)", to_string(protocol), to_string(type)), nullptr);
		};

		urlRequestData(request);
	}
}

void LobbyServerHostingHandlerImpl::handleHostGameStatus(const HostGameStatus& statusInfo)
{
	if (statusInfo.gameId != hostGameCtx.gameId)
	{
		debug(LOG_LOBBY, "Old game status reply for old game - ignoring");
		return;
	}

	if (statusInfo.published)
	{
		if (state == LobbyListingState::PendingPublish)
		{
			// transition to published state
			eventGamePublished();
		}
	}
	else
	{
		// Check for a heads-up that this game listing will never be published
		// i.e. if no connections were able to be validated by the lobby server

		bool allLobbyReceivedConnectionsAreInErrorState = std::all_of(statusInfo.connectionStatusInfo.begin(), statusInfo.connectionStatusInfo.end(), [](const HostConnectionStatusInfo& conn) -> bool {
			return conn.status == ConnectionStatus::Error;
		});

		// 1. Check if all connections are Error state
		// 2. Check that lobby server has received all connections
		if (allLobbyReceivedConnectionsAreInErrorState && statusInfo.connectionStatusInfo.size() == publishedConnectionInfo.size())
		{
			// Every connection we tried to add to the game couldn't be verified by the lobby :(
			// Host appears to be inaccessible from the outside world
			// Immediately remove game and transition to state gone (with synthesized errCodeHostNotAccessible)

			std::string translatedHostNotAccessibleString = _("Error hosting game in lobby server");
			translatedHostNotAccessibleString += "\n";
			translatedHostNotAccessibleString += astringf(_("Make sure port %d can receive incoming connections."), (int)NETgetGameserverPort());
			translatedHostNotAccessibleString += "\n";
			translatedHostNotAccessibleString += _("If you're using a router, configure it to enable UPnP/NAT-PMP/PCP or to forward the port to your system.");

			transitionToGameListingGone(false, LobbyError(errCodeHostNotAccessible, translatedHostNotAccessibleString));
		}
	}
}

void LobbyServerHostingHandlerImpl::eventGameCreated(const WrappedCreateGameResponse& resp)
{
	if (state != LobbyListingState::PendingCreate)
	{
		ASSERT(state == LobbyListingState::PendingCreate, "Unexpected state (%d)", static_cast<int>(state));
		LobbyDeleteGameImpl(lobbyServerAddress, resp.ctx, false);
		return;
	}

	hostGameCtx = resp.ctx;
	state = LobbyListingState::PendingPublish;

	createMotd = resp.motd; // store, but don't provide to callbacks until game is published
	supportedHostJoinOpts = resp.supportedHostJoinOpts;

	lastSuccessfulLobbyUpdate = std::chrono::steady_clock::now();
	lastLobbyUpdateAttempt = std::chrono::steady_clock::now();

	// Attempt to add connections to game
	addConnectionsToGame(publishedConnectionInfo);

	// Trigger event callback
	if (callbacks.lobbyGameCreatedCallback)
	{
		callbacks.lobbyGameCreatedCallback(hostGameCtx.gameId);
	}
}

void LobbyServerHostingHandlerImpl::eventGamePublished()
{
	if (state == LobbyListingState::Listed)
	{
		return;
	}

	state = LobbyListingState::Listed;

	// Trigger event callback
	if (callbacks.lobbyGamePublishedCallback)
	{
		callbacks.lobbyGamePublishedCallback(hostGameCtx.gameId, createMotd);
	}
}

void LobbyServerHostingHandlerImpl::eventGameListingGone(optional<LobbyError> error, bool skipEventCallback)
{
	if (state == LobbyListingState::None)
	{
		return;
	}

	state = LobbyListingState::None;

	lastGameDetails = GameDetails();
	lastJoinOptions = HostJoinOptions();

	pendingUpdate.reset();
	awaitingAsyncUpdateResponse = false;
	lastLobbyUpdateAttempt = {};
	lastSuccessfulLobbyUpdate = {};

	hostGameCtx = HostGameContext();

	if (!skipEventCallback)
	{
		// Trigger lobbyGameGoneCallback event callback
		if (callbacks.lobbyGameGoneCallback)
		{
			callbacks.lobbyGameGoneCallback(error);
		}
	}
}

std::shared_ptr<LobbyServerHostingHandlerProtocol> MakeLobbyHostListingHandler(const std::string& lobbyAddress, LobbyServerHostingHandlerImpl::EventCallbacks callbacks)
{

	return std::make_shared<LobbyServerHostingHandlerImpl>(lobbyAddress, callbacks);
}

// MARK: - Enumerating game listings

// MARK: ListHeader

struct EnumGamesHeader
{
	std::string motd;
};

void from_json(const nlohmann::json& j, EnumGamesHeader& v)
{
	v.motd = j.at("motd").get<std::string>();
}

// MARK: GameListing

void from_json(const nlohmann::json& j, GameListing& v)
{
	v.gameId = j.at("id").get<std::string>();
	from_json(j, v.details);
	v.availableConnectionTypes = j.at("conn").get<std::vector<ConnectionType>>();
}

typedef std::function<bool(const std::string_view& line)> ProcessLineFunc;

static void processMemoryStructLines(const std::shared_ptr<MemoryStruct>& data, const ProcessLineFunc& f)
{
	std::string_view view(data->memory, data->size);
	size_t start = 0;
	size_t end;

	while ((end = view.find('\n', start)) != std::string_view::npos)
	{
		std::string_view line = view.substr(start, end - start);
		if (!f(line))
		{
			return;
		}
		start = end + 1;
	}
	// Handle the last line if no trailing newline
	if (start < view.size())
	{
		f(view.substr(start));
	}
}

bool EnumerateGames(const std::string& lobbyServerAddress, CompletionHandlerFunc completionHandlerFunc, optional<size_t> resultsLimit, optional<NetcodeVer> matchingVersion)
{
	URLDataRequest request;

	// We always want direct connections to the lobby server (skipping any locally-configured proxies or redirects)
	request.noProxy = true;
	request.noFollowRedirects = true;

	request.userAgent = getWZUserAgent();

	request.setRequestHeader("Origin", "wz2100://wz2100.net");
	request.setRequestHeader("Referer", "https://github.com/Warzone2100/warzone2100?wz_internal=list");
	request.setRequestHeader("Accept", "application/jsonl");

	request.maxDownloadSizeLimit = 20 * 1024 * 1024; // response should never be > 20 MB

	request.url = buildLobbyRequestBaseUrl(lobbyServerAddress) + "/api/v1/games";

	request.connectTimeoutMs = 10000;

	request.onResponse = [completionHandlerFunc, resultsLimit, matchingVersion](const std::string& url, const HTTPResponseDetails& responseDetails, const std::shared_ptr<MemoryStruct>& data) -> URLRequestHandlingBehavior
	{
		long httpStatusCode = responseDetails.httpStatusCode();
		if (httpStatusCode == 200)
		{
			optional<EnumGamesHeader> headerObject = nullopt;
			ListResponse result;

			// Expected response is JSON-Lines of either header object or GameListing objects
			processMemoryStructLines(data, [&result, &headerObject, resultsLimit, matchingVersion](const std::string_view& line) -> bool {
				nlohmann::json jsonData;
				try {
					jsonData = nlohmann::json::parse(line.data(), line.data() + line.size());
				}
				catch (const std::exception &e) {
					jsonData = nlohmann::json();
					logExceptionOnMainThread("Failed to parse JSON response: ", e);
					return false;
				}
				ASSERT_OR_RETURN(false, jsonData.is_object(), "Received non-object item");

				auto it = jsonData.find("type");
				if (it != jsonData.end() && it.value().get<std::string>() == "header")
				{
					// Found header object
					if (!headerObject.has_value())
					{
						// Try to parse as EnumGamesHeader
						try {
							headerObject = jsonData.get<EnumGamesHeader>();
						}
						catch (const std::exception &e) {
							// failed to convert json header object
							debug(LOG_NEVER, "%s", jsonData.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace).c_str());
							logExceptionOnMainThread("Invalid header: ", e);
							return true; // continue enumeration
						}

						result.lobbyMOTD = headerObject.value().motd;
					}
					return true;
				}

				// Try to parse as GameListing
				GameListing listing;
				try {
					listing = jsonData.get<GameListing>();
				}
				catch (const std::exception &e) {
					// failed to convert json response
					debug(LOG_NEVER, "%s", jsonData.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace).c_str());
					logExceptionOnMainThread("Invalid listing: ", e);
					return true; // continue enumeration
				}

				if (!matchingVersion.has_value() || matchingVersion.value() == listing.details.netcodeVer)
				{
					result.games.push_back(std::move(listing));
				}

				if (resultsLimit.has_value())
				{
					return result.games.size() < resultsLimit.value();
				}
				return true; // continue enumeration
			});

			// call the completionHandlerFunc on the main thread
			wzAsyncExecOnMainThread([completionHandlerFunc, result = std::move(result)]() {
				completionHandlerFunc(result);
			});
		}
		else
		{
			// Parse the (expected) json LobbyError response
			nlohmann::json jsonData;
			try {
				jsonData = nlohmann::json::parse(data->memory, data->memory + data->size);
			}
			catch (const std::exception &e) {
				jsonData = nlohmann::json();
				logExceptionOnMainThread("Failed to parse JSON response: ", e);
				if (httpStatusCode == 429)
				{
					// synthesize expected data for this specific code
					jsonData = nlohmann::json(LobbyError("RATE_LIMITED", "Rate limit exceeded. Please try again later."));
				}
			}

			// Unexpected / failure response code
			wzAsyncExecOnMainThread([completionHandlerFunc, httpStatusCode, jsonData = std::move(jsonData)]() {

				optional<LobbyError> err;
				try {
					err = jsonData.get<LobbyError>();
				}
				catch (const std::exception&) {
					// Failed to parse response as LobbyError - non-fatal
					// (might not have sent a response besides the httpStatusCode)
					if (httpStatusCode == 429)
					{
						// synthesize LobbyError for this specific code
						err = LobbyError("RATE_LIMITED", "Rate limit exceeded. Please try again later.");
					}
				}

				// Log + call resultHandler
				if (err.has_value())
				{
					debug(LOG_LOBBY, "Lobby returned error: (%ld) [%s] %s", httpStatusCode, err.value().errCode.c_str(), err.value().errMessage.c_str());
					completionHandlerFunc(::tl::make_unexpected(err.value()));
				}
				else
				{
					debug(LOG_LOBBY, "Lobby error: (%ld)", httpStatusCode);
					completionHandlerFunc(::tl::make_unexpected(LobbyError("GENERIC_ERROR", astringf("Status Code: %ld", httpStatusCode))));
				}
			});
		}
		return URLRequestHandlingBehavior::Done();
	};

	request.onFailure = [completionHandlerFunc](const std::string& url, URLRequestFailureType type, std::shared_ptr<HTTPResponseDetails> transferDetails) {

		// Failure response - call resultHandler on the main thread
		std::string urlCopy = url;
		long httpStatusCode = 0;
		if (transferDetails)
		{
			httpStatusCode = transferDetails->httpStatusCode();
		}
		wzAsyncExecOnMainThread([completionHandlerFunc, type, httpStatusCode]() {
			debug(LOG_LOBBY, "Lobby connection error: %s (%ld)", to_string(type), httpStatusCode);

			completionHandlerFunc(::tl::make_unexpected(LobbyError("SERVER_CONNECTION_FAILURE", astringf("Status Code: %ld", httpStatusCode))));
		});

	};

	return urlRequestData(request) != nullptr;
}

// MARK: - Requesting join details

// MARK: RequestJoinResponse

struct RequestJoinResponse
{
	std::vector<ConnectionInfo> connections;
	std::string joinToken;
};

void from_json(const nlohmann::json& j, RequestJoinResponse& v)
{
	v.connections = j.at("connections").get<std::vector<ConnectionInfo>>();
	v.joinToken = j.at("joinToken").get<std::string>();
}

// MARK: RequestJoinReturnedConnectionInfo

struct RequestJoinReturnedConnectionInfo
{
	ConnectionType type;
	std::string host;
	uint16_t port = 0;
};

void from_json(const nlohmann::json& j, RequestJoinReturnedConnectionInfo& v)
{
	v.type = j.at("type").get<ConnectionType>();
	v.host = j.at("host").get<std::string>();
	v.port = j.at("port").get<uint16_t>();
}

// MARK: PendingJoinRequestData

struct PendingIPVersionJoinRequestStatus
{
	bool received = false;
};

struct PendingJoinRequestData
{
public:
	PendingJoinRequestData(std::vector<IPVersion> ipVersions, RequestJoinResultHandlerFunc resultHandler);
public:
	void HandleResponseSuccess(IPVersion ver, nlohmann::json &&jsonData);
	void HandleResponseError(IPVersion ver, long httpStatusCode, nlohmann::json &&jsonData);
	void HandleFailure(IPVersion ver, URLRequestFailureType failureType, const std::string& internalErrorReason);

private:
	void DispatchResultsIfReady();

private:
	// must be called while lock is held!
	bool isReady();

private:
	std::mutex mu;
	std::unordered_map<IPVersion, PendingIPVersionJoinRequestStatus> pendingIPVersionRequests;
	RequestJoinDetailsResults pendingResults;
	RequestJoinResultHandlerFunc resultHandler;
};

PendingJoinRequestData::PendingJoinRequestData(std::vector<IPVersion> ipVersions, RequestJoinResultHandlerFunc resultHandler)
: resultHandler(resultHandler)
{
	for (auto ver : ipVersions)
	{
		pendingIPVersionRequests[ver] = PendingIPVersionJoinRequestStatus();
	}
}

void PendingJoinRequestData::HandleResponseSuccess(IPVersion ver, nlohmann::json &&jsonData)
{
	bool isNowReady = false;
	{
		std::lock_guard guard(mu);
		if (!pendingIPVersionRequests[ver].received)
		{
			pendingIPVersionRequests[ver].received = true;

			optional<RequestJoinResponse> responseOpt;
			try {
				responseOpt = jsonData.get<RequestJoinResponse>();
			}
			catch (const std::exception&) {
				// Failed to parse response as RequestJoinResponse
				responseOpt = nullopt;
			}

			if (responseOpt.has_value())
			{
				// Add to pendingResults
				const auto& response = responseOpt.value();
				for (const auto& conn : response.connections)
				{
					pendingResults.joinDetails.connections.push_back(conn);
					pendingResults.joinDetails.joinTokens.push_back(response.joinToken);
				}
			}
			else
			{
				// Failed to parse response as RequestJoinResponse - Record as error
				pendingResults.errors[ver] = LobbyError("RESPONSE_PARSE_ERROR", "Invalid or unrecognized response");
			}
		}
		else
		{
			// ERROR - received twice for same ipversion?!?
		}
		isNowReady = isReady();
	}
	if (isNowReady) { DispatchResultsIfReady(); }
}

void PendingJoinRequestData::HandleResponseError(IPVersion ver, long httpStatusCode, nlohmann::json &&jsonData)
{
	bool isNowReady = false;
	{
		std::lock_guard guard(mu);
		if (!pendingIPVersionRequests[ver].received)
		{
			pendingIPVersionRequests[ver].received = true;

			// Try to decode LobbyError json
			optional<LobbyError> err;
			try {
				err = jsonData.get<LobbyError>();
			}
			catch (const std::exception&) {
				// Failed to parse response as LobbyError - non-fatal
				// (might not have sent a response besides the httpStatusCode)
			}

			if (err.has_value())
			{
				pendingResults.errors[ver] = err.value();
			}
			else
			{
				pendingResults.errors[ver] = LobbyError("GENERIC_ERROR", astringf("Status Code: %ld", httpStatusCode));
			}
		}
		isNowReady = isReady();
	}
	if (isNowReady) { DispatchResultsIfReady(); }
}

static RequestJoinDetailsResults::ConnectivityFailureType JoinConnectivityFailureFromURLRequestFailure(URLRequestFailureType failureType)
{
	switch (failureType)
	{
		case URLRequestFailureType::OPERATION_TIMEOUT:
			return RequestJoinDetailsResults::ConnectivityFailureType::ERR_TIMEOUT;
		case URLRequestFailureType::COULDNT_CONNECT:
			return RequestJoinDetailsResults::ConnectivityFailureType::ERR_COULD_NOT_CONNECT;
		default:
			break;
	}
	return RequestJoinDetailsResults::ConnectivityFailureType::ERR_OTHER;
}

void PendingJoinRequestData::HandleFailure(IPVersion ver, URLRequestFailureType failureType, const std::string& internalErrorReason)
{
	bool isNowReady = false;
	{
		std::lock_guard guard(mu);
		if (!pendingIPVersionRequests[ver].received)
		{
			pendingIPVersionRequests[ver].received = true;

			pendingResults.errors[ver] = RequestJoinDetailsResults::ConnectivityFailure{JoinConnectivityFailureFromURLRequestFailure(failureType), internalErrorReason};
		}
		isNowReady = isReady();
	}
	if (isNowReady) { DispatchResultsIfReady(); }
}

// IMPORTANT: must be called while lock is held!
bool PendingJoinRequestData::isReady()
{
	return std::all_of(pendingIPVersionRequests.begin(), pendingIPVersionRequests.end(), [](const auto& pair) -> bool {
		return pair.second.received;
	});
}

void PendingJoinRequestData::DispatchResultsIfReady()
{
	optional<RequestJoinDetailsResults> readyResults = nullopt;
	{
		std::lock_guard guard(mu);
		if (isReady())
		{
			readyResults = std::move(pendingResults);
			pendingResults = RequestJoinDetailsResults();
		}
	}

	if (readyResults.has_value())
	{
		// Dispatch results to resultHandler on main thread
		wzAsyncExecOnMainThread([readyResults, resultHandlerCopy = resultHandler]() {
			resultHandlerCopy(readyResults.value());
		});
	}
}

bool RequestJoinDetails(const std::string& lobbyServerAddress, const std::string& lobbyGameId, const WzString& playerName, const EcKey& playerIdentity, bool asSpectator, RequestJoinResultHandlerFunc resultHandler, optional<IPVersion> optIpVersion, std::shared_ptr<LobbyErrorResolutionData> errorResolution)
{
	URLDataRequest baseRequest;

	// We always want direct connections to the lobby server (skipping any locally-configured proxies or redirects)
	baseRequest.noProxy = true;
	baseRequest.noFollowRedirects = true;

	baseRequest.userAgent = getWZUserAgent();

	baseRequest.setRequestHeader("Origin", "wz2100://wz2100.net");
	baseRequest.setRequestHeader("Referer", "https://github.com/Warzone2100/warzone2100?wz_internal=request.join");
	baseRequest.setRequestHeader("Accept", "application/json");

	baseRequest.maxDownloadSizeLimit = 2 * 1024 * 1024; // response should never be > 2 MB

	baseRequest.url = buildLobbyRequestBaseUrl(lobbyServerAddress) + "/api/v1/game/" + lobbyGameId + "/request-join";

	baseRequest.connectTimeoutMs = 10000;

	baseRequest.autoRetryStrategy = URLRequestAutoRetryStrategy::RetryAfter(std::chrono::milliseconds(2000), 1, {HttpStatusTooManyRequests});

	// construct request body, and setPost
	nlohmann::ordered_json requestBody = nlohmann::ordered_json::object();
	requestBody["asSpec"] = asSpectator;
	if (isTrustedLobbyAddress(lobbyServerAddress))
	{
		auto attestation = nlohmann::ordered_json(RequestAttestation(playerName, playerIdentity, lobbyServerAddress, lobbyGameId));
		ASSERT_OR_RETURN(false, attestation.is_object(), "Not an object?");
		requestBody["attest"] = attestation;
	}
	if (errorResolution != nullptr)
	{
		// Get errorResolution data json and append to sent body
		auto errorResolutionJsonData = std::dynamic_pointer_cast<LobbyErrorResolutionJsonData>(errorResolution);
		if (errorResolutionJsonData != nullptr)
		{
			requestBody["captcha"] = errorResolutionJsonData->jsonData;
		}
		else
		{
			debug(LOG_LOBBY, "errorResolution was unexpected type?");
		}
	}
	baseRequest.setPost(requestBody.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace));

	// Determine how many requests to dispatch, depending on optIpVersion
	std::vector<IPVersion> requestIPVersions;
	if (!optIpVersion.has_value())
	{
		// Request both IPv4 and IPv6 results
		requestIPVersions.push_back(IPVersion::IPv4);
		requestIPVersions.push_back(IPVersion::IPv6);
	}
	else
	{
		requestIPVersions.push_back(optIpVersion.value());
	}

	auto sharedPendingData = std::make_shared<PendingJoinRequestData>(requestIPVersions, resultHandler);

	auto buildResponseHandler = [sharedPendingData](IPVersion ipVersion) {
		return [sharedPendingData, ipVersion](const std::string& url, const HTTPResponseDetails& responseDetails, const std::shared_ptr<MemoryStruct>& data) -> URLRequestHandlingBehavior
		{
			long httpStatusCode = responseDetails.httpStatusCode();

			// Parse the (expected) json response
			nlohmann::json jsonData;
			try {
				jsonData = nlohmann::json::parse(data->memory, data->memory + data->size);
			}
			catch (const std::exception &e) {
				jsonData = nlohmann::json();
				logExceptionOnMainThread("Failed to parse JSON response: ", e);
			}

			if (httpStatusCode == 200)
			{
				sharedPendingData->HandleResponseSuccess(ipVersion, std::move(jsonData));
			}
			else
			{
				sharedPendingData->HandleResponseError(ipVersion, httpStatusCode, std::move(jsonData));
			}
			return URLRequestHandlingBehavior::Done();
		};
	};

	auto buildFailureHandler = [sharedPendingData](IPVersion ipVersion) {
		return [sharedPendingData, ipVersion](const std::string& url, URLRequestFailureType type, std::shared_ptr<HTTPResponseDetails> transferDetails) {
			sharedPendingData->HandleFailure(ipVersion, type, (transferDetails) ? transferDetails->getInternalResultDescription() : std::string());
		};
	};

	size_t requestInitFailures = 0;
	for (auto ver : requestIPVersions)
	{
		auto request = baseRequest;
		request.protocol = ipVersionToInternetProtocol(ver);

		request.onResponse = buildResponseHandler(ver);
		request.onFailure = buildFailureHandler(ver);

		if (!urlRequestData(request))
		{
			// Failed to dispatch request!
			++requestInitFailures;
			sharedPendingData->HandleFailure(ver, URLRequestFailureType::TRANSFER_FAILED, "Unable to dispatch request");
		}
	}

	return requestInitFailures < requestIPVersions.size();
}

} // namespace netlobby
