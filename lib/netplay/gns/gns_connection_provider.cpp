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

static std::once_flag gnsInitFlag;
static GNSConnectionProvider* activeServer = nullptr;

void GNSConnectionProvider::initialize()
{
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
	}
	{
		// Initialize base client-side options for client connections
		SteamNetworkingConfigValue_t connStatusCb;
		connStatusCb.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, reinterpret_cast<void*>(GNSConnectionProvider::ClientConnectionStateChanged));
		clientConnOpts_.emplace_back(std::move(connStatusCb));
	}

	activeServer = this;
}

void GNSConnectionProvider::shutdown()
{
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

	SteamNetworkingConfigValue_t timeoutConnectedConf;
	// Hardcode the connected timeout to a large enough value (let it be 60 seconds, for now),
	// so that other built-in measures to close the connection will be able to kick in
	// and close the connection (e.g., lag auto-kick).
	timeoutConnectedConf.SetInt32(k_ESteamNetworkingConfig_TimeoutConnected, 60);
	connectOpts.emplace_back(std::move(timeoutConnectedConf));

	auto h = networkInterface_->ConnectByIPAddress(gnsAddr->asSteamNetworkingIPAddr(), connectOpts.size(), connectOpts.data());
	if (h == k_HSteamNetConnection_Invalid)
	{
		return tl::make_unexpected(make_network_error_code(EBADF)); // bad file descriptor
	}
	auto conn = new GNSClientConnection(*this, WzCompressionProvider::Instance(), PendingWritesManagerMap::instance().get(type()), networkInterface_, h);
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
		ASSERT(connProvider->activeClients_.count(pInfo->m_hConn) == 0, "Expected to have a new connection");
		ASSERT(pInfo->m_info.m_hListenSocket == connProvider->activeListenSocket_.first, "Unexpected listen socket handle");
		if (connProvider->networkInterface_->AcceptConnection(pInfo->m_hConn) != k_EResultOK)
		{
			connProvider->networkInterface_->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
			// FIXME: debug logging
			break;
		}
		// We don't yet have a live `IClientConnection` instance for this connection
		connProvider->activeClients_.emplace(pInfo->m_hConn, nullptr);
		break;
	case k_ESteamNetworkingConnectionState_FindingRoute:
		break;
	case k_ESteamNetworkingConnectionState_Connected:
		ASSERT(connProvider->activeClients_.count(pInfo->m_hConn) != 0, "Expected to be a connection that we are aware of");
		ASSERT(pInfo->m_info.m_hListenSocket == connProvider->activeListenSocket_.first, "Unexpected listen socket handle");
		connProvider->activeListenSocket_.second->addPendingAcceptedConnection(pInfo->m_hConn);
		break;
	case k_ESteamNetworkingConnectionState_ClosedByPeer:
		debug(LOG_ERROR, "Connection closed by peer: %u", pInfo->m_hConn);
		connProvider->activeClients_.erase(pInfo->m_hConn);
		connProvider->networkInterface_->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
		break;
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		debug(LOG_ERROR, "Connection closed, problem detected locally: %u", pInfo->m_hConn);
		connProvider->activeClients_.erase(pInfo->m_hConn);
		connProvider->networkInterface_->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
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
	{
		debug(LOG_WARNING, "Connection closed by peer: %u", pInfo->m_hConn);

		const auto connIt = connProvider->activeClients_.find(pInfo->m_hConn);
		ASSERT_OR_RETURN(, connIt != connProvider->activeClients_.end() && connIt->second != nullptr, "Expected to have a valid client connection");
		GNSClientConnection* conn = connIt->second;
		// Perform cleanup for the associated connection object and its internal GNS handle
		connProvider->disposeConnection(conn);
		break;
	}
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		debug(LOG_ERROR, "Connection closed, problem detected locally: %u", pInfo->m_hConn);

		const auto connIt = connProvider->activeClients_.find(pInfo->m_hConn);
		ASSERT_OR_RETURN(, connIt != connProvider->activeClients_.end() && connIt->second != nullptr, "Expected to have a valid client connection");
		GNSClientConnection* conn = connIt->second;
		// Perform cleanup for the associated connection object and its internal GNS handle
		connProvider->disposeConnection(conn);
		break;
	}
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

void GNSConnectionProvider::disposeConnection(GNSClientConnection* conn)
{
	if (!conn)
	{
		return;
	}
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
	// Close the internal GNS connection handle
	networkInterface_->CloseConnection(pInfo->m_hConn, 0, nullptr, true);
	// This call renders the current connection object invalid!
	conn->expireConnectionHandle();
}

} // namespace gns
