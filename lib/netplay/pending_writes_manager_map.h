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

#include <memory>

#include "lib/netplay/connection_provider_registry.h"

class PendingWritesManager;

/// <summary>
/// Global singleton mapping from connection provider types to `PendingWritesManager` instances.
///
/// Each `PendingWritesManager` instance obtained via `get(ConnectionProviderType)`,
/// should be `initialize()`-d with a corresponding `WzConnectionProvider` instance.
/// </summary>
class PendingWritesManagerMap
{
public:

	static PendingWritesManagerMap& instance();

	/// <summary>
	/// Get or create a `PendingWritesManager` instance for a given connection provider type.
	///
	/// If a new instance is created, it's automatically initialized with a reference to the
	/// corresponding `WzConnectionProvider` instance.
	/// </summary>
	/// <param name="pt"></param>
	/// <returns>Reference to the `PendingWritesManager` instance bound to the connection
	/// provider corresponding to `pt`.</returns>
	PendingWritesManager& get(ConnectionProviderType pt);
	PendingWritesManager& get(const WzConnectionProvider& connProvider);

	void Shutdown();

private:

	explicit PendingWritesManagerMap() = default;
	PendingWritesManagerMap(const PendingWritesManagerMap&) = delete;
	PendingWritesManagerMap(PendingWritesManagerMap&&) = delete;

	// `PendingWritesManager` is stored as `unique_ptr` to avoid undesired theoretical
	// side effects in case `pendingWritesManagers_` map is rehashed upon insertion.
	// We don't want `PendingWritesManager` to ever be destroyed and reallocated (possibly midway
	// during some work).
	std::unordered_map<ConnectionProviderType, std::unique_ptr<PendingWritesManager>> pendingWritesManagers_;
};
