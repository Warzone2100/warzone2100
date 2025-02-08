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
#include "lib/netplay/net_result.h"

#ifdef WZ_OS_WIN
# include <winsock2.h>
#elif defined(WZ_OS_UNIX)
using SOCKET = int;
#endif

#include <chrono>

namespace tcp
{

enum class PollEventType
{
	READABLE,
	WRITABLE
};

/// <summary>
/// Abstract class for describing polling descriptor sets used by various polling APIs (e.g. `select()` and `poll()`).
/// </summary>
class IDescriptorSet
{
public:

	virtual ~IDescriptorSet() = default;

	virtual bool add(SOCKET fd) = 0;
	virtual void clear() = 0;

	/// <summary>
	/// Polling algorithm implementation for this descriptor set kind.
	/// Should represent the same semantics as `select()` or `poll()` APIs, i.e. after calling `pollImpl()` one
	/// should check individual descriptors via `isSet()` to see, which of them were marked as ready.
	/// </summary>
	/// <param name="timeout">Timeout value in milliseconds</param>
	/// <returns>
	/// * Non-negative integer indicating the number of descriptors that have a return event set
	/// by an internal polling function.
	/// * Zero if none of the descriptors have been set (polling function timed out).
	/// * `std::error_code` containing error condition otherwise.
	/// </returns>
	virtual net::result<int> poll(std::chrono::milliseconds timeout) = 0;

	virtual bool isSet(SOCKET fd) const = 0;
};

} // namespace tcp
