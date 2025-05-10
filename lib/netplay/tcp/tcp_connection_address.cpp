/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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
		struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(addr_->ai_addr);
		char ipStr[INET_ADDRSTRLEN] = {};
		if (inet_ntop(addr_->ai_family, &sin->sin_addr, ipStr, INET_ADDRSTRLEN))
		{
			return fmt::format("{}:{}", ipStr, ntohs(sin->sin_port));
		}
		return tl::make_unexpected(make_network_error_code(tcp::getSockErr()));
	}
	else if (addr_->ai_family == AF_INET6)
	{
		struct sockaddr_in6* sin = reinterpret_cast<struct sockaddr_in6*>(addr_->ai_addr);
		char ipStr[INET6_ADDRSTRLEN] = {};
		if (inet_ntop(addr_->ai_family, &sin->sin6_addr, ipStr, INET_ADDRSTRLEN))
		{
			return fmt::format("[{}]:{}", ipStr, ntohs(sin->sin6_port));
		}
		return tl::make_unexpected(make_network_error_code(tcp::getSockErr()));
	}
	return tl::make_unexpected(make_network_error_code(EFAULT)); // Unknown family
}

} // namespace tcp
