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

#include "gns_connection_poll_group.h"

#include "lib/netplay/gns/gns_connection_poll_group.h"
#include "lib/netplay/gns/gns_client_connection.h"
#include "lib/netplay/gns/gns_error_categories.h"

#include "lib/framework/frame.h" // for `ASSERT`

#include <unordered_set>

#include <steam/isteamnetworkingsockets.h>

namespace gns
{

GNSConnectionPollGroup::GNSConnectionPollGroup(ISteamNetworkingSockets* networkInterface, HSteamNetPollGroup group)
	: networkInterface_(networkInterface), group_(group)
{
	ASSERT(networkInterface_ != nullptr && group_ != k_HSteamNetPollGroup_Invalid, "Invalid internal GNS handles");
}

GNSConnectionPollGroup::~GNSConnectionPollGroup()
{
	ASSERT(networkInterface_->DestroyPollGroup(group_) == true, "Passed invalid group handle");
}

net::result<int> GNSConnectionPollGroup::checkConnectionsReadable(std::chrono::milliseconds timeout)
{
	if (connections_.empty())
	{
		return 0;
	}
	constexpr size_t MAX_PENDING_BYTES_PER_CONN = 512 * 1024;
	constexpr size_t MAX_PENDING_MESSAGES_PER_CONN = 1024;
	readyConns_.clear();
	for (const auto& it : connections_)
	{
		GNSClientConnection* conn = it.second;
		if (!conn->isValid())
		{
			continue;
		}
		while (conn->pendingBytes() < MAX_PENDING_BYTES_PER_CONN && conn->pendingMessageCount() < MAX_PENDING_MESSAGES_PER_CONN)
		{
			SteamNetworkingMessage_t* msg = nullptr;
			if (networkInterface_->ReceiveMessagesOnConnection(it.first, &msg, 1) != 1 || msg == nullptr)
			{
				break;
			}
			conn->enqueueMessage(msg);
			readyConns_.emplace(it.first);
		}
	}
	if (readyConns_.empty())
	{
		return connections_.size();
	}
	return readyConns_.size();
}

void GNSConnectionPollGroup::add(IClientConnection* conn)
{
	auto* gnsConn = dynamic_cast<GNSClientConnection*>(conn);
	ASSERT_OR_RETURN(, gnsConn != nullptr, "Expected GNSClientConnection instance");
	ASSERT_OR_RETURN(, gnsConn->getPollGroup() == nullptr, "Connection belongs to another poll group");
	ASSERT_OR_RETURN(, gnsConn->isValid(), "Invalid GNS client connection handle");
	ASSERT_OR_RETURN(, networkInterface_->SetConnectionPollGroup(gnsConn->connectionHandle(), group_),
		"Failed to assign connection %u to poll group %u", gnsConn->connectionHandle(), group_);
	connections_.emplace(gnsConn->connectionHandle(), gnsConn);
	gnsConn->setPollGroup(this);
}

void GNSConnectionPollGroup::remove(IClientConnection* conn)
{
	auto* gnsConn = dynamic_cast<GNSClientConnection*>(conn);
	ASSERT_OR_RETURN(, gnsConn != nullptr, "Expected GNSClientConnection instance");
	if (gnsConn->isValid())
	{
		ASSERT(networkInterface_->SetConnectionPollGroup(gnsConn->connectionHandle(), k_HSteamNetPollGroup_Invalid),
			"Failed to remove connection %u from poll group %u", gnsConn->connectionHandle(), group_);
		connections_.erase(gnsConn->connectionHandle());
	}
	gnsConn->setPollGroup(nullptr);
}

} // namespace gns
