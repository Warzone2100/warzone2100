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

#include "lib/framework/wzapp.h"
#include "lib/framework/debug.h"
#include "lib/framework/string_ext.h"
#include "lib/netplay/error_categories.h"
#include "lib/netplay/polling_util.h"
#include "lib/netplay/wz_connection_provider.h"
#include "lib/netplay/tcp/tcp_client_connection.h"
#include "lib/netplay/tcp/netsocket.h"
#include "lib/netplay/tcp/sock_error.h"

namespace tcp
{

TCPClientConnection::TCPClientConnection(WzConnectionProvider& connProvider, WzCompressionProvider& compressionProvider, PendingWritesManager& pwm, Socket* rawSocket)
	: IClientConnection(connProvider, compressionProvider, pwm),
	socket_(rawSocket),
	connStatusDescriptorSet_(connProvider.newDescriptorSet(PollEventType::READABLE))
{
	ASSERT(socket_ != nullptr, "Null socket passed to TCPClientConnection ctor");
}

TCPClientConnection::~TCPClientConnection()
{
	if (!socket_)
	{
		return;
	}
	tcp::socketCloseNow(socket_);
	socket_ = nullptr;
}

net::result<ssize_t> TCPClientConnection::sendImpl(const std::vector<uint8_t>& data)
{
	if (!isValid())
	{
		debug(LOG_ERROR, "Invalid socket (EBADF)");
		return tl::make_unexpected(make_network_error_code(EBADF));
	}

	ssize_t retSent = ::send(getRawSocketFd(), reinterpret_cast<const char*>(data.data()), data.size(), MSG_NOSIGNAL);
	if (retSent != SOCKET_ERROR)
	{
		return retSent;
	}
	return tl::make_unexpected(make_network_error_code(tcp::getSockErr()));
}

net::result<ssize_t> TCPClientConnection::recvImpl(char* dst, size_t maxSize)
{
	const auto fd = getRawSocketFd();
	int lastSockErr = 0;
	ssize_t received;
	do
	{
		received = ::recv(fd, dst, maxSize, 0);
		if (received == SOCKET_ERROR)
		{
			lastSockErr = tcp::getSockErr();
		} else if (received == 0)
		{
			// Socket got disconnected. No reason to do anything else here.
			return tl::make_unexpected(make_network_error_code(ECONNRESET));
		}
	} while (received == SOCKET_ERROR && lastSockErr == EINTR);

	if (received == SOCKET_ERROR)
	{
		return tl::make_unexpected(make_network_error_code(lastSockErr));
	}
	return received;
}

bool TCPClientConnection::readReady() const
{
	return tcp::socketReadReady(*socket_);
}

void TCPClientConnection::setReadReady(bool ready)
{
	tcp::socketSetReadReady(*socket_, ready);
}

void TCPClientConnection::useNagleAlgorithm(bool enable)
{
	tcp::socketSetTCPNoDelay(*socket_, !enable);
}

std::string TCPClientConnection::textAddress() const
{
	return getSocketTextAddress(*socket_);
}

bool TCPClientConnection::isValid() const
{
	return socket_ != nullptr && tcp::isValidSocket(*socket_);
}

net::result<void> TCPClientConnection::connectionStatus() const
{
	ASSERT_OR_RETURN(tl::make_unexpected(make_network_error_code(EBADF)),
		isValid(), "Invalid connection object");

	// Check whether the socket is still connected
	const auto checkConnRes = checkConnectionsReadable(selfConnList_, *connStatusDescriptorSet_, std::chrono::milliseconds{ 0 });
	if (!checkConnRes.has_value())
	{
		return tl::make_unexpected(checkConnRes.error());
	}
	else if (checkConnRes.value() == (int)selfConnList_.size() && readReady())
	{
		/* The next recv(2) call won't block, but we're writing. So
		 * check the read queue to see if the connection is closed.
		 * If there's no data in the queue that means the connection
		 * is closed.
		 */
#if defined(WZ_OS_WIN)
		unsigned long readQueue;
		const auto ioctlRet = ioctlsocket(getRawSocketFd(), FIONREAD, &readQueue);
#else
		int readQueue;
		const auto ioctlRet = ioctl(getRawSocketFd(), FIONREAD, &readQueue);
#endif
		if (ioctlRet == SOCKET_ERROR)
		{
			debug(LOG_NET, "socket error");
			return tl::make_unexpected(make_network_error_code(tcp::getSockErr()));
		}
		else if (readQueue == 0)
		{
			// Disconnected
			debug(LOG_NET, "Read queue empty - failing (ECONNRESET)");
			return tl::make_unexpected(make_network_error_code(ECONNRESET));
		}
	}
	return {};
}

void TCPClientConnection::setConnectedTimeout(std::chrono::milliseconds timeout)
{
	// NO-OP
}

SOCKET TCPClientConnection::getRawSocketFd() const
{
	return tcp::getRawSocketFd(*socket_);
}

} // namespace tcp
