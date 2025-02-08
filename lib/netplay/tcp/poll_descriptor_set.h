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

#include "descriptor_set.h"

#include "lib/netplay/nettypes.h" // for MAX_CONNECTED_PLAYERS, MAX_TMP_SOCKETS

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

	virtual bool add(SOCKET fd) override
	{
		constexpr short evt = EventType == PollEventType::READABLE ? POLLIN : POLLOUT;

		assert(size_ < MAX_FDS_COUNT);
		if (size_ >= MAX_FDS_COUNT)
		{
			return false;
		}
		fds_[size_++] = { fd, evt, 0 };
		return true;
	}

	virtual void clear() override
	{
		fds_.fill({});
		size_ = 0;
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

	virtual bool isSet(SOCKET fd) const override
	{
		constexpr short evt = EventType == PollEventType::READABLE ? POLLIN : POLLOUT;

		const auto it = std::find_if(fds_.begin(), fds_.end(), [fd](const pollfd& pfd) { return pfd.fd == fd; });
		return it != fds_.end() && (it->revents & evt);
	}

private:

	int pollImpl(std::chrono::milliseconds timeout)
	{
#ifdef WZ_OS_WIN
		return WSAPoll(fds_.data(), size_, timeout.count());
#else
		return ::poll(fds_.data(), size_, timeout.count());
#endif
	}

	// The size limit directly corresponds to the theoretical limit on
	// the number of connections (from the host side) opened at the same time, which is:
	// `MAX_CONNECTED_PLAYERS` for `connected_bsocket` (in netplay.cpp)
	// `MAX_TMP_SOCKETS` for `tmp_socket_set` (netplay.cpp)
	// another 1 for the `rs_socket` (also in netplay.cpp)
	static constexpr size_t MAX_FDS_COUNT = MAX_CONNECTED_PLAYERS + MAX_TMP_SOCKETS + 1;

	std::array<pollfd, MAX_FDS_COUNT> fds_;
	size_t size_ = 0;
};

} // namespace tcp
