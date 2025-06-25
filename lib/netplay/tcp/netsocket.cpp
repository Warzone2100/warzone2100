/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/**
 * @file netsocket.cpp
 *
 * Basic raw socket handling code.
 */

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "netsocket.h"
#include "lib/netplay/error_categories.h"
#include "lib/netplay/client_connection.h"
#include "lib/netplay/descriptor_set.h"

#include <vector>

#if defined(__clang__)
	#pragma clang diagnostic ignored "-Wshorten-64-to-32" // FIXME!!
#endif

#if defined(WZ_OS_UNIX)
# include <netinet/tcp.h> // For TCP_NODELAY
#elif defined(WZ_OS_WIN)
// Already included Winsock2.h which defines TCP_NODELAY
#endif

namespace tcp
{

enum
{
	SOCK_CONNECTION,
	SOCK_IPV4_LISTEN = SOCK_CONNECTION,
	SOCK_IPV6_LISTEN,
	SOCK_COUNT,
};

struct Socket
{
	/* Multiple socket handles only for listening sockets. This allows us
	 * to listen on multiple protocols and address families (e.g. IPv4 and
	 * IPv6).
	 *
	 * All non-listening sockets will only use the first socket handle.
	 */
	SOCKET fd[SOCK_COUNT];
	bool ready = false;
	char textAddress[40] = {};
};

SOCKET getRawSocketFd(const Socket& sock)
{
	return sock.fd[SOCK_CONNECTION];
}

bool isValidSocket(const Socket& sock)
{
	return sock.fd[SOCK_CONNECTION] != INVALID_SOCKET;
}

bool socketReadReady(const Socket& sock)
{
	return sock.ready;
}

void socketSetReadReady(Socket& sock, bool ready)
{
	sock.ready = ready;
}


} // namespace tcp

