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

#include "lib/framework/frame.h" // for ASSERT
#include "lib/netplay/descriptor_set.h"
#include "lib/netplay/tcp/tcp_client_connection.h"
#include "lib/netplay/tcp/netsocket.h"

#ifdef WZ_OS_WIN
# include <winsock2.h>
#elif defined(WZ_OS_UNIX)
# include <sys/select.h> // for fd_set
#endif

#include "lib/netplay/error_categories.h"
#include "lib/netplay/tcp/sock_error.h"

#include <algorithm>

namespace tcp
{

/// <summary>
/// Descriptor set interface specialization using the `select()` API for actual polling.
/// </summary>
/// <typeparam name="EventType">Type of updates (readable/writable sockets) to poll for.</typeparam>
template <PollEventType EventType>
class SelectDescriptorSet : public IDescriptorSet
{
public:

	explicit SelectDescriptorSet()
	{
		FD_ZERO(&fds_);
	}

	virtual bool add(IClientConnection* conn) override
	{
		TCPClientConnection* tcpConn = dynamic_cast<TCPClientConnection*>(conn);
		ASSERT_OR_RETURN(false, tcpConn, "Invalid connection type: expected TCPClientConnection");
		ASSERT_OR_RETURN(false, tcpConn->isValid(), "Connection object is not valid: socket is not opened");

		const SOCKET fd = tcpConn->getRawSocketFd();
		FD_SET(fd, &fds_);
		maxfd_ = std::max(maxfd_, fd);
		return true;
	}

	virtual bool remove(IClientConnection* conn) override
	{
		TCPClientConnection* tcpConn = dynamic_cast<TCPClientConnection*>(conn);
		ASSERT_OR_RETURN(false, tcpConn, "Invalid connection type: expected TCPClientConnection");

		const SOCKET fd = tcpConn->getRawSocketFd();
		FD_CLR(fd, &fds_);
		return true;
	}

	virtual void clear() override
	{
		FD_ZERO(&fds_);
		maxfd_ = 0;
	}

	virtual net::result<int> poll(std::chrono::milliseconds timeout) override
	{
		int ret;
		do
		{
			ret = pollImpl(timeout);
		} while (ret == SOCKET_ERROR && (getSockErr() == EINTR || getSockErr() == EAGAIN));

		if (ret == SOCKET_ERROR)
		{
			return tl::make_unexpected(make_network_error_code(getSockErr()));
		}

		return ret;
	}

	virtual bool isSet(const IClientConnection* conn) const override
	{
		const TCPClientConnection* tcpConn = dynamic_cast<const TCPClientConnection*>(conn);
		ASSERT_OR_RETURN(false, tcpConn, "Invalid connection type: expected TCPClientConnection");

		// Force conversion to `fd_set*` since `FD_ISSET` expects a pointer to non-const `fd_set`.
		return FD_ISSET(tcpConn->getRawSocketFd(), const_cast<fd_set*>(&fds_));
	}

	virtual bool empty() const override
	{
		return maxfd_ == 0;
	}


private:

	int pollImpl(std::chrono::milliseconds timeout)
	{
		const int msCount = timeout.count();
		struct timeval tv = { msCount / 1000, (msCount % 1000) * 1000 };

		switch (EventType)
		{
		case PollEventType::READABLE:
			return select(maxfd_ + 1, &fds_, nullptr, nullptr, &tv);
		case PollEventType::WRITABLE:
			return select(maxfd_ + 1, nullptr, &fds_, nullptr, &tv);
		default:
			return -1;
		}
	}

	fd_set fds_;
	SOCKET maxfd_ = 0;
};

} // namespace tcp
