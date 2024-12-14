/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2024  Warzone 2100 Project

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

#ifndef _net_socket_h
#define _net_socket_h

#include "lib/framework/types.h"
#include <string>
#include <system_error>
#include <vector>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;
#include <tl/expected.hpp>

namespace net
{
	template <typename T>
	using result = ::tl::expected<T, std::error_code>;
} // namespace net

#if   defined(WZ_OS_UNIX)
# include <arpa/inet.h>
# include <errno.h>
# include <fcntl.h>
# include <netdb.h>
# include <netinet/in.h>
# include <sys/ioctl.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <sys/select.h>
# include <unistd.h>
typedef int SOCKET;
static const SOCKET INVALID_SOCKET = -1;
#endif

#ifdef WZ_OS_WIN
# include <winsock2.h>
# include <ws2tcpip.h>
# undef EAGAIN
# undef EBADF
# undef ECONNRESET
# undef EINPROGRESS
# undef EINTR
# undef EISCONN
# undef ETIMEDOUT
# undef EWOULDBLOCK
# define EAGAIN      WSAEWOULDBLOCK
# define EBADF       WSAEBADF
# define ECONNRESET  WSAECONNRESET
# define EINPROGRESS WSAEINPROGRESS
# define EINTR       WSAEINTR
# define EISCONN     WSAEISCONN
# define ETIMEDOUT   WSAETIMEDOUT
# define EWOULDBLOCK WSAEWOULDBLOCK
# ifndef AI_V4MAPPED
#  define AI_V4MAPPED	0x0008	/* IPv4 mapped addresses are acceptable.  */
# endif
# ifndef AI_ADDRCONFIG
#  define AI_ADDRCONFIG	0x0020	/* Use configuration of this host to choose returned address type..  */
# endif
#endif

// Fallback for systems that don't #define this flag
#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

struct Socket;
struct SocketSet;
typedef struct addrinfo SocketAddress;

#ifndef WZ_OS_WIN
static const int SOCKET_ERROR = -1;
#endif


// Init/shutdown.
void SOCKETinit();
void SOCKETshutdown();

// Socket addresses.
net::result<SocketAddress*> resolveHost(const char *host, unsigned port);            ///< Looks up a socket address.
WZ_DECL_NONNULL(1) void deleteSocketAddress(SocketAddress *addr);       ///< Destroys the socket address.

// Sockets.
net::result<Socket*> socketOpen(const SocketAddress *addr, unsigned timeout);        ///< Opens a Socket, using the first address in addr.
net::result<Socket*> socketListen(unsigned int port);                                ///< Creates a listen-only Socket, which listens for incoming connections.
WZ_DECL_NONNULL(1) Socket *socketAccept(Socket *sock);                  ///< Accepts an incoming Socket connection from a listening Socket.
WZ_DECL_NONNULL(1) void socketClose(Socket *sock);                      ///< Destroys the Socket.
net::result<Socket*> socketOpenAny(const SocketAddress *addr, unsigned timeout);     ///< Opens a Socket, using the first address that works in addr.
bool socketHasIPv4(const Socket& sock);
bool socketHasIPv6(const Socket& sock);

char const *getSocketTextAddress(const Socket& sock); ///< Gets a string with the socket address.
std::vector<unsigned char> ipv4_AddressString_To_NetBinary(const std::string& ipv4Address);
std::vector<unsigned char> ipv6_AddressString_To_NetBinary(const std::string& ipv6Address);
std::string ipv4_NetBinary_To_AddressString(const std::vector<unsigned char>& ip4NetBinaryForm);
std::string ipv6_NetBinary_To_AddressString(const std::vector<unsigned char>& ip6NetBinaryForm);
bool socketReadReady(const Socket& sock);            ///< Returns if checkSockets found data to read from this Socket.
WZ_DECL_NONNULL(2)
net::result<ssize_t> readNoInt(Socket& sock, void *buf, size_t max_size, size_t *rawByteCount = nullptr);  ///< Reads up to max_size bytes from the Socket. Raw count of bytes (after compression) returned in rawByteCount.
WZ_DECL_NONNULL(2)
net::result<ssize_t> readAll(Socket& sock, void *buf, size_t size, unsigned timeout);///< Reads exactly size bytes from the Socket, or blocks until the timeout expires.
WZ_DECL_NONNULL(2)
net::result<ssize_t> writeAll(Socket& sock, const void *buf, size_t size, size_t *rawByteCount = nullptr);  ///< Nonblocking write of size bytes to the Socket. All bytes will be written asynchronously, by a separate thread. Raw count of bytes (after compression) returned in rawByteCount, which will often be 0 until the socket is flushed.

bool socketSetTCPNoDelay(Socket& sock, bool nodelay); ///< nodelay = true disables the Nagle algorithm for TCP socket

// Sockets, compressed.
void socketBeginCompression(Socket& sock); ///< Makes future data sent compressed, and future data received expected to be compressed.
void socketFlush(Socket& sock, uint8_t player, size_t *rawByteCount = nullptr); ///< Actually sends the data written with writeAll. Only useful on compressed sockets. Note that flushing too often makes compression less effective. Raw count of bytes (after compression) returned in rawByteCount.

// Socket sets.
WZ_DECL_ALLOCATION SocketSet *allocSocketSet();                         ///< Constructs a SocketSet.
WZ_DECL_NONNULL(1) void deleteSocketSet(SocketSet *set);                ///< Destroys the SocketSet.

WZ_DECL_NONNULL(2) void SocketSet_AddSocket(SocketSet& set, Socket *socket);  ///< Adds a Socket to a SocketSet.
WZ_DECL_NONNULL(2) void SocketSet_DelSocket(SocketSet& set, Socket *socket);  ///< Removes a Socket from a SocketSet.
int checkSockets(const SocketSet& set, unsigned int timeout); ///< Checks which Sockets are ready for reading. Returns the number of ready Sockets, or returns SOCKET_ERROR on error.

// Higher-level functions for opening a connection / socket
struct OpenConnectionResult
{
	OpenConnectionResult(std::error_code ec, std::string errorString)
	: errorCode(ec)
	, errorString(errorString)
	{ }

	OpenConnectionResult(Socket* open_socket)
	: open_socket(open_socket)
	{ }

	bool hasError() const { return errorCode.has_value(); }

	OpenConnectionResult( const OpenConnectionResult& other ) = delete; // non construction-copyable
	OpenConnectionResult& operator=( const OpenConnectionResult& ) = delete; // non copyable
	OpenConnectionResult(OpenConnectionResult&&) = default;
	OpenConnectionResult& operator=(OpenConnectionResult&&) = default;

	struct SocketDeleter {
		void operator()(Socket* b) { if (b) { socketClose(b); } }
	};
	std::unique_ptr<Socket, SocketDeleter> open_socket;
	optional<std::error_code> errorCode = nullopt;
	std::string errorString;
};
typedef std::function<void (OpenConnectionResult&& result)> OpenConnectionToHostResultCallback;
bool socketOpenTCPConnectionAsync(const std::string& host, uint32_t port, OpenConnectionToHostResultCallback callback);
OpenConnectionResult socketOpenTCPConnectionSync(const char *host, uint32_t port);

#endif //_net_socket_h
