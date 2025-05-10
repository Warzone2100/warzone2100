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

#include "lib/netplay/wz_connection_provider.h"

#include <stdint.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <steam/steamnetworkingtypes.h>

#include "lib/netplay/address_resolver.h"

class ISteamNetworkingSockets;

namespace gns
{

class GNSListenSocket;
class GNSClientConnection;

/// <summary>
/// An implementation of the `WzConnectionProvider` interface
/// for the Valve GameNetworkingSockets (GNS, for short) network backend.
///
/// Holds a reference to the GNS networking interface and manages
/// the lifecycle of GNS listen sockets and client connections.
/// </summary>
class GNSConnectionProvider final : public WzConnectionProvider
{
public:

	virtual void initialize() override;
	virtual void shutdown() override;

	virtual ConnectionProviderType type() const noexcept override;

	/// <summary>
	/// Currently piggy-backs on the `TCPAddressResolver` class
	/// to perform hostname resolution and transforms the TCP-specific
	/// addresses to the form which GNS library understands.
	/// </summary>
	virtual net::result<std::unique_ptr<IConnectionAddress>> resolveHost(const char* host, uint16_t port) const override;

	virtual net::result<IListenSocket*> openListenSocket(uint16_t port) override;

	virtual net::result<IClientConnection*> openClientConnectionAny(const IConnectionAddress& addr, unsigned timeout) override;

	virtual IConnectionPollGroup* newConnectionPollGroup() override;

	virtual std::unique_ptr<IDescriptorSet> newDescriptorSet(PollEventType eventType) override;

	// NOTE: This will trigger `ClientConnectionStateChanged()` and `ServerConnectionStateChanged()`
	// for every managed connection in a blocking manner!
	//
	// DO NOT do anything heavyweight in the callbacks!
	virtual void processConnectionStateChanges() override;

	virtual PortMappingInternetProtocolMask portMappingProtocolTypes() const override;

private:

	static void ServerConnectionStateChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);
	static void ClientConnectionStateChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);

	// Generic cleanup routine for failed client connections (i.e., those that
	// have transitioned into some failure state).
	//
	// This will remove any pending writes for this connection, flush any
	// outstanding messages that are waiting to be sent in the internal GNS queue
	// and discards any unprocessed (but already received) messages.
	//
	// The connection object is automatically removed from its owning poll group (if there's any)
	// and the internal GNS connection handle is closed, so that the connection object
	// will be left in an invalid state (that is, `isValid()` will become `false`).
	void disposeConnection(GNSClientConnection* conn);

	ISteamNetworkingSockets* networkInterface_ = nullptr;
	std::unordered_map<HSteamNetConnection, GNSClientConnection*> activeClients_;
	std::pair<HSteamListenSocket, GNSListenSocket*> activeListenSocket_ = { k_HSteamListenSocket_Invalid, nullptr };
	std::vector<SteamNetworkingConfigValue_t> listenSocketOpts_;
	std::vector<SteamNetworkingConfigValue_t> clientConnOpts_;
	std::unique_ptr<IAddressResolver> addressResolver_;
};

} // namespace tcp
