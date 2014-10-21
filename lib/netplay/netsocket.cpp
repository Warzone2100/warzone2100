/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#include <vector>
#include <algorithm>
#include <map>

#if defined(WZ_CC_MSVC)
# define ZLIB_WINAPI
#endif
#include <zlib.h>

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
	Socket() : ready(false), writeError(false), deleteLater(false), isCompressed(false), readDisconnected(false), zDeflateInSize(0)
	{
		memset(&zDeflate, 0, sizeof(zDeflate));
		memset(&zInflate, 0, sizeof(zInflate));
	}
	~Socket();

	SOCKET fd[SOCK_COUNT];
	bool ready;
	bool writeError;
	bool deleteLater;
	char textAddress[40];

	bool isCompressed;
	bool readDisconnected;  ///< True iff a call to recv() returned 0.
	z_stream zDeflate;
	z_stream zInflate;
	unsigned zDeflateInSize;
	bool zInflateNeedInput;
	std::vector<uint8_t> zDeflateOutBuf;
	std::vector<uint8_t> zInflateInBuf;
};

struct SocketSet
{
	std::vector<Socket *> fds;
};


static WZ_MUTEX *socketThreadMutex;
static WZ_SEMAPHORE *socketThreadSemaphore;
static WZ_THREAD *socketThread = NULL;
static bool socketThreadQuit;
typedef std::map<Socket *, std::vector<uint8_t> > SocketThreadWriteMap;
static SocketThreadWriteMap socketThreadWrites;


static void socketCloseNow(Socket *sock);


bool socketReadReady(Socket const *sock)
{
	return sock->ready;
}

int getSockErr(void)
{
#if   defined(WZ_OS_UNIX)
	return errno;
#elif defined(WZ_OS_WIN)
	return WSAGetLastError();
#endif
}

void setSockErr(int error)
{
#if   defined(WZ_OS_UNIX)
	errno = error;
#elif defined(WZ_OS_WIN)
	WSASetLastError(error);
#endif
}

#if defined(WZ_OS_WIN)
typedef int (WINAPI * GETADDRINFO_DLL_FUNC)(const char *node, const char *service,
                                           const struct addrinfo *hints,
                                           struct addrinfo **res);
typedef int (WINAPI * FREEADDRINFO_DLL_FUNC)(struct addrinfo *res);

static HMODULE winsock2_dll = NULL;
static unsigned int major_windows_version = 0;

static GETADDRINFO_DLL_FUNC getaddrinfo_dll_func = NULL;
static FREEADDRINFO_DLL_FUNC freeaddrinfo_dll_func = NULL;

# define getaddrinfo  getaddrinfo_dll_dispatcher
# define freeaddrinfo freeaddrinfo_dll_dispatcher

static int getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints,
                       struct addrinfo **res)
{
	struct addrinfo hint;
	if (hints)
	{
		memcpy(&hint, hints, sizeof(hint));
	}

	switch (major_windows_version)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		// Windows 95, 98 and ME
		case 4:
			debug(LOG_ERROR, "Name resolution isn't supported on this version (%u) of Windows", major_windows_version);
			return EAI_FAIL;

		// Windows 2000, XP and Server 2003
		case 5:
			if (hints)
			{
				// These flags are only supported from version 6 and onward
				hint.ai_flags &= ~(AI_V4MAPPED | AI_ADDRCONFIG);
			}
		// Windows Vista and Server 2008
		case 6:
		// Onward (aka: in the future)
		default:
			if (!winsock2_dll)
			{
				debug(LOG_ERROR, "Failed to load winsock2 DLL. Required for name resolution.");
				return EAI_FAIL;
			}

			if (!getaddrinfo_dll_func)
			{
				debug(LOG_ERROR, "Failed to retrieve \"getaddrinfo\" function from winsock2 DLL. Required for name resolution.");
				return EAI_FAIL;
			}

			return getaddrinfo_dll_func(node, service, hints ? &hint: NULL, res);
	}
}

static void freeaddrinfo(struct addrinfo *res)
{
	switch (major_windows_version)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		// Windows 95, 98 and ME
		case 4:
			debug(LOG_ERROR, "Name resolution isn't supported on this version (%u) of Windows", major_windows_version);
			return;

		// Windows 2000, XP and Server 2003
		case 5:
		// Windows Vista and Server 2008
		case 6:
		// Onward (aka: in the future)
		default:
			if (!winsock2_dll)
			{
				debug(LOG_ERROR, "Failed to load winsock2 DLL. Required for name resolution.");
				return;
			}

			if (!freeaddrinfo_dll_func)
			{
				debug(LOG_ERROR, "Failed to retrieve \"freeaddrinfo\" function from winsock2 DLL. Required for name resolution.");
				return;
			}

			freeaddrinfo_dll_func(res);
	}
}
#endif

