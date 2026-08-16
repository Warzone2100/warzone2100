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

#include <nonstd/optional.hpp>
using nonstd::optional;

// Loads the build manifest (`<exe basename>.buildmanifest`) and self-checks it:
// the manifest is valid only if its `artifacts.exe` entry matches the self-measured executable hash.

// Obtain the validated manifest's exact bytes, or nullopt.
// `resultFunc` is always invoked asynchronously on the main thread.
// The result is computed once and cached for the session.
// NOTE: Main thread only.
void getValidatedManifestBytes(std::function<void(const optional<std::string>&)> resultFunc);
