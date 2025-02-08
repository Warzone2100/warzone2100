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
#include "lib/framework/wzapp.h"
#include "lib/framework/debug.h"

namespace tcp
{

TCPConnectionPollGroup::TCPConnectionPollGroup(SocketSet* sset)
	: sset_(sset)
{}

TCPConnectionPollGroup::~TCPConnectionPollGroup()
{
	if (sset_)
	{
		deleteSocketSet(sset_);
	}
}

int TCPConnectionPollGroup::checkSocketsReadable(unsigned timeout)
{
	return tcp::checkSocketsReadable(*sset_, timeout);
}

void TCPConnectionPollGroup::add(IClientConnection* conn)
{
	auto* tcpConn = dynamic_cast<TCPClientConnection*>(conn);
	ASSERT_OR_RETURN(, tcpConn != nullptr, "Expected to have TCPClientConnection instance");
	SocketSet_AddSocket(*sset_, tcpConn->socket_);
}

void TCPConnectionPollGroup::remove(IClientConnection* conn)
{
	auto tcpConn = dynamic_cast<TCPClientConnection*>(conn);
	ASSERT_OR_RETURN(, tcpConn != nullptr, "Expected to have TCPClientConnection instance");
	SocketSet_DelSocket(*sset_, tcpConn->socket_);
}

} // namespace tcp
