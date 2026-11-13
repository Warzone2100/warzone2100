// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2024-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/*
 * @file wzfile.h
 *
 * `WZFile` thin wrapper over physfs file handle. Used for network code related to
 * downloading files.
 */

#pragma once

#include "lib/framework/crc.h"

#include <physfs.h>

#include <string>
#include <memory>
#include <stdint.h>

void physfs_file_safe_close(PHYSFS_file* f);

struct WZFile
{
public:

	WZFile(PHYSFS_file* handle, const std::string& filename, Sha256 hash, uint32_t size = 0) : handle_(handle, physfs_file_safe_close), filename(filename), hash(hash), size(size), pos(0) {}

	~WZFile();

	// Prevent copies
	WZFile(const WZFile&) = delete;
	void operator=(const WZFile&) = delete;

	// Allow move semantics
	WZFile(WZFile&& other) = default;
	WZFile& operator=(WZFile&& other) = default;

public:
	bool closeFile();
	inline PHYSFS_file* handle() const
	{
		return handle_.get();
	}

private:
	std::unique_ptr<PHYSFS_file, void(*)(PHYSFS_file*)> handle_;
public:
	std::string filename;
	Sha256 hash;
	uint32_t size = 0;
	uint32_t pos = 0;  // Current position, the range [0; currPos[ has been sent or received already.
};
