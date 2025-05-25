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

#include "gns_connection_poll_group.h"

#include "lib/netplay/gns/gns_connection_poll_group.h"
#include "lib/netplay/gns/gns_client_connection.h"
#include "lib/netplay/gns/gns_error_categories.h"

#include "lib/framework/frame.h" // for `ASSERT`

#include <array>
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
	constexpr int MAX_RECV_MESG_COUNT = 1024;
	std::array<SteamNetworkingMessage_t*, MAX_RECV_MESG_COUNT> receivedMsgs;
	auto msgCount = networkInterface_->ReceiveMessagesOnPollGroup(group_, receivedMsgs.data(), MAX_RECV_MESG_COUNT);
	if (msgCount < 0)
	{
		return tl::make_unexpected(make_gns_error_code(-msgCount));
	}
	else if (msgCount == 0)
	{
		return connections_.size();
	}
	readyConns_.clear();
	// Put each message in the corresponding `GNSClientConnection` object's message queue
	for (size_t i = 0; i < msgCount; ++i)
	{
		SteamNetworkingMessage_t* msg = receivedMsgs[i];
		connections_.at(msg->m_conn)->enqueueMessage(msg);
		readyConns_.emplace(msg->m_conn);
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
