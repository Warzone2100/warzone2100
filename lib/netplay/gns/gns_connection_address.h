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

#include "lib/netplay/connection_address.h"

#include <steam/steamnetworkingtypes.h>

namespace gns
{

/// <summary>
/// GNS-specific implementation of the `IConnectionAddress` interface,
/// suitable for direct consumption by the GNS network backend.
///
/// Wraps a `SteamNetworkingIPAddr` object, which is used to represent
/// a network address (IP address with port) in the GNS library.
/// </summary>
class GNSConnectionAddress : public IConnectionAddress
{
public:

	explicit GNSConnectionAddress(SteamNetworkingIPAddr addr)
		: addr_(addr)
	{}

	virtual ~GNSConnectionAddress() override = default;

	const SteamNetworkingIPAddr& asSteamNetworkingIPAddr() const { return addr_; }

	virtual net::result<std::string> toString() const override;

private:

	SteamNetworkingIPAddr addr_;
};

} // namespace gns
