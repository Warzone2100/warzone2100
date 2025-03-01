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

#include "lib/netplay/tcp/tcp_listen_socket.h"
#include "lib/netplay/tcp/tcp_client_connection.h"
#include "lib/netplay/tcp/netsocket.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/debug.h"

namespace tcp
{

TCPListenSocket::TCPListenSocket(WzConnectionProvider& connProvider, WzCompressionProvider& compressionProvider, PendingWritesManager& pwm, Socket* rawSocket)
	: IListenSocket(connProvider, compressionProvider, pwm),
	listenSocket_(rawSocket)
{}

TCPListenSocket::~TCPListenSocket()
{
	if (listenSocket_)
	{
		socketCloseNow(listenSocket_);
	}
}

IClientConnection* TCPListenSocket::accept()
{
	ASSERT(listenSocket_ != nullptr, "Internal socket handle shouldn't be null!");
	if (!listenSocket_)
	{
		return nullptr;
	}
	auto* s = socketAccept(listenSocket_);
	if (!s)
	{
		return nullptr;
	}
	return new TCPClientConnection(*connProvider_, *compressionProvider_, *pwm_, s);
}

IListenSocket::IPVersionsMask TCPListenSocket::supportedIpVersions() const
{
	IPVersionsMask resMask = 0;
	if (socketHasIPv4(*listenSocket_))
	{
		resMask |= static_cast<IPVersionsMask>(IPVersions::IPV4);
	}
	if (socketHasIPv6(*listenSocket_))
	{
		resMask |= static_cast<IPVersionsMask>(IPVersions::IPV6);
	}
	return resMask;
}

} // namespace tcp
