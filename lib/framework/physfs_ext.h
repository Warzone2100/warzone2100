/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2010  Warzone 2100 Project

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

#include <physfs.h>

#define PHYSFS_APPEND 1
#define PHYSFS_PREPEND 0

static inline bool PHYSFS_writeSLE8(PHYSFS_file* file, int8_t val)
{
	return (PHYSFS_write(file, &val, sizeof(int8_t), 1) == 1);
}

static inline bool PHYSFS_writeULE8(PHYSFS_file* file, uint8_t val)
{
	return (PHYSFS_write(file, &val, sizeof(uint8_t), 1) == 1);
}

static inline bool PHYSFS_readSLE8(PHYSFS_file* file, int8_t* val)
{
	return (PHYSFS_read(file, val, sizeof(int8_t), 1) == 1);
}

static inline bool PHYSFS_readULE8(PHYSFS_file* file, uint8_t* val)
{
	return (PHYSFS_read(file, val, sizeof(uint8_t), 1) == 1);
}

static inline bool PHYSFS_writeSBE8(PHYSFS_file* file, int8_t val)
{
	return (PHYSFS_write(file, &val, sizeof(int8_t), 1) == 1);
}

static inline bool PHYSFS_writeUBE8(PHYSFS_file* file, uint8_t val)
{
	return (PHYSFS_write(file, &val, sizeof(uint8_t), 1) == 1);
}

static inline bool PHYSFS_readSBE8(PHYSFS_file* file, int8_t* val)
{
	return (PHYSFS_read(file, val, sizeof(int8_t), 1) == 1);
}

static inline bool PHYSFS_readUBE8(PHYSFS_file* file, uint8_t* val)
{
	return (PHYSFS_read(file, val, sizeof(uint8_t), 1) == 1);
}

static inline bool PHYSFS_writeBEFloat(PHYSFS_file* file, float val)
{
	// For the purpose of endian conversions a IEEE754 float can be considered
	// the same to a 32bit integer.
	// We're using a union here to prevent type punning of pointers.
	union {
		float f;
		uint32_t i;
	} writeValue;
	writeValue.f = val;
	return (PHYSFS_writeUBE32(file, writeValue.i) != 0);
}

static inline bool PHYSFS_readBEFloat(PHYSFS_file* file, float* val)
{
	// For the purpose of endian conversions a IEEE754 float can be considered
	// the same to a 32bit integer.
	uint32_t* readValue = (uint32_t*)val;
	return (PHYSFS_readUBE32(file, readValue) != 0);
}

bool PHYSFS_printf(PHYSFS_file *file, const char *format, ...) WZ_DECL_FORMAT(printf, 2, 3);

#endif // _physfs_ext_h
