/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#ifndef _file_h
#define _file_h

#include <physfs.h>

#include "crc.h"


/*! Open a file for reading */
extern PHYSFS_file* openLoadFile(const char* fileName, bool hard_fail);

/*! Open a file for writing */
extern PHYSFS_file* openSaveFile(const char* fileName);

/** Load the file with name pointed to by pFileName into a memory buffer. */
extern bool loadFile(const char *pFileName,		// The filename
              char **ppFileData,	// A buffer containing the file contents
              UDWORD *pFileSize);	// The size of this buffer

/** Save the data in the buffer into the given file */
extern bool saveFile(const char *pFileName, const char *pFileData, UDWORD fileSize);

/** Load a file from disk into a fixed memory buffer. */
extern bool loadFileToBuffer(const char *pFileName, char *pFileBuffer, UDWORD bufferSize, UDWORD *pSize);

/** Load a file from disk, but returns quietly if no file found. */
extern bool loadFileToBufferNoError(const char *pFileName, char *pFileBuffer, UDWORD bufferSize,
                             UDWORD *pSize);

Sha256 findHashOfFile(char const *realFileName);

#endif // _file_h
