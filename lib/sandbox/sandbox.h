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

/**
 * Process hardening and filesystem confinement.
 *
 * Both operations are best-effort. A platform with nothing to offer leaves the process as it was and says
 * so in the returned Status. Nothing here is ever fatal.
 */

#pragma once

#include <string>
#include <vector>

namespace wz::sandbox
{

enum class Tier
{
	Off,
	WriteConfined,
	Strict
};

/// Absolute directories the game needs. An entry that does not exist is skipped.
struct Plan
{
	std::vector<std::string> readRoots;
	std::vector<std::string> writeRoots;
	std::vector<std::string> socketDirs;        ///< a unix socket is created and unlinked here
	std::vector<std::string> writeFileParents;  ///< a single output file is created and truncated here
	std::vector<std::string> execRoots;
};

/// What confine() applied, which may be less than was requested.
struct Status
{
	bool active = false;
	Tier effective = Tier::Off;
	std::string backend = "none";        ///< "landlock", "macos-app-sandbox", "none"
	std::string detail = "not applied";  ///< ex. "ABI 4", "unavailable: ENOSYS", "disabled"
};

enum class LogLevel
{
	Info,
	Error
};

struct LogLine
{
	LogLevel level = LogLevel::Info;
	std::string text;
};

/// Applies the platform mitigations. Safe to call first thing in main().
/// Tier::Off skips the ones that could get in the way and applies the rest.
void harden(Tier requested);

/**
 * Applies the plan as far as the platform allows, and records the result for status().
 *
 * NOTE: Must be called while the process is still single-threaded - a backend may only be able to restrict
 * the calling thread.
 */
Status confine(const Plan& plan, Tier requested);

/// What the last confine() call applied, or a default "none" if it has not been called.
const Status& status();

/// Messages from harden() and confine(), in order. harden() runs before the game's log exists, so the
/// caller writes these out once it does.
const std::vector<LogLine>& pendingLog();

/// Parses "off" (or "0"), "write" or "strict". Anything else returns false and leaves out untouched.
bool parseTier(const char* text, Tier& out);

const char* tierName(Tier t);

} // namespace wz::sandbox
