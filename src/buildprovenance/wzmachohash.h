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

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <nonstd/optional.hpp>

// The signature-invariant "canonical" hash of a Mach-O executable.
//
// Per architecture slice: SHA-256 over bytes [0, dataoff), where dataoff is the LC_CODE_SIGNATURE blob offset
// (slice end if unsigned), with the fields codesign rewrites masked to zero (LC_CODE_SIGNATURE's
// dataoff/datasize and the __LINKEDIT segment's vmsize/filesize).
// NOTE: Invariant across re-signing, but not between unsigned and signed (as signing adds a load command).
//
// Works on any platform, for thin 64-bit or fat Mach-O. Callers must have initialized libsodium.

namespace wzmachohash
{

using nonstd::optional;
using nonstd::nullopt;

// pread-style reader: fills buffer with exactly len bytes at offset, returns false on failure
typedef std::function<bool(uint64_t offset, void* buffer, size_t len)> ReadAtFunc;

struct SliceCanonicalHash
{
	std::string arch;		// "x86_64", "arm64", or "cputype-0x<hex>"
	std::string sha256Hex;
};

optional<std::vector<SliceCanonicalHash>> computeCanonicalHashes(const ReadAtFunc& readAt, uint64_t fileSize, std::string& errorDetails);

} // namespace wzmachohash
