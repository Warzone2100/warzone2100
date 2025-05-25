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
#include "lib/netplay/tcp/netsocket.h"
#include "lib/netplay/tcp/tcp_client_connection.h"

#ifdef WZ_OS_WIN
# include <winsock2.h>
#elif defined(WZ_OS_UNIX)
# include <poll.h> // for pollfd, poll
#endif

#include "lib/netplay/error_categories.h"
#include "lib/netplay/tcp/sock_error.h"

#include <array>
#include <stdexcept>
#include <algorithm>

namespace tcp
{

/// <summary>
/// Descriptor set interface specialization using the `poll()` API for actual polling.
/// </summary>
/// <typeparam name="EventType">Type of updates (readable/writable sockets) to poll for.</typeparam>
template <PollEventType EventType>
class PollDescriptorSet : public IDescriptorSet
{
public:

	explicit PollDescriptorSet() = default;

	virtual bool add(IClientConnection* conn) override
	{
		TCPClientConnection* tcpConn = dynamic_cast<TCPClientConnection*>(conn);
		ASSERT_OR_RETURN(false, tcpConn, "Invalid connection type: expected TCPClientConnection");

		constexpr short evt = EventType == PollEventType::READABLE ? POLLIN : POLLOUT;

		const auto fd = tcpConn->getRawSocketFd();
		const auto it = std::find_if(fds_.begin(), fds_.end(), [fd] (const pollfd& pfd) { return fd == pfd.fd; });
		ASSERT_OR_RETURN(false, it == fds_.end(), "Connection already present in the descriptor set: fd=%d", fd);

		fds_.emplace_back(pollfd{ fd, evt, 0 });
		return true;
	}

	virtual bool remove(IClientConnection* conn) override
	{
		TCPClientConnection* tcpConn = dynamic_cast<TCPClientConnection*>(conn);
		ASSERT_OR_RETURN(false, tcpConn, "Invalid connection type: expected TCPClientConnection");

		const auto fd = tcpConn->getRawSocketFd();
		const auto it = std::find_if(fds_.begin(), fds_.end(), [fd](const pollfd& pfd) { return fd == pfd.fd; });
		if (it != fds_.end())
		{
			fds_.erase(it);
		}
		return true;
	}

	virtual void clear() override
	{
		fds_.clear();
	}

	virtual net::result<int> poll(std::chrono::milliseconds timeout) override
	{
		int ret;
		int sockErr = 0;
		do
		{
			ret = pollImpl(timeout);
			if (ret == SOCKET_ERROR)
			{
				sockErr = getSockErr();
			}
		} while (ret == SOCKET_ERROR && (sockErr == EINTR || sockErr == EAGAIN));

		if (ret == SOCKET_ERROR)
		{
			return tl::make_unexpected(make_network_error_code(sockErr));
		}

		return ret;
	}

	virtual bool isSet(const IClientConnection* conn) const override
	{
		const TCPClientConnection* tcpConn = dynamic_cast<const TCPClientConnection*>(conn);
		ASSERT_OR_RETURN(false, tcpConn, "Invalid connection type: expected TCPClientConnection");

		constexpr short evt = EventType == PollEventType::READABLE ? POLLIN : POLLOUT;

		const auto it = std::find_if(fds_.begin(), fds_.end(), [fd = tcpConn->getRawSocketFd()](const pollfd& pfd) { return pfd.fd == fd; });
		return it != fds_.end() && (it->revents & evt);
	}

	virtual bool empty() const override
	{
		return fds_.empty();
	}

private:

	int pollImpl(std::chrono::milliseconds timeout)
	{
#ifdef WZ_OS_WIN
		return WSAPoll(fds_.data(), fds_.size(), timeout.count());
#else
		return ::poll(fds_.data(), fds_.size(), timeout.count());
#endif
	}

	std::vector<pollfd> fds_;
};

} // namespace tcp
