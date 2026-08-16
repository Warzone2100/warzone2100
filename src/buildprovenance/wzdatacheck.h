// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include <functional>
#include <string>
#include <vector>

// Checks the mounted core data archives (base.wz, mp.wz) against the validated manifest.
// The result is diagnostic only.

struct CoreDataCheckResult
{
	bool checkPerformed = false;					// manifest + mounted archives were available
	std::vector<std::string> modifiedArchives;		// archive names that do not match the manifest
};

// Obtain the core data check result. Hashing runs on a background thread on the first call,
// and the result is cached for the session. `resultFunc` is always invoked asynchronously on the main thread.
// NOTE: Main thread only.
void getCoreDataModifiedCheck(std::function<void(const CoreDataCheckResult&)> resultFunc);

// Request cancellation and join the background thread. Safe to call in any state.
// NOTE: Main thread only.
void coreDataCheckShutdown();
