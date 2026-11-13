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
