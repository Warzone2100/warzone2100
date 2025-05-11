// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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

#include "gns_listen_socket.h"

#include "lib/netplay/gns/gns_listen_socket.h"
#include "lib/netplay/gns/gns_client_connection.h"
#include "lib/netplay/gns/gns_connection_provider.h"

#include "lib/framework/frame.h" // for `ASSERT`

#include <type_traits>

namespace gns
{

GNSListenSocket::GNSListenSocket(WzConnectionProvider& connProvider, WzCompressionProvider& compressionProvider, PendingWritesManager& pwm,
	ISteamNetworkingSockets* networkInterface, HSteamListenSocket sock)
	: IListenSocket(connProvider, compressionProvider, pwm),
	networkInterface_(networkInterface),
	sock_(sock)
{
	ASSERT(networkInterface_ != nullptr && sock != k_HSteamListenSocket_Invalid, "Invalid internal GNS handles");
}

GNSListenSocket::~GNSListenSocket()
{
	ASSERT(networkInterface_->CloseListenSocket(sock_) == true, "Failed to close listen socket properly");
}

IListenSocket::IPVersionsMask GNSListenSocket::supportedIpVersions() const
{
	using MaskT = std::underlying_type_t<IListenSocket::IPVersions>;
	return static_cast<MaskT>(IListenSocket::IPVersions::IPV4) | static_cast<MaskT>(IListenSocket::IPVersions::IPV6);
}

IClientConnection* GNSListenSocket::accept()
{
	if (pendingAcceptedConnections_.empty())
	{
		return nullptr;
	}
	const auto clientConnHandle = pendingAcceptedConnections_.front();
	pendingAcceptedConnections_.pop();
	auto res = new GNSClientConnection(*connProvider_, *compressionProvider_, *pwm_, networkInterface_, clientConnHandle);
	// Make the connection provider to update its internal mapping, so that it knows `clientConnHandle`
	// has been used to create a new client connection object (so it can properly dispose of it, if something
	// bad happens).
	static_cast<GNSConnectionProvider*>(connProvider_)->registerAcceptedConnection(res);
	return res;
}

void GNSListenSocket::addPendingAcceptedConnection(HSteamNetConnection conn)
{
	pendingAcceptedConnections_.push(conn);
}

} // namespace gns
