/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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
