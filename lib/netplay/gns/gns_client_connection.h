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

#include "lib/netplay/client_connection.h"

#include <steam/steamnetworkingtypes.h>

#include <queue>

class ISteamNetworkingSockets;

namespace gns
{

class GNSConnectionPollGroup;

/// <summary>
/// GNS-specific implementation of the `IClientConnection` interface.
///
/// Wraps a `HSteamNetConnection` handle to interact with the underlying
/// `ISteamNetworkingSockets` library interface.
///
/// Manages an internal queue of incoming messages (GNS messages) to be read.
/// This queue is populated by the owning `GNSConnectionPollGroup` instance,
/// which is responsible for polling the underlying GNS sockets for incoming
/// messages.
/// </summary>
class GNSClientConnection : public IClientConnection
{
public:

	explicit GNSClientConnection(WzConnectionProvider& connProvider, WzCompressionProvider& compressionProvider, PendingWritesManager& pwm,
		ISteamNetworkingSockets* networkInterface, HSteamNetConnection conn);
	virtual ~GNSClientConnection() override;

	virtual net::result<ssize_t> sendImpl(const std::vector<uint8_t>& data) override;
	virtual net::result<ssize_t> recvImpl(char* dst, size_t maxSize) override;

	virtual void setReadReady(bool /*ready*/) override { /* no-op */ }
	virtual bool readReady() const override { return !pendingMessagesToRead_.empty(); }

	virtual void useNagleAlgorithm(bool enable) override;
	virtual std::string textAddress() const override;

	virtual bool isValid() const override;
	virtual net::result<void> connectionStatus() const override;

	virtual void setConnectedTimeout(std::chrono::milliseconds timeout) override;

	HSteamNetConnection connectionHandle() const { return conn_; }

private:

	friend class GNSConnectionPollGroup;
	friend class GNSConnectionProvider;

	void enqueueMessage(SteamNetworkingMessage_t* msg);
	// Flush any outstanding messages, that are waiting to be sent in the internal GNS queue.
	void flushPendingMessages();
	// Reset internal GNS connection handle to an invalid state.
	// Post-effect: `isValid() == false`.
	void expireConnectionHandle();
	// Update the reference to the owning poll group. This will be called by `GNSConnectionPollGroup::add/remove()`.
	void setPollGroup(GNSConnectionPollGroup* pollGroup) { pollGroup_ = pollGroup; }
	GNSConnectionPollGroup* getPollGroup() const { return pollGroup_; }

	ISteamNetworkingSockets* networkInterface_ = nullptr;
	HSteamNetConnection conn_ = k_HSteamNetConnection_Invalid;
	std::queue<SteamNetworkingMessage_t*> pendingMessagesToRead_;
	size_t currentMsgReadPos_ = 0;
	bool useNagle_ = true;
	GNSConnectionPollGroup* pollGroup_ = nullptr;
};

} // namespace gns
