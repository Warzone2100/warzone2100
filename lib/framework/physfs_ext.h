/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#ifndef _physfs_ext_h
#define _physfs_ext_h

#if defined(__MACOSX__)
# include <PhysFS/physfs.h>
#else
# include <physfs.h>
#endif
#include <QtCore/QString>

#include "wzglobal.h"

#define PHYSFS_APPEND 1
#define PHYSFS_PREPEND 0

// Detect the version of PhysFS
#if PHYSFS_VER_MAJOR > 2 || (PHYSFS_VER_MAJOR == 2 && PHYSFS_VER_MINOR >= 1)
	#define WZ_PHYSFS_2_1_OR_GREATER
#elif (PHYSFS_VER_MAJOR == 2 && PHYSFS_VER_MINOR == 0)
	#define WZ_PHYSFS_2_0_OR_GREATER
#else
	#error WZ requires PhysFS 2.0+
#endif

// WZ PHYSFS wrappers to provide consistent naming (and functionality) on PhysFS 2.0 and 2.1+

// NOTE: This uses PHYSFS_uint32 for `len` because PHYSFS_read takes a PHYSFS_uint32 objCount
static inline PHYSFS_sint64 WZ_PHYSFS_readBytes (PHYSFS_File * handle, void * buffer, PHYSFS_uint32 len)
{
#if defined(WZ_PHYSFS_2_1_OR_GREATER)
	return PHYSFS_readBytes(handle, buffer, len);
#else
	return PHYSFS_read(handle, buffer, 1, len);
#endif
}

// NOTE: This uses PHYSFS_uint32 for `len` because PHYSFS_write takes a PHYSFS_uint32 objCount
static inline PHYSFS_sint64 WZ_PHYSFS_writeBytes (PHYSFS_File * handle, const void * buffer, PHYSFS_uint32 len)
{
#if defined(WZ_PHYSFS_2_1_OR_GREATER)
	return PHYSFS_writeBytes(handle, buffer, len);
#else
	return PHYSFS_write(handle, buffer, 1, len);
#endif
}

static inline int WZ_PHYSFS_unmount (const char * oldDir)
{
#if defined(WZ_PHYSFS_2_1_OR_GREATER)
	return PHYSFS_unmount(oldDir);
#else
	// PHYSFS_unmount is functionally equivalent to PHYSFS_removeFromSearchPath (the vocabulary just changed)
	return PHYSFS_removeFromSearchPath(oldDir);
#endif
}

#if defined(WZ_PHYSFS_2_1_OR_GREATER)
	#define WZ_PHYSFS_getLastError() \
		PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
#else
	#define WZ_PHYSFS_getLastError() \
		PHYSFS_getLastError()
#endif

static inline PHYSFS_sint64 WZ_PHYSFS_getLastModTime (const char *filename)
{
#if defined(WZ_PHYSFS_2_1_OR_GREATER)
	PHYSFS_Stat metaData;
	PHYSFS_stat(filename, &metaData);
	return metaData.modtime;
#else
	return PHYSFS_getLastModTime(filename);
#endif
}

static inline int WZ_PHYSFS_isDirectory (const char * fname)
{
#if defined(WZ_PHYSFS_2_1_OR_GREATER)
	PHYSFS_Stat metaData;
	PHYSFS_stat(fname, &metaData);
	return (metaData.filetype == PHYSFS_FILETYPE_DIRECTORY) ? 1 : 0;
#else
	return PHYSFS_isDirectory(fname);
#endif
}

// Older wrappers

static inline bool PHYSFS_exists(const QString &filename)
{
	return PHYSFS_exists(filename.toUtf8().constData());
}

static inline bool PHYSFS_writeSLE8(PHYSFS_file *file, int8_t val)
{
	return (WZ_PHYSFS_writeBytes(file, &val, sizeof(int8_t)) == sizeof(int8_t));
}

static inline bool PHYSFS_writeULE8(PHYSFS_file *file, uint8_t val)
{
	return (WZ_PHYSFS_writeBytes(file, &val, sizeof(uint8_t)) == sizeof(uint8_t));
}

