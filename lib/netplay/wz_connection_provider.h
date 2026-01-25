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

#pragma once

#include <stdint.h>
#include <chrono>
#include <memory>
#include <string>

#include "lib/netplay/connection_address.h"
#include "lib/netplay/descriptor_set.h"
#include "lib/netplay/net_result.h"
#include "lib/netplay/open_connection_result.h"

class IListenSocket;
class IClientConnection;
class IConnectionPollGroup;
struct IConnectionAddress;

enum class ConnectionProviderType : uint8_t;
enum class PortMappingInternetProtocol : uint8_t;

using PortMappingInternetProtocolMask = std::underlying_type_t<PortMappingInternetProtocol>;

/// <summary>
/// Abstraction layer to facilitate creating client/server connections and
/// provide host resolution routines for a given network backend.
///
/// A typical implementation of this interface should at least provide the following
/// things:
///
/// 1. Initialization/teardown routines (setup some common state, like write/submission
///    queues or service threads, plus initialization of low-level backend code, e.g.
///    calls to init/deinit functions from a 3rd-party library).
/// 2. Host resolution.
/// 3. Opening server-side listen sockets.
/// 4. Opening client-side connections (sync and async).
/// 5. Creating connection poll groups.
/// </summary>
class WzConnectionProvider : public std::enable_shared_from_this<WzConnectionProvider>
{
public:

	virtual ~WzConnectionProvider() = default;

	virtual void initialize() = 0;
	virtual void shutdown() = 0;

	virtual ConnectionProviderType type() const noexcept = 0;

	/// <summary>
	/// Resolve host + port combination and return an opaque `ConnectionAddress` handle
	/// representing the resolved network address.
	/// </summary>
	virtual net::result<std::unique_ptr<IConnectionAddress>> resolveHost(const char* host, uint16_t port) const = 0;
	/// <summary>
	/// Open a listening socket bound to a specified local port.
	/// </summary>
	virtual net::result<IListenSocket*> openListenSocket(uint16_t port) = 0;
	/// <summary>
	/// Synchronously open a client connection bound to one of the addresses
	/// represented by `addr` (the first one that succeeds).
	/// </summary>
	/// <param name="addr">Connection address to bind the client connection to.</param>
	/// <param name="timeout">Timeout in milliseconds.</param>
	virtual net::result<IClientConnection*> openClientConnectionAny(const IConnectionAddress& addr, unsigned timeout) = 0;
	/// <summary>
	/// Async variant of `openClientConnectionAny()` with the default implementation, which
	/// spawns a new thread and piggybacks on the `resolveHost()` and `openClientConnectionAny()` combination.
	/// </summary>
	virtual bool openClientConnectionAsync(const std::string& host, uint32_t port, std::chrono::milliseconds timeout, OpenConnectionToHostResultCallback callback);
	/// <summary>
	/// Create a group for polling client connections.
	/// </summary>
	virtual IConnectionPollGroup* newConnectionPollGroup() = 0;
	/// <summary>
	/// Create a new descriptor set, suitable for polling a set of client connections for a certain kind of event.
	///
	/// NOTE: this is a low-level networking primitive.
	/// Thus, descriptor sets aren't intended for direct use in high-level networking code.
	/// </summary>
	/// <param name="eventType">Event that we're interested in.
	/// Can be one of the folowing:
	/// * PollEventType::READABLE
	/// * PollEventType::WRITABLE
	/// </param>
	virtual std::unique_ptr<IDescriptorSet> newDescriptorSet(PollEventType eventType) = 0;

	/// <summary>
	/// Process any pending connection state change events. This should be called regularly
	/// at the beginning of each network game loop iteration to ensure that the connections
	/// remain valid and synced properly with the underlying network backend.
	/// </summary>
	virtual void processConnectionStateChanges() = 0;

	virtual PortMappingInternetProtocolMask portMappingProtocolTypes() const = 0;

	/// <summary>
	/// Perform generic cleanup actions for a given connection, i.e.: update the bookkeeping
	/// information for the connection provider, flush any pending writes, remove the connection
	/// from owning poll group (if there's any), etc.
	/// </summary>
	/// <param name="conn">Connection object to dispose of.</param>
	virtual void disposeConnection(IClientConnection* conn) = 0;
};
