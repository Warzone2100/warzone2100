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

#include "lib/framework/wzglobal.h"

#ifdef WZ_OS_WIN
# include <winerror.h> // WSA* error codes
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
#endif

namespace tcp
{

/// <summary>
/// Cross-platform wrapper around `errno` (for Linux/macOS/*BSD) and `WSAGetLastError()` (for Windows).
/// </summary>
/// <returns>Returns the last error for the calling thread.</returns>
int getSockErr();

} // namespace tcp
