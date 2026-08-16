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
#include <cstdint>
#include <string>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#include "lib/framework/crc.h"

#include "wzmachohash.h"

// Digest-only hashing over a closed set of files.
//
// No function returns file contents, and a HashableFile is constructible only through the factories below.
class HashableFile
{
public:
	// How the underlying file path was determined.
	enum class PathConfidence
	{
		KernelAuthoritative,	// OS-provided (GetModuleFileNameW, /proc/self/exe, _NSGetExecutablePath)
		Heuristic		// reconstructed (ex. argv[0] resolution)
	};

	struct WholeFileHash
	{
		Sha256 hash;
		uint64_t fileSize = 0;
	};

public:
	// The running process's own on-disk executable.
	// Returns nullopt when the platform provides no way to locate it.
	static optional<HashableFile> ownExecutable();

	// A data archive currently mounted in the PhysFS search path.
	// Returns nullopt if realPath is not a mounted search path entry.
	static optional<HashableFile> mountedDataArchive(const std::string& realPath);

	// The resolved path (for display and diagnostics only).
	const std::string& displayPath() const { return m_displayPath; }
	PathConfidence pathConfidence() const { return m_confidence; }

	// Chunked whole-file SHA-256.
	// Returns nullopt on open/read failure, or if *stopFlag became true.
	optional<WholeFileHash> hashWholeFile(const std::atomic<bool>* stopFlag = nullptr) const;

	// Per-slice signature-invariant canonical hashes (Mach-O executables only)
	optional<std::vector<wzmachohash::SliceCanonicalHash>> machoCanonicalHashes(const std::atomic<bool>* stopFlag = nullptr) const;

private:
	HashableFile(std::string displayPath, bool openViaProcSelfExe, PathConfidence confidence)
	: m_displayPath(std::move(displayPath))
	, m_openViaProcSelfExe(openViaProcSelfExe)
	, m_confidence(confidence)
	{ }

private:
	std::string m_displayPath;
	// Linux: open /proc/self/exe itself, not its resolved path
	// (a binary replaced on disk mid-run still hashes the running image)
	bool m_openViaProcSelfExe = false;
	PathConfidence m_confidence = PathConfidence::KernelAuthoritative;
};
