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

#include "tcp_connection_provider.h"

#include "lib/netplay/tcp/netsocket.h"
#include "lib/netplay/tcp/tcp_connection_address.h"
#include "lib/netplay/tcp/tcp_connection_poll_group.h"
#include "lib/netplay/tcp/tcp_client_connection.h"
#include "lib/netplay/tcp/tcp_listen_socket.h"

#include "lib/netplay/open_connection_result.h"
#include "lib/framework/wzapp.h"

#ifdef WZ_OS_WIN
# include "lib/netplay/tcp/select_descriptor_set.h"
#else
# include "lib/netplay/tcp/poll_descriptor_set.h"
#endif

namespace tcp
{

void TCPConnectionProvider::initialize()
{
	SOCKETinit();
}

void TCPConnectionProvider::shutdown()
{
	SOCKETshutdown();
}

net::result<std::unique_ptr<IConnectionAddress>> TCPConnectionProvider::resolveHost(const char* host, uint16_t port)
{
	auto resolved = tcp::resolveHost(host, port);
	if (!resolved.has_value())
	{
		return tl::make_unexpected(resolved.error());
	}
	return std::make_unique<TCPConnectionAddress>(resolved.value());
}

net::result<IListenSocket*> TCPConnectionProvider::openListenSocket(uint16_t port)
{
	auto res = tcp::socketListen(port);
	if (!res.has_value())
	{
		return tl::make_unexpected(res.error());
	}
	return new TCPListenSocket(*this, res.value());
}

net::result<IClientConnection*> TCPConnectionProvider::openClientConnectionAny(const IConnectionAddress& addr, unsigned timeout)
{
	const auto* tcpAddr = dynamic_cast<const TCPConnectionAddress*>(&addr);
	ASSERT(tcpAddr != nullptr, "Expected TCPConnectionAddress instance");
	if (!tcpAddr)
	{
		throw std::runtime_error("Expected TCPConnectionAddress instance");
	}
	const auto* rawAddr = tcpAddr->asRawSocketAddress();
	auto res = socketOpenAny(rawAddr, timeout);
	if (!res.has_value())
	{
		return tl::make_unexpected(res.error());
	}
	return new TCPClientConnection(*this, res.value());
}

IConnectionPollGroup* TCPConnectionProvider::newConnectionPollGroup()
{
	return new TCPConnectionPollGroup(*this);
}

std::unique_ptr<IDescriptorSet> TCPConnectionProvider::newDescriptorSet(PollEventType eventType)
{
	// For now, use `select()` on Windows instead of `poll()` because of a bug in
	// Windows versions prior to "Windows 10 2004", which can lead to `poll()`
	// function timing out on socket connection errors instead of returning an error early.
	//
	// For more information on the bug, see: https://stackoverflow.com/questions/21653003/is-this-wsapoll-bug-for-non-blocking-sockets-fixed
	// and also https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsapoll#remarks
	switch (eventType)
	{
	case PollEventType::READABLE:
	{
#ifdef WZ_OS_WIN
		IDescriptorSet* rawPtr = new tcp::SelectDescriptorSet<PollEventType::READABLE>();
		return std::unique_ptr<IDescriptorSet>(rawPtr);
#else
		IDescriptorSet* rawPtr = new tcp::PollDescriptorSet<PollEventType::READABLE>();
		return std::unique_ptr<IDescriptorSet>(rawPtr);
#endif
	}
	case PollEventType::WRITABLE:
	{
#ifdef WZ_OS_WIN
		IDescriptorSet* rawPtr = new tcp::SelectDescriptorSet<PollEventType::WRITABLE>();
		return std::unique_ptr<IDescriptorSet>(rawPtr);
#else
		IDescriptorSet* rawPtr = new tcp::PollDescriptorSet<PollEventType::WRITABLE>();
		return std::unique_ptr<IDescriptorSet>(rawPtr);
#endif
	}
	default:
		ASSERT(false, "Unexpected PollEventType value: %d", static_cast<int>(eventType));
		return nullptr;
	}
}

} // namespace tcp
