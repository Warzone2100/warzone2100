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

#include "gns_connection_provider.h"

#include "lib/framework/wzapp.h"

#include "lib/netplay/error_categories.h"
#include "lib/netplay/pending_writes_manager_map.h"
#include "lib/netplay/pending_writes_manager.h"
#include "lib/netplay/port_mapping_manager.h"
#include "lib/netplay/wz_compression_provider.h"

#include "lib/netplay/gns/dummy_descriptor_set.h"
#include "lib/netplay/gns/gns_connection_address.h"
#include "lib/netplay/gns/gns_listen_socket.h"
#include "lib/netplay/gns/gns_client_connection.h"
#include "lib/netplay/gns/gns_connection_poll_group.h"

#include "lib/netplay/tcp/tcp_address_resolver.h"
#include "lib/netplay/tcp/tcp_connection_address.h"

#include <fmt/format.h>
#include <steam/steamnetworkingtypes.h>
#include <steam/isteamnetworkingsockets.h>
#include <steam/steamnetworkingsockets.h>

#include <mutex> // for std::once_flag

namespace gns
{

namespace
{

void setupCommonConnOptions(std::vector<SteamNetworkingConfigValue_t>& opts)
{
	constexpr int32_t SEND_BUFFER_SIZE = 1024 * 1024; // 1 MB
	constexpr int32_t RECV_BUFFER_SIZE = 2 * 1024 * 1024; // 2 MB
	constexpr int32_t SEND_RATE_LIMIT = 10 * 1024 * 1024; // 10 MB/s

	// Adjust the default Send/Receive buffer sizes for the client connections.
	// Send buffer size defaults to 512 KB and receive buffer size defaults to 1024 KB, which may not
	// be enough for some edge cases, so increase them to 1 MB and 2 MB, respectively.
	SteamNetworkingConfigValue_t sendBufferSize;
	sendBufferSize.SetInt32(k_ESteamNetworkingConfig_SendBufferSize, SEND_BUFFER_SIZE);
	opts.emplace_back(std::move(sendBufferSize));

	SteamNetworkingConfigValue_t recvBufferSize;
	recvBufferSize.SetInt32(k_ESteamNetworkingConfig_RecvBufferSize, RECV_BUFFER_SIZE);
	opts.emplace_back(std::move(recvBufferSize));

	// Increase the send rate limit for the created connections from the default 256 KB/s to 10 MB/s
	// to ensure that we're not artificially rate-limited in any realistic scenario.
	SteamNetworkingConfigValue_t sendRateMin;
	sendRateMin.SetInt32(k_ESteamNetworkingConfig_SendRateMin, SEND_RATE_LIMIT);
	opts.emplace_back(std::move(sendRateMin));

	SteamNetworkingConfigValue_t sendRateMax;
	sendRateMax.SetInt32(k_ESteamNetworkingConfig_SendRateMax, SEND_RATE_LIMIT);
	opts.emplace_back(std::move(sendRateMax));
}

} // anonymous namespace

static std::once_flag gnsInitFlag;
static GNSConnectionProvider* activeServer = nullptr;

void GNSConnectionProvider::initialize()
{
	if (initialized_) { return; }

	std::call_once(gnsInitFlag, []()
	{
		SteamDatagramErrMsg errMsg;
		if (!GameNetworkingSockets_Init(nullptr, errMsg)) {
			throw std::runtime_error(fmt::format("GameNetworkingSockets_Init failure: {}", errMsg));
		}
	});

	networkInterface_ = SteamNetworkingSockets();
	if (!networkInterface_)
	{
		throw std::runtime_error("Failed to initialize GNS network interface");
	}

	initialized_ = true;

	addressResolver_ = std::make_unique<tcp::TCPAddressResolver>();

	// FIXME: This doesn't work for some reason (the pointer is not set to the correct value
	// in the connection state change callback), although this would be a better alternative to
	// having a static `activeServer` variable.

	//SteamNetworkingConfigValue_t connUserData;
	//connUserData.SetPtr(k_ESteamNetworkingConfig_ConnectionUserData, reinterpret_cast<void*>(this));
	//listenSocketOpts_.emplace_back(std::move(connUserData));

	{
		// Initialize server-side options for listen socket
		SteamNetworkingConfigValue_t connStatusCb;
		connStatusCb.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, reinterpret_cast<void*>(GNSConnectionProvider::ServerConnectionStateChanged));
		listenSocketOpts_.emplace_back(std::move(connStatusCb));

		SteamNetworkingConfigValue_t initialTimeoutMs;
		// Initial connection timeout is hardcoded to be 15 seconds
		initialTimeoutMs.SetInt32(k_ESteamNetworkingConfig_TimeoutInitial, 15000);
		listenSocketOpts_.emplace_back(std::move(initialTimeoutMs));

		SteamNetworkingConfigValue_t connectedTimeoutMs;
		// Connected timeout is hardcoded to be 10 seconds
		//
		// Fresh connections will emerge into existence waiting in the lobby room, so will
		// need a smaller timeout value, compared to the moment when the game transitions to
		// the main game loop. In this particular case, the timeout will be increased to 60
		// seconds, so that auto lag-kick mechanism will handle timed out connections, instead.
		constexpr int32_t CONNECTED_TIMEOUT_MS = 10000;
		connectedTimeoutMs.SetInt32(k_ESteamNetworkingConfig_TimeoutConnected, CONNECTED_TIMEOUT_MS);
		listenSocketOpts_.emplace_back(std::move(connectedTimeoutMs));

		setupCommonConnOptions(listenSocketOpts_);
	}
	{
		// Initialize base client-side options for client connections
		SteamNetworkingConfigValue_t connStatusCb;
		connStatusCb.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, reinterpret_cast<void*>(GNSConnectionProvider::ClientConnectionStateChanged));
		clientConnOpts_.emplace_back(std::move(connStatusCb));

