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
	return new TCPListenSocket(res.value());
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
	return new TCPClientConnection(res.value());
}

namespace
{

OpenConnectionResult socketOpenTCPConnectionSync(const char* host, uint32_t port)
{
	const auto hostsResult = resolveHost(host, port);
	SocketAddress* hosts = hostsResult.value_or(nullptr);
	if (hosts == nullptr)
	{
		const auto hostsErr = hostsResult.error();
		const auto hostsErrMsg = hostsErr.message();
		return OpenConnectionResult(hostsErr, astringf("Cannot resolve host \"%s\": [%d]: %s", host, hostsErr.value(), hostsErrMsg.c_str()));
	}

	auto sockResult = socketOpenAny(hosts, 15000);
	Socket* client_transient_socket = sockResult.value_or(nullptr);
	deleteSocketAddress(hosts);
	hosts = nullptr;

	if (client_transient_socket == nullptr)
	{
		const auto errValue = sockResult.error();
		const auto errMsg = errValue.message();
		return OpenConnectionResult(errValue, astringf("Cannot connect to [%s]:%d, [%d]:%s", host, port, errValue.value(), errMsg.c_str()));
	}

	return OpenConnectionResult(new TCPClientConnection(client_transient_socket));
}

struct OpenConnectionRequest
{
	std::string host;
	uint32_t port = 0;
	OpenConnectionToHostResultCallback callback;
};

static int openDirectTCPConnectionAsyncImpl(void* data)
{
	OpenConnectionRequest* pRequestInfo = (OpenConnectionRequest*)data;
	if (!pRequestInfo)
	{
		return 1;
	}

	pRequestInfo->callback(socketOpenTCPConnectionSync(pRequestInfo->host.c_str(), pRequestInfo->port));
	delete pRequestInfo;
	return 0;
}

} // anonymous namespace

bool TCPConnectionProvider::openClientConnectionAsync(const std::string& host, uint32_t port, OpenConnectionToHostResultCallback callback)
{
	// spawn background thread to handle this
	auto pRequest = new OpenConnectionRequest();
	pRequest->host = host;
	pRequest->port = port;
	pRequest->callback = callback;

	WZ_THREAD* pOpenConnectionThread = wzThreadCreate(openDirectTCPConnectionAsyncImpl, pRequest);
	if (pOpenConnectionThread == nullptr)
	{
		debug(LOG_ERROR, "Failed to create thread for opening connection");
		delete pRequest;
		return false;
	}

	wzThreadDetach(pOpenConnectionThread);
	// the thread handles deleting pRequest
	pOpenConnectionThread = nullptr;

	return true;
}

IConnectionPollGroup* TCPConnectionProvider::newConnectionPollGroup()
{
	auto* sset = allocSocketSet();
	if (!sset)
	{
		return nullptr;
	}
	return new TCPConnectionPollGroup(sset);
}

} // namespace tcp