static int addressToText(const struct sockaddr* addr, char* buf, size_t size)
{
	switch (addr->sa_family)
	{
		case AF_INET:
		{
			unsigned char* address = (unsigned char*)&((const struct sockaddr_in*)addr)->sin_addr.s_addr;

			return snprintf(buf, size,
			"%hhu.%hhu.%hhu.%hhu",
				address[0],
				address[1],
				address[2],
				address[3]);
		}
		case AF_INET6:
		{
			uint16_t* address = (uint16_t*)&((const struct sockaddr_in6*)addr)->sin6_addr.s6_addr;

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
		default:
			ASSERT(!"Unknown address family", "Got non IPv4 or IPv6 address!");
			return -1;
	}
}

const char* strSockError(int error)
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

/**
 * Test whether the given socket still has an open connection.
 *
 * @return true when the connection is open, false when it's closed or in an
 *         error state, check getSockErr() to find out which.
 */
static bool connectionIsOpen(Socket* sock)
{
	const SocketSet set = {std::vector<Socket *>(1, sock)};

	ASSERT_OR_RETURN((setSockErr(EBADF), false),
		sock && sock->fd[SOCK_CONNECTION] != INVALID_SOCKET, "Invalid socket");

	// Check whether the socket is still connected
	int ret = checkSockets(&set, 0);
	if (ret == SOCKET_ERROR)
	{
		return false;
	}
	else if (ret == (int)set.fds.size() && sock->ready)
	{
		/* The next recv(2) call won't block, but we're writing. So
		 * check the read queue to see if the connection is closed.
		 * If there's no data in the queue that means the connection
		 * is closed.
		 */
#if defined(WZ_OS_WIN)
		unsigned long readQueue;
		ret = ioctlsocket(sock->fd[SOCK_CONNECTION], FIONREAD, &readQueue);
#else
		int readQueue;
		ret = ioctl(sock->fd[SOCK_CONNECTION], FIONREAD, &readQueue);
#endif
		if (ret == SOCKET_ERROR)
		{
			debug(LOG_NET, "socket error");
			return false;
		}
		else if (readQueue == 0)
		{
			// Disconnected
			setSockErr(ECONNRESET);
			debug(LOG_NET, "Read queue empty - failing (ECONNRESET)");
			return false;
		}
	}

	return true;
}

static int socketThreadFunction(void *)
{
	wzMutexLock(socketThreadMutex);
	while (!socketThreadQuit)
	{
#if   defined(WZ_OS_UNIX)
		SOCKET maxfd = INT_MIN;
#elif defined(WZ_OS_WIN)
		SOCKET maxfd = 0;
#endif
		fd_set fds;
		FD_ZERO(&fds);
		for (SocketThreadWriteMap::iterator i = socketThreadWrites.begin(); i != socketThreadWrites.end(); ++i)
		{
			if (!i->second.empty())
			{
				SOCKET fd = i->first->fd[SOCK_CONNECTION];
				maxfd = std::max(maxfd, fd);
				ASSERT(!FD_ISSET(fd, &fds), "Duplicate file descriptor!");  // Shouldn't be possible, but blocking in send, after select says it won't block, shouldn't be possible either.
				FD_SET(fd, &fds);
			}
		}
		struct timeval tv = {0, 50 * 1000};

		// Check if we can write to any sockets.
		wzMutexUnlock(socketThreadMutex);
		int ret = select(maxfd + 1, NULL, &fds, NULL, &tv);
		wzMutexLock(socketThreadMutex);

		// We can write to some sockets. (Ignore errors from select, we may have deleted the socket after unlocking the mutex, and before calling select.)
		if (ret > 0)
		{
			for (SocketThreadWriteMap::iterator i = socketThreadWrites.begin(); i != socketThreadWrites.end(); )
			{
				SocketThreadWriteMap::iterator w = i;
				++i;

				Socket *sock = w->first;
				std::vector<uint8_t> &writeQueue = w->second;
				ASSERT(!writeQueue.empty(), "writeQueue[sock] must not be empty.");

				if (!FD_ISSET(sock->fd[SOCK_CONNECTION], &fds))
				{
					continue;  // This socket is not ready for writing, or we don't have anything to write.
				}

				// Write data.
				// FIXME SOMEHOW AAARGH This send() call can't block, but unless the socket is not set to blocking (setting the socket to nonblocking had better work, or else), does anyway (at least sometimes, when someone quits). Not reproducible except in public releases.
				ssize_t ret = send(sock->fd[SOCK_CONNECTION], reinterpret_cast<char *>(&writeQueue[0]), writeQueue.size(), MSG_NOSIGNAL);
				if (ret != SOCKET_ERROR)
				{
					// Erase as much data as written.
					writeQueue.erase(writeQueue.begin(), writeQueue.begin() + ret);
					if (writeQueue.empty())
					{
						socketThreadWrites.erase(w);  // Nothing left to write, delete from pending list.
						if (sock->deleteLater)
						{
							socketCloseNow(sock);
						}
					}
				}
				else
				{
					switch (getSockErr())
					{
						case EAGAIN:
#if defined(EWOULDBLOCK) && EAGAIN != EWOULDBLOCK
						case EWOULDBLOCK:
#endif
							if (!connectionIsOpen(sock))
							{
								debug(LOG_NET, "Socket error");
								sock->writeError = true;
								socketThreadWrites.erase(w);  // Socket broken, don't try writing to it again.
								if (sock->deleteLater)
								{
									socketCloseNow(sock);
								}
								break;
							}
						case EINTR:
							break;
#if defined(EPIPE)
						case EPIPE:
							debug(LOG_NET, "EPIPE generated");
							// fall through
#endif
						default:
							sock->writeError = true;
							socketThreadWrites.erase(w);  // Socket broken, don't try writing to it again.
							if (sock->deleteLater)
							{
								socketCloseNow(sock);
							}
							break;
					}
				}
			}
		}

		if (socketThreadWrites.empty())
		{
			// Nothing to do, expect to wait.
			wzMutexUnlock(socketThreadMutex);
			wzSemaphoreWait(socketThreadSemaphore);
			wzMutexLock(socketThreadMutex);
		}
	}
	wzMutexUnlock(socketThreadMutex);

	return 42;  // Return value arbitrary and unused.
}

/**
 * Similar to read(2) with the exception that this function won't be
 * interrupted by signals (EINTR).
 */
ssize_t readNoInt(Socket* sock, void* buf, size_t max_size, size_t *rawByteCount)
{
	size_t ignored;
	size_t &rawBytes = rawByteCount != NULL? *rawByteCount : ignored;
	rawBytes = 0;

	if (sock->fd[SOCK_CONNECTION] == INVALID_SOCKET)
	{
		debug(LOG_ERROR, "Invalid socket");
		setSockErr(EBADF);
		return SOCKET_ERROR;
	}

	if (sock->isCompressed)
	{
		if (sock->zInflateNeedInput)
		{
			// No input data, read some.

			sock->zInflateInBuf.resize(max_size + 1000);

			ssize_t received;
			do
			{  //                                                  v----- This weird cast is because recv() takes a char * on windows instead of a void *...
				received = recv(sock->fd[SOCK_CONNECTION], (char *)&sock->zInflateInBuf[0], sock->zInflateInBuf.size(), 0);
			} while (received == SOCKET_ERROR && getSockErr() == EINTR);
			if (received < 0)
			{
				return received;
			}

			sock->zInflate.next_in = &sock->zInflateInBuf[0];
			sock->zInflate.avail_in = received;
			rawBytes = received;

			if (received == 0)
			{
				sock->readDisconnected = true;
			}
			else
			{
				sock->zInflateNeedInput = false;
			}
		}

		sock->zInflate.next_out = (Bytef *)buf;
		sock->zInflate.avail_out = max_size;
		int ret = inflate(&sock->zInflate, Z_NO_FLUSH);
		ASSERT(ret != Z_STREAM_ERROR, "zlib inflate not working!");
		char const *err = NULL;
		switch (ret)
		{
			case Z_NEED_DICT:  err = "Z_NEED_DICT";  break;
			case Z_DATA_ERROR: err = "Z_DATA_ERROR"; break;
			case Z_MEM_ERROR:  err = "Z_MEM_ERROR";  break;
		}
		if (err != NULL)
		{
			debug(LOG_ERROR, "Couldn't decompress data from socket. zlib error %s", err);
			return -1;  // Bad data!
		}

		if (sock->zInflate.avail_out != 0)
		{
			sock->zInflateNeedInput = true;
			ASSERT(sock->zInflate.avail_in == 0, "zlib not consuming all input!");
		}

		return max_size - sock->zInflate.avail_out;  // Got some data, return how much.
	}

	ssize_t received;
	do
	{
		received = recv(sock->fd[SOCK_CONNECTION], (char*)buf, max_size, 0);
		if (received == 0)
		{
			sock->readDisconnected = true;
		}
	} while (received == SOCKET_ERROR && getSockErr() == EINTR);

	sock->ready = false;

	rawBytes = received;
	return received;
}

bool socketReadDisconnected(Socket *sock)
{
	return sock->readDisconnected;
}

/**
 * Similar to write(2) with the exception that this function will block until
 * <em>all</em> data has been written or an error occurs.
 *
 * @return @c size when succesful or @c SOCKET_ERROR if an error occurred.
 */
ssize_t writeAll(Socket* sock, const void* buf, size_t size, size_t *rawByteCount)
{
	size_t ignored;
	size_t &rawBytes = rawByteCount != NULL? *rawByteCount : ignored;
	rawBytes = 0;

	if (!sock
	 || sock->fd[SOCK_CONNECTION] == INVALID_SOCKET)
	{
		debug(LOG_ERROR, "Invalid socket (EBADF)");
		setSockErr(EBADF);
		return SOCKET_ERROR;
	}

	if (sock->writeError)
	{
		return SOCKET_ERROR;
	}

	if (size > 0)
	{
		if (!sock->isCompressed)
		{
			wzMutexLock(socketThreadMutex);
			if (socketThreadWrites.empty())
			{
				wzSemaphorePost(socketThreadSemaphore);
			}
			std::vector<uint8_t> &writeQueue = socketThreadWrites[sock];
			writeQueue.insert(writeQueue.end(), static_cast<char const *>(buf), static_cast<char const *>(buf) + size);
			wzMutexUnlock(socketThreadMutex);
			rawBytes = size;
		}
		else
		{
			sock->zDeflate.next_in = (Bytef *)buf;
			sock->zDeflate.avail_in = size;
			sock->zDeflateInSize += sock->zDeflate.avail_in;
			do
			{
				size_t alreadyHave = sock->zDeflateOutBuf.size();
				sock->zDeflateOutBuf.resize(alreadyHave + size + 20);  // A bit more than size should be enough to always do everything in one go.
				sock->zDeflate.next_out = (Bytef *)&sock->zDeflateOutBuf[alreadyHave];
				sock->zDeflate.avail_out = sock->zDeflateOutBuf.size() - alreadyHave;

				int ret = deflate(&sock->zDeflate, Z_NO_FLUSH);
				ASSERT(ret != Z_STREAM_ERROR, "zlib compression failed!");

				// Remove unused part of buffer.
				sock->zDeflateOutBuf.resize(sock->zDeflateOutBuf.size() - sock->zDeflate.avail_out);
			} while(sock->zDeflate.avail_out == 0);

			ASSERT(sock->zDeflate.avail_in == 0, "zlib didn't compress everything!");
		}
	}

	return size;
}

void socketFlush(Socket *sock, size_t *rawByteCount)
{
	size_t ignored;
	size_t &rawBytes = rawByteCount != NULL? *rawByteCount : ignored;
	rawBytes = 0;

	if (!sock->isCompressed)
	{
		return;  // Not compressed, so don't mess with zlib.
	}

	// Flush data out of zlib compression state.
	do
	{
		sock->zDeflate.next_in = (Bytef *)NULL;
		sock->zDeflate.avail_in = 0;
		size_t alreadyHave = sock->zDeflateOutBuf.size();
		sock->zDeflateOutBuf.resize(alreadyHave + 1000);  // 100 bytes would probably be enough to flush the rest in one go.
		sock->zDeflate.next_out = (Bytef *)&sock->zDeflateOutBuf[alreadyHave];
		sock->zDeflate.avail_out = sock->zDeflateOutBuf.size() - alreadyHave;

		int ret = deflate(&sock->zDeflate, Z_PARTIAL_FLUSH);
		ASSERT(ret != Z_STREAM_ERROR, "zlib compression failed!");

		// Remove unused part of buffer.
		sock->zDeflateOutBuf.resize(sock->zDeflateOutBuf.size() - sock->zDeflate.avail_out);
	} while(sock->zDeflate.avail_out == 0);

	if (sock->zDeflateOutBuf.empty())
	{
		return;  // No data to flush out.
	}

	wzMutexLock(socketThreadMutex);
	if (socketThreadWrites.empty())
	{
		wzSemaphorePost(socketThreadSemaphore);
	}
	std::vector<uint8_t> &writeQueue = socketThreadWrites[sock];
	writeQueue.insert(writeQueue.end(), sock->zDeflateOutBuf.begin(), sock->zDeflateOutBuf.end());
	wzMutexUnlock(socketThreadMutex);

	// Primitive network logging, uncomment to use.
	//printf("Size %3u ->%3zu, buf =", sock->zDeflateInSize, sock->zDeflateOutBuf.size());
	//for (unsigned n = 0; n < std::min<unsigned>(sock->zDeflateOutBuf.size(), 40); ++n) printf(" %02X", sock->zDeflateOutBuf[n]);
	//printf("\n");

	// Data sent, don't send again.
	rawBytes = sock->zDeflateOutBuf.size();
	sock->zDeflateInSize = 0;
	sock->zDeflateOutBuf.clear();
}

void socketBeginCompression(Socket *sock)
{
	if (sock->isCompressed)
	{
		return;  // Nothing to do.
	}

	wzMutexLock(socketThreadMutex);

	// Init deflate.
	sock->zDeflate.zalloc = Z_NULL;
	sock->zDeflate.zfree = Z_NULL;
	sock->zDeflate.opaque = Z_NULL;
	int ret = deflateInit(&sock->zDeflate, 6);
	ASSERT(ret == Z_OK, "deflateInit failed! Sockets won't work.");

	sock->zInflate.zalloc = Z_NULL;
	sock->zInflate.zfree = Z_NULL;
	sock->zInflate.opaque = Z_NULL;
	sock->zInflate.avail_in = 0;
	sock->zInflate.next_in = Z_NULL;
	ret = inflateInit(&sock->zInflate);
	ASSERT(ret == Z_OK, "deflateInit failed! Sockets won't work.");

	sock->zInflateNeedInput = true;

	sock->isCompressed = true;
	wzMutexUnlock(socketThreadMutex);
}

Socket::~Socket()
{
	if (isCompressed)
	{
		deflateEnd(&zDeflate);
		deflateEnd(&zInflate);
	}
}

SocketSet *allocSocketSet()
{
	return new SocketSet;
}

void deleteSocketSet(SocketSet *set)
{
	delete set;
}

/**
 * Add the given socket to the given socket set.
 *
 * @return true if @c socket is succesfully added to @set.
 */
void SocketSet_AddSocket(SocketSet* set, Socket* socket)
{
	ASSERT_OR_RETURN(, set != NULL, "NULL SocketSet provided");
	ASSERT_OR_RETURN(, socket != NULL, "NULL Socket provided");

	/* Check whether this socket is already present in this set (i.e. it
	 * shouldn't be added again).
	 */
	size_t i = std::find(set->fds.begin(), set->fds.end(), socket) - set->fds.begin();
	if (i != set->fds.size())
	{
		debug(LOG_NET, "Already found, socket: (set->fds[%lu]) %p", (unsigned long)i, socket);
		return;
	}

	set->fds.push_back(socket);
	debug(LOG_NET, "Socket added: set->fds[%lu] = %p", (unsigned long)i, socket);
}

/**
 * Remove the given socket from the given socket set.
 */
void SocketSet_DelSocket(SocketSet* set, Socket* socket)
{
	ASSERT_OR_RETURN(, set != NULL, "NULL SocketSet provided");
	ASSERT_OR_RETURN(, socket != NULL, "NULL Socket provided");

	size_t i = std::find(set->fds.begin(), set->fds.end(), socket) - set->fds.begin();
	if (i != set->fds.size())
	{
		debug(LOG_NET, "Socket %p erased (set->fds[%lu])", socket, (unsigned long)i);
		set->fds.erase(set->fds.begin() + i);
	}
}

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
		sockopts &= ~O_NONBLOCK;
	else
		sockopts |= O_NONBLOCK;

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

int checkSockets(const SocketSet* set, unsigned int timeout)
{
	if (set->fds.empty())
	{
		return 0;
	}

#if   defined(WZ_OS_UNIX)
	SOCKET maxfd = INT_MIN;
#elif defined(WZ_OS_WIN)
	SOCKET maxfd = 0;
#endif

	bool compressedReady = false;
	for (size_t i = 0; i < set->fds.size(); ++i)
	{
		ASSERT(set->fds[i]->fd[SOCK_CONNECTION] != INVALID_SOCKET, "Invalid file descriptor!");

		if (set->fds[i]->isCompressed && !set->fds[i]->zInflateNeedInput)
		{
			compressedReady = true;
			break;
		}

		maxfd = std::max(maxfd, set->fds[i]->fd[SOCK_CONNECTION]);
	}

	if (compressedReady)
	{
		// A socket already has some data ready. Don't really poll the sockets.

		int ret = 0;
		for (size_t i = 0; i < set->fds.size(); ++i)
		{
			set->fds[i]->ready = set->fds[i]->isCompressed && !set->fds[i]->zInflateNeedInput;
			++ret;
		}
		return ret;
	}

	int ret;
	fd_set fds;
	do
	{
		struct timeval tv = {(int)(timeout / 1000), (int)(timeout % 1000) * 1000};  // Cast to int to avoid narrowing needed for C++11.

		FD_ZERO(&fds);
		for (size_t i = 0; i < set->fds.size(); ++i)
		{
			const SOCKET fd = set->fds[i]->fd[SOCK_CONNECTION];

			FD_SET(fd, &fds);
		}

		ret = select(maxfd + 1, &fds, NULL, NULL, &tv);
	} while (ret == SOCKET_ERROR && getSockErr() == EINTR);

	if (ret == SOCKET_ERROR)
	{
		debug(LOG_ERROR, "select failed: %s", strSockError(getSockErr()));
		return SOCKET_ERROR;
	}

	for (size_t i = 0; i < set->fds.size(); ++i)
	{
		set->fds[i]->ready = FD_ISSET(set->fds[i]->fd[SOCK_CONNECTION], &fds);
	}

	return ret;
}

/**
 * Similar to read(2) with the exception that this function won't be
 * interrupted by signals (EINTR) and will only return when <em>exactly</em>
 * @c size bytes have been received. I.e. this function blocks until all data
 * has been received or a timeout occurred.
 *
 * @param timeout When non-zero this function times out after @c timeout
 *                milliseconds. When zero this function blocks until success or
 *                an error occurs.
 *
 * @c return @c size when succesful, less than @c size but at least zero (0)
 * when the other end disconnected or a timeout occurred. Or @c SOCKET_ERROR if
 * an error occurred.
 */
ssize_t readAll(Socket* sock, void* buf, size_t size, unsigned int timeout)
{
	ASSERT_OR_RETURN( SOCKET_ERROR, sock, "We don't have a valid socket!");
	ASSERT(!sock->isCompressed, "readAll on compressed sockets not implemented.");

	const SocketSet set = {std::vector<Socket *>(1, sock)};

	size_t received = 0;

	if (!sock
	 || sock->fd[SOCK_CONNECTION] == INVALID_SOCKET)
	{
		debug(LOG_ERROR, "Invalid socket (%p), sock->fd[SOCK_CONNECTION]=%x  (error: EBADF)", sock, sock->fd[SOCK_CONNECTION]);
		setSockErr(EBADF);
		return SOCKET_ERROR;
	}

	while (received < size)
	{
		ssize_t ret;

		// If a timeout is set, wait for that amount of time for data to arrive (or abort)
		if (timeout)
		{
			ret = checkSockets(&set, timeout);
			if (ret < (ssize_t)set.fds.size()
			 || !sock->ready)
			{
				if (ret == 0)
				{
					debug(LOG_NET, "socket (%p) has timed out.", socket);
					setSockErr(ETIMEDOUT);
				}
				debug(LOG_NET, "socket (%p) error.", socket);
				return SOCKET_ERROR;
			}
		}

		ret = recv(sock->fd[SOCK_CONNECTION], &((char*)buf)[received], size - received, 0);
		sock->ready = false;
		if (ret == 0)
		{
			debug(LOG_NET, "Socket %x disconnected.", sock->fd[SOCK_CONNECTION]);
			sock->readDisconnected = true;
			setSockErr(ECONNRESET);
			return received;
		}

		if (ret == SOCKET_ERROR)
		{
			switch (getSockErr())
			{
				case EAGAIN:
#if defined(EWOULDBLOCK) && EAGAIN != EWOULDBLOCK
				case EWOULDBLOCK:
#endif
				case EINTR:
					continue;

				default:
					return SOCKET_ERROR;
			}
		}

		received += ret;
	}

	return received;
}

static void socketCloseNow(Socket *sock)
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
				debug(LOG_ERROR, "Failed to close socket %p: %s", sock, strSockError(getSockErr()));
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

void socketClose(Socket* sock)
{
	if (sock == NULL)
	{
		return;
	}
	wzMutexLock(socketThreadMutex);
	//Instead of socketThreadWrites.erase(sock);, try sending the data before actually deleting.
	if (socketThreadWrites.find(sock) != socketThreadWrites.end())
	{
		// Wait until the data is written, then delete the socket.
		sock->deleteLater = true;
	}
	else
	{
		// Delete the socket.
		socketCloseNow(sock);
	}
	wzMutexUnlock(socketThreadMutex);
}

Socket *socketAccept(Socket *sock)
{
	unsigned int i;

	ASSERT_OR_RETURN(NULL, sock != NULL, "NULL Socket provided");

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

			const SOCKET newConn = accept(sock->fd[i], (struct sockaddr*)&addr, &addr_len);
			if (newConn == INVALID_SOCKET)
			{
				// Ignore the case where no connection is pending
				if (getSockErr() != EAGAIN
				 && getSockErr() != EWOULDBLOCK)
				{
					debug(LOG_ERROR, "accept failed for socket %p: %s", sock, strSockError(getSockErr()));
				}

				continue;
			}

			conn = new Socket;
			if (conn == NULL)
			{
				debug(LOG_ERROR, "Out of memory!");
				abort();
				return NULL;
			}

			debug(LOG_NET, "setting socket (%p) blocking status (false).", conn);
			if (!setSocketBlocking(newConn, false))
			{
				debug(LOG_NET, "Couldn't set socket (%p) blocking status (false).  Closing.", conn);
				socketClose(conn);
				return NULL;
			}

			socketBlockSIGPIPE(newConn, true);

			// Mark all unused socket handles as invalid
			for (j = 0; j < ARRAY_SIZE(conn->fd); ++j)
			{
				conn->fd[j] = INVALID_SOCKET;
			}

			conn->fd[SOCK_CONNECTION] = newConn;

			sock->ready = false;

			addressToText((const struct sockaddr*)&addr, conn->textAddress, sizeof(conn->textAddress));
			debug(LOG_NET, "Incoming connection from [%s]:/*%%d*/ (FIXME: gives strict-aliasing error)", conn->textAddress/*, (unsigned int)ntohs(((const struct sockaddr_in*)&addr)->sin_port)*/);
			debug(LOG_NET, "Using socket %p", conn);
			return conn;
		}
	}

	return NULL;
}