		setupCommonConnOptions(clientConnOpts_);
	}

	activeServer = this;
}

void GNSConnectionProvider::shutdown()
{
	if (!initialized_) { return; }
	initialized_ = false;
	addressResolver_.reset();
	activeServer = nullptr;
}

ConnectionProviderType GNSConnectionProvider::type() const noexcept
{
	return ConnectionProviderType::GNS_DIRECT;
}

net::result<std::unique_ptr<IConnectionAddress>> GNSConnectionProvider::resolveHost(const char* host, uint16_t port) const
{
	ASSERT_OR_RETURN(tl::make_unexpected(make_network_error_code(EINVAL)), addressResolver_ != nullptr, "Address resolver not initialized");

	auto resolvedAddr = addressResolver_->resolveHost(host, port);
	ASSERT_OR_RETURN(tl::make_unexpected(resolvedAddr.error()), resolvedAddr.has_value(), "Address resolver error");

	auto addrStrRes = resolvedAddr.value()->toString();
	ASSERT_OR_RETURN(tl::make_unexpected(addrStrRes.error()), addrStrRes.has_value(), "Failed to cast network address to string");

	SteamNetworkingIPAddr addr;
	addr.Clear();
	if (!addr.ParseString(addrStrRes.value().c_str()))
	{
		return tl::make_unexpected(make_network_error_code(EFAULT)); // bad address
	}
	return std::make_unique<GNSConnectionAddress>(std::move(addr));
}

net::result<IListenSocket*> GNSConnectionProvider::openListenSocket(uint16_t port)
{
	SteamNetworkingIPAddr addr;
	addr.Clear();
	addr.m_port = port;

	HSteamListenSocket h = networkInterface_->CreateListenSocketIP(addr, listenSocketOpts_.size(), listenSocketOpts_.data());
	if (h == k_HSteamListenSocket_Invalid)
	{
		return tl::make_unexpected(make_network_error_code(EBADF)); // bad file descriptor
	}
	auto gnsListenSock = new GNSListenSocket(*this, WzCompressionProvider::Instance(), PendingWritesManagerMap::instance().get(type()), networkInterface_, h);
	activeListenSocket_ = { h, gnsListenSock };
	return gnsListenSock;
}

