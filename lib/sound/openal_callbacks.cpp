/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2020  Warzone 2100 Project

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

#include "openal_callbacks.h"

int wz_opus_read(void *_stream, unsigned char *_ptr, int _nbytes)
{
	PHYSFS_file *fileHandle;

	ASSERT(_stream != nullptr, "NULL decoder passed!");

	fileHandle = ((PHYSFS_file *)_stream);
	ASSERT(fileHandle != nullptr, "Bad PhysicsFS file handle passed in");

	ASSERT(_nbytes <= static_cast<size_t>(std::numeric_limits<PHYSFS_uint32>::max()), "readLen (%i) exceeds PHYSFS_uint32::max", _nbytes);
	const int32_t didread = WZ_PHYSFS_readBytes(fileHandle, _ptr, static_cast<PHYSFS_uint32>(_nbytes));
	return didread;
}

/**
 *
 * \param	_stream	    The stream to read from.
 * \param 	[out]	 _ptr	The buffer to store the data in.
 * \param _nbytes	    The maximum number of bytes to read.
	This function may return fewer, though it will not return zero unless it reaches end-of-file.
*/
size_t wz_ogg_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	PHYSFS_file *fileHandle;

	ASSERT(datasource != nullptr, "NULL decoder passed!");

	fileHandle = ((PHYSFS_file *)datasource);
	ASSERT(fileHandle != nullptr, "Bad PhysicsFS file handle passed in");

	size_t readLen = size * nmemb;
	ASSERT(readLen <= static_cast<size_t>(std::numeric_limits<PHYSFS_uint32>::max()), "readLen (%zu) exceeds PHYSFS_uint32::max", readLen);
	return WZ_PHYSFS_readBytes(fileHandle, ptr, static_cast<PHYSFS_uint32>(readLen));
}

int wz_ogg_seek(void *datasource, int64_t offset, int whence)
{
	PHYSFS_file *fileHandle;
	int64_t newPos;
	ASSERT(datasource != nullptr, "NULL decoder passed!");
	fileHandle = ((PHYSFS_file *)datasource);
	ASSERT(fileHandle != nullptr, "Bad PhysicsFS file handle passed in");

	switch (whence)
	{
	// Seek to absolute position
	case SEEK_SET:
		newPos = offset;
		break;

	// Seek `offset` ahead
	case SEEK_CUR:
		{
			int64_t curPos = PHYSFS_tell(fileHandle);
			if (curPos == -1)
			{
				return -1;
			}

			newPos = curPos + offset;
			break;
		}

	// Seek backwards from the end of the file
	case SEEK_END:
		{
			int64_t fileSize = PHYSFS_fileLength(fileHandle);
			if (fileSize == -1)
			{
				return -1;
			}

			newPos = fileSize - 1 - offset;
			break;
		}

	// unrecognized seek instruction
	default:
		// indicate failure
		return -1;
	}

	// PHYSFS_seek return value of non-zero means success
	if (PHYSFS_seek(fileHandle, newPos) != 0)
	{
		// success is NON-ZERO!
		return newPos;
	}
	else
	{
		// indicate failure
		return -1;    // failure
	}
}

int wz_opus_seek(void *datasource, int64_t offset, int whence)
{
	PHYSFS_file *fileHandle;
	int64_t newPos;
	ASSERT(datasource != nullptr, "NULL decoder passed!");
	fileHandle = ((PHYSFS_file *)datasource);
	ASSERT(fileHandle != nullptr, "Bad PhysicsFS file handle passed in");

	switch (whence)
	{
	// Seek to absolute position
	case SEEK_SET:
		newPos = offset;
		break;

	// Seek `offset` ahead
	case SEEK_CUR:
		{
			int64_t curPos = PHYSFS_tell(fileHandle);
			if (curPos == -1)
			{
				return -1;
			}

			newPos = curPos + offset;
			break;
		}

	// Seek backwards from the end of the file
	case SEEK_END:
		{
			int64_t fileSize = PHYSFS_fileLength(fileHandle);
			if (fileSize == -1)
			{
				return -1;
			}

			newPos = fileSize - 1 - offset;
			break;
		}

	// unrecognized seek instruction
	default:
		// indicate failure
		return -1;
	}

	// PHYSFS_seek return value of non-zero means success
	if (PHYSFS_seek(fileHandle, newPos) != 0)
	{
	 // success is zero! opposite of what PHYSFS thinks
		return 0;  
	}
	else
	{
		// indicate failure
		return -1;    // failure
	}
}

long wz_ogg_tell(void *datasource)
{
	PHYSFS_file *fileHandle;
	ASSERT(datasource != nullptr, "NULL decoder passed!");

	fileHandle = ((PHYSFS_file *)datasource);
	ASSERT(fileHandle != nullptr, "Bad PhysicsFS file handle passed in");
	const int64_t out = PHYSFS_tell(fileHandle);
	return out;
}

int64_t wz_opus_tell(void *datasource)
{
	PHYSFS_file *fileHandle;
	ASSERT(datasource != nullptr, "NULL decoder passed!");

	fileHandle = ((PHYSFS_file *)datasource);
	ASSERT(fileHandle != nullptr, "Bad PhysicsFS file handle passed in");
	const int64_t out = PHYSFS_tell(fileHandle);
	return out;
}
