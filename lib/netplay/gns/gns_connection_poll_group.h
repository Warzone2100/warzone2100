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

#include "lib/netplay/connection_poll_group.h"

#include <unordered_map>
#include <unordered_set>

#include <steam/steamnetworkingtypes.h>

class ISteamNetworkingSockets;

namespace gns
{

class GNSClientConnection;

/// <summary>
/// GNS-specific implementation of the `IConnectionPollGroup` interface.
///
/// Manages a group of GNS client connections, and polls them for
/// incoming messages, enqueueing them to the respective connection
/// queues.
/// </summary>
class GNSConnectionPollGroup : public IConnectionPollGroup
{
public:

	explicit GNSConnectionPollGroup(ISteamNetworkingSockets* networkInterface, HSteamNetPollGroup group);
	virtual ~GNSConnectionPollGroup() override;

	virtual net::result<int> checkConnectionsReadable(std::chrono::milliseconds timeout) override;

	virtual void add(IClientConnection* conn) override;
	virtual void remove(IClientConnection* conn) override;

private:

	ISteamNetworkingSockets* networkInterface_ = nullptr;
	HSteamNetPollGroup group_ = k_HSteamNetPollGroup_Invalid;
	std::unordered_map<HSteamNetConnection, GNSClientConnection*> connections_;
	std::unordered_set<HSteamNetConnection> readyConns_;
};

} // namespace gns