namespace tcp
{

static int addressToText(const struct sockaddr *addr, char *buf, size_t size)
{
	auto handleIpv4 = [&](uint32_t addr) {
		uint32_t val = ntohl(addr);
		return snprintf(buf, size, "%u.%u.%u.%u", (val>>24)&0xFF, (val>>16)&0xFF, (val>>8)&0xFF, val&0xFF);
	};

	switch (addr->sa_family)
	{
	case AF_INET:
		{
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wcast-align"
#endif
			return handleIpv4((reinterpret_cast<const sockaddr_in *>(addr))->sin_addr.s_addr);
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
# pragma GCC diagnostic pop
#endif
		}
	case AF_INET6:
		{

			// Check to see if this is really a IPv6 address
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wcast-align"
#endif
			const struct sockaddr_in6 *mappedIP = reinterpret_cast<const sockaddr_in6 *>(addr);
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
# pragma GCC diagnostic pop
#endif
			if (IN6_IS_ADDR_V4MAPPED(&mappedIP->sin6_addr))
			{
				// looks like it is ::ffff:(...) so lets set up a IPv4 socket address structure
				// slightly overkill for our needs, but it shows exactly what needs to be done.
				// At this time, we only care about the address, nothing else.
				struct sockaddr_in addr4;
				memcpy(&addr4.sin_addr.s_addr, mappedIP->sin6_addr.s6_addr + 12, sizeof(addr4.sin_addr.s_addr));
				return handleIpv4(addr4.sin_addr.s_addr);
			}
			else
			{
				static_assert(sizeof(in6_addr::s6_addr) == 16, "Standard expects in6_addr structure that contains member s6_addr[16], a 16-element array of uint8_t");
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wcast-align"
#endif
				const uint8_t *address_u8 = &((reinterpret_cast<const sockaddr_in6 *>(addr))->sin6_addr.s6_addr[0]);
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
# pragma GCC diagnostic pop
#endif
				uint16_t address[8] = {0};
				memcpy(&address, address_u8, sizeof(uint8_t) * 16);
				return snprintf(buf, size,
								"%hx:%hx:%hx:%hx:%hx:%hx:%hx:%hx",
								ntohs(address[0]),
								ntohs(address[1]),
								ntohs(address[2]),
								ntohs(address[3]),
								ntohs(address[4]),
								ntohs(address[5]),
								ntohs(address[6]),
								ntohs(address[7]));
			}
		}
	default:
		ASSERT(!"Unknown address family", "Got non IPv4 or IPv6 address!");
		return -1;
	}
}

static const char *strSockError(int error)
{
#if   defined(WZ_OS_WIN)
	switch (error)
	{
	case 0:                     return "No error";
	case WSAEINTR:              return "Interrupted system call";
	case WSAEBADF:              return "Bad file number";
	case WSAEACCES:             return "Permission denied";
	case WSAEFAULT:             return "Bad address";
	case WSAEINVAL:             return "Invalid argument";
	case WSAEMFILE:             return "Too many open sockets";
	case WSAEWOULDBLOCK:        return "Operation would block";
	case WSAEINPROGRESS:        return "Operation now in progress";
	case WSAEALREADY:           return "Operation already in progress";
	case WSAENOTSOCK:           return "Socket operation on non-socket";
	case WSAEDESTADDRREQ:       return "Destination address required";
	case WSAEMSGSIZE:           return "Message too long";
	case WSAEPROTOTYPE:         return "Protocol wrong type for socket";
	case WSAENOPROTOOPT:        return "Bad protocol option";
	case WSAEPROTONOSUPPORT:    return "Protocol not supported";
	case WSAESOCKTNOSUPPORT:    return "Socket type not supported";
	case WSAEOPNOTSUPP:         return "Operation not supported on socket";
	case WSAEPFNOSUPPORT:       return "Protocol family not supported";
	case WSAEAFNOSUPPORT:       return "Address family not supported";
	case WSAEADDRINUSE:         return "Address already in use";
	case WSAEADDRNOTAVAIL:      return "Can't assign requested address";
	case WSAENETDOWN:           return "Network is down";
	case WSAENETUNREACH:        return "Network is unreachable";
	case WSAENETRESET:          return "Net connection reset";
	case WSAECONNABORTED:       return "Software caused connection abort";
	case WSAECONNRESET:         return "Connection reset by peer";
	case WSAENOBUFS:            return "No buffer space available";
	case WSAEISCONN:            return "Socket is already connected";
	case WSAENOTCONN:           return "Socket is not connected";
	case WSAESHUTDOWN:          return "Can't send after socket shutdown";
	case WSAETOOMANYREFS:       return "Too many references, can't splice";
	case WSAETIMEDOUT:          return "Connection timed out";
	case WSAECONNREFUSED:       return "Connection refused";
	case WSAELOOP:              return "Too many levels of symbolic links";
	case WSAENAMETOOLONG:       return "File name too long";
	case WSAEHOSTDOWN:          return "Host is down";
	case WSAEHOSTUNREACH:       return "No route to host";
	case WSAENOTEMPTY:          return "Directory not empty";
	case WSAEPROCLIM:           return "Too many processes";
	case WSAEUSERS:             return "Too many users";
	case WSAEDQUOT:             return "Disc quota exceeded";
	case WSAESTALE:             return "Stale NFS file handle";
	case WSAEREMOTE:            return "Too many levels of remote in path";
	case WSASYSNOTREADY:        return "Network system is unavailable";
	case WSAVERNOTSUPPORTED:    return "Winsock version out of range";
	case WSANOTINITIALISED:     return "WSAStartup not yet called";
	case WSAEDISCON:            return "Graceful shutdown in progress";
	case WSAHOST_NOT_FOUND:     return "Host not found";
	case WSANO_DATA:            return "No host data of that type was found";
	default:                    return "Unknown error";
	}
#elif defined(WZ_OS_UNIX)
	return strerror(error);
#endif
}

bool socketSetTCPNoDelay(Socket& sock, bool nodelay)
{
#if defined(TCP_NODELAY)
	int value = (nodelay) ? 1 : 0;
	int result = setsockopt(sock.fd[SOCK_CONNECTION], IPPROTO_TCP, TCP_NODELAY, (char *) &value, sizeof(int));
	debug(LOG_NET, "Setting TCP_NODELAY on socket: %s", (result != SOCKET_ERROR) ? "success" : "failure");
	return result != SOCKET_ERROR;
#else
	debug(LOG_NET, "Unable to set TCP_NODELAY on socket - unsupported");
	return false;
#endif
}

#if !defined(SOCK_CLOEXEC)
static bool setSocketInheritable(SOCKET fd, bool inheritable)
{
#if   defined(WZ_OS_UNIX)
	int sockopts = fcntl(fd, F_SETFD);
	if (sockopts == SOCKET_ERROR)
	{
		debug(LOG_NET, "Failed to retrieve current socket options: %s", strSockError(getSockErr()));
		return false;
	}

	// Set or clear FD_CLOEXEC flag
	if (inheritable)
	{
		sockopts &= ~FD_CLOEXEC;
	}
	else
	{
		sockopts |= FD_CLOEXEC;
	}

	if (fcntl(fd, F_SETFD, sockopts) == SOCKET_ERROR)
	{
		debug(LOG_NET, "Failed to set socket %sinheritable: %s", (inheritable ? "" : "non-"), strSockError(getSockErr()));
		return false;
	}
#elif defined(WZ_OS_WIN)
	DWORD dwFlags = (inheritable) ? HANDLE_FLAG_INHERIT : 0;
	if (::SetHandleInformation((HANDLE)fd, HANDLE_FLAG_INHERIT, dwFlags) == 0)
	{
		DWORD dwErr = GetLastError();
		debug(LOG_NET, "Failed to set socket %sinheritable: %s", (inheritable ? "" : "non-"), std::to_string(dwErr).c_str());
		return false;
	}
#endif

	debug(LOG_NET, "Socket is set to %sinheritable.", (inheritable ? "" : "non-"));
	return true;
}
#endif // !defined(SOCK_CLOEXEC)

static bool setSocketBlocking(const SOCKET fd, bool blocking)
{
#if   defined(WZ_OS_UNIX)
	int sockopts = fcntl(fd, F_GETFL);
	if (sockopts == SOCKET_ERROR)
	{
		debug(LOG_NET, "Failed to retrieve current socket options: %s", strSockError(getSockErr()));
		return false;
	}

	// Set or clear O_NONBLOCK flag
	if (blocking)
	{
		sockopts &= ~O_NONBLOCK;
	}
	else
	{
		sockopts |= O_NONBLOCK;
	}

	if (fcntl(fd, F_SETFL, sockopts) == SOCKET_ERROR)
#elif defined(WZ_OS_WIN)
	unsigned long nonblocking = !blocking;
	if (ioctlsocket(fd, FIONBIO, &nonblocking) == SOCKET_ERROR)
#endif
	{
		debug(LOG_NET, "Failed to set socket %sblocking: %s", (blocking ? "" : "non-"), strSockError(getSockErr()));
		return false;
	}

	debug(LOG_NET, "Socket is set to %sblocking.", (blocking ? "" : "non-"));
	return true;
}

static void socketBlockSIGPIPE(const SOCKET fd, bool block_sigpipe)
{
#if defined(SO_NOSIGPIPE)
	const int no_sigpipe = block_sigpipe ? 1 : 0;

	if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe, sizeof(no_sigpipe)) == SOCKET_ERROR)
	{
		debug(LOG_INFO, "Failed to set SO_NOSIGPIPE on socket, SIGPIPE might be raised when connections gets broken. Error: %s", strSockError(getSockErr()));
	}
	// this is only for unix, windows don't have SIGPIPE
	debug(LOG_NET, "Socket fd %x sets SIGPIPE to %sblocked.", fd, (block_sigpipe ? "" : "non-"));
