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

#include "gns_client_connection.h"

#include "lib/framework/frame.h" // for ASSERT, ASSERT_OR_RETURN
#include "lib/netplay/descriptor_set.h"
#include "lib/netplay/error_categories.h"
#include "lib/netplay/gns/gns_error_categories.h"

#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingtypes.h>

#include <cstring>

namespace gns
{

GNSClientConnection::GNSClientConnection(WzConnectionProvider& connProvider, WzCompressionProvider& compressionProvider, PendingWritesManager& pwm,
		ISteamNetworkingSockets* networkInterface, HSteamNetConnection conn)
	: IClientConnection(connProvider, compressionProvider, pwm),
	networkInterface_(networkInterface),
	conn_(conn)
{
	ASSERT(isValid(), "Invalid GNS client connection handle");
}

GNSClientConnection::~GNSClientConnection()
{
	if (!isValid())
	{
		debug(LOG_WARNING, "~GNSClientConnection: expired connection handle");
		return;
	}
	// This is a best-effort attempt to flush the connection before closing it.
	// If it fails, we can still close the connection.
	flushPendingMessages();
	ASSERT(networkInterface_->CloseConnection(conn_, 0, nullptr, true) == true, "Failed to close client connection properly");
}

net::result<ssize_t> GNSClientConnection::sendImpl(const std::vector<uint8_t>& data)
{
	ASSERT_OR_RETURN(tl::make_unexpected(make_gns_error_code(EINVAL)), isValid(), "Invalid GNS client connection handle");

	int sendFlags = k_nSteamNetworkingSend_Reliable;
	if (!useNagle_)
	{
		sendFlags |= k_nSteamNetworkingSend_NoNagle;
	}
	const auto size = data.size();
	auto res = networkInterface_->SendMessageToConnection(conn_, data.data(), static_cast<uint32_t>(size), sendFlags, nullptr);
	if (res != k_EResultOK)
	{
		return tl::make_unexpected(make_gns_error_code(res));
	}
	return size;
}

net::result<ssize_t> GNSClientConnection::recvImpl(char* dst, size_t maxSize)
{
	ASSERT_OR_RETURN(tl::make_unexpected(make_gns_error_code(EINVAL)), isValid(), "Invalid GNS client connection handle");
	ASSERT_OR_RETURN(0, readReady(), "No data to read from the connection");

	size_t currentProcessedSize = 0;
	while (!pendingMessagesToRead_.empty() && currentProcessedSize < maxSize)
	{
		SteamNetworkingMessage_t* msg = pendingMessagesToRead_.front();
		// Message payloads exceeding `maxSize` bytes are illegal and shouldn't happen.
		if (msg->m_cbSize > maxSize)
		{
			return tl::make_unexpected(make_gns_error_code(k_EResultLimitExceeded));
		}
		// Peek into the next message to see if it will fit into our buffer.
		if ((currentProcessedSize + msg->m_cbSize) > maxSize)
		{
			break;
		}
		pendingMessagesToRead_.pop();
		std::memcpy(dst, msg->m_pData, static_cast<size_t>(msg->m_cbSize));
		dst += msg->m_cbSize;
		currentProcessedSize += msg->m_cbSize;

		msg->Release();
	}
	return currentProcessedSize;
}

void GNSClientConnection::useNagleAlgorithm(bool enable)
{
	useNagle_ = enable;
}

std::string GNSClientConnection::textAddress() const
{
	SteamNetConnectionInfo_t connInfo;
	if (!networkInterface_->GetConnectionInfo(conn_, &connInfo))
	{
		return {};
	}
	char buf[SteamNetworkingIdentity::k_cchMaxString] = {};
	connInfo.m_identityRemote.ToString(buf, SteamNetworkingIdentity::k_cchMaxString);
	return std::string(buf);
}

bool GNSClientConnection::isValid() const
{
	return networkInterface_ != nullptr && conn_ != k_HSteamNetConnection_Invalid;
}

net::result<void> GNSClientConnection::connectionStatus() const
{
	ASSERT_OR_RETURN(tl::make_unexpected(make_network_error_code(EINVAL)), isValid(), "Invalid GNS network interface pointer");

	SteamNetConnectionRealTimeStatus_t status;
	auto res = networkInterface_->GetConnectionRealTimeStatus(conn_, &status, 0, nullptr);
	if (res != k_EResultOK)
	{
		return tl::make_unexpected(make_gns_error_code(res));
	}
	if (status.m_eState != k_ESteamNetworkingConnectionState_Connected)
	{
		return tl::make_unexpected(make_gns_error_code(k_EResultNoConnection));
	}
	return {};
}

void GNSClientConnection::setConnectedTimeout(std::chrono::milliseconds timeout)
{
	ASSERT_OR_RETURN(, isValid(), "Invalid GNS client connection handle");
	ASSERT(SteamNetworkingUtils()->SetConnectionConfigValueInt32(conn_, k_ESteamNetworkingConfig_TimeoutConnected, static_cast<int32_t>(timeout.count())),
		"Failed to set connected timeout for connection %u", conn_);
}

void GNSClientConnection::enqueueMessage(SteamNetworkingMessage_t* msg)
{
	pendingMessagesToRead_.push(msg);
}

void GNSClientConnection::flushPendingMessages()
{
	ASSERT_OR_RETURN(, isValid(), "Invalid GNS client connection handle");

	const auto flushRes = networkInterface_->FlushMessagesOnConnection(conn_);
	if (flushRes != k_EResultOK && flushRes != k_EResultIgnored)
	{
		const auto errCode = make_gns_error_code(flushRes);
		const auto errMsg = errCode.message();
		debug(LOG_WARNING, "Failed to flush messages on connection: %s", errMsg.c_str());
	}

	// Discard any pending messages, that weren't yet processed by this connection object
	for (; !pendingMessagesToRead_.empty(); pendingMessagesToRead_.pop())
	{
		pendingMessagesToRead_.front()->Release();
	}
}

void GNSClientConnection::expireConnectionHandle()
{
	conn_ = k_HSteamNetConnection_Invalid;
}


} // namespace gns
