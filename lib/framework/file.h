/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2019  Warzone 2100 Project

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
WZ_DECL_NONNULL(1) PHYSFS_file *openLoadFile(const char *fileName, bool hard_fail);

/*! Open a file for writing */
WZ_DECL_NONNULL(1) PHYSFS_file *openSaveFile(const char *fileName);

/** Load the file with name pointed to by pFileName into a memory buffer. */
WZ_DECL_NONNULL(1) bool loadFile(const char *pFileName, char **ppFileData, UDWORD *pFileSize);

/** Save the data in the buffer into the given file */
WZ_DECL_NONNULL(1) bool saveFile(const char *pFileName, const char *pFileData, UDWORD fileSize);

/** Load a file from disk into a fixed memory buffer. */
WZ_DECL_NONNULL(1, 2) bool loadFileToBuffer(const char *pFileName, char *pFileBuffer, UDWORD bufferSize, UDWORD *pSize);

/** Load a file from disk, but returns quietly if no file found. */
WZ_DECL_NONNULL(1, 2) bool loadFileToBufferNoError(const char *pFileName, char *pFileBuffer, UDWORD bufferSize, UDWORD *pSize);

WZ_DECL_NONNULL(1) Sha256 findHashOfFile(char const *realFileName);

#endif // _file_h
