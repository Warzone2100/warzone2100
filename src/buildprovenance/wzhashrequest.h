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

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

#include <nlohmann/json.hpp>

// Answers a keyed hash request.
//
// Request: { "ver": 1, "reqId": "<id>", "nonce": "<base64, 32 bytes>",
//            "targets": [ { "kind": "exe", "ranges": [[offset, len], ...] } ] }
// Response: { "reqId": "<id>", "status": "ok", "answers": ["<base64>", ...] }
//        or { "reqId": "<id>", "status": "declined" }
//
// Only the client's own executable is readable, via HashableFile.
// A request exceeding the limits below is declined, never partially answered.
// Ranges must not overlap within a request.
// Answered requests are limited by a token bucket, refilled to capacity by hashRequestRateLimitStartEvent.
// resultFunc is always invoked asynchronously on the main thread.

constexpr size_t WZ_HASH_REQUEST_MAX_TARGETS = 8;
constexpr size_t WZ_HASH_REQUEST_MAX_RANGES_PER_TARGET = 32;
constexpr size_t WZ_HASH_REQUEST_MAX_ID_LENGTH = 64;
constexpr uint64_t WZ_HASH_REQUEST_MIN_RANGE_BYTES = 4096;
constexpr uint32_t WZ_HASH_REQUEST_RATE_BUCKET_CAPACITY = 16;
constexpr uint32_t WZ_HASH_REQUEST_RATE_REFILL_SECONDS = 10 * 60;

void answerHashRequest(const nlohmann::json& hashRequest,
						std::shared_ptr<std::atomic<bool>> cancelFlag,
						std::function<void(nlohmann::ordered_json hashResponse)> resultFunc);

void hashRequestRateLimitStartEvent();

void hashRequestShutdown();
