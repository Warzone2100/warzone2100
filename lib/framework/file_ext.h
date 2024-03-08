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
#ifndef _file_ext_h
#define _file_ext_h

#include "frame.h"
#include "file.h"
#include "physfs_ext.h"

template<class T, std::enable_if_t<sizeof(T) == sizeof(char), bool> = true>
bool loadFileToBufferVectorT(const char *pFileName, std::vector<T>& outputBuffer, bool hard_fail, bool appendNullCharacter = true)
{
	if (WZ_PHYSFS_isDirectory(pFileName))
	{
		return false;
	}

	PHYSFS_file *pfile = openLoadFile(pFileName, hard_fail);
	if (!pfile)
	{
		return false;
	}

	PHYSFS_sint64 filesize = PHYSFS_fileLength(pfile);
	if (filesize < 0)
	{
		PHYSFS_close(pfile);
		return false;  // File size could not be determined. Is a directory?
	}
	if (!(filesize < static_cast<PHYSFS_sint64>(std::numeric_limits<PHYSFS_sint32>::max())))
	{
		PHYSFS_close(pfile);
		ASSERT(filesize < static_cast<PHYSFS_sint64>(std::numeric_limits<PHYSFS_sint32>::max()), "\"%s\" filesize >= std::numeric_limits<PHYSFS_sint32>::max()", pFileName);
		return false;
	}
	if (!(static_cast<PHYSFS_uint64>(filesize) < static_cast<PHYSFS_uint64>(std::numeric_limits<size_t>::max())))
	{
		PHYSFS_close(pfile);
		ASSERT(static_cast<PHYSFS_uint64>(filesize) < static_cast<PHYSFS_uint64>(std::numeric_limits<size_t>::max()), "\"%s\" filesize >= std::numeric_limits<size_t>::max()", pFileName);
		return false;
	}

	outputBuffer.clear();
	outputBuffer.resize(static_cast<size_t>(filesize + ((appendNullCharacter) ? 1 : 0)));

	/* Load the file data */
	PHYSFS_sint64 length_read = WZ_PHYSFS_readBytes(pfile, outputBuffer.data(), static_cast<PHYSFS_uint32>(filesize));
	if (length_read != filesize)
	{
		outputBuffer.clear();

		PHYSFS_close(pfile);
		debug(LOG_ERROR, "Reading %s short: %s", pFileName, WZ_PHYSFS_getLastError());
		assert(false);
		return false;
	}

	if (!PHYSFS_close(pfile))
	{
		outputBuffer.clear();

		debug(LOG_ERROR, "Error closing %s: %s", pFileName, WZ_PHYSFS_getLastError());
		assert(false);
		return false;
	}

	if (appendNullCharacter)
	{
		outputBuffer[outputBuffer.size() - 1] = 0;
	}

	ASSERT(static_cast<PHYSFS_uint64>(filesize) <= static_cast<PHYSFS_uint64>(std::numeric_limits<UDWORD>::max()), "filesize exceeds std::numeric_limits<UDWORD>::max()");

	return true;
}

#endif // _file_ext_h
