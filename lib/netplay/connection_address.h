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

#include <string>

#include "lib/netplay/net_result.h"

/// <summary>
/// Opaque class representing abstract connection address to use with various
/// network backend implementations. The internal representation is made
/// hidden on purpose since we don't want to actually leak internal data layout
/// to clients.
///
/// Instead, we would like to introduce "conversion routines" yielding
/// various representations for convenient consumption with various network
/// backends.
///
/// NOTE: this class may or may not represent a chain of resolved network addresses
/// instead of just a single one, much like a `addrinfo` structure.
/// </summary>
struct IConnectionAddress
{
	virtual ~IConnectionAddress() = default;

	/// <summary>
	/// Converts the address to a string representation, which can be consumed
	/// by various network backends.
	/// </summary>
	/// <returns>String representation of the address or an error code if the conversion
	/// process ended with an error.</returns>
	virtual net::result<std::string> toString() const = 0;
};
