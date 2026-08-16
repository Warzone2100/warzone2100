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

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <nonstd/optional.hpp>

// Keyed hash computation.
//
// answer = BLAKE2b-256(key = nonce,
//                      input = "wz-hash-request-v1" 0x00 reqId 0x00 decimal(target_index) 0x00
//                              target_descriptor 0x00 range bytes in request order)
//
// Ranges extending past the end of the file contribute only the bytes that exist
// (a size mismatch yields a wrong answer, not an error).
//
// Pure computation over a caller-supplied reader. Callers must have initialized libsodium.

namespace wzkeyedhash
{

using nonstd::optional;
using nonstd::nullopt;

constexpr size_t NONCE_BYTES = 32;
constexpr size_t HASH_BYTES = 32;

// Hard cap on the total requested bytes per answer
constexpr uint64_t MAX_TOTAL_REQUESTED_BYTES = 8 * 1024 * 1024;

struct Range
{
	uint64_t offset = 0;
	uint64_t len = 0;
};

// pread-style reader: fills buffer with exactly len bytes at offset, returns false on failure
typedef std::function<bool(uint64_t offset, void* buffer, size_t len)> ReadAtFunc;

// The canonical descriptor string for a target: kind, then " <offset>+<len>" per range
std::string targetDescriptor(const std::string& kind, const std::vector<Range>& ranges);

// Returns nullopt when the nonce is not NONCE_BYTES, requestId or targetDescriptor embeds a NUL,
// the total requested length exceeds MAX_TOTAL_REQUESTED_BYTES, or a read fails
optional<std::array<uint8_t, HASH_BYTES>> computeKeyedHash(
	const std::vector<uint8_t>& nonce,
	const std::string& requestId,
	uint32_t targetIndex,
	const std::string& targetDescriptor,
	const std::vector<Range>& ranges,
	uint64_t fileSize,
	const ReadAtFunc& readAt);

} // namespace wzkeyedhash