Socket *socketOpen(const SocketAddress *addr, unsigned timeout)
{
	unsigned int i;
	int ret;

	Socket *const conn = new Socket;
	if (conn == NULL)
	{
		debug(LOG_ERROR, "Out of memory!");
		abort();
		return NULL;
	}

	ASSERT(addr != NULL, "NULL Socket provided");

	addressToText(addr->ai_addr, conn->textAddress, sizeof(conn->textAddress));
	debug(LOG_NET, "Connecting to [%s]:%d", conn->textAddress, (int)ntohs(((const struct sockaddr_in*)addr->ai_addr)->sin_port));

	// Mark all unused socket handles as invalid
	for (i = 0; i < ARRAY_SIZE(conn->fd); ++i)
	{
		conn->fd[i] = INVALID_SOCKET;
	}

	conn->fd[SOCK_CONNECTION] = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

	if (conn->fd[SOCK_CONNECTION] == INVALID_SOCKET)
	{
		debug(LOG_ERROR, "Failed to create a socket (%p): %s", conn, strSockError(getSockErr()));
		socketClose(conn);
		return NULL;
	}

	debug(LOG_NET, "setting socket (%p) blocking status (false).", conn);
	if (!setSocketBlocking(conn->fd[SOCK_CONNECTION], false))
	{
		debug(LOG_NET, "Couldn't set socket (%p) blocking status (false).  Closing.", conn);
		socketClose(conn);
		return NULL;
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
			debug(LOG_NET, "Failed to start connecting: %s, using socket %p", strSockError(getSockErr()), conn);
			socketClose(conn);
			return NULL;
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
			ret = select(conn->fd[SOCK_CONNECTION] + 1, NULL, &conReady, NULL, &tv);
#endif
		} while (ret == SOCKET_ERROR && getSockErr() == EINTR);

		if (ret == SOCKET_ERROR)
		{
			debug(LOG_NET, "Failed to wait for connection: %s, socket %p.  Closing.", strSockError(getSockErr()), conn);
			socketClose(conn);
			return NULL;
		}

		if (ret == 0)
		{
			setSockErr(ETIMEDOUT);
			debug(LOG_NET, "Timed out while waiting for connection to be established: %s, using socket %p.  Closing.", strSockError(getSockErr()), conn);
			socketClose(conn);
			return NULL;
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
			debug(LOG_NET, "Failed to connect: %s, with socket %p.  Closing.", strSockError(getSockErr()), conn);
			socketClose(conn);
			return NULL;
		}
	}

	return conn;
}

