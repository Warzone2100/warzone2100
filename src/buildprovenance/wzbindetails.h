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

#include <cstdint>
#include <functional>
#include <string>

#include <nonstd/optional.hpp>
using nonstd::optional;

#include "lib/framework/crc.h"
#include "wzhashablefile.h"

// Details of the running process's own on-disk executable.
struct ExeDetails
{
	std::string path;							// resolved path (empty if resolution failed)
	HashableFile::PathConfidence pathConfidence = HashableFile::PathConfidence::KernelAuthoritative;
	optional<uint64_t> fileSize;
	optional<Sha256> rawHash;						// whole-file SHA-256 of the on-disk executable
	std::vector<wzmachohash::SliceCanonicalHash> machoCanonicalSlices;	// macOS only, empty elsewhere
	std::string errorDetails;						// populated when rawHash is empty
};

// Begin computing the executable details on a background thread.
// Call once at startup (after LaunchInfo::initialize and sodium_init).
// NOTE: Main thread only.
void selfExeDetailsInit();

// Request cancellation and join the background thread. Safe to call in any state.
// NOTE: Main thread only.
void selfExeDetailsShutdown();

// Obtain the executable details.
// `resultFunc` is always invoked asynchronously on the main thread.
// A failed computation still invokes `resultFunc` (with rawHash empty and errorDetails set).
// NOTE: Main thread only.
void getSelfExecutableDetails(std::function<void(const ExeDetails&)> resultFunc);
