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

#pragma once

#include "lib/netplay/connection_address.h"

#if defined WZ_OS_UNIX
# include <netdb.h>
#elif defined WZ_OS_WIN
# include <ws2tcpip.h>
#endif

typedef struct addrinfo SocketAddress;

namespace tcp
{

/// <summary>
/// TCP-specific implementation of the `IConnectionAddress` interface.
///
/// Internally, stores a pointer to a `struct addrinfo` object, which is used to
/// represent a network address.
/// </summary>
class TCPConnectionAddress : public IConnectionAddress
{
public:

	/// Assumes ownership of `addr`
	explicit TCPConnectionAddress(SocketAddress* addr);
	virtual ~TCPConnectionAddress() override;

	// NOTE: The lifetime of the returned `addrinfo` struct is bounded by the parent object's lifetime!
	const SocketAddress* asRawSocketAddress() const { return addr_; }

	/// <summary>
	/// Converts the address to a generic string representation, containing
	/// both the IP address and the port number ("IP:port" in case of IPv4, "[IP]:port" in case of IPv6).
	/// </summary>
	virtual net::result<std::string> toString() const override;

private:

	SocketAddress* addr_;
};

} // namespace tcp