#else
	// Prevent warnings
	(void)fd;
	(void)block_sigpipe;
#endif
}

void socketCloseNow(Socket *sock)
{
	for (unsigned i = 0; i < ARRAY_SIZE(sock->fd); ++i)
	{
		if (sock->fd[i] != INVALID_SOCKET)
		{
#if defined(WZ_OS_WIN)
			int err = closesocket(sock->fd[i]);
#else
			int err = close(sock->fd[i]);
#endif
			if (err)
			{
				debug(LOG_ERROR, "Failed to close socket %p: %s", static_cast<void *>(sock), strSockError(getSockErr()));
			}

			/* Make sure that dangling pointers to this
			 * structure don't think they've got their
			 * hands on a valid socket.
			 */
			sock->fd[i] = INVALID_SOCKET;
		}
	}
	delete sock;
}

Socket *socketAccept(Socket *sock)
{
	unsigned int i;

	/* Search for a socket that has a pending connection on it and accept
	 * the first one.
	 */
	for (i = 0; i < ARRAY_SIZE(sock->fd); ++i)
	{
		if (sock->fd[i] != INVALID_SOCKET)
		{
			struct sockaddr_storage addr;
			socklen_t addr_len = sizeof(addr);
			Socket *conn;
			unsigned int j;

#if defined(SOCK_CLOEXEC)
			const SOCKET newConn = accept4(sock->fd[i], (struct sockaddr *)&addr, &addr_len, SOCK_CLOEXEC);
#else
			const SOCKET newConn = accept(sock->fd[i], (struct sockaddr *)&addr, &addr_len);
#endif
			if (newConn == INVALID_SOCKET)
			{
				// Ignore the case where no connection is pending
				if (getSockErr() != EAGAIN
				    && getSockErr() != EWOULDBLOCK)
				{
					debug(LOG_ERROR, "accept failed for socket %p: %s", static_cast<void *>(sock), strSockError(getSockErr()));
				}

				continue;
			}

			conn = new Socket;
			if (conn == nullptr)
			{
				debug(LOG_ERROR, "Out of memory!");
				abort();
				return nullptr;
			}

#if !defined(SOCK_CLOEXEC)
			if (!setSocketInheritable(newConn, false))
			{
				debug(LOG_NET, "Couldn't set socket (%p) inheritable status (false). Ignoring...", static_cast<void *>(conn));
				// ignore and continue
			}
#endif

			debug(LOG_NET, "setting socket (%p) blocking status (false).", static_cast<void *>(conn));
			if (!setSocketBlocking(newConn, false))
			{
				debug(LOG_NET, "Couldn't set socket (%p) blocking status (false).  Closing.", static_cast<void *>(conn));
				socketCloseNow(conn);
				return nullptr;
			}

			socketBlockSIGPIPE(newConn, true);

			// Mark all unused socket handles as invalid
			for (j = 0; j < ARRAY_SIZE(conn->fd); ++j)
			{
				conn->fd[j] = INVALID_SOCKET;
			}

			conn->fd[SOCK_CONNECTION] = newConn;

			sock->ready = false;

			addressToText((const struct sockaddr *)&addr, conn->textAddress, sizeof(conn->textAddress));
			debug(LOG_NET, "Incoming connection from [%s]:/*%%d*/ (FIXME: gives strict-aliasing error)", conn->textAddress/*, (unsigned int)ntohs(((const struct sockaddr_in*)&addr)->sin_port)*/);
			debug(LOG_NET, "Using socket %p", static_cast<void *>(conn));
			return conn;
		}
	}

	return nullptr;
}

