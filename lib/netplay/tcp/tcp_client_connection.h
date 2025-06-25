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

#include "lib/netplay/client_connection.h"
#include "lib/netplay/descriptor_set.h"
#include "lib/netplay/tcp/netsocket.h" // for SOCKET

class PendingWritesManager;
class WzCompressionProvider;
class WzConnectionProvider;

namespace tcp
{

struct Socket;

/// <summary>
/// TCP-specific implementation of the `IClientConnection` interface, holding
/// a reference to the raw TCP socket.
///
/// Intended to work with the TCP network backend.
/// </summary>
class TCPClientConnection : public IClientConnection
{
public:

	explicit TCPClientConnection(WzConnectionProvider& connProvider, WzCompressionProvider& compressionProvider, PendingWritesManager& pwm, Socket* rawSocket);
	virtual ~TCPClientConnection() override;

	virtual net::result<ssize_t> sendImpl(const std::vector<uint8_t>& data) override;
	virtual net::result<ssize_t> recvImpl(char* dst, size_t maxSize) override;

	virtual void setReadReady(bool ready) override;
	virtual bool readReady() const override;

	virtual void useNagleAlgorithm(bool enable) override;
	virtual std::string textAddress() const override;

	virtual bool isValid() const override;
	virtual net::result<void> connectionStatus() const override;

	virtual void setConnectedTimeout(std::chrono::milliseconds timeout) override;

	SOCKET getRawSocketFd() const;

private:

	friend class TCPConnectionPollGroup;

	Socket* socket_ = nullptr;

	std::unique_ptr<IDescriptorSet> connStatusDescriptorSet_;
};

} // namespace tcp
