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

#pragma once

#include "lib/netplay/listen_socket.h"

#include <steam/isteamnetworkingsockets.h>
#include <steam/steamnetworkingtypes.h>

#include <queue>

class WzCompressionProvider;
class WzConnectionProvider;

namespace gns
{

/// <summary>
/// GNS-specific implementation of the `IListenSocket` interface.
///
/// Wraps a `HSteamListenSocket` handle to interact with the underlying
/// library interface.
///
/// Manages a queue of accepted connections, which are already internally
/// accepted the GNS backend, but not yet returned to the user.
///
/// These are then returned to user one-by-one via the `accept()` method.
///
/// The queue is populated by the owning connection provider instance upon
/// processing the pending connection state changes.
/// </summary>
class GNSListenSocket : public IListenSocket
{
public:

	explicit GNSListenSocket(WzConnectionProvider& connProvider, WzCompressionProvider& compressionProvider, PendingWritesManager& pwm,
		ISteamNetworkingSockets* networkInterface, HSteamListenSocket sock);
	virtual ~GNSListenSocket() override;

	virtual IClientConnection* accept() override;
	virtual IPVersionsMask supportedIpVersions() const override;

private:

	void addPendingAcceptedConnection(HSteamNetConnection conn);

	ISteamNetworkingSockets* networkInterface_;
	HSteamListenSocket sock_;
	std::queue<HSteamNetConnection> pendingAcceptedConnections_;

	friend class GNSConnectionProvider;
};

} // namespace gns