static inline bool PHYSFS_readSLE8(PHYSFS_file *file, int8_t *val)
{
	return (WZ_PHYSFS_readBytes(file, val, sizeof(int8_t)) == sizeof(int8_t));
}

static inline bool PHYSFS_readULE8(PHYSFS_file *file, uint8_t *val)
{
	return (WZ_PHYSFS_readBytes(file, val, sizeof(uint8_t)) == sizeof(uint8_t));
}

static inline bool PHYSFS_writeSBE8(PHYSFS_file *file, int8_t val)
{
	return (WZ_PHYSFS_writeBytes(file, &val, sizeof(int8_t)) == sizeof(int8_t));
}

static inline bool PHYSFS_writeUBE8(PHYSFS_file *file, uint8_t val)
{
	return (WZ_PHYSFS_writeBytes(file, &val, sizeof(uint8_t)) == sizeof(uint8_t));
}

static inline bool PHYSFS_readSBE8(PHYSFS_file *file, int8_t *val)
{
	return (WZ_PHYSFS_readBytes(file, val, sizeof(int8_t)) == sizeof(int8_t));
}

static inline bool PHYSFS_readUBE8(PHYSFS_file *file, uint8_t *val)
{
	return (WZ_PHYSFS_readBytes(file, val, sizeof(uint8_t)) == sizeof(uint8_t));
}

static inline bool PHYSFS_writeBEFloat(PHYSFS_file *file, float val)
{
	// For the purpose of endian conversions a IEEE754 float can be considered
	// the same to a 32bit integer.
	// We're using a union here to prevent type punning of pointers.
	union
	{
		float f;
		uint32_t i;
	} writeValue;
	writeValue.f = val;
	return (PHYSFS_writeUBE32(file, writeValue.i) != 0);
}

static inline bool PHYSFS_readBEFloat(PHYSFS_file *file, float *val)
{
	// For the purpose of endian conversions a IEEE754 float can be considered
	// the same to a 32bit integer.
	uint32_t *readValue = (uint32_t *)val;
	return (PHYSFS_readUBE32(file, readValue) != 0);
}

#ifdef WZ_CC_MINGW
bool PHYSFS_printf(PHYSFS_file *file, const char *format, ...) WZ_DECL_FORMAT(__MINGW_PRINTF_FORMAT, 2, 3);
#else
bool PHYSFS_printf(PHYSFS_file *file, const char *format, ...) WZ_DECL_FORMAT(printf, 2, 3);
#endif

/**
 * @brief      fgets() implemented using PHYSFS.
 * @param      s Pointer to an array where the read chars will be stored.
 * @param      size Maximum number of chars to read (includes terminating null character).
 * @param      stream PHYSFS file handle.
 * @return     s on success, NULL on error or if no characters were read.
 * @note       WZ_PHYSFS_getLastError() or PHYSFS_eof() can help find the source
 *                     of the error.
 * @note       If a EOF is encountered before any chars are read, the chars
 *                     pointed by s are not changed.
 *
 * This function reads in at most size - 1 characters from stream
 * and stores them into the buffer pointed to by s.
 * Reading stops after an EOF or a newline and a null char is automatically appended.
 * If a newline is read, it is stored into the buffer.
 */
static inline char *PHYSFS_fgets(char *s, int size, PHYSFS_file *stream)
{
	char c;
	int i = 0;
	PHYSFS_sint64 retval;

	if (size <= 0 || !stream || !s || PHYSFS_eof(stream))
	{
		return nullptr;
	}
	do
	{
		retval = WZ_PHYSFS_readBytes(stream, &c, 1);

		if (retval < 1)
		{
			break;
		}
		s[i++] = c;
	}
	while (c != '\n' && i < size - 1);
	s[i] = '\0';

	// Success conditions
	if (retval == 1 // Read maximum chars or newline
	    || (i != 0 && PHYSFS_eof(stream) != 0)) /* Read something and stopped at EOF
	                                          * (note: i!=0 *should* be redundant) */
	{
		return s;
	}

	// Complete failure
	return nullptr;
}

#endif // _physfs_ext_h
