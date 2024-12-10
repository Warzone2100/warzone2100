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

#include <stdexcept>

#include "lib/netplay/connection_provider_registry.h"
#include "lib/netplay/tcp/tcp_connection_provider.h"

ConnectionProviderRegistry& ConnectionProviderRegistry::Instance()
{
	static ConnectionProviderRegistry instance;
	return instance;
}

WzConnectionProvider& ConnectionProviderRegistry::Get(ConnectionProviderType pt)
{
	const auto it = registeredProviders_.find(pt);
	if (it == registeredProviders_.end())
	{
		throw std::runtime_error("Attempt to get nonexistent connection provider");
	}
	return *it->second;
}

void ConnectionProviderRegistry::Register(ConnectionProviderType pt)
{
	// No-op in case this provider has been already registered.
	switch (pt)
	{
	case ConnectionProviderType::TCP_DIRECT:
		registeredProviders_.emplace(pt, std::make_unique<tcp::TCPConnectionProvider>());
		break;
	default:
		throw std::runtime_error("Unknown connection provider type");
	}
}

void ConnectionProviderRegistry::Deregister(ConnectionProviderType pt)
{
	registeredProviders_.erase(pt);
}
