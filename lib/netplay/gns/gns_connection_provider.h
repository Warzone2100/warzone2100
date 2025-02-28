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
#include <unordered_set>
#include <utility>
#include <vector>

#include <steam/steamnetworkingtypes.h>

class ISteamNetworkingSockets;

namespace gns
{

class GNSListenSocket;

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

	virtual net::result<std::unique_ptr<IConnectionAddress>> resolveHost(const char* host, uint16_t port) override;

	virtual net::result<IListenSocket*> openListenSocket(uint16_t port) override;

	virtual net::result<IClientConnection*> openClientConnectionAny(const IConnectionAddress& addr, unsigned timeout) override;

	virtual IConnectionPollGroup* newConnectionPollGroup() override;

	virtual std::unique_ptr<IDescriptorSet> newDescriptorSet(PollEventType eventType) override;

	// NOTE: This will trigger `ClientConnectionStateChanged()` and `ServerConnectionStateChanged()`
	// for every managed connection in a blocking manner!
	//
	// DO NOT do anything heavyweight in the callbacks!
	virtual void processConnectionStateChanges() override;

private:

	static void ServerConnectionStateChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);
	static void ClientConnectionStateChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);

	ISteamNetworkingSockets* networkInterface_ = nullptr;
	std::unordered_set<HSteamNetConnection> activeClients_;
	std::pair<HSteamListenSocket, GNSListenSocket*> activeListenSocket_ = { k_HSteamListenSocket_Invalid, nullptr };
	std::vector<SteamNetworkingConfigValue_t> listenSocketOpts_;
	std::vector<SteamNetworkingConfigValue_t> clientConnOpts_;
};

} // namespace tcp