net::result<Socket*> socketOpen(const SocketAddress *addr, unsigned timeout)
{
	unsigned int i;
	int ret;

	Socket *const conn = new Socket;

	ASSERT(addr != nullptr, "NULL Socket provided");

	addressToText(addr->ai_addr, conn->textAddress, sizeof(conn->textAddress));
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wcast-align"
#endif
	debug(LOG_NET, "Connecting to [%s]:%d", conn->textAddress, (int)ntohs((reinterpret_cast<const sockaddr_in *>(addr->ai_addr))->sin_port));
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
# pragma GCC diagnostic pop
#endif

	// Mark all unused socket handles as invalid
	for (i = 0; i < ARRAY_SIZE(conn->fd); ++i)
	{
		conn->fd[i] = INVALID_SOCKET;
	}

	int socket_type = addr->ai_socktype;
#if defined(SOCK_CLOEXEC)
	socket_type |= SOCK_CLOEXEC;
#endif
	conn->fd[SOCK_CONNECTION] = socket(addr->ai_family, socket_type, addr->ai_protocol);

	if (conn->fd[SOCK_CONNECTION] == INVALID_SOCKET)
	{
		const auto sockErr = getSockErr();
		debug(LOG_ERROR, "Failed to create a socket (%p): %s", static_cast<void *>(conn), strSockError(sockErr));
		socketCloseNow(conn);
		return tl::make_unexpected(make_network_error_code(sockErr));
	}

#if !defined(SOCK_CLOEXEC)
	if (!setSocketInheritable(conn->fd[SOCK_CONNECTION], false))
	{
		debug(LOG_NET, "Couldn't set socket (%p) inheritable status (false). Ignoring...", static_cast<void *>(conn));
		// ignore and continue
	}
#endif

	debug(LOG_NET, "setting socket (%p) blocking status (false).", static_cast<void *>(conn));
	if (!setSocketBlocking(conn->fd[SOCK_CONNECTION], false))
	{
		const auto sockErr = getSockErr();
		debug(LOG_NET, "Couldn't set socket (%p) blocking status (false).  Closing.", static_cast<void *>(conn));
		socketCloseNow(conn);
		return tl::make_unexpected(make_network_error_code(sockErr));
	}

	socketBlockSIGPIPE(conn->fd[SOCK_CONNECTION], true);

	ret = connect(conn->fd[SOCK_CONNECTION], addr->ai_addr, addr->ai_addrlen);
	if (ret == SOCKET_ERROR)
	{
		fd_set conReady;
#if   defined(WZ_OS_WIN)
		fd_set conFailed;
#endif

		if ((getSockErr() != EINPROGRESS
		     && getSockErr() != EAGAIN
		     && getSockErr() != EWOULDBLOCK)
#if   defined(WZ_OS_UNIX)
		    || conn->fd[SOCK_CONNECTION] >= FD_SETSIZE
#endif
		    || timeout == 0)
		{
			const auto sockErr = getSockErr();
			debug(LOG_NET, "Failed to start connecting: %s, using socket %p", strSockError(sockErr), static_cast<void *>(conn));
			socketCloseNow(conn);
			return tl::make_unexpected(make_network_error_code(sockErr));
		}

		do
		{
			struct timeval tv = {(int)(timeout / 1000), (int)(timeout % 1000) * 1000};  // Cast to int to avoid narrowing needed for C++11.

			FD_ZERO(&conReady);
			FD_SET(conn->fd[SOCK_CONNECTION], &conReady);
#if   defined(WZ_OS_WIN)
			FD_ZERO(&conFailed);
			FD_SET(conn->fd[SOCK_CONNECTION], &conFailed);
#endif

#if   defined(WZ_OS_WIN)
			ret = select(conn->fd[SOCK_CONNECTION] + 1, NULL, &conReady, &conFailed, &tv);
#else
			ret = select(conn->fd[SOCK_CONNECTION] + 1, nullptr, &conReady, nullptr, &tv);
#endif
		}
		while (ret == SOCKET_ERROR && getSockErr() == EINTR);

		if (ret == SOCKET_ERROR)
		{
			const auto sockErr = getSockErr();
			debug(LOG_NET, "Failed to wait for connection: %s, socket %p.  Closing.", strSockError(sockErr), static_cast<void *>(conn));
			socketCloseNow(conn);
			return tl::make_unexpected(make_network_error_code(sockErr));
		}

		if (ret == 0)
		{
			const auto sockErr = ETIMEDOUT;
			debug(LOG_NET, "Timed out while waiting for connection to be established: %s, using socket %p.  Closing.", strSockError(sockErr), static_cast<void *>(conn));
			socketCloseNow(conn);
			return tl::make_unexpected(make_network_error_code(sockErr));
		}

#if   defined(WZ_OS_WIN)
		ASSERT(FD_ISSET(conn->fd[SOCK_CONNECTION], &conReady) || FD_ISSET(conn->fd[SOCK_CONNECTION], &conFailed), "\"sock\" is the only file descriptor in set, it should be the one that is set.");
#else
		ASSERT(FD_ISSET(conn->fd[SOCK_CONNECTION], &conReady), "\"sock\" is the only file descriptor in set, it should be the one that is set.");
#endif

#if   defined(WZ_OS_WIN)
		if (FD_ISSET(conn->fd[SOCK_CONNECTION], &conFailed))
#elif defined(WZ_OS_UNIX)
		if (connect(conn->fd[SOCK_CONNECTION], addr->ai_addr, addr->ai_addrlen) == SOCKET_ERROR
		    && getSockErr() != EISCONN)
#endif
		{
			const auto sockErr = getSockErr();
			debug(LOG_NET, "Failed to connect: %s, with socket %p.  Closing.", strSockError(sockErr), static_cast<void *>(conn));
			socketCloseNow(conn);
			return tl::make_unexpected(make_network_error_code(sockErr));
		}
	}

	return conn;
}

