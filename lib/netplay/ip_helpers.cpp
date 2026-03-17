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

#include "ip_helpers.h"
#include "lib/framework/wzglobal.h"

#if defined(WZ_OS_WIN)
# include <winsock2.h>
# include <ws2ipdef.h>
# include <ws2tcpip.h>
#elif defined(WZ_OS_UNIX)
# include <arpa/inet.h>
# include <netinet/in.h>
#endif

# include <cstring> // std::memcpy

const char* to_string(IPFormat ipFormat)
{
	switch (ipFormat)
	{
		case IPFormat::IPv4:
			return "ip4";
		case IPFormat::IPv6:
			return "ip6";
		case IPFormat::IPv4InIPv6:
			return "ip4in6";
	}
	return ""; // silence gcc warning
}

optional<IPClassification> getIPv4Classification(const struct in_addr& addr4)
{
	// Convert to host byte order
	uint32_t host_addr = ntohl(addr4.s_addr);
	uint8_t b1 = (host_addr >> 24) & 0xFF;
	uint8_t b2 = (host_addr >> 16) & 0xFF;

	// Private IPv4 Ranges (RFC 1918)
	// 10.0.0.0/8
	if (b1 == 10) return IPClassification::PrivateOrULA;
	// 172.16.0.0/12
	if (b1 == 172 && (b2 >= 16 && b2 <= 31)) return IPClassification::PrivateOrULA;
	// 192.168.0.0/16
	if (b1 == 192 && b2 == 168) return IPClassification::PrivateOrULA;

	// Link-Local: 169.254.0.0/16
	if (b1 == 169 && b2 == 254) return IPClassification::LinkLocal;

	// Loopback: 127.0.0.0/8
	if (b1 == 127) return IPClassification::Loopback;

	return IPClassification::Other;
}

optional<IPClassification> GetIPClassification(const char *ipAddress)
{
	if (ipAddress == nullptr)
	{
		return nullopt;
	}

	struct in6_addr addr6;
	if (inet_pton(AF_INET6, ipAddress, &addr6) > 0)
	{
		if (!IN6_IS_ADDR_V4MAPPED(&addr6))
		{
			// Process IPv6 Address

			// Link-local: fe80::/10 (First byte is 0xFE, second byte has high bits 10xx)
			if (IN6_IS_ADDR_LINKLOCAL(&addr6)) return IPClassification::LinkLocal;

			// Unique Local (ULA): fc00::/7 (First byte is 0xFC or 0xFD)
			if ((addr6.s6_addr[0] & 0xFE) == 0xFC) return IPClassification::PrivateOrULA;

			// Loopback: ::1 (all zeros except last byte is 1)
			if (IN6_IS_ADDR_LOOPBACK(&addr6)) return IPClassification::Loopback;

			return IPClassification::Other;
		}
		else
		{
			// Unmap IPv4 address & fall-through to processing IPv4 address
			struct in_addr addr4;
			std::memcpy(&addr4.s_addr, addr6.s6_addr + 12, sizeof(addr4.s_addr));
			return getIPv4Classification(addr4);
		}
	}

	struct in_addr addr4;
	if (inet_pton(AF_INET, ipAddress, &addr4) > 0)
	{
		return getIPv4Classification(addr4);
	}

	return nullopt;
}

optional<IPFormat> GetIPFormat(const char *ipAddress)
{
	if (ipAddress == nullptr)
	{
		return nullopt;
	}

	struct in6_addr addr6;
	if (inet_pton(AF_INET6, ipAddress, &addr6) > 0)
	{
		if (!IN6_IS_ADDR_V4MAPPED(&addr6))
		{
			// Actual IPv6 Address
			return IPFormat::IPv6;
		}
		else
		{
			// IPv4-mapped IPv6 address
			return IPFormat::IPv4InIPv6;
		}
	}

	struct in_addr addr4;
	if (inet_pton(AF_INET, ipAddress, &addr4) > 0)
	{
		return IPFormat::IPv4;
	}

	return nullopt;
}