Socket *socketListen(unsigned int port)
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
	if (conn == NULL)
	{
		debug(LOG_ERROR, "Out of memory!");
		abort();
		return NULL;
	}

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

	conn->fd[SOCK_IPV4_LISTEN] = socket(addr4.sin_family, SOCK_STREAM, 0);
	conn->fd[SOCK_IPV6_LISTEN] = socket(addr6.sin6_family, SOCK_STREAM, 0);

	if (conn->fd[SOCK_IPV4_LISTEN] == INVALID_SOCKET
	 && conn->fd[SOCK_IPV6_LISTEN] == INVALID_SOCKET)
	{
		debug(LOG_ERROR, "Failed to create an IPv4 and IPv6 (only supported address families) socket (%p): %s.  Closing.", conn, strSockError(getSockErr()));
		socketClose(conn);
		return NULL;
	}

	if (conn->fd[SOCK_IPV4_LISTEN] != INVALID_SOCKET)
	{
		debug(LOG_NET, "Successfully created an IPv4 socket (%p)", conn);
	}

	if (conn->fd[SOCK_IPV6_LISTEN] != INVALID_SOCKET)
	{
		debug(LOG_NET, "Successfully created an IPv6 socket (%p)", conn);
	}

#if defined(IPV6_V6ONLY)
	if (conn->fd[SOCK_IPV6_LISTEN] != INVALID_SOCKET)
	{
		if (setsockopt(conn->fd[SOCK_IPV6_LISTEN], IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6_v6only, sizeof(ipv6_v6only)) == SOCKET_ERROR)
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
		if (setsockopt(conn->fd[SOCK_IPV4_LISTEN], SOL_SOCKET, SO_REUSEADDR, (char*)&so_reuseaddr, sizeof(so_reuseaddr)) == SOCKET_ERROR)
		{
			debug(LOG_WARNING, "Failed to set SO_REUSEADDR on IPv4 socket. Error: %s", strSockError(getSockErr()));
		}

		debug(LOG_NET, "setting socket (%p) blocking status (false, IPv4).", conn);
		if (bind(conn->fd[SOCK_IPV4_LISTEN], (const struct sockaddr*)&addr4, sizeof(addr4)) == SOCKET_ERROR
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
		if (setsockopt(conn->fd[SOCK_IPV6_LISTEN], SOL_SOCKET, SO_REUSEADDR, (char*)&so_reuseaddr, sizeof(so_reuseaddr)) == SOCKET_ERROR)
		{
			debug(LOG_INFO, "Failed to set SO_REUSEADDR on IPv6 socket. Error: %s", strSockError(getSockErr()));
		}

		debug(LOG_NET, "setting socket (%p) blocking status (false, IPv6).", conn);
		if (bind(conn->fd[SOCK_IPV6_LISTEN], (const struct sockaddr*)&addr6, sizeof(addr6)) == SOCKET_ERROR
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
		debug(LOG_NET, "No IPv4 or IPv6 sockets created.");
		socketClose(conn);
		return NULL;
	}

	return conn;
}

Socket *socketOpenAny(const SocketAddress *addr, unsigned timeout)
{
	Socket *ret = NULL;
	while (addr != NULL && ret == NULL)
	{
		ret = socketOpen(addr, timeout);

		addr = addr->ai_next;
	}

	return ret;
}

size_t socketArrayOpen(Socket **sockets, size_t maxSockets, const SocketAddress *addr, unsigned timeout)
{
	size_t i = 0;
	while (i < maxSockets && addr != NULL)
	{
		if (addr->ai_family == AF_INET || addr->ai_family == AF_INET6)
		{
			sockets[i] = socketOpen(addr, timeout);
			i += sockets[i] != NULL;
		}

		addr = addr->ai_next;
	}
	std::fill(sockets + i, sockets + maxSockets, (Socket *)NULL);
	return i;
}

void socketArrayClose(Socket **sockets, size_t maxSockets)
{
	std::for_each(sockets, sockets + maxSockets, socketClose);     // Close any open sockets.
	std::fill    (sockets, sockets + maxSockets, (Socket *)NULL);  // Set the pointers to NULL.
}

char const *getSocketTextAddress(Socket const *sock)
{
	return sock->textAddress;
}

SocketAddress *resolveHost(const char *host, unsigned int port)
{
	struct addrinfo* results;
	char* service;
	struct addrinfo hint;
	int error, flags = 0;

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
	hint.ai_addr      = NULL;
	hint.ai_canonname = NULL;
	hint.ai_next      = NULL;

	sasprintf(&service, "%u", port);

	error = getaddrinfo(host, service, &hint, &results);
	if (error != 0)
	{
		debug(LOG_NET, "getaddrinfo failed for %s:%s: %s", host, service, gai_strerror(error));
		return NULL;
	}

	return results;
}

void deleteSocketAddress(SocketAddress *addr)
{
	freeaddrinfo(addr);
}

// ////////////////////////////////////////////////////////////////////////
// setup stuff
void SOCKETinit()
{
#if defined(WZ_OS_WIN)
	static bool firstCall = true;
	if (firstCall)
	{
		firstCall = false;

		static WSADATA stuff;
		WORD ver_required = (2 << 8) + 2;
		if (WSAStartup(ver_required, &stuff) != 0)
		{
			debug(LOG_ERROR, "Failed to initialize Winsock: %s", strSockError(getSockErr()));
			return;
		}

		winsock2_dll = LoadLibraryA("ws2_32.dll");
		if (winsock2_dll)
		{
			getaddrinfo_dll_func = (GETADDRINFO_DLL_FUNC) GetProcAddress(winsock2_dll, "getaddrinfo");
			freeaddrinfo_dll_func = (FREEADDRINFO_DLL_FUNC) GetProcAddress(winsock2_dll, "freeaddrinfo");
		}

		// Determine major Windows version
		major_windows_version = LOBYTE(LOWORD(GetVersion()));
	}
#endif

	if (socketThread == NULL)
	{
		socketThreadQuit = false;
		socketThreadMutex = wzMutexCreate();
		socketThreadSemaphore = wzSemaphoreCreate(0);
		socketThread = wzThreadCreate(socketThreadFunction, NULL);
		wzThreadStart(socketThread);
	}
}

void SOCKETshutdown()
{
	if (socketThread != NULL)
	{
		wzMutexLock(socketThreadMutex);
		socketThreadQuit = true;
		socketThreadWrites.clear();
		wzMutexUnlock(socketThreadMutex);
		wzSemaphorePost(socketThreadSemaphore);  // Wake up the thread, so it can quit.
		wzThreadJoin(socketThread);
		wzMutexDestroy(socketThreadMutex);
		wzSemaphoreDestroy(socketThreadSemaphore);
		socketThread = NULL;
	}

#if defined(WZ_OS_WIN)
	WSACleanup();

	if (winsock2_dll)
	{
		FreeLibrary(winsock2_dll);
		winsock2_dll = NULL;
		getaddrinfo_dll_func = NULL;
		freeaddrinfo_dll_func = NULL;
	}
#endif
}