net::result<Socket*> socketListen(unsigned int port)
{
	/* Enable the V4 to V6 mapping, but only when available, because it
	 * isn't available on all platforms.
	 */
#if defined(IPV6_V6ONLY)
	static const int ipv6_v6only = 0;
#endif
	static const int so_reuseaddr = 1;

	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;
	unsigned int i;

	Socket *const conn = new Socket;

	// Mark all unused socket handles as invalid
	for (i = 0; i < ARRAY_SIZE(conn->fd); ++i)
	{
		conn->fd[i] = INVALID_SOCKET;
	}

	strncpy(conn->textAddress, "LISTENING SOCKET", sizeof(conn->textAddress));

	// Listen on all local IPv4 and IPv6 addresses for the given port
	addr4.sin_family      = AF_INET;
	addr4.sin_port        = htons(port);
	addr4.sin_addr.s_addr = INADDR_ANY;

	addr6.sin6_family   = AF_INET6;
	addr6.sin6_port     = htons(port);
	addr6.sin6_addr     = in6addr_any;
	addr6.sin6_flowinfo = 0;
	addr6.sin6_scope_id = 0;

	int socket_type = SOCK_STREAM;
#if defined(SOCK_CLOEXEC)
	socket_type |= SOCK_CLOEXEC;
#endif
	conn->fd[SOCK_IPV4_LISTEN] = socket(addr4.sin_family, socket_type, 0);
	conn->fd[SOCK_IPV6_LISTEN] = socket(addr6.sin6_family, socket_type, 0);

	if (conn->fd[SOCK_IPV4_LISTEN] == INVALID_SOCKET
	    && conn->fd[SOCK_IPV6_LISTEN] == INVALID_SOCKET)
	{
		const auto errorCode = getSockErr();
		debug(LOG_ERROR, "Failed to create an IPv4 and IPv6 (only supported address families) socket (%p): %s.  Closing.", static_cast<void *>(conn), strSockError(errorCode));
		socketCloseNow(conn);
		return tl::make_unexpected(make_network_error_code(errorCode));
	}

	if (conn->fd[SOCK_IPV4_LISTEN] != INVALID_SOCKET)
	{
		debug(LOG_NET, "Successfully created an IPv4 socket (%p)", static_cast<void *>(conn));
	}

	if (conn->fd[SOCK_IPV6_LISTEN] != INVALID_SOCKET)
	{
		debug(LOG_NET, "Successfully created an IPv6 socket (%p)", static_cast<void *>(conn));
	}

#if defined(IPV6_V6ONLY)
	if (conn->fd[SOCK_IPV6_LISTEN] != INVALID_SOCKET)
	{
		if (setsockopt(conn->fd[SOCK_IPV6_LISTEN], IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&ipv6_v6only, sizeof(ipv6_v6only)) == SOCKET_ERROR)
		{
			debug(LOG_INFO, "Failed to set IPv6 socket to perform IPv4 to IPv6 mapping. Falling back to using two sockets. Error: %s", strSockError(getSockErr()));
		}
		else
		{
			debug(LOG_NET, "Successfully enabled IPv4 to IPv6 mapping. Cleaning up IPv4 socket.");
#if   defined(WZ_OS_WIN)
			closesocket(conn->fd[SOCK_IPV4_LISTEN]);
#else
			close(conn->fd[SOCK_IPV4_LISTEN]);
#endif
			conn->fd[SOCK_IPV4_LISTEN] = INVALID_SOCKET;
		}
	}
#endif

	if (conn->fd[SOCK_IPV4_LISTEN] != INVALID_SOCKET)
	{
#if !defined(SOCK_CLOEXEC)
		if (!setSocketInheritable(conn->fd[SOCK_IPV4_LISTEN], false))
		{
			debug(LOG_NET, "Couldn't set socket (%p) inheritable status (false). Ignoring...", static_cast<void *>(conn));
			// ignore and continue
		}
#endif

		if (setsockopt(conn->fd[SOCK_IPV4_LISTEN], SOL_SOCKET, SO_REUSEADDR, (const char *)&so_reuseaddr, sizeof(so_reuseaddr)) == SOCKET_ERROR)
		{
			debug(LOG_WARNING, "Failed to set SO_REUSEADDR on IPv4 socket. Error: %s", strSockError(getSockErr()));
		}

		debug(LOG_NET, "setting socket (%p) blocking status (false, IPv4).", static_cast<void *>(conn));
		if (bind(conn->fd[SOCK_IPV4_LISTEN], (const struct sockaddr *)&addr4, sizeof(addr4)) == SOCKET_ERROR
		    || listen(conn->fd[SOCK_IPV4_LISTEN], 5) == SOCKET_ERROR
		    || !setSocketBlocking(conn->fd[SOCK_IPV4_LISTEN], false))
		{
			debug(LOG_ERROR, "Failed to set up IPv4 socket for listening on port %u: %s", port, strSockError(getSockErr()));
#if   defined(WZ_OS_WIN)
			closesocket(conn->fd[SOCK_IPV4_LISTEN]);
#else
			close(conn->fd[SOCK_IPV4_LISTEN]);
#endif
			conn->fd[SOCK_IPV4_LISTEN] = INVALID_SOCKET;
		}
	}

	if (conn->fd[SOCK_IPV6_LISTEN] != INVALID_SOCKET)
	{
#if !defined(SOCK_CLOEXEC)
		if (!setSocketInheritable(conn->fd[SOCK_IPV6_LISTEN], false))
		{
			debug(LOG_NET, "Couldn't set socket (%p) inheritable status (false). Ignoring...", static_cast<void *>(conn));
			// ignore and continue
		}
#endif

		if (setsockopt(conn->fd[SOCK_IPV6_LISTEN], SOL_SOCKET, SO_REUSEADDR, (const char *)&so_reuseaddr, sizeof(so_reuseaddr)) == SOCKET_ERROR)
		{
			debug(LOG_INFO, "Failed to set SO_REUSEADDR on IPv6 socket. Error: %s", strSockError(getSockErr()));
		}

		debug(LOG_NET, "setting socket (%p) blocking status (false, IPv6).", static_cast<void *>(conn));
		if (bind(conn->fd[SOCK_IPV6_LISTEN], (const struct sockaddr *)&addr6, sizeof(addr6)) == SOCKET_ERROR
		    || listen(conn->fd[SOCK_IPV6_LISTEN], 5) == SOCKET_ERROR
		    || !setSocketBlocking(conn->fd[SOCK_IPV6_LISTEN], false))
		{
			debug(LOG_ERROR, "Failed to set up IPv6 socket for listening on port %u: %s", port, strSockError(getSockErr()));
#if   defined(WZ_OS_WIN)
			closesocket(conn->fd[SOCK_IPV6_LISTEN]);
#else
			close(conn->fd[SOCK_IPV6_LISTEN]);
#endif
			conn->fd[SOCK_IPV6_LISTEN] = INVALID_SOCKET;
		}
	}

	// Check whether we still have at least a single (operating) socket.
	if (conn->fd[SOCK_IPV4_LISTEN] == INVALID_SOCKET
	    && conn->fd[SOCK_IPV6_LISTEN] == INVALID_SOCKET)
	{
		const auto errorCode = getSockErr();
		debug(LOG_NET, "No IPv4 or IPv6 sockets created.");
		socketCloseNow(conn);
		return tl::make_unexpected(make_network_error_code(errorCode));
	}

	return conn;
}

