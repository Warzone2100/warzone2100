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

#include "wzmachohash.h"

#include <sodium.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace wzmachohash
{

namespace
{

constexpr uint32_t FAT_MAGIC_BE = 0xcafebabe;
constexpr uint32_t FAT_MAGIC_64_BE = 0xcafebabf;
constexpr uint32_t MH_MAGIC_64 = 0xfeedfacf;
constexpr uint32_t LC_SEGMENT_64 = 0x19;
constexpr uint32_t LC_CODE_SIGNATURE = 0x1d;
constexpr int32_t CPU_TYPE_X86_64 = 0x01000007;
constexpr int32_t CPU_TYPE_ARM64 = 0x0100000c;

constexpr uint32_t MAX_FAT_SLICES = 16;
constexpr uint32_t MAX_SIZEOFCMDS = 16 * 1024 * 1024;
constexpr size_t HASH_CHUNK_SIZE = 1024 * 1024;

uint32_t readU32BE(const unsigned char* p)
{
	return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}

uint64_t readU64BE(const unsigned char* p)
{
	return (uint64_t(readU32BE(p)) << 32) | readU32BE(p + 4);
}

uint32_t readU32LE(const unsigned char* p)
{
	return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

std::string archName(int32_t cputype)
{
	switch (cputype)
	{
		case CPU_TYPE_X86_64: return "x86_64";
		case CPU_TYPE_ARM64: return "arm64";
		default: break;
	}
	char buf[32];
	snprintf(buf, sizeof(buf), "cputype-0x%x", static_cast<uint32_t>(cputype));
	return buf;
}

struct MaskRange
{
	uint64_t offset;	// slice-relative
	uint64_t len;
};

optional<SliceCanonicalHash> hashSlice(const ReadAtFunc& readAt, uint64_t sliceOffset, uint64_t sliceSize, std::string& errorDetails)
{
	unsigned char header[32];
	if (sliceSize < sizeof(header) || !readAt(sliceOffset, header, sizeof(header)))
	{
		errorDetails = "Failed to read slice header";
		return nullopt;
	}
	if (readU32LE(header) != MH_MAGIC_64)
	{
		errorDetails = "Unsupported slice format (not 64-bit little-endian Mach-O)";
		return nullopt;
	}
	int32_t cputype = static_cast<int32_t>(readU32LE(header + 4));
	uint32_t ncmds = readU32LE(header + 16);
	uint32_t sizeofcmds = readU32LE(header + 20);
	if (sizeofcmds > MAX_SIZEOFCMDS || uint64_t(sizeofcmds) + sizeof(header) > sliceSize)
	{
		errorDetails = "Invalid load commands size";
		return nullopt;
	}
	std::vector<unsigned char> cmds(sizeofcmds);
	if (!readAt(sliceOffset + sizeof(header), cmds.data(), cmds.size()))
	{
		errorDetails = "Failed to read load commands";
		return nullopt;
	}

	optional<uint64_t> codeSigCmdOffset;	// slice-relative
	uint32_t codeSigDataOff = 0;
	optional<uint64_t> linkeditCmdOffset;	// slice-relative
	uint64_t walkPos = 0;
	for (uint32_t i = 0; i < ncmds; ++i)
	{
		if (walkPos + 8 > sizeofcmds)
		{
			errorDetails = "Truncated load commands";
			return nullopt;
		}
		uint32_t cmd = readU32LE(cmds.data() + walkPos);
		uint32_t cmdsize = readU32LE(cmds.data() + walkPos + 4);
		if (cmdsize < 8 || walkPos + cmdsize > sizeofcmds)
		{
			errorDetails = "Invalid load command size";
			return nullopt;
		}
		if (cmd == LC_CODE_SIGNATURE && cmdsize >= 16)
		{
			codeSigCmdOffset = sizeof(header) + walkPos;
			codeSigDataOff = readU32LE(cmds.data() + walkPos + 8);
		}
		else if (cmd == LC_SEGMENT_64 && cmdsize >= 72)
		{
			if (memcmp(cmds.data() + walkPos + 8, "__LINKEDIT\0\0\0\0\0\0", 16) == 0)
			{
				linkeditCmdOffset = sizeof(header) + walkPos;
			}
		}
		walkPos += cmdsize;
	}

	uint64_t cutoff = codeSigCmdOffset.has_value() ? codeSigDataOff : sliceSize;
	if (cutoff > sliceSize || cutoff < sizeof(header) + sizeofcmds)
	{
		errorDetails = "Invalid code signature offset";
		return nullopt;
	}

	std::vector<MaskRange> masks;
	if (codeSigCmdOffset.has_value())
	{
		masks.push_back({codeSigCmdOffset.value() + 8, 8});	// dataoff + datasize
	}
	if (linkeditCmdOffset.has_value())
	{
		masks.push_back({linkeditCmdOffset.value() + 32, 8});	// vmsize
		masks.push_back({linkeditCmdOffset.value() + 48, 8});	// filesize
	}

	crypto_hash_sha256_state state;
	crypto_hash_sha256_init(&state);
	std::vector<unsigned char> buffer(HASH_CHUNK_SIZE);
	uint64_t pos = 0;
	while (pos < cutoff)
	{
		size_t chunkLen = static_cast<size_t>(std::min<uint64_t>(buffer.size(), cutoff - pos));
		if (!readAt(sliceOffset + pos, buffer.data(), chunkLen))
		{
			errorDetails = "Failed to read slice contents";
			return nullopt;
		}
		for (const auto& mask : masks)
		{
			uint64_t begin = std::max(mask.offset, pos);
			uint64_t end = std::min(mask.offset + mask.len, pos + chunkLen);
			if (begin < end)
			{
				memset(buffer.data() + (begin - pos), 0, static_cast<size_t>(end - begin));
			}
		}
		crypto_hash_sha256_update(&state, buffer.data(), chunkLen);
		pos += chunkLen;
	}
	unsigned char digest[crypto_hash_sha256_BYTES];
	crypto_hash_sha256_final(&state, digest);
	char hex[crypto_hash_sha256_BYTES * 2 + 1];
	sodium_bin2hex(hex, sizeof(hex), digest, sizeof(digest));

	SliceCanonicalHash result;
	result.arch = archName(cputype);
	result.sha256Hex = hex;
	return result;
}

} // anonymous namespace

optional<std::vector<SliceCanonicalHash>> computeCanonicalHashes(const ReadAtFunc& readAt, uint64_t fileSize, std::string& errorDetails)
{
	unsigned char magicBytes[8];
	if (fileSize < sizeof(magicBytes) || !readAt(0, magicBytes, sizeof(magicBytes)))
	{
		errorDetails = "Failed to read file header";
		return nullopt;
	}
	uint32_t magicBE = readU32BE(magicBytes);

	std::vector<SliceCanonicalHash> results;
	if (magicBE == FAT_MAGIC_BE || magicBE == FAT_MAGIC_64_BE)
	{
		bool is64 = (magicBE == FAT_MAGIC_64_BE);
		uint32_t nfat = readU32BE(magicBytes + 4);
		if (nfat == 0 || nfat > MAX_FAT_SLICES)
		{
			errorDetails = "Invalid fat arch count";
			return nullopt;
		}
		size_t entrySize = is64 ? 32 : 20;
		std::vector<unsigned char> entries(nfat * entrySize);
		if (!readAt(8, entries.data(), entries.size()))
		{
			errorDetails = "Failed to read fat arch entries";
			return nullopt;
		}
		for (uint32_t i = 0; i < nfat; ++i)
		{
			const unsigned char* entry = entries.data() + (i * entrySize);
			uint64_t sliceOffset = is64 ? readU64BE(entry + 8) : readU32BE(entry + 8);
			uint64_t sliceSize = is64 ? readU64BE(entry + 16) : readU32BE(entry + 12);
			if (sliceOffset + sliceSize > fileSize || sliceOffset + sliceSize < sliceOffset)
			{
				errorDetails = "Fat slice exceeds file bounds";
				return nullopt;
			}
			optional<SliceCanonicalHash> slice = hashSlice(readAt, sliceOffset, sliceSize, errorDetails);
			if (!slice.has_value())
			{
				return nullopt;
			}
			results.push_back(std::move(slice.value()));
		}
	}
	else if (readU32LE(magicBytes) == MH_MAGIC_64)
	{
		optional<SliceCanonicalHash> slice = hashSlice(readAt, 0, fileSize, errorDetails);
		if (!slice.has_value())
		{
			return nullopt;
		}
		results.push_back(std::move(slice.value()));
	}
	else
	{
		errorDetails = "Not a Mach-O or universal binary";
		return nullopt;
	}
	return results;
}

} // namespace wzmachohash
