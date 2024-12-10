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

#include "lib/netplay/tcp/tcp_client_connection.h"
#include "lib/netplay/tcp/netsocket.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/debug.h"
#include "lib/framework/string_ext.h"

namespace tcp
{

TCPClientConnection::TCPClientConnection(Socket* rawSocket)
	: socket_(rawSocket)
{
	ASSERT(socket_ != nullptr, "Null socket passed to TCPClientConnection ctor");
}

TCPClientConnection::~TCPClientConnection()
{
	if (socket_)
	{
		socketClose(socket_);
	}
}

net::result<ssize_t> TCPClientConnection::readAll(void* buf, size_t size, unsigned timeout)
{
	return tcp::readAll(*socket_, buf, size, timeout);
}

net::result<ssize_t> TCPClientConnection::readNoInt(void* buf, size_t maxSize, size_t* rawByteCount)
{
	return tcp::readNoInt(*socket_, buf, maxSize, rawByteCount);
}

net::result<ssize_t> TCPClientConnection::writeAll(const void* buf, size_t size, size_t* rawByteCount)
{
	return tcp::writeAll(*socket_, buf, size, rawByteCount);
}

bool TCPClientConnection::readReady() const
{
	return socketReadReady(*socket_);
}

void TCPClientConnection::flush(size_t* rawByteCount)
{
	socketFlush(*socket_, std::numeric_limits<uint8_t>::max()/*unused*/, rawByteCount);
}

void TCPClientConnection::enableCompression()
{
	socketBeginCompression(*socket_);
}

void TCPClientConnection::useNagleAlgorithm(bool enable)
{
	socketSetTCPNoDelay(*socket_, !enable);
}

std::string TCPClientConnection::textAddress() const
{
	return getSocketTextAddress(*socket_);
}

} // namespace tcp
