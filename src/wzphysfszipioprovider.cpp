// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include "wzphysfszipioprovider.h"

WzZipIOPHYSFSSourceReadProvider::WzZipIOPHYSFSSourceReadProvider()
{ }

WzZipIOPHYSFSSourceReadProvider::~WzZipIOPHYSFSSourceReadProvider()
{
	if (fp != nullptr)
	{
		PHYSFS_close(fp);
		fp = nullptr;
	}
}

std::shared_ptr<WzZipIOPHYSFSSourceReadProvider> WzZipIOPHYSFSSourceReadProvider::make(const std::string& path)
{
	PHYSFS_File *fp = PHYSFS_openRead(path.c_str());
	if (!fp)
	{
		return nullptr;
	}
	class make_shared_enabler: public WzZipIOPHYSFSSourceReadProvider {};
	auto result = std::make_shared<make_shared_enabler>();
	result->fp = fp;
	PHYSFS_sint64 len = PHYSFS_fileLength(result->fp);
	if (len >= 0)
	{
		result->fileLength = static_cast<uint64_t>(len);
	}
	else
	{
		return nullptr;
	}
	if (PHYSFS_stat(path.c_str(), &result->metaData) == 0)
	{
		// stat failed - non-fatal error, but make sure metaData is reinitialized
		result->metaData = {};
	}
	return result;
}

optional<uint64_t> WzZipIOPHYSFSSourceReadProvider::tell()
{
	auto currentOffset = PHYSFS_tell(fp);
	if (currentOffset < 0)
	{
		return nullopt;
	}
	return static_cast<uint64_t>(currentOffset);
}

bool WzZipIOPHYSFSSourceReadProvider::seek(uint64_t pos)
{
	return PHYSFS_seek(fp, pos) != 0;
}

optional<uint64_t> WzZipIOPHYSFSSourceReadProvider::readBytes(void *buffer, uint64_t len)
{
	auto bytesRead = PHYSFS_readBytes(fp, buffer, len);
	if (bytesRead < 0)
	{
		return nullopt;
	}
	return static_cast<uint64_t>(bytesRead);
}

optional<uint64_t> WzZipIOPHYSFSSourceReadProvider::fileSize()
{
	return fileLength;
}

optional<uint64_t> WzZipIOPHYSFSSourceReadProvider::modTime()
{
	if (metaData.modtime > 0)
	{
		return metaData.modtime;
	}
	return nullopt;
}

