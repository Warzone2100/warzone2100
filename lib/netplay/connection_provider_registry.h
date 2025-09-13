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

#include <stdint.h>
#include <memory>
#include <unordered_map>

#include "lib/framework/wzglobal.h" // for WZ_GNS_NETWORK_BACKEND_ENABLED

#include "lib/netplay/wz_connection_provider.h"

/// <summary>
/// Available types of connection providers (i.e. network backend implementations).
/// </summary>
enum class ConnectionProviderType : uint8_t
{
	TCP_DIRECT,
#ifdef WZ_GNS_NETWORK_BACKEND_ENABLED
	GNS_DIRECT,
#endif
};

/// <summary>
/// Global singleton registry containing available network connection providers.
/// </summary>
class ConnectionProviderRegistry
{
public:

	static ConnectionProviderRegistry& Instance();

	std::shared_ptr<WzConnectionProvider> Get(ConnectionProviderType pt);
	bool IsRegistered(ConnectionProviderType) const;

	void Register(ConnectionProviderType pt);

	void Shutdown();

private:

	ConnectionProviderRegistry() = default;
	ConnectionProviderRegistry(const ConnectionProviderRegistry&) = delete;
	ConnectionProviderRegistry(ConnectionProviderRegistry&&) = delete;

	std::unordered_map<ConnectionProviderType, std::shared_ptr<WzConnectionProvider>> registeredProviders_;
};
