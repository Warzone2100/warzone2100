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

#include "lib/framework/wzglobal.h"
#include "lib/framework/types.h"
#include <string>
#include <system_error>
#include <vector>
#include <type_traits>

#include "lib/netplay/net_result.h"

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
# ifndef AI_V4MAPPED
#  define AI_V4MAPPED	0x0008	/* IPv4 mapped addresses are acceptable.  */
# endif
# ifndef AI_ADDRCONFIG
#  define AI_ADDRCONFIG	0x0020	/* Use configuration of this host to choose returned address type..  */
# endif
#endif

#include "lib/netplay/tcp/sock_error.h"

// Fallback for systems that don't #define this flag
#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

class IDescriptorSet;
class IClientConnection;

namespace tcp
{

struct Socket;

} // namespace tcp

typedef struct addrinfo SocketAddress;

#ifndef WZ_OS_WIN
static const int SOCKET_ERROR = -1;
#endif

namespace tcp
{

// Init/shutdown.
void SOCKETinit();
void SOCKETshutdown();

SOCKET getRawSocketFd(const Socket& sock);
bool isValidSocket(const Socket& sock);

// Socket addresses.
net::result<SocketAddress*> resolveHost(const char *host, unsigned port);            ///< Looks up a socket address.

// Sockets.
net::result<Socket*> socketOpen(const SocketAddress *addr, unsigned timeout);        ///< Opens a Socket, using the first address in addr.
net::result<Socket*> socketListen(unsigned int port);                                ///< Creates a listen-only Socket, which listens for incoming connections.
WZ_DECL_NONNULL(1) Socket *socketAccept(Socket *sock);                  ///< Accepts an incoming Socket connection from a listening Socket.
WZ_DECL_NONNULL(1) void socketCloseNow(Socket* sock); ///< Immediately destroys the socket.
net::result<Socket*> socketOpenAny(const SocketAddress *addr, unsigned timeout);     ///< Opens a Socket, using the first address that works in addr.
bool socketHasIPv4(const Socket& sock);
bool socketHasIPv6(const Socket& sock);

char const *getSocketTextAddress(const Socket& sock); ///< Gets a string with the socket address.
std::vector<unsigned char> ipv4_AddressString_To_NetBinary(const std::string& ipv4Address);
std::vector<unsigned char> ipv6_AddressString_To_NetBinary(const std::string& ipv6Address);
std::string ipv4_NetBinary_To_AddressString(const std::vector<unsigned char>& ip4NetBinaryForm);
std::string ipv6_NetBinary_To_AddressString(const std::vector<unsigned char>& ip6NetBinaryForm);
bool socketReadReady(const Socket& sock);            ///< Returns if checkSockets found data to read from this Socket.
void socketSetReadReady(Socket& sock, bool ready);

bool socketSetTCPNoDelay(Socket& sock, bool nodelay); ///< nodelay = true disables the Nagle algorithm for TCP socket

} // namespace tcp

#endif //_net_socket_h
