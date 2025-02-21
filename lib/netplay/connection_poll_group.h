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
