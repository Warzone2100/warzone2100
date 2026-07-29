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

#include "lib/framework/wzstring.h"
#include "lib/framework/physfs_ext.h"
#include <physfs.h>
#include <cstdint>
#include <memory>
#include <vector>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

/** Abstraction for reading video data from a file or a memory buffer.
 *
 * A positioned byte source: sequential reads plus random-access seeks, which
 * container demuxers are layered on top of (Ogg only needs sequential reads;
 * Matroska/WebM parsing requires random access).
 */
class VideoProvider
{
public:
	VideoProvider(const WzString& filename)
	: m_filename(filename)
	{ }
	virtual ~VideoProvider();

	const WzString& filename() const { return m_filename; }

	virtual bool seek(int64_t pos) = 0;
	virtual int64_t tell() const = 0;
	/** Read up to len bytes into dest.
	 * \returns the number of bytes read: 0 on end-of-data, < 0 on error */
	virtual int64_t read(void *dest, uint64_t len) = 0;
	/** Total size in bytes, if known */
	virtual optional<int64_t> length() const = 0;
	virtual bool end_of_data() const = 0;

private:
	WzString m_filename;
};

// Takes ownership of the PHYSFS_file*
std::shared_ptr<VideoProvider> makeVideoProvider(PHYSFS_file *in, const WzString& filename);
std::shared_ptr<VideoProvider> makeVideoProvider(std::shared_ptr<const std::vector<char>> memoryBuffer, const WzString& filename);
