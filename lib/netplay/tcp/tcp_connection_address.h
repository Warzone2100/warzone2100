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

class TCPConnectionAddress : public IConnectionAddress
{
public:

	/// Assumes ownership of `addr`
	explicit TCPConnectionAddress(SocketAddress* addr);
	virtual ~TCPConnectionAddress() override;

	// NOTE: The lifetime of the returned `addrinfo` struct is bounded by the parent object's lifetime!
	const SocketAddress* asRawSocketAddress() const { return addr_; }

private:

	SocketAddress* addr_;
};
