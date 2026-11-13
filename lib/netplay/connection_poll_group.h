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

#pragma once

#include <chrono>

#include "lib/netplay/net_result.h"

class IClientConnection;

/// <summary>
/// Abstract representation of a poll group comprised of several client connections.
/// </summary>
class IConnectionPollGroup
{
public:

	virtual ~IConnectionPollGroup() = default;

	/// <summary>
	/// Polls the sockets in the poll group for read updates.
	/// </summary>
	/// <param name="timeout">Timeout value after which the internal implementation should abandon
	/// polling the client connections and return.</param>
	/// <returns>On success, returns the number of connection descriptors in the poll group.
	/// On failure, `0` can returned if the timeout expired before any connection descriptors
	/// became ready, or `-1` if there was an error during the internal poll operation.</returns>
	virtual net::result<int> checkConnectionsReadable(std::chrono::milliseconds timeout) = 0;

	virtual void add(IClientConnection* conn) = 0;
	virtual void remove(IClientConnection* conn) = 0;
};