net::result<Socket*> socketOpenAny(const SocketAddress *addr, unsigned timeout)
{
	net::result<Socket*> res;
	while (addr != nullptr)
	{
		res = socketOpen(addr, timeout);
		if (res)
		{
			return res;
		}
		addr = addr->ai_next;
	}

	return res;
}

bool socketHasIPv4(const Socket& sock)
{
	if (sock.fd[SOCK_IPV4_LISTEN] != INVALID_SOCKET)
	{
		return true;
	}
	else
	{
#if defined(IPV6_V6ONLY)
		if (sock.fd[SOCK_IPV6_LISTEN] != INVALID_SOCKET)
		{
			int ipv6_v6only = 1;
			socklen_t len = sizeof(ipv6_v6only);
			if (getsockopt(sock.fd[SOCK_IPV6_LISTEN], IPPROTO_IPV6, IPV6_V6ONLY, (char *)&ipv6_v6only, &len) == 0)
			{
				return ipv6_v6only == 0;
			}
		}
#endif
		return false;
	}
}

bool socketHasIPv6(const Socket& sock)
{
	return sock.fd[SOCK_IPV6_LISTEN] != INVALID_SOCKET;
}

char const *getSocketTextAddress(const Socket& sock)
{
	return sock.textAddress;
}

std::vector<unsigned char> ipv4_AddressString_To_NetBinary(const std::string& ipv4Address)
{
	std::vector<unsigned char> binaryForm(sizeof(struct in_addr), 0);
	if (inet_pton(AF_INET, ipv4Address.c_str(), binaryForm.data()) <= 0)
	{
		// inet_pton failed
		binaryForm.clear();
	}
	return binaryForm;
}

