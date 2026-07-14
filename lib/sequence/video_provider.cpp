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

#include "lib/framework/frame.h"
#include "lib/framework/physfs_ext.h"
#include "video_provider.h"

#include <algorithm>
#include <cstring>
#include <limits>

VideoProvider::~VideoProvider()
{ }

class PhysFSVideoProvider : public VideoProvider
{
public:
	// Takes ownership of the PHYSFS_file*
	PhysFSVideoProvider(PHYSFS_file *in, const WzString& filename)
	: VideoProvider(filename)
	, in(in)
	{ }

	~PhysFSVideoProvider()
	{
		if (in && PHYSFS_isInit())
		{
			PHYSFS_close(in);
			in = nullptr;
		}
	}

	bool seek(int64_t pos) override
	{
		if (!in || pos < 0) return false;
		return PHYSFS_seek(in, static_cast<PHYSFS_uint64>(pos)) != 0;
	}

	int64_t tell() const override
	{
		if (!in) return -1;
		return PHYSFS_tell(in);
	}

	int64_t read(void *dest, uint64_t len) override
	{
		if (!in) return -1;
		PHYSFS_sint64 bytes = WZ_PHYSFS_readBytes(in, dest, static_cast<PHYSFS_uint32>(std::min<uint64_t>(len, std::numeric_limits<PHYSFS_uint32>::max())));
		if (bytes < 0)
		{
			// a read error may still have consumed data; treat "some bytes at EOF" as a short read
			return PHYSFS_eof(in) ? 0 : -1;
		}
		return bytes;
	}

	optional<int64_t> length() const override
	{
		if (!in) return nullopt;
		PHYSFS_sint64 len = PHYSFS_fileLength(in);
		if (len < 0) return nullopt;
		return len;
	}

	bool end_of_data() const override
	{
		if (!in) return true;
		return PHYSFS_eof(in) != 0;
	}

private:
	PHYSFS_file *in = nullptr;
};

class MemoryBufferVideoProvider : public VideoProvider
{
public:
	MemoryBufferVideoProvider(std::shared_ptr<const std::vector<char>> memoryBuffer, const WzString& filename)
	: VideoProvider(filename)
	, memoryBuffer(memoryBuffer)
	{ }

	bool seek(int64_t pos) override
	{
		if (!memoryBuffer || pos < 0 || static_cast<uint64_t>(pos) > memoryBuffer->size())
		{
			return false;
		}
		currPos = static_cast<size_t>(pos);
		return true;
	}

	int64_t tell() const override
	{
		return static_cast<int64_t>(currPos);
	}

	int64_t read(void *dest, uint64_t len) override
	{
		if (!memoryBuffer) return -1;
		if (currPos >= memoryBuffer->size()) return 0;

		size_t bytesToRead = static_cast<size_t>(std::min<uint64_t>(len, memoryBuffer->size() - currPos));
		memcpy(dest, memoryBuffer->data() + currPos, bytesToRead);
		currPos += bytesToRead;
		return static_cast<int64_t>(bytesToRead);
	}

	optional<int64_t> length() const override
	{
		if (!memoryBuffer) return nullopt;
		return static_cast<int64_t>(memoryBuffer->size());
	}

	bool end_of_data() const override
	{
		if (!memoryBuffer) return true;
		return currPos >= memoryBuffer->size();
	}

private:
	std::shared_ptr<const std::vector<char>> memoryBuffer;
	size_t currPos = 0;
};

std::shared_ptr<VideoProvider> makeVideoProvider(PHYSFS_file *in, const WzString& filename)
{
	return std::make_shared<PhysFSVideoProvider>(in, filename);
}

std::shared_ptr<VideoProvider> makeVideoProvider(std::shared_ptr<const std::vector<char>> memoryBuffer, const WzString& filename)
{
	return std::make_shared<MemoryBufferVideoProvider>(memoryBuffer, filename);
}
