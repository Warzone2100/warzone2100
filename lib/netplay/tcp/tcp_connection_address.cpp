// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2024-2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#include "lib/netplay/tcp/tcp_connection_address.h"

#include "lib/netplay/tcp/sock_error.h"
#include "lib/netplay/tcp/netsocket.h" // for `freeaddrinfo`
#include "lib/netplay/error_categories.h"
#include "lib/framework/frame.h" // for `ASSERT`

#include <fmt/format.h>

#if defined(WZ_OS_WIN)
# include <winsock2.h>
# include <ws2ipdef.h>
# include <ws2tcpip.h>
#elif defined(WZ_OS_UNIX)
# include <arpa/inet.h>
# include <netinet/in.h>
#endif

namespace tcp
{

TCPConnectionAddress::TCPConnectionAddress(SocketAddress* addr)
	: addr_(addr)
{}

TCPConnectionAddress::~TCPConnectionAddress()
{
	ASSERT(addr_ != nullptr, "Invalid addrinfo stored in the connection address");
	freeaddrinfo(addr_);
}

net::result<std::string> TCPConnectionAddress::toString() const
{
	ASSERT_OR_RETURN(tl::make_unexpected(make_network_error_code(EINVAL)), addr_ != nullptr, "Invalid addrinfo");
	if (addr_->ai_family == AF_INET)
	{
		struct sockaddr_in ipv4_addr;
		memcpy(&ipv4_addr, addr_->ai_addr, sizeof(ipv4_addr));
		char ipStr[INET_ADDRSTRLEN] = {};
		if (inet_ntop(addr_->ai_family, &ipv4_addr.sin_addr, ipStr, INET_ADDRSTRLEN))
		{
			return fmt::format("{}:{}", ipStr, ntohs(ipv4_addr.sin_port));
		}
		return tl::make_unexpected(make_network_error_code(tcp::getSockErr()));
	}
	else if (addr_->ai_family == AF_INET6)
	{
		struct sockaddr_in6 ipv6_addr;
		memcpy(&ipv6_addr, addr_->ai_addr, sizeof(ipv6_addr));
		char ipStr[INET6_ADDRSTRLEN] = {};
		if (inet_ntop(addr_->ai_family, &ipv6_addr.sin6_addr, ipStr, INET6_ADDRSTRLEN))
		{
			return fmt::format("[{}]:{}", ipStr, ntohs(ipv6_addr.sin6_port));
		}
		return tl::make_unexpected(make_network_error_code(tcp::getSockErr()));
	}
	return tl::make_unexpected(make_network_error_code(EFAULT)); // Unknown family
}

} // namespace tcp
