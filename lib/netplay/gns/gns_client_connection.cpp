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
#include <algorithm>

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
	// Limit the size of the message to the maximum size allowed by the GNS library.
	// Higher-level code will automatically handle this case and split the payload into several messages if needed.
	const auto size = std::min(data.size(), static_cast<size_t>(k_cbMaxSteamNetworkingSocketsMessageSizeSend));
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

		// Determine how much bytes left to read from the top of the queue.
		size_t bytesToRead = static_cast<size_t>(msg->m_cbSize) - currentMsgReadPos_;
		if (bytesToRead > maxSize)
		{
			// If there's more than `maxSize`, just copy the max size bytes and adjust the offset accordingly.
			std::memcpy(dst, reinterpret_cast<const char*>(msg->m_pData) + currentMsgReadPos_, maxSize);
			currentMsgReadPos_ += maxSize;
			return maxSize;
		}
		// Consume the current message and dispose of it.
		pendingMessagesToRead_.pop();
		std::memcpy(dst, reinterpret_cast<const char*>(msg->m_pData) + currentMsgReadPos_, bytesToRead);
		dst += bytesToRead;
		currentProcessedSize += bytesToRead;
		// Reset the read position for the next message.
		currentMsgReadPos_ = 0;

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

	switch (connInfo.m_identityRemote.m_eType)
	{
		case k_ESteamNetworkingIdentityType_IPAddress:
		{
			// Special handling of the ip address case
			// Return the ip address (without the "ip:" prefix that SteamNetworkingIdentity::ToString would add, and without the port), to match TCP backend behavior
			auto pIPAddr = connInfo.m_identityRemote.GetIPAddr();
			ASSERT_OR_RETURN({}, pIPAddr != nullptr, "Unable to get ip address?");
			pIPAddr->ToString(buf, SteamNetworkingIdentity::k_cchMaxString, false);
			return std::string(buf);
		}
		case k_ESteamNetworkingIdentityType_Invalid:
			ASSERT(false, "Connection identity type is invalid?");
			return {};
		default:
			// Other cases - use SteamNetworkingIdentity::ToString, which returns a string with a "prefix:" indicating what type of identity it is
			connInfo.m_identityRemote.ToString(buf, SteamNetworkingIdentity::k_cchMaxString);
			return std::string(buf);
	}
	return {}; // silence compiler warning
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