#ifndef INET_ADDRSTRLEN
# define INET_ADDRSTRLEN 16
#endif

std::string ipv4_NetBinary_To_AddressString(const std::vector<unsigned char>& ip4NetBinaryForm)
{
	if (ip4NetBinaryForm.size() != sizeof(struct in_addr))
	{
		return "";
	}

	char buf[INET_ADDRSTRLEN] = {0};
	if (inet_ntop(AF_INET, ip4NetBinaryForm.data(), buf, sizeof(buf)) == nullptr)
	{
		return "";
	}
	std::string ipv4Address = buf;
	return ipv4Address;
}

std::vector<unsigned char> ipv6_AddressString_To_NetBinary(const std::string& ipv6Address)
{
	std::vector<unsigned char> binaryForm(sizeof(struct in6_addr), 0);
	if (inet_pton(AF_INET6, ipv6Address.c_str(), binaryForm.data()) <= 0)
	{
		// inet_pton failed
		binaryForm.clear();
	}
	return binaryForm;
}

#ifndef INET6_ADDRSTRLEN
# define INET6_ADDRSTRLEN 46
#endif

std::string ipv6_NetBinary_To_AddressString(const std::vector<unsigned char>& ip6NetBinaryForm)
{
	if (ip6NetBinaryForm.size() != sizeof(struct in6_addr))
	{
		return "";
	}

	char buf[INET6_ADDRSTRLEN] = {0};
	if (inet_ntop(AF_INET6, ip6NetBinaryForm.data(), buf, sizeof(buf)) == nullptr)
	{
		return "";
	}
	std::string ipv6Address = buf;
	return ipv6Address;
}

