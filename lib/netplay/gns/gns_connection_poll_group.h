// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
