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

#include "pending_writes_manager_map.h"
#include "pending_writes_manager.h"

#include "lib/framework/frame.h" // for ASSERT

PendingWritesManagerMap& PendingWritesManagerMap::instance()
{
	static PendingWritesManagerMap instance;
	return instance;
}

PendingWritesManager& PendingWritesManagerMap::get(ConnectionProviderType pt)
{
	auto& cpr = ConnectionProviderRegistry::Instance();
	ASSERT(cpr.IsRegistered(pt), "Connection provider %d is not registered", static_cast<int>(pt));
	auto it = pendingWritesManagers_.find(pt);
	if (it != pendingWritesManagers_.end())
	{
		return *it->second;
	}
	auto connProvider = cpr.Get(pt);
	ASSERT(connProvider != nullptr, "Null connection provider");
	auto pwm = std::make_unique<PendingWritesManager>();
	pwm->initialize(*connProvider);
	it = pendingWritesManagers_.emplace(pt, std::move(pwm)).first;
	return *it->second;
}

PendingWritesManager& PendingWritesManagerMap::get(const WzConnectionProvider& connProvider)
{
	return get(connProvider.type());
}

void PendingWritesManagerMap::Shutdown()
{
	for (auto& it : pendingWritesManagers_)
	{
		it.second->deinitialize();
	}
	pendingWritesManagers_.clear();
}