net::result<SocketAddress*> resolveHost(const char *host, unsigned int port)
{
	struct addrinfo *results;
	std::string service;
	struct addrinfo hint;
	int flags = 0;

	hint.ai_family    = AF_UNSPEC;
	hint.ai_socktype  = SOCK_STREAM;
	hint.ai_protocol  = 0;
#ifdef AI_V4MAPPED
	flags             |= AI_V4MAPPED;
#endif
#ifdef AI_ADDRCONFIG
	flags             |= AI_ADDRCONFIG;
#endif
	hint.ai_flags     = flags;
	hint.ai_addrlen   = 0;
	hint.ai_addr      = nullptr;
	hint.ai_canonname = nullptr;
	hint.ai_next      = nullptr;

	service = astringf("%u", port);

	auto error = getaddrinfo(host, service.c_str(), &hint, &results);
	if (error != 0)
	{
		const auto ec = make_getaddrinfo_error_code(error);
		const auto errMsg = ec.message();
		debug(LOG_NET, "getaddrinfo failed for %s:%s: %s", host, service.c_str(), errMsg.c_str());
		return tl::make_unexpected(ec);
	}

	return results;
}

// ////////////////////////////////////////////////////////////////////////
// setup stuff
void SOCKETinit()
{
#if defined(WZ_OS_WIN)
	static WSADATA stuff;
	constexpr WORD ver_required = (2 << 8) + 2; // Winsock 2.2, should always be available

	// `WSAStartup`/ `WSACleanup` can be called many times, just need to make sure that
	// for each `WSAStartup` there's a matching `WSACleanup` call.
	//
	// For more details, see: https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup#remarks
	if (WSAStartup(ver_required, &stuff) != 0)
	{
		debug(LOG_ERROR, "Failed to initialize Winsock: %s", strSockError(getSockErr()));
		return;
	}
#endif
}

void SOCKETshutdown()
{
#if defined(WZ_OS_WIN)
	WSACleanup();
#endif
}

} // namespace tcp
