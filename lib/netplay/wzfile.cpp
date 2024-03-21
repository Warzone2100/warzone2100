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
/**
 * @file wzfile.cpp
 *
 * `WZFile` thin wrapper over physfs file handle. Used for network code related to
 * downloading files.
 */

#include "wzfile.h"

static inline bool physfs_file_safe_close_impl(PHYSFS_file* f)
{
	if (!f)
	{
		return false;
	}
	if (!PHYSFS_isInit())
	{
		return false;
	}
	PHYSFS_close(f);
	f = nullptr;
	return true;
}

void physfs_file_safe_close(PHYSFS_file* f)
{
	physfs_file_safe_close_impl(f);
}

WZFile::~WZFile()
{ }

bool WZFile::closeFile()
{
	if (!handle_)
	{
		return false;
	}
	PHYSFS_file* file = handle_.release();
	return physfs_file_safe_close_impl(file);
}