net::result<IClientConnection*> GNSConnectionProvider::openClientConnectionAny(const IConnectionAddress& addr, unsigned timeout)
{
	const auto gnsAddr = dynamic_cast<const GNSConnectionAddress*>(&addr);
	ASSERT_OR_RETURN(tl::make_unexpected(make_network_error_code(EINVAL)), gnsAddr != nullptr, "Expected to have GNSConnectionAddress instance");

	std::vector<SteamNetworkingConfigValue_t> connectOpts = clientConnOpts_;
	SteamNetworkingConfigValue_t timeoutInitialConf;
	timeoutInitialConf.SetInt32(k_ESteamNetworkingConfig_TimeoutInitial, static_cast<int32_t>(timeout));
	connectOpts.emplace_back(std::move(timeoutInitialConf));

	// Connected timeout is hardcoded to be 10 seconds
	//
	// Fresh connections will emerge into existence waiting in the lobby room, so will
	// need a smaller timeout value, compared to the moment when the game transitions to
	// the main game loop. In this particular case, the timeout will be increased to 60
	// seconds, so that auto lag-kick mechanism will handle timed out connections, instead.
	SteamNetworkingConfigValue_t timeoutConnectedConf;
	constexpr int32_t CONNECTED_TIMEOUT_MS = 10000;
	timeoutConnectedConf.SetInt32(k_ESteamNetworkingConfig_TimeoutConnected, CONNECTED_TIMEOUT_MS);
	connectOpts.emplace_back(std::move(timeoutConnectedConf));

	auto h = networkInterface_->ConnectByIPAddress(gnsAddr->asSteamNetworkingIPAddr(), connectOpts.size(), connectOpts.data());
	if (h == k_HSteamNetConnection_Invalid)
	{
		return tl::make_unexpected(make_network_error_code(EBADF)); // bad file descriptor
	}
	auto conn = new GNSClientConnection(*this, WzCompressionProvider::Instance(), PendingWritesManagerMap::instance().get(type()), networkInterface_, h);
	std::unique_lock lock(activeClientsMtx_);
	activeClients_.emplace(h, conn);
	return conn;
}

IConnectionPollGroup* GNSConnectionProvider::newConnectionPollGroup()
{
	auto gnsGroup = networkInterface_->CreatePollGroup();
	ASSERT_OR_RETURN(nullptr, gnsGroup != k_HSteamNetPollGroup_Invalid, "Failed to create poll group");
	return new GNSConnectionPollGroup(networkInterface_, gnsGroup);
}

void GNSConnectionProvider::ServerConnectionStateChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	ASSERT_OR_RETURN(, pInfo != nullptr, "Invalid SteamNetConnectionStatusChangedCallback_t pointer");

	const SteamNetConnectionInfo_t connInfo = pInfo->m_info;
	// FIXME: The current instance of GNSConnectionProvider should really be encoded as a pointer
	// in `pInfo->m_info->m_nUserData`, but doesn't work for some reason.
	GNSConnectionProvider* connProvider = activeServer; // reinterpret_cast<GNSConnectionProvider*>(connInfo.m_nUserData);

	switch (connInfo.m_eState)
	{
	case k_ESteamNetworkingConnectionState_None:
		break;
	case k_ESteamNetworkingConnectionState_Connecting:
	{
		ASSERT(pInfo->m_info.m_hListenSocket == connProvider->activeListenSocket_.first, "Unexpected listen socket handle");
		if (connProvider->networkInterface_->AcceptConnection(pInfo->m_hConn) != k_EResultOK)
		{
			debug(LOG_WARNING, "ServerConnectionStateChanged: Failed to accept connection %u", pInfo->m_hConn);
			connProvider->networkInterface_->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
			break;
		}
		std::unique_lock lock(connProvider->activeClientsMtx_);

		ASSERT(connProvider->activeClients_.count(pInfo->m_hConn) == 0, "Expected to have a new connection");
		// We don't yet have a live `IClientConnection` instance for this connection
		connProvider->activeClients_.emplace(pInfo->m_hConn, nullptr);
		break;
	}
	case k_ESteamNetworkingConnectionState_FindingRoute:
		break;
	case k_ESteamNetworkingConnectionState_Connected:
	{
		std::unique_lock lock(connProvider->activeClientsMtx_);

		ASSERT(connProvider->activeClients_.count(pInfo->m_hConn) != 0, "Expected to be a connection that we are aware of");
		ASSERT(pInfo->m_info.m_hListenSocket == connProvider->activeListenSocket_.first, "Unexpected listen socket handle");
		connProvider->activeListenSocket_.second->addPendingAcceptedConnection(pInfo->m_hConn);
		break;
	}
	case k_ESteamNetworkingConnectionState_ClosedByPeer:
		debug(LOG_ERROR, "Connection closed by peer: %u", pInfo->m_hConn);
		connProvider->disposeConnectionImpl(pInfo->m_hConn);
		break;
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		debug(LOG_ERROR, "Connection closed, problem detected locally: %u", pInfo->m_hConn);
		connProvider->disposeConnectionImpl(pInfo->m_hConn);
		break;
	default:
		break;
	}
}

