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

#include <vector>
#include <chrono>

#include "lib/netplay/net_result.h"

class IClientConnection;
class IDescriptorSet;

/// <summary>
/// Checks which client connections are ready for reading within the specified
/// `timeout` by performing poll operation on `readableSet`.
///
/// User of the function should check the `readableSet` after calling this function
/// to see which descriptors were set by the internal poll operation (by calling `IDescriptorSet::isSet()`).
/// </summary>
/// <param name="conns">List of client connections to poll.</param>
/// <param name="readableSet">`IDescriptorSet` instance, which may have some of
/// the descriptors being "set" for read-readiness after internal polling call completes.</param>
/// <param name="timeout">Timeout in milliseconds.</param>
/// <returns>On success, returns the number of connections being "set" to "ready to read" state.
/// On failure, an `std::error_code` describing the error will be returned.</returns>
net::result<int> checkConnectionsReadable(const std::vector<IClientConnection*>& conns,
	IDescriptorSet& readableSet, std::chrono::milliseconds timeout);
