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

#include "wzkeyedhash.h"

#include <sodium.h>

#include <algorithm>
#include <cstring>
#include <limits>

namespace wzkeyedhash
{

namespace
{

constexpr char DOMAIN_PREFIX[] = "wz-hash-request-v1";
constexpr size_t READ_CHUNK_SIZE = 1024 * 1024;

void updateWithSeparator(crypto_generichash_state& state, const std::string& value)
{
	static const unsigned char nul = 0;
	crypto_generichash_update(&state, reinterpret_cast<const unsigned char*>(value.data()), value.size());
	crypto_generichash_update(&state, &nul, 1);
}

} // anonymous namespace

std::string targetDescriptor(const std::string& kind, const std::vector<Range>& ranges)
{
	std::string result = kind;
	for (const auto& range : ranges)
	{
		result += " " + std::to_string(range.offset) + "+" + std::to_string(range.len);
	}
	return result;
}

optional<std::array<uint8_t, HASH_BYTES>> computeKeyedHash(
	const std::vector<uint8_t>& nonce,
	const std::string& requestId,
	uint32_t targetIndex,
	const std::string& targetDescriptor,
	const std::vector<Range>& ranges,
	uint64_t fileSize,
	const ReadAtFunc& readAt)
{
	if (nonce.size() != NONCE_BYTES)
	{
		return nullopt;
	}
	if (requestId.find('\0') != std::string::npos || targetDescriptor.find('\0') != std::string::npos)
	{
		return nullopt;
	}
	uint64_t totalRequested = 0;
	for (const auto& range : ranges)
	{
		if (range.len > MAX_TOTAL_REQUESTED_BYTES || totalRequested + range.len > MAX_TOTAL_REQUESTED_BYTES)
		{
			return nullopt;
		}
		totalRequested += range.len;
	}

	crypto_generichash_state state;
	crypto_generichash_init(&state, nonce.data(), nonce.size(), HASH_BYTES);
	updateWithSeparator(state, DOMAIN_PREFIX);
	updateWithSeparator(state, requestId);
	updateWithSeparator(state, std::to_string(targetIndex));
	updateWithSeparator(state, targetDescriptor);

	std::vector<unsigned char> buffer(READ_CHUNK_SIZE);
	for (const auto& range : ranges)
	{
		if (range.offset >= fileSize)
		{
			continue;
		}
		uint64_t remaining = std::min(range.len, fileSize - range.offset);
		uint64_t offset = range.offset;
		while (remaining > 0)
		{
			size_t chunkLen = static_cast<size_t>(std::min<uint64_t>(remaining, buffer.size()));
			if (!readAt(offset, buffer.data(), chunkLen))
			{
				return nullopt;
			}
			crypto_generichash_update(&state, buffer.data(), chunkLen);
			offset += chunkLen;
			remaining -= chunkLen;
		}
	}

	std::array<uint8_t, HASH_BYTES> answer;
	crypto_generichash_final(&state, answer.data(), answer.size());
	return answer;
}

} // namespace wzkeyedhash
