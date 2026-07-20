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


#include "multijoin_helpers.h"
#include "lib/framework/frame.h"
#include "lib/framework/debug.h"
#include "lib/netplay/netlobby.h"
#include "screens/joiningscreen.h"
#include "multiint.h"

#include <vector>
#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

// MARK: String parsing helper functions

struct HostAndPort
{
	std::string host;
	uint16_t port = 0;
};

optional<HostAndPort> splitStrIntoHostAndPort(const std::string& input)
{
	size_t lastColon = input.find_last_of(':');
	if (lastColon != std::string::npos && lastColon < input.size() - 1)
	{
		std::string hostPart = input.substr(0, lastColon);
		std::string portPart = input.substr(lastColon + 1);

		// Validate port
		int p;
		try {
			p = std::stoi(portPart);
		} catch (...) {
			return nullopt;
		}
		if (p < 0 || p > 65535)
		{
			return nullopt;
		}

		return HostAndPort{hostPart, static_cast<uint16_t>(p)};
	}

	return nullopt;
}

// MARK: IP address parsing helper functions

#if defined(WZ_OS_WIN)
# include <winsock2.h>
# include <ws2ipdef.h>
# include <ws2tcpip.h>
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netinet/in.h>
#endif

struct IPAddressInfo
{
	netlobby::IPVersion ipVersion;
	std::string ip;
	std::optional<int> port;
};

optional<IPAddressInfo> parseAsIPAddress(const std::string& input)
{
	if (input.empty()) return nullopt;

	struct in_addr ipv4_buf;
	struct in6_addr ipv6_buf;

	// 1. Try raw IPv4
	if (inet_pton(AF_INET, input.c_str(), &ipv4_buf) == 1)
	{
		return IPAddressInfo{ netlobby::IPVersion::IPv4, input, std::nullopt };
	}

	// 2. Try raw IPv6
	if (inet_pton(AF_INET6, input.c_str(), &ipv6_buf) == 1)
	{
		return IPAddressInfo{ netlobby::IPVersion::IPv6, input, std::nullopt };
	}

	// 3. Check for port suffix
	auto optHostAndPort = splitStrIntoHostAndPort(input);
	if (optHostAndPort.has_value())
	{
		const std::string& ipPart = optHostAndPort.value().host;
		const uint16_t p = optHostAndPort.value().port;

		// Check: IPv4 with port (e.g. 1.2.3.4:80)
		if (inet_pton(AF_INET, ipPart.c_str(), &ipv4_buf) == 1)
		{
			return IPAddressInfo{ netlobby::IPVersion::IPv4, ipPart, p };
		}

		// Check: Bracketed IPv6 with port (e.g. [::1]:80)
		if (ipPart.size() > 2 && ipPart.front() == '[' && ipPart.back() == ']')
		{
			std::string rawIp6 = ipPart.substr(1, ipPart.size() - 2);
			if (inet_pton(AF_INET6, rawIp6.c_str(), &ipv6_buf) == 1)
			{
				return IPAddressInfo{ netlobby::IPVersion::IPv6, rawIp6, p };
			}
		}
	}

	return nullopt;
}

// MARK: - Public API

// Start a join with an expected ip, ip:port, hostname, or hostname:port string
// Note: Does not attempt a lobby join
void joinGameFromIPOrHostnameConnectionStr(const std::string& ipOrHostnameConnectionStr, bool asSpectator /*= false*/)
{
	// 1. Try to parse as an IP address or IP:port
	auto parseIPAddressDetails = parseAsIPAddress(ipOrHostnameConnectionStr);
	if (parseIPAddressDetails.has_value())
	{
		// Parsed as an IP address
		// Start joining attempt
		auto host = parseIPAddressDetails.value().ip;
		auto port = parseIPAddressDetails.value().port.value_or(0 /* 0 = try default port */);
		debug(LOG_INFO, "Connecting to ip [%s] port %d", host.c_str(), port);
		joinGameFromIPPort(host, port, asSpectator);
		return;
	}

	// 2. Try to do a very basic split at the last ":", if one is present
	auto optHostAndPort = splitStrIntoHostAndPort(ipOrHostnameConnectionStr);
	if (optHostAndPort.has_value())
	{
		// hopefully hostPart is either a valid ip or hostname - give it a try
		debug(LOG_INFO, "Connecting to host [%s] port %d", optHostAndPort.value().host.c_str(), optHostAndPort.value().port);
		joinGameFromIPPort(optHostAndPort.value().host, optHostAndPort.value().port, asSpectator);
		return;
	}

	// 3. Otherwise, perhaps this is just a hostname (without a port specified) - give that a try with the default port
	joinGameFromIPPort(ipOrHostnameConnectionStr, 0, asSpectator);
}


// Start a join with an explicit ip & port
void joinGameFromIPPort(const std::string& host, uint32_t port, bool asSpectator /*= false*/)
{
	// Construct connection list
	std::vector<JoinConnectionDescription> connList;
	connList.emplace_back(JoinConnectionDescription::JoinConnectionType::TCP_DIRECT, host, port);
#ifdef WZ_GNS_NETWORK_BACKEND_ENABLED
	connList.emplace_back(JoinConnectionDescription::JoinConnectionType::GNS_DIRECT, host, port);
#endif
	// Start joining attempt
	startJoiningAttempt(sPlayer, connList, asSpectator);
}

// Parses a user-provided connection string, which may be either:
// - An IP address or IP:port combination
// - A lobby game ID
// And attempts to start a join
optional<JoinGameFromConnectionStrOutcome> joinGameFromConnectionStr(const std::string& userProvidedConnectionStr, bool asSpectator /*= false*/)
{
	if (userProvidedConnectionStr.empty())
	{
		return nullopt;
	}

	auto parseIPAddressDetails = parseAsIPAddress(userProvidedConnectionStr);
	if (parseIPAddressDetails.has_value())
	{
		// Parsed as an IP address
		// Start joining attempt
		joinGameFromIPPort(parseIPAddressDetails.value().ip, parseIPAddressDetails.value().port.value_or(0 /* 0 = try default port */), asSpectator);
		return JoinGameFromConnectionStrOutcome::Attempting_IP_Join;
	}

	// Otherwise, try treating as a lobby game id
	startLobbyJoiningAttempt(sPlayer, NETgetLobbyserverAddress(), userProvidedConnectionStr, asSpectator);
	return JoinGameFromConnectionStrOutcome::Attempting_Lobby_Join;
}

void joinGame(const std::vector<JoinConnectionDescription>& connection_list, bool asSpectator /*= false*/)
{
	startJoiningAttempt(sPlayer, connection_list, asSpectator);
}

void joinLobbyGame(const std::string& lobbyAddress, const std::string& lobbyGameId, bool asSpectator /*= false*/)
{
	startLobbyJoiningAttempt(sPlayer, lobbyAddress, lobbyGameId, asSpectator);
}