void GNSConnectionProvider::ClientConnectionStateChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	ASSERT_OR_RETURN(, pInfo != nullptr, "Invalid SteamNetConnectionStatusChangedCallback_t pointer");

	const SteamNetConnectionInfo_t connInfo = pInfo->m_info;
	// FIXME: The current instance of GNSConnectionProvider should really be encoded as a pointer
	// in `pInfo->m_info->m_nUserData`, but doesn't work for some reason.
	GNSConnectionProvider* connProvider = activeServer; // reinterpret_cast<GNSConnectionProvider*>(connInfo.m_nUserData);

	switch (connInfo.m_eState)
	{
	case k_ESteamNetworkingConnectionState_None:
	case k_ESteamNetworkingConnectionState_Connecting:
	case k_ESteamNetworkingConnectionState_FindingRoute:
	case k_ESteamNetworkingConnectionState_Connected:
		break;
	case k_ESteamNetworkingConnectionState_ClosedByPeer:
		debug(LOG_WARNING, "Connection closed by peer: %u", pInfo->m_hConn);
		connProvider->disposeConnectionImpl(pInfo->m_hConn);
		break;
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		debug(LOG_ERROR, "Connection closed, problem detected locally: %u", pInfo->m_hConn);
		connProvider->disposeConnectionImpl(pInfo->m_hConn);
		break;
	default:
		break;
	}
}

std::unique_ptr<IDescriptorSet> GNSConnectionProvider::newDescriptorSet(PollEventType /*eventType*/)
{
	return std::make_unique<DummyDescriptorSet>();
}

void GNSConnectionProvider::processConnectionStateChanges()
{
	ASSERT_OR_RETURN(, networkInterface_ != nullptr, "Invalid GNS network interface pointer");
	// This will trigger `GNSConnectionProvider::Client/ServerConnectionStateChanged()` for every managed
	// connection in a blocking manner! DO NOT do any heavy computations in the callbacks!
	networkInterface_->RunCallbacks();
}

PortMappingInternetProtocolMask GNSConnectionProvider::portMappingProtocolTypes() const
{
	return static_cast<PortMappingInternetProtocolMask>(PortMappingInternetProtocol::UDP_IPV4) | static_cast<PortMappingInternetProtocolMask>(PortMappingInternetProtocol::UDP_IPV6);
}

void GNSConnectionProvider::registerAcceptedConnection(GNSClientConnection* conn)
{
	ASSERT_OR_RETURN(, conn->isValid(), "Invalid GNS client connection handle");
	std::unique_lock lock(activeClientsMtx_);
	activeClients_[conn->connectionHandle()] = conn;
}

void GNSConnectionProvider::disposeConnection(IClientConnection* conn)
{
	auto* gnsConn = dynamic_cast<GNSClientConnection*>(conn);
	ASSERT_OR_RETURN(, gnsConn != nullptr, "Expected GNSClientConnection instance");
	disposeConnectionImpl(gnsConn->connectionHandle());
}

void GNSConnectionProvider::disposeConnectionImpl(HSteamNetConnection hConn)
{
	if (hConn == k_HSteamNetConnection_Invalid)
	{
		return;
	}
	std::unique_lock lock(activeClientsMtx_);
	const auto connIt = activeClients_.find(hConn);
	if (connIt != activeClients_.end())
	{
		GNSClientConnection* conn = connIt->second;
		// Remove this connection handle from active clients list
		activeClients_.erase(connIt);
		// No more need to hold the lock since the connection is not visible anymore
		lock.unlock();

		if (conn)
		{
			// Remove any pending writes for this connection
			auto& pwm = PendingWritesManagerMap::instance().get(type());
			pwm.clearPendingWrites(conn);
			// Attempt to flush any messages, that are waiting in the internal GNS queue for this connection
			// and discard any pending messages that weren't yet processed by this connection object
			conn->flushPendingMessages();
			// Automatically remove this connection from the poll group, if there's any
			if (auto* pollGroup = conn->getPollGroup())
			{
				pollGroup->remove(conn);
			}
			// This call renders the current connection object invalid!
			conn->expireConnectionHandle();
		}
	}
	// Close the internal GNS connection handle
	networkInterface_->CloseConnection(hConn, 0, nullptr, true);
}

} // namespace gns
