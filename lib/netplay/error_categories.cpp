// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2024-2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#include "error_categories.h"

#include "lib/framework/wzglobal.h"

#ifdef WZ_OS_WIN
# include <winerror.h>
# include <ws2tcpip.h>
#elif defined(WZ_OS_UNIX)
# include <string.h>
# include <netdb.h>
#endif

#include <zlib.h>

std::string GenericSystemErrorCategory::message(int ev) const
{
#if defined(WZ_OS_WIN)
	switch (ev)
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
	return strerror(ev);
#endif
}

std::error_condition GenericSystemErrorCategory::default_error_condition(int ev) const noexcept
{
	// Try to map the raw error values either to POSIX or Windows error codes (depending on the OS).
	// The default system category should capture them all well.
	return std::system_category().default_error_condition(ev);
}

std::string GetaddrinfoErrorCategory::message(int ev) const
{
	return gai_strerror(ev);
}

std::string ZlibErrorCategory::message(int ev) const
{
	switch (ev)
	{
	case Z_STREAM_ERROR:
		return "Z_STREAM_ERROR";
	case Z_NEED_DICT:
		return "Z_NEED_DICT";
	case Z_DATA_ERROR:
		return "Z_DATA_ERROR";
	case Z_MEM_ERROR:
		return "Z_MEM_ERROR";
	case Z_STREAM_END:
		return "Z_STREAM_END";
	default:
		return "Unknown zlib error";
	}
}

const std::error_category& generic_system_error_category()
{
	static GenericSystemErrorCategory instance;
	return instance;
}

const std::error_category& getaddrinfo_error_category()
{
	static GetaddrinfoErrorCategory instance;
	return instance;
}

const std::error_category& zlib_error_category()
{
	static ZlibErrorCategory instance;
	return instance;
}

std::error_code make_network_error_code(int ev)
{
	return { ev, generic_system_error_category() };
}

std::error_code make_getaddrinfo_error_code(int ev)
{
	return { ev, getaddrinfo_error_category() };
}

std::error_code make_zlib_error_code(int ev)
{
	return { ev, zlib_error_category() };
}
