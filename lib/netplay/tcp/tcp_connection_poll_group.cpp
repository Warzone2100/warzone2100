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

#include "lib/netplay/tcp/tcp_connection_poll_group.h"
#include "lib/netplay/tcp/tcp_client_connection.h"
#include "lib/netplay/tcp/netsocket.h"
#include "lib/netplay/polling_util.h"
#include "lib/netplay/wz_connection_provider.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/debug.h"

#include <algorithm>

namespace tcp
{

TCPConnectionPollGroup::TCPConnectionPollGroup(WzConnectionProvider& connProvider)
	: connProvider_(&connProvider),
	readableSet_(connProvider_->newDescriptorSet(PollEventType::READABLE))
{}

net::result<int> TCPConnectionPollGroup::checkConnectionsReadable(std::chrono::milliseconds timeout)
{
	return ::checkConnectionsReadable(conns_, *readableSet_, timeout);
}

void TCPConnectionPollGroup::add(IClientConnection* conn)
{
	auto* tcpConn = dynamic_cast<TCPClientConnection*>(conn);
	ASSERT_OR_RETURN(, tcpConn != nullptr, "Expected to have TCPClientConnection instance");

	conns_.emplace_back(conn);
	ASSERT(readableSet_->add(conn), "Failed to add connection to internal descriptor set");
}

void TCPConnectionPollGroup::remove(IClientConnection* conn)
{
	auto tcpConn = dynamic_cast<TCPClientConnection*>(conn);
	ASSERT_OR_RETURN(, tcpConn != nullptr, "Expected to have TCPClientConnection instance");
	auto it = std::find(conns_.begin(), conns_.end(), conn);
	if (it != conns_.end())
	{
		conns_.erase(it);
	}
	ASSERT(readableSet_->remove(conn), "Failed to remove connection from internal descriptor set");
}

} // namespace tcp
