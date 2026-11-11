// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include <functional>
#include <string>

#include <nonstd/optional.hpp>
using nonstd::optional;

#include <nlohmann/json_fwd.hpp>

// Loads the build manifest (`<exe basename>.buildmanifest`) and self-checks it:
// the manifest is valid only if its `artifacts.exe` entry matches the self-measured executable hash.

// Obtain the validated manifest's exact bytes, or nullopt.
// `resultFunc` is always invoked asynchronously on the main thread.
// The result is computed once and cached for the session.
// NOTE: Main thread only.
void getValidatedManifestBytes(std::function<void(const optional<std::string>&)> resultFunc);

// Obtain the validated manifest, parsed, or nullopt.
// NOTE: Main thread only.
void getValidatedManifest(std::function<void(const optional<nlohmann::json>&)> resultFunc);
